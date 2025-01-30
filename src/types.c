#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "types.h"
#include "symboltable.h"
#include "ast.h"



Type type_from_tokenkind(TokenKind kind) {
    switch (kind) {
        case TOK_TYPE_INT:
            return TYPE_INT;
            break;
        case TOK_TYPE_SIZE:
            return TYPE_SIZE;
            break;
        case TOK_TYPE_BYTE:
            return TYPE_BYTE;
            break;
        default:
            return TYPE_INVALID;
            break;
    }
}

const char *type_to_string(Type type) {
    switch (type) {
        case TYPE_BYTE:
            return "byte";
            break;
        case TYPE_INT:
            return "int";
            break;
        case TYPE_SIZE:
            return "size";
            break;
        case TYPE_POINTER:
            return "pointer";
            break;
        case TYPE_FUNCTION:
            return "function";
            break;
        default:
            return NULL;
            break;
    }
}



static Type traverse_ast(AstNode *root, Hashtable *symboltable) {

    switch (root->kind) {
        case ASTNODE_BLOCK: {
            Block *block     = &root->block;
            AstNodeList list = block->stmts;

            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], symboltable);

        } break;

        case ASTNODE_FUNC: {
            const StmtFunc *func = &root->stmt_func;

            traverse_ast(func->body, symboltable);
        } break;

        case ASTNODE_VARDECL: {
            const StmtVarDecl *vardecl = &root->stmt_vardecl;

            traverse_ast(vardecl->value, symboltable);
        } break;

        case ASTNODE_CALL: {
            ExprCall call = root->expr_call;
            traverse_ast(call.callee, symboltable);

            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], symboltable);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(root->expr_grouping.expr, symboltable);
            break;

        case ASTNODE_BINOP: {
            const ExprBinOp *binop = &root->expr_binop;
            Type lhs = traverse_ast(binop->lhs, symboltable);
            Type rhs = traverse_ast(binop->rhs, symboltable);

            if (lhs != rhs)
                throw_error_simple(
                    "Types do not match (`%s` and `%s`)",
                    type_to_string(lhs),
                    type_to_string(rhs)
                );

        } break;

        case ASTNODE_UNARYOP: {
            const ExprUnaryOp *unaryop = &root->expr_unaryop;
            traverse_ast(unaryop->node, symboltable);
        } break;

        case ASTNODE_LITERAL: {
            const ExprLiteral* literal = &root->expr_literal;
            Symbol *sym = symboltable_lookup(symboltable, literal->op.value);
            assert(sym != NULL);
            return sym->type;
        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

}

void check_types(AstNode *root) {
    traverse_ast(root, NULL);
}
