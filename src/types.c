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
        case TYPE_BUILTIN:
            return "builtin";
            break;
        case TYPE_FUNCTION:
            return "function";
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




static Type traverse_ast(AstNode *root, Hashtable *symboltable) {

    switch (root->kind) {
        case ASTNODE_BLOCK: {
            Block *block     = &root->block;
            AstNodeList list = block->stmts;

            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], block->symboltable);
        } break;

        case ASTNODE_FUNC: {
            const StmtFunc *func = &root->stmt_func;
            traverse_ast(func->body, symboltable);
        } break;

        case ASTNODE_VARDECL: {
            const StmtVarDecl *vardecl = &root->stmt_vardecl;
            Type type = traverse_ast(vardecl->value, symboltable);

            if (vardecl->type != type)
                throw_error(
                    &vardecl->op,
                    "Type annotation does not match assigned expression (`%s` and `%s`)",
                    type_to_string(vardecl->type),
                    type_to_string(type)
                );
        } break;

        case ASTNODE_CALL: {
            ExprCall *call = &root->expr_call;
            Type callee = traverse_ast(call->callee, symboltable);

            // TODO: function pointers
            if (callee != TYPE_FUNCTION && callee != TYPE_BUILTIN)
                throw_error(&call->op, "Callee must be a procedure or builtin");

            AstNodeList list = call->args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], symboltable);
        } break;

        case ASTNODE_GROUPING:
            return traverse_ast(root->expr_grouping.expr, symboltable);
            break;

        case ASTNODE_BINOP: {
            const ExprBinOp *binop = &root->expr_binop;
            Type lhs = traverse_ast(binop->lhs, symboltable);
            Type rhs = traverse_ast(binop->rhs, symboltable);

            if (lhs != rhs)
                throw_error(
                    &binop->op,
                    "Types do not match (`%s` and `%s`)",
                    type_to_string(lhs),
                    type_to_string(rhs)
                );

        } break;

        case ASTNODE_UNARYOP: {
            const ExprUnaryOp *unaryop = &root->expr_unaryop;
            traverse_ast(unaryop->node, symboltable);
            // TODO: check operand type. eg: logical operators cannot be used on integers
        } break;

        case ASTNODE_LITERAL: {
            const ExprLiteral* literal = &root->expr_literal;

            switch (literal->op.kind) {

                case TOK_IDENTIFIER: {
                    const char *value = literal->op.value;

                    if (string_to_builtinfunc(value) != BUILTINFUNC_NONE)
                        return TYPE_BUILTIN;

                    Symbol *sym = symboltable_lookup(symboltable, value);
                    assert(sym != NULL);
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

        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

}

void check_types(AstNode *root) {
    traverse_ast(root, NULL);
}
