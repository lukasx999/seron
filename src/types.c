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
        case TOK_TYPE_VOID:
            return TYPE_VOID;
            break;
        default:
            assert(!"Unknown Token");
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
        case TYPE_VOID:
            return "void";
            break;
        case TYPE_FUNCTION:
            return "proc";
            break;
        default:
            assert(!"Unknown Type");
            break;
    }
}

size_t type_get_size(Type type) {
    switch (type) {
        case TYPE_BYTE:
            return 1;
            break;
        case TYPE_INT:
            return 4;
            break;
        case TYPE_SIZE:
            return 8;
            break;
        case TYPE_POINTER:
            return 8;
            break;
        default:
            assert(!"Unknown Type");
            break;
    }
}

const char *type_get_size_operand(Type type) {
    switch (type) {
        case TYPE_BYTE:
            return "byte";
            break;
        case TYPE_INT:
            return "dword";
            break;
        case TYPE_SIZE:
            return "qword";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}

const char *type_get_register_rax(Type type) {
    switch (type) {
        case TYPE_BYTE:
            return "al";
            break;
        case TYPE_INT:
            return "eax";
            break;
        case TYPE_SIZE:
            return "rax";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}

const char *type_get_register_rdi(Type type) {
    switch (type) {
        case TYPE_BYTE:
            return "dil";
            break;
        case TYPE_INT:
            return "edi";
            break;
        case TYPE_SIZE:
            return "rdi";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}


static Type traverse_ast(AstNode *root, Hashtable *scope);


static Type ast_assignment(ExprAssignment *assign, Hashtable *scope) {
    Type type = traverse_ast(assign->value, scope);

    const char *ident = assign->identifier.value;
    Symbol *sym = symboltable_lookup(scope, ident);

    if (sym == NULL)
        throw_error(assign->identifier, "Symbol `%s` does not exist", ident);

    if (sym->type != type) {
        throw_error(
            assign->op,
            "Invalid type %s, expected %s",
            type_to_string(type),
            type_to_string(sym->type)
        );
    }

    return type;
}

static void ast_vardecl(StmtVarDecl *vardecl, Hashtable *scope) {
    Type type = traverse_ast(vardecl->value, scope);

    if (vardecl->type != type) {
        throw_error(
            vardecl->op,
            "Invalid type %s, expected %s",
            type_to_string(type),
            type_to_string(vardecl->type)
        );
    }
}

static Type ast_call(ExprCall *call, Hashtable *scope) {

    AstNodeList list = call->args;
    for (size_t i=0; i < list.size; ++i)
        traverse_ast(list.items[i], scope);

    if (call->builtin != BUILTIN_NONE)
        return TYPE_VOID;


    Type callee = traverse_ast(call->callee, scope);

    if (callee != TYPE_FUNCTION)
        throw_error(call->op, "Callee must be a procedure");


    // TODO: get function signature

    // HACK: returntype
    /*
    const char *ident = call->callee->expr_literal.op.value;
    Symbol *sym = symboltable_lookup(scope, ident);
    assert(sym != NULL);
    Type returntype = sym->sig.returntype;
    return returntype;
    */

    return TYPE_VOID;
}

static Type ast_literal(ExprLiteral *literal, Hashtable *scope) {

    switch (literal->op.kind) {

        case TOK_IDENTIFIER: {
            const char *value = literal->op.value;
            Symbol *sym = symboltable_lookup(scope, value);

            if (sym == NULL)
                throw_error_simple("Symbol `%s` does not exist", value);

            return sym->type;
        } break;

        case TOK_NUMBER:
            return INTLITERAL;
            break;

        case TOK_STRING:
            return TYPE_POINTER;
            break;

        default:
            assert(!"Unknown Type");
            break;
    }
}


static Type traverse_ast(AstNode *root, Hashtable *scope) {
    assert(root != NULL);

    switch (root->kind) {

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

        case ASTNODE_IF:
            traverse_ast(root->stmt_if.condition, scope);
            traverse_ast(root->stmt_if.then_body, scope);
            if (root->stmt_if.else_body != NULL)
                traverse_ast(root->stmt_if.else_body, scope);
            break;

        case ASTNODE_BINOP: {
            const ExprBinOp *binop = &root->expr_binop;
            Type lhs = traverse_ast(binop->lhs, scope);
            Type rhs = traverse_ast(binop->rhs, scope);

            if (lhs != rhs)
                throw_error(
                    binop->op,
                    "Types do not match (`%s` and `%s`)",
                    type_to_string(lhs),
                    type_to_string(rhs)
                );

            return lhs;
        } break;

        case ASTNODE_UNARYOP: {
            const ExprUnaryOp *unaryop = &root->expr_unaryop;
            traverse_ast(unaryop->node, scope);
            // TODO: check operand type. eg: logical operators cannot be used on integers
        } break;

        case ASTNODE_LITERAL:
            return ast_literal(&root->expr_literal, scope);
            break;

        case ASTNODE_GROUPING:
            return traverse_ast(root->expr_grouping.expr, scope);
            break;

        case ASTNODE_ASSIGN:
            ast_assignment(&root->expr_assign, scope);
            break;

        case ASTNODE_PROCEDURE: {
            const StmtProcedure *func = &root->stmt_procedure;
            traverse_ast(func->body, scope);
        } break;

        case ASTNODE_VARDECL:
            ast_vardecl(&root->stmt_vardecl, scope);
            break;

        case ASTNODE_CALL:
            return ast_call(&root->expr_call, scope);
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    return TYPE_VOID;

}

void check_types(AstNode *root) {
    traverse_ast(root, NULL);
}
