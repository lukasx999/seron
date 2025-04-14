#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "types.h"
#include "parser.h"





static void compare_types(const Type *type, const Type *expected, Token tok) {
    if (type->kind != expected->kind) {
        compiler_message_tok(
            MSG_ERROR, tok,
            "Invalid type %s, expected %s",
            typekind_to_string(type->kind),
            typekind_to_string(expected->kind)
        );
        exit(EXIT_FAILURE);
    }
}



static Type traverse_ast(AstNode *root, Hashtable *scope);


static Type ast_assignment(ExprAssignment *assign, Hashtable *scope) {
    Type type = traverse_ast(assign->value, scope);

    const char *ident = assign->identifier.value;
    Symbol *sym = symboltable_list_lookup(scope, ident);

    if (sym == NULL) {
        compiler_message_tok(
            MSG_ERROR,
            assign->identifier,
            "Symbol `%s` does not exist",
            ident
        );
        exit(EXIT_FAILURE);
    }

    compare_types(&sym->type, &type, assign->op);

    return type;
}

static void ast_vardecl(StmtVarDecl *vardecl, Hashtable *scope) {
    if (vardecl->init == NULL)
        return;

    Type type     = traverse_ast(vardecl->init, scope);
    Type expected = vardecl->type;
    compare_types(&type, &expected, vardecl->op);
}

static Type ast_call(ExprCall *call, Hashtable *scope) {

    Type callee = traverse_ast(call->callee, scope);
    ProcSignature *sig = &callee.type_signature;

    if (callee.kind != TYPE_FUNCTION) {
        compiler_message_tok(MSG_ERROR, call->op, "Callee must be a procedure");
        exit(EXIT_FAILURE);
    }


    AstNodeList list = call->args;

    if (list.size != sig->params_count) {
        compiler_message_tok(MSG_ERROR, call->op, "Expected %lu arguments, got %lu", sig->params_count, list.size);
        exit(EXIT_FAILURE);
    }

    for (size_t i=0; i < list.size; ++i) {
        Type *paramtype = sig->params[i].type;
        Type type = traverse_ast(list.items[i], scope);
        compare_types(&type, paramtype, call->op);
    }

    return *sig->returntype;
}

static Type ast_literal(ExprLiteral *literal, Hashtable *scope) {

    switch (literal->op.kind) {

        case TOK_IDENTIFIER: {
            const char *value = literal->op.value;
            Symbol *sym = symboltable_list_lookup(scope, value);

            if (sym == NULL) {
                compiler_message(MSG_ERROR, "Symbol `%s` does not exist", value);
                exit(1);
            }

            return sym->type;
        } break;

        case TOK_NUMBER:
            return (Type) { .kind = TYPE_INT };
            break;

        case TOK_STRING:
            return (Type) { .kind = TYPE_POINTER };
            break;

        default:
            assert(!"Unknown Type");
            break;
    }
}

static Type ast_binop(ExprBinOp *binop, Hashtable *scope) {
    Type lhs = traverse_ast(binop->lhs, scope);
    Type rhs = traverse_ast(binop->rhs, scope);
    compare_types(&lhs, &rhs, binop->op);
    return lhs;
}

static Type traverse_ast(AstNode *root, Hashtable *scope) {
    assert(root != NULL);

    switch (root->kind) {

        case ASTNODE_LITERAL:
            return ast_literal(&root->expr_literal, scope);
            break;

        case ASTNODE_BINOP:
            return ast_binop(&root->expr_binop, scope);
            break;

        case ASTNODE_ASSIGN:
            return ast_assignment(&root->expr_assign, scope);
            break;

        case ASTNODE_VARDECL:
            ast_vardecl(&root->stmt_vardecl, scope);
            break;

        case ASTNODE_CALL:
            return ast_call(&root->expr_call, scope);
            break;

        case ASTNODE_UNARYOP: {
            const ExprUnaryOp *unaryop = &root->expr_unaryop;
            traverse_ast(unaryop->node, scope);
            // TODO: check operand type. eg: logical operators cannot be used on integers
        } break;

        case ASTNODE_BLOCK: {
            Block *block     = &root->block;
            AstNodeList list = block->stmts;

            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], block->symboltable);
        } break;

        case ASTNODE_WHILE:
            traverse_ast(root->stmt_while.condition, scope);
            traverse_ast(root->stmt_while.body, scope);
            break;

        case ASTNODE_RETURN:
            traverse_ast(root->stmt_return.expr, scope);
            break;

        case ASTNODE_IF:
            traverse_ast(root->stmt_if.condition, scope);
            traverse_ast(root->stmt_if.then_body, scope);
            if (root->stmt_if.else_body != NULL)
                traverse_ast(root->stmt_if.else_body, scope);
            break;

        case ASTNODE_GROUPING:
            return traverse_ast(root->expr_grouping.expr, scope);
            break;

        case ASTNODE_PROCEDURE: {
            const StmtProc *func = &root->stmt_proc;
            AstNode *body = func->body;
            if (body != NULL)
                traverse_ast(body, scope);
        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    return (Type) { .kind = TYPE_VOID };

}

void check_types(AstNode *root) {
    traverse_ast(root, NULL);
}
