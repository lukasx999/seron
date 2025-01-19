#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./util.h"
#include "./lexer.h"
#include "./parser.h"







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

static Token get_current_token(const Parser *p) {
    return *tokenlist_get(p->tokens, p->current);
}

static void parser_advance(Parser *p) {
    p->current++;
}



void print_ast(const AstNode *root) {
    static int indent = 0;

    const int spacing = 3;
    for (int i=0; i < indent*spacing; ++i)
        printf("%s⋅%s", COLOR_GRAY, COLOR_END);


    switch (root->kind) {

        case ASTNODE_BLOCK: {
            printf("block\n");
            AstNodeList list = root->block.stmts;
            indent++;
            for (size_t i=0; i < list.size; ++i)
                print_ast(list.items[i]);
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = root->expr_binop;
            const char *repr = tokenkind_to_string(binop.op.kind);
            puts(repr);
            indent++;
            print_ast(binop.lhs);
            print_ast(binop.rhs);
            indent--;
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






static AstNode *rule_primary(Parser *p) {
    // primary ::= <number> | <identifier>

    AstNode *astnode = malloc(sizeof(AstNode));

    if (get_current_token(p).kind == TOK_NUMBER ||
        get_current_token(p).kind == TOK_IDENTIFIER ) {

        astnode->kind    = ASTNODE_LITERAL;
        astnode->expr_literal = (ExprLiteral) { .op = get_current_token(p) };

        parser_advance(p);

    } else {
        assert(!"Unexpected Token");
    }

    return astnode;
}

static AstNode *rule_term(Parser *p) {
    // term ::= primary (("+" | "-") primary)*

    AstNode *lhs = rule_primary(p);

    while (get_current_token(p).kind == TOK_PLUS ||
        get_current_token(p).kind == TOK_MINUS) {

        Token op = get_current_token(p);
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
    assert(get_current_token(p).kind == TOK_SEMICOLON);
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

    assert(get_current_token(p).kind == TOK_LBRACE);
    parser_advance(p);

    AstNode *node = malloc(sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) { .stmts = astnodelist_new() };

    while (get_current_token(p).kind != TOK_RBRACE) {
        astnodelist_append(&node->block.stmts, rule_stmt(p));
    }

    return node;
}

static AstNode *rule_program(Parser *p) {
    return rule_block(p);
}




AstNode *parse(const TokenList *tokens) {
    Parser parser = { .current = 0, .tokens = tokens };

    AstNode *root = rule_program(&parser);

    return root;
}
