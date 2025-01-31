#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <alloca.h>

#include "util.h"
#include "lexer.h"
#include "gen.h"
#include "symboltable.h"
#include "main.h"


CodeGenerator codegen;


static Symbol traverse_ast(AstNode *node, Hashtable *symboltable);


static void builtin_inlineasm(ExprCall *call, Hashtable *symboltable) {

    AstNodeList list = call->args;
    AstNode *first   = list.items[0];

    bool is_literal = first->kind == ASTNODE_LITERAL;
    if (!(is_literal && first->expr_literal.op.kind == TOK_STRING)) {
        throw_error(&call->op, "First argument to `asm()` must be a string");
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
            &call->op,
            "Expected `%d` argument(s), got `%d`",
            expected_args,
            list.size - 1
        );
    }

    size_t addrs_len = list.size - 1;
    Symbol *addrs    = alloca(addrs_len * sizeof(Symbol));
    size_t addrs_i   = 0;

    for (size_t i=1; i < list.size; ++i) {
        Symbol sym = traverse_ast(list.items[i], symboltable);
        addrs[addrs_i++] = sym;
    }

    gen_inlineasm(&codegen, asm_src, addrs, addrs_len);

}

static Symbol ast_binop(ExprBinOp *binop, Hashtable *symboltable) {
    Symbol sym_lhs  = traverse_ast(binop->lhs, symboltable);
    Symbol sym_rhs  = traverse_ast(binop->rhs, symboltable);
    return gen_binop(&codegen, sym_lhs, sym_rhs, binop->kind);
}

static Symbol ast_literal(ExprLiteral *literal, Hashtable *symboltable) {
    switch (literal->op.kind) {

        case TOK_NUMBER: {
            const char *str = literal->op.value;
            int64_t num = atoll(str);
            return gen_store_literal(&codegen, num, INTLITERAL);
        } break;

        case TOK_IDENTIFIER: {
            const char *variable = literal->op.value;
            Symbol *sym = symboltable_lookup(symboltable, variable);
            assert(sym != NULL);
            return *sym;
        } break;

        default:
            assert(!"Literal Unimplemented");
            break;
    }
}

static void ast_vardecl(StmtVarDecl *vardecl, Hashtable *symboltable) {
    const char *variable = vardecl->identifier.value;

    Symbol sym = traverse_ast(vardecl->value, symboltable);
    int    ret = hashtable_set(symboltable, variable, sym);
    assert(ret != -1);
}

static void ast_func(StmtFunc *func, Hashtable *symboltable) {
    gen_func_start(&codegen, func->identifier.value);
    traverse_ast(func->body, symboltable);
    gen_func_end(&codegen);
}

static void ast_block(Block *block) {
    assert(block->symboltable != NULL);

    AstNodeList list = block->stmts;
    for (size_t i=0; i < list.size; ++i)
        traverse_ast(list.items[i], block->symboltable);
}

static void ast_call(ExprCall *call, Hashtable *symboltable) {

    if (call->builtin == BUILTINFUNC_ASM) {
        builtin_inlineasm(call, symboltable);
        return;
    }

    AstNodeList list = call->args;
    for (size_t i=0; i < list.size; ++i)
        traverse_ast(list.items[i], symboltable);

    // HACK: call into address instead of identifier
    // size_t callee_addr = traverse_ast(call->callee, codegen, symboltable);
    gen_call(&codegen, call->callee->expr_literal.op.value);
}





// returns the location of the evaluated expression in memory
static Symbol traverse_ast(AstNode *node, Hashtable *symboltable) {

    switch (node->kind) {
        case ASTNODE_BLOCK:
            ast_block(&node->block);
            break;

        case ASTNODE_CALL:
            // TODO: return address of returnvalue
            ast_call(&node->expr_call, symboltable);
            break;

        case ASTNODE_GROUPING:
            return traverse_ast(node->expr_grouping.expr, symboltable);
            break;

        case ASTNODE_FUNC:
            ast_func(&node->stmt_func, symboltable);
            break;

        case ASTNODE_VARDECL:
            ast_vardecl(&node->stmt_vardecl, symboltable);
            break;

        case ASTNODE_BINOP:
            return ast_binop(&node->expr_binop, symboltable);
            break;

        case ASTNODE_UNARYOP:
            assert(!"Unimplemented");
            break;

        case ASTNODE_LITERAL:
            return ast_literal(&node->expr_literal, symboltable);
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    return (Symbol) {
        .kind = SYMBOLKIND_NONE,
    };

}

void generate_code(AstNode *root) {

    gen_init(&codegen, compiler_context.filename.asm, compiler_context.opts.debug_asm);

    gen_prelude(&codegen);
    traverse_ast(root, NULL);
    gen_postlude(&codegen);

    gen_destroy(&codegen);
}
