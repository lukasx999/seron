#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symboltable.h"


static void traverse_ast(const AstNode *root, Hashtable *parent, Symboltable *st) {

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = root->block.stmts;

            symboltable_append(st, parent);
            parent = symboltable_get_last(st);

            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], parent, st);


        } break;

        case ASTNODE_CALL: {
            ExprCall call = root->expr_call;

            traverse_ast(call.callee, parent, st);

            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], parent, st);

        } break;

        case ASTNODE_GROUPING:
            traverse_ast(root->expr_grouping.expr, parent, st);
            break;

        case ASTNODE_FUNC:
            traverse_ast(root->stmt_func.body, parent, st);
            break;

        case ASTNODE_VARDECL:
            traverse_ast(root->stmt_vardecl.value, parent, st);
            break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = root->expr_binop;
            traverse_ast(binop.lhs, parent, st);
            traverse_ast(binop.rhs, parent, st);
        } break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp unaryop = root->expr_unaryop;
            traverse_ast(unaryop.node, parent, st);
        } break;

        case ASTNODE_LITERAL:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }


}

Symboltable symboltable_construct(const AstNode *root, size_t table_size) {
    Symboltable st = { 0 };
    symboltable_init(&st, table_size);

    traverse_ast(root, NULL, &st);

    symboltable_print(&st);
    return st;
}
