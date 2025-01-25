#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "hashtable.h"
#include "asm.h"








// returns the location of the evaluated expression in memory
static size_t traverse_ast(AstNode *node, CodeGenerator *codegen, Hashtable *symboltable) {
    // TODO: free registers and keep track of allocated registers

    switch (node->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = node->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, symboltable);
        } break;

        case ASTNODE_CALL: {
            ExprCall call    = node->expr_call;
            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, symboltable);

            // HACK: call into address instead of identifier
            gen_call(codegen, call.callee->expr_literal.op.value);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(node->expr_grouping.expr, codegen, symboltable);
            break;

        case ASTNODE_FUNC: {
            StmtFunc func = node->stmt_func;
            gen_func_start(codegen, func.identifier.value);
            traverse_ast(func.body, codegen, symboltable);
            gen_func_end(codegen);
        } break;

        case ASTNODE_INLINEASM: {
            StmtInlineAsm inlineasm = node->stmt_inlineasm;
            gen_inlineasm(codegen, inlineasm.src.value);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &node->stmt_vardecl;

            size_t addr = traverse_ast(vardecl->value, codegen, symboltable);
            hashtable_insert(symboltable, vardecl->identifier.value, addr);
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = node->expr_binop;
            size_t addr_lhs = traverse_ast(binop.lhs, codegen, symboltable);
            size_t addr_rhs = traverse_ast(binop.rhs, codegen, symboltable);

            switch (binop.op.kind) {
                case TOK_PLUS: {
                    gen_addition(codegen, addr_lhs, addr_rhs);
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
                    int num = atoi(literal->op.value);
                    assert(num != 0);
                    gen_store_value(codegen, num, INTTYPE_INT);
                } break;

                case TOK_IDENTIFIER: {
                    HashtableValue *addr = hashtable_get(symboltable, literal->op.value);
                    assert(addr != NULL);
                    // TODO: handle type
                    gen_copy_value(codegen, *addr, INTTYPE_INT);
                } break;

                default:
                    assert(!"Unimplemented");
                    break;
            }
        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    return codegen->rbp_offset;

}

void generate_code(AstNode *root, const char *filename, bool print_comments) {
    CodeGenerator codegen = gen_new(filename, print_comments);

    Hashtable symboltable = hashtable_new();

    gen_prelude(&codegen);
    traverse_ast(root, &codegen, &symboltable);
    gen_postlude(&codegen);

    hashtable_print(&symboltable);
    hashtable_destroy(&symboltable);

    gen_destroy(&codegen);
}
