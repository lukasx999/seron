#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./util.h"
#include "./lexer.h"
#include "./parser.h"


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


static AstNode *rule_primary(Parser *p) {
    // primary ::= <number> | <identifier>

    AstNode *astnode = malloc(sizeof(AstNode));

    if (get_current_token(p).kind == TOK_NUMBER ||
        get_current_token(p).kind == TOK_IDENTIFIER ) {

        astnode->kind    = ASTNODE_LITERAL;
        astnode->literal = (Literal) { .op = get_current_token(p) };

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
        astnode->binop   = (BinOp) {
            .lhs = lhs,
            .op  = op,
            .rhs = rhs,
        };

        // FIX: get_current_token() out of bounds
        lhs = astnode;
    }

    return lhs;

}



AstNode *parse(const TokenList *tokens) {

    Parser parser = {
        .current = 0,
        .tokens  = tokens,
    };

    AstNode *root = rule_term(&parser);
    return root;

}
