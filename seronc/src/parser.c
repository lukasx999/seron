#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "ast.h"
#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "grammar.h"




Token *parser_get_current_token(const Parser *p) {
    Token *tok = tokenlist_get(p->tokens, p->current);
    assert(tok != NULL);
    return tok;
}

void parser_advance(Parser *p) {
    p->current++;
}

// Checks if the current token is of one of the supplied kinds
// Last variadic argument should be `TOK_INVALID`
bool parser_match_tokenkinds(const Parser *p, ...) {
    va_list va;
    va_start(va, p);

    while (1) {
        TokenKind tok = va_arg(va, TokenKind);
        bool matched = parser_get_current_token(p)->kind == tok;
        if (matched || tok == SENTINEL) {
            va_end(va);
            return matched;
        }
    }

    assert(!"unreachable");
}

void parser_expect_token(
    const Parser *p,
    TokenKind tokenkind,
    const char *expected
) {
    if (!parser_match_tokenkinds(p, tokenkind, SENTINEL)) {
        Token *tok = parser_get_current_token(p);
        throw_error(*tok, "Expected `%s`", expected);
    }
}

void parser_throw_error(const Parser *p, const char *msg) {
    Token *tok = parser_get_current_token(p);
    throw_error(*tok, "%s", msg);
}

bool parser_is_at_end(const Parser *p) {
    return parser_match_tokenkinds(p, TOK_EOF, SENTINEL);
}

void parser_traverse_ast(AstNode *root, AstCallback callback, bool top_down, void *args) {
    assert(root != NULL);

    static int depth = 0;
    if (top_down)
        callback(root, depth, args);

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            depth++;
            AstNodeList list = root->block.stmts;

            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback, top_down, args);

            depth--;
        } break;

        case ASTNODE_ASSIGN: {
            depth++;
            parser_traverse_ast(root->expr_assign.value, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_CALL: {
            depth++;
            ExprCall *call = &root->expr_call;

            if (call->builtin == BUILTIN_NONE)
                parser_traverse_ast(call->callee, callback, top_down, args);

            AstNodeList list = call->args;
            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback, top_down, args);

            depth--;
        } break;

        case ASTNODE_WHILE: {
            depth++;
            StmtWhile *while_ = &root->stmt_while;
            parser_traverse_ast(while_->condition, callback, top_down, args);
            parser_traverse_ast(while_->body, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_RETURN: {
            depth++;
            StmtReturn *return_ = &root->stmt_return;
            parser_traverse_ast(return_->expr, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_IF: {
            depth++;
            StmtIf if_ = root->stmt_if;
            parser_traverse_ast(if_.condition, callback, top_down, args);
            parser_traverse_ast(if_.then_body, callback, top_down, args);
            if (if_.else_body != NULL)
                parser_traverse_ast(if_.else_body, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_GROUPING:
            depth++;
            parser_traverse_ast(root->expr_grouping.expr, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_PROCEDURE:
            depth++;
            parser_traverse_ast(root->stmt_procedure.body, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_VARDECL:
            depth++;
            parser_traverse_ast(root->stmt_vardecl.value, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_BINOP: {
            depth++;
            ExprBinOp *binop = &root->expr_binop;
            parser_traverse_ast(binop->lhs, callback, top_down, args);
            parser_traverse_ast(binop->rhs, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_UNARYOP: {
            depth++;
            ExprUnaryOp *unaryop = &root->expr_unaryop;
            parser_traverse_ast(unaryop->node, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_LITERAL:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    if (!top_down)
        callback(root, depth, args);

}

// Prints a value in the following format: `<str>: <arg>`
// <arg> is omitted if arg == NULL
static void print_ast_value(const char *str, const char *color, const char *value, const char *opt) {
    printf("%s%s%s", color, str, COLOR_END);
    if (value != NULL)
        printf("%s: %s%s", COLOR_GRAY, value, COLOR_END);
    if (opt != NULL)
        printf(" %s(%s)%s", COLOR_GRAY, opt, COLOR_END);
    puts("");
}

static void parser_print_ast_callback(AstNode *root, int depth, void *args) {
    assert(root != NULL);
    int spacing = *(int*) args;

    for (int _=0; _ < depth * spacing; ++_)
        printf("%s⋅%s", COLOR_GRAY, COLOR_END);

    switch (root->kind) {
        case ASTNODE_BLOCK: {
            print_ast_value(
                "block",
                COLOR_BLUE,
                NULL,
                NULL
            );
        } break;

        case ASTNODE_GROUPING: {
            print_ast_value("grouping", COLOR_BLUE, NULL, NULL);
        } break;

        case ASTNODE_ASSIGN: {
            print_ast_value("assign", COLOR_RED, root->expr_assign.identifier.value, NULL);
        } break;

        case ASTNODE_IF: {
            print_ast_value("if", COLOR_RED, NULL, NULL);
        } break;

        case ASTNODE_WHILE: {
            print_ast_value("while", COLOR_RED, NULL, NULL);
        } break;

        case ASTNODE_RETURN: {
            print_ast_value("return", COLOR_RED, NULL, NULL);
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp *binop = &root->expr_binop;
            print_ast_value(tokenkind_to_string(binop->op.kind), COLOR_PURPLE, NULL, NULL);
        } break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp *unaryop = &root->expr_unaryop;
            print_ast_value(tokenkind_to_string(unaryop->op.kind), COLOR_PURPLE, NULL, NULL);
        } break;

        case ASTNODE_CALL: {
            ExprCall *call = &root->expr_call;
            print_ast_value(
                "call",
                COLOR_BLUE,
                NULL,
                call->builtin != BUILTIN_NONE ? "builtin" : NULL
            );
        } break;

        case ASTNODE_PROCEDURE: {
            StmtProcedure *func = &root->stmt_procedure;
            print_ast_value(tokenkind_to_string(func->op.kind), COLOR_RED, func->identifier.value, NULL);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &root->stmt_vardecl;
            print_ast_value(
                tokenkind_to_string(vardecl->op.kind),
                COLOR_RED,
                vardecl->identifier.value,
                typekind_to_string(vardecl->type.kind)
            );
        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral *literal = &root->expr_literal;
            Token *tok = &literal->op;
            print_ast_value(tokenkind_to_string(tok->kind), COLOR_RED, tok->value, NULL);
        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }
}

void parser_print_ast(AstNode *root, int spacing) {
    printf("\n");
    parser_traverse_ast(root, parser_print_ast_callback, true, &spacing);
    printf("\n");
}

static void free_signature(ProcSignature *sig) {
    free(sig->returntype);
    for (size_t i=0; i < sig->params_count; ++i)
        free(sig->params[i].type);
}

static void parser_free_ast_callback(AstNode *node, int _depth, void *_args) {
    assert(node != NULL);
    (void) _depth;
    (void) _args;

    switch (node->kind) {
        case ASTNODE_BLOCK:
            astnodelist_destroy(&node->block.stmts);
            break;

        case ASTNODE_CALL:
            astnodelist_destroy(&node->expr_call.args);
            break;

        case ASTNODE_PROCEDURE: {
            StmtProcedure *proc = &node->stmt_procedure;
            assert(proc->type.kind == TYPE_FUNCTION);
            free_signature(&proc->type.type_signature);
        } break;

        case ASTNODE_GROUPING:
        case ASTNODE_ASSIGN:
        case ASTNODE_LITERAL:
        case ASTNODE_VARDECL:
        case ASTNODE_BINOP:
        case ASTNODE_UNARYOP:
        case ASTNODE_IF:
        case ASTNODE_WHILE:
        case ASTNODE_RETURN:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }
    free(node);
}

void parser_free_ast(AstNode *root) {
    parser_traverse_ast(root, parser_free_ast_callback, false, NULL);
}

AstNode *parser_parse(const TokenList *tokens) {
    Parser parser = {
        .current  = 0,
        .tokens   = tokens,
    };
    AstNode *root = rule_program(&parser);
    return root;
}
