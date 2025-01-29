#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "ast.h"
#include "symboltable.h"

// TODO: ast traversal context struct

static void traverse_ast(AstNode *root, Hashtable *parent, Symboltable *st) {

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            Block *block     = &root->block;
            AstNodeList list = block->stmts;

            symboltable_append(st, parent);
            Hashtable *last = symboltable_get_last(st);
            assert(last != NULL);
            block->symboltable = last;

            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], last, st);

        } break;

        case ASTNODE_FUNC: {
            const StmtFunc *func = &root->stmt_func;
            const char *ident = func->identifier.value;

            // TODO: value is label
            hashtable_insert(parent, ident, 0);

            traverse_ast(func->body, parent, st);
        } break;

        case ASTNODE_VARDECL: {
            const StmtVarDecl *vardecl = &root->stmt_vardecl;
            const char *ident = vardecl->identifier.value;
            // TODO: generate addresses here or fill them in at codegen?

            int ret = hashtable_insert(parent, ident, 0);
            if (ret == -1) {
                // TODO: add shadowing feature
                throw_warning_simple("Variable `%s` already exists", ident);
            }

            traverse_ast(vardecl->value, parent, st);
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

        case ASTNODE_BINOP: {
            const ExprBinOp *binop = &root->expr_binop;
            traverse_ast(binop->lhs, parent, st);
            traverse_ast(binop->rhs, parent, st);
        } break;

        case ASTNODE_UNARYOP: {
            const ExprUnaryOp *unaryop = &root->expr_unaryop;
            traverse_ast(unaryop->node, parent, st);
        } break;

        case ASTNODE_LITERAL: {
            const ExprLiteral* literal = &root->expr_literal;

            if (literal->op.kind == TOK_IDENTIFIER) {
                const char *variable = literal->op.value;

                // TODO: refactor
                // Builtin functions
                if (!strcmp(variable, "asm"))
                    break;

                HashtableValue *addr = hashtable_lookup(parent, variable);

                if (addr == NULL)
                    throw_error_simple("Symbol `%s` does not exist", variable);
            }

        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

}

Symboltable symboltable_construct(AstNode *root, size_t table_size) {
    Symboltable st = { 0 };
    symboltable_init(&st, table_size);

    traverse_ast(root, NULL, &st);

    return st;
}
