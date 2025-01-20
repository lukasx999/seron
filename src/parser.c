#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "./util.h"
#include "./lexer.h"
#include "./parser.h"

#define SENTINEL TOK_INVALID






AstNodeList astnodelist_new(void) {
    AstNodeList list = {
        .capacity = 5,
        .size     = 0,
        .items    = NULL,
    };
    list.items = malloc(list.capacity * sizeof(AstNode*));
    return list;
}

void astnodelist_append(AstNodeList *list, AstNode *node) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(AstNode*));
    }
    list->items[list->size++] = node;
}

void astnodelist_destroy(AstNodeList *list) {
    free(list->items);
    list->items = NULL;
}




// TODO: remove assertions







typedef struct {
    size_t current;
    const TokenList *tokens;
} Parser;

static Token parser_get_current_token(const Parser *p) {
    const Token *tok = tokenlist_get(p->tokens, p->current);
    assert(tok != NULL);
    return *tok;
}

static void parser_advance(Parser *p) {
    p->current++;
}

static bool parser_is_at_end(const Parser *p) {
    return p->tokens->size == p->current;
}

// Checks if the current token is of one of the supplied kinds
// Last variadic argument should be `TOK_INVALID`
static bool parser_match_tokenkinds(const Parser *p, ...) {
    va_list va;
    va_start(va, p);

    while (1) {
        TokenKind tok = va_arg(va, TokenKind);
        bool matched = parser_get_current_token(p).kind == tok;
        if (matched || tok == SENTINEL) {
            va_end(va);
            return matched;
        }
    }

    assert(false);
}








static AstNode *rule_primary(Parser *p) {
    // primary ::= <number> | <identifier>

    AstNode *astnode = malloc(sizeof(AstNode));

    if (parser_get_current_token(p).kind == TOK_NUMBER ||
        parser_get_current_token(p).kind == TOK_IDENTIFIER ) {

        astnode->kind    = ASTNODE_LITERAL;
        astnode->expr_literal = (ExprLiteral) { .op = parser_get_current_token(p) };

        parser_advance(p);

    } else {
        assert(!"Unexpected Token");
    }

    return astnode;
}

static AstNode *rule_term(Parser *p) {
    // term ::= primary (("+" | "-") primary)*

    AstNode *lhs = rule_primary(p);

    while (parser_match_tokenkinds(p, TOK_PLUS, TOK_MINUS, SENTINEL)) {

        Token op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *rhs = rule_primary(p);

        AstNode *astnode = malloc(sizeof(AstNode));
        astnode->kind    = ASTNODE_BINOP;
        astnode->expr_binop   = (ExprBinOp) {
            .lhs = lhs,
            .op  = op,
            .rhs = rhs,
        };

        // FIX: get_current_token() out of bounds
        lhs = astnode;
    }

    return lhs;

}

static AstNode *rule_exprstmt(Parser *p) {
    // exprstmt ::= expr ";"

    AstNode *node = rule_term(p);
    assert(parser_match_tokenkinds(p, TOK_SEMICOLON, SENTINEL));
    parser_advance(p);
    return node;
}

static AstNode *rule_stmt(Parser *p) {
    // statement ::= block | function | if | while | expr ";"

    // TODO: add statements
    return rule_exprstmt(p);

}

static AstNode *rule_block(Parser *p) {
    // block ::= "{" statement* "}"

    assert(parser_match_tokenkinds(p, TOK_LBRACE, SENTINEL));
    parser_advance(p);

    AstNode *node = malloc(sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) { .stmts = astnodelist_new() };

    while (!parser_match_tokenkinds(p, TOK_RBRACE, SENTINEL)) {
        assert(!parser_is_at_end(p) && "Unmatching brace");
        astnodelist_append(&node->block.stmts, rule_stmt(p));
    }

    return node;
}

static AstNode *rule_program(Parser *p) {
    return rule_block(p);
}




AstNode *parser_parse(const TokenList *tokens) {
    Parser parser = { .current = 0, .tokens = tokens };
    AstNode *root = rule_program(&parser);
    return root;
}


void parser_traverse_ast(AstNode *root, AstCallback callback) {
    static int depth = 0;
    callback(root, depth);

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = root->block.stmts;
            depth++;
            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback);
        } break;

        case ASTNODE_FUNC:
            depth++;
            parser_traverse_ast(root->stmt_func.body, callback);
            break;

        case ASTNODE_WHILE:
            depth++;
            parser_traverse_ast(root->stmt_while.body, callback);
            break;

        case ASTNODE_IF:
            depth++;
            parser_traverse_ast(root->stmt_if.then_body, callback);
            parser_traverse_ast(root->stmt_if.else_body, callback);
            depth--;
            break;

        case ASTNODE_BINOP: {
            depth++;
            ExprBinOp binop = root->expr_binop;
            parser_traverse_ast(binop.lhs, callback);
            parser_traverse_ast(binop.rhs, callback);
            depth--;
        } break;

        case ASTNODE_LITERAL: {
        } break;

    }

}

void parser_print_ast_callback(AstNode *root, int depth) {
    const int spacing = 3; // TODO: pass this in through void* argument
    for (int i=0; i < depth * spacing; ++i)
        printf("%s⋅%s", COLOR_GRAY, COLOR_END);

    switch (root->kind) {
        case ASTNODE_BLOCK: {
            printf("block\n");
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = root->expr_binop;
            puts(tokenkind_to_string(binop.op.kind));
        } break;

        case ASTNODE_FUNC: {
            StmtFunc func = root->stmt_func;
            puts(tokenkind_to_string(func.op.kind));
        } break;

        case ASTNODE_IF: {
            StmtIf if_ = root->stmt_if;
            puts(tokenkind_to_string(if_.op.kind));
        } break;

        case ASTNODE_WHILE: {
            StmtWhile while_ = root->stmt_while;
            puts(tokenkind_to_string(while_.op.kind));
        } break;

        case ASTNODE_LITERAL: {
            Token tok = root->expr_literal.op;
            printf("%s", tokenkind_to_string(tok.kind));
            if (strcmp(tok.value, ""))
                printf("(%s)", tok.value);
            puts("");
        } break;
    }
}

void parser_print_ast(AstNode *root) {
    parser_traverse_ast(root, parser_print_ast_callback);
}



static void parser_free_ast_callback(AstNode *node, int _depth) {
    (void) _depth;

    switch (node->kind) {
        case ASTNODE_BLOCK:
            astnodelist_destroy(&node->block.stmts);
            break;

        case ASTNODE_IF:
        case ASTNODE_WHILE:
        case ASTNODE_LITERAL:
        case ASTNODE_FUNC:
        case ASTNODE_BINOP:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }
    free(node);
}

void parser_free_ast(AstNode *root) {
    parser_traverse_ast(root, parser_free_ast_callback);
}
