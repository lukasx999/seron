#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "analysis.h"



static void check_global_scope(const AstNode *root) {

    assert(root->kind == ASTNODE_BLOCK);
    assert(root->block.global);
    AstNodeList globals = root->block.stmts;

    AstNodeKind allowed_kinds[] = {
        ASTNODE_FUNC,
        ASTNODE_VARDECL,
        ASTNODE_INLINEASM,
    };

    for (size_t i=0; i < globals.size; ++i) {
        AstNode *node = globals.items[i];

        bool found = false;
        for (size_t j=0; j < ARRAY_LEN(allowed_kinds); ++j) {
            if (node->kind == allowed_kinds[j]) {
                found = true;
                break;
            }
        }

        if (!found)
            throw_error_simple("Cant do that in global scope");

    }

}



// TODO: type checker returns type of expression

static void typechecking(AstNode *node) {

    switch (node->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = node->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                typechecking(list.items[i]);
        } break;

        case ASTNODE_CALL: {
            ExprCall call    = node->expr_call;
            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                typechecking(list.items[i]);
        } break;

        case ASTNODE_GROUPING:
            typechecking(node->expr_grouping.expr);
            break;

        case ASTNODE_FUNC: {
            StmtFunc func = node->stmt_func;
            typechecking(func.body);
        } break;

        case ASTNODE_INLINEASM: {
            StmtInlineAsm inlineasm = node->stmt_inlineasm;
        } break;

        case ASTNODE_VARDECL:
            typechecking(node->stmt_vardecl.value);
            break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = node->expr_binop;
            typechecking(binop.lhs);
            typechecking(binop.rhs);

        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral literal = node->expr_literal;

        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

}





void check_semantics(AstNode *root) {
    check_global_scope(root);
    typechecking(root);
}
