#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <alloca.h>

#include "util.h"
#include "lexer.h"
#include "codegen.h"
#include "asm.h"
#include "symboltable.h"



#if 0
// TODO: do sth with this
static void parser_print_error_cool(Parser *p) {

    Token tok = parser_get_current_token(p);
    throw_cool_error(p->filename, &tok, "you messed up!");

    // TODO: wrapper for dealing with this mess
    int linecounter = 1;
    for (size_t i=0; i < strlen(p->src); ++i) {
        char c = p->src[i];

        if (linecounter == tok.pos_line) {
            if (i == tok.pos_absolute) {
                fprintf(stderr, "%s%.*s%s", COLOR_RED, (int) tok.length, p->src+i, COLOR_END);
                i += tok.length - 1;
            }
            fprintf(stderr, "%c", c);
        }

        if (c == '\n')
            linecounter++;

    }

    exit(1);

}
#endif




static size_t traverse_ast(AstNode *node, CodeGenerator *codegen, Symboltable *symboltable);


static void builtinasm(
    AstNode       *node,
    CodeGenerator *codegen,
    Symboltable   *symboltable
) {

    ExprCall *call   = &node->expr_call;
    AstNodeList list = call->args;
    AstNode *first   = list.items[0];

    bool is_literal = first->kind == ASTNODE_LITERAL;
    if (!(is_literal && first->expr_literal.op.kind == TOK_STRING)) {
        throw_error(
            codegen->filename_src,
            &call->op,
            "First argument to `asm()` must be a string"
        );
    }

    // union member access only save after check
    const char *asm_src = first->expr_literal.op.value;


    // get the count of placeholders to check count of arguments
    size_t expected_args = 0;
    const char *str = asm_src;
    while ((str = strstr(str, "{}")) != NULL) {
        str++;
        expected_args++;
    }

    // `- 1`: first arg is src
    if (list.size - 1 != expected_args) {
        throw_error(
            codegen->filename_src,
            &call->op,
            "Expected `%d` argument(s), got `%d`",
            expected_args,
            list.size - 1
        );
    }

    size_t addrs_len = list.size - 1;
    size_t *addrs    = alloca(addrs_len * sizeof(size_t));
    size_t addrs_i   = 0;

    for (size_t i=1; i < list.size; ++i) {
        size_t addr = traverse_ast(list.items[i], codegen, symboltable);
        addrs[addrs_i++] = addr;
    }

    gen_inlineasm(codegen, asm_src, addrs, addrs_len);

}






// returns the location of the evaluated expression in memory
static size_t traverse_ast(
    AstNode       *node,
    CodeGenerator *codegen,
    Symboltable   *symboltable
) {

    switch (node->kind) {

        case ASTNODE_BLOCK: {
            symboltable_push(symboltable);

            AstNodeList list = node->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, symboltable);

            symboltable_pop(symboltable);
        } break;

        case ASTNODE_CALL: {
            ExprCall *call = &node->expr_call;

            if (call->builtin != BUILTINFUNC_NONE) {
                switch (call->builtin) {

                    case BUILTINFUNC_ASM:
                        builtinasm(node, codegen, symboltable);
                        break;

                    default:
                        assert(!"Unimplemented builtin function");
                        break;
                }

                break;
            }

            AstNodeList list = call->args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, symboltable);

            // HACK: call into address instead of identifier
            // size_t callee_addr = traverse_ast(call->callee, codegen, symboltable);
            gen_call(codegen, call->callee->expr_literal.op.value);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(node->expr_grouping.expr, codegen, symboltable);
            break;

        case ASTNODE_FUNC: {
            StmtFunc *func = &node->stmt_func;
            gen_func_start(codegen, func->identifier.value);
            traverse_ast(func->body, codegen, symboltable);

            // TODO: insert function name into symboltable for lookup
            gen_func_end(codegen);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &node->stmt_vardecl;
            const char *variable = vardecl->identifier.value;

            size_t addr = traverse_ast(vardecl->value, codegen, symboltable);
            int ret = symboltable_insert(symboltable, variable, addr);

            if (ret == -1) {
                // TODO: add shadowing feature
                symboltable_override(symboltable, variable, addr);
                throw_warning(
                    codegen->filename_src,
                    &vardecl->identifier,
                    "Variable `%s` already exists",
                    variable
                );
            }
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp *binop = &node->expr_binop;
            size_t addr_lhs  = traverse_ast(binop->lhs, codegen, symboltable);
            size_t addr_rhs  = traverse_ast(binop->rhs, codegen, symboltable);

            switch (binop->op.kind) {
                case TOK_PLUS: {
                    // TODO: get type
                    return gen_addition(codegen, addr_lhs, addr_rhs);
                } break;
                case TOK_MINUS: {
                    // TODO:
                    assert(!"TODO");
                } break;
                default:
                    assert(!"Unimplemented");
                    break;
            }

        } break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp *unaryop = &node->expr_unaryop;
            size_t addr = traverse_ast(unaryop->node, codegen, symboltable);

            switch (unaryop->op.kind) {
                case TOK_MINUS: {
                    assert(!"TODO"); // TODO:
                } break;
                default:
                    assert(!"Unimplemented");
                    break;
            }
        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral *literal = &node->expr_literal;

            switch (literal->op.kind) {

                case TOK_NUMBER: {
                    const char *str = literal->op.value;
                    int64_t num = atoll(str);
                    return gen_store_value(codegen, num, INTTYPE_INT);
                } break;

                case TOK_IDENTIFIER: {
                    const char *variable = literal->op.value;
                    HashtableValue *addr = symboltable_get(symboltable, variable);

                    if (addr == NULL) {
                        throw_error(
                            codegen->filename_src,
                            &literal->op,
                            "Variable `%s` does not exist",
                            variable
                        );
                    }

                    // TODO: handle type
                    return *addr;
                } break;

                default:
                    assert(!"Literal Unimplemented");
                    break;
            }
        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    // TODO:
    // assert(!"unreachable");

}

void generate_code(
    AstNode    *root,
    const char *filename_asm,
    bool        print_comments,
    const char *filename_src
) {

    CodeGenerator codegen   = gen_new(filename_asm, print_comments, filename_src);
    Symboltable symboltable = symboltable_new();

    gen_prelude(&codegen);
    traverse_ast(root, &codegen, &symboltable);
    gen_postlude(&codegen);

    assert(symboltable.size == 0);

    symboltable_destroy(&symboltable);
    gen_destroy(&codegen);
}
