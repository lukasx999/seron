#ifndef _PARSER_H
#define _PARSER_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "lexer.h"
#include "ast.h"



#define SENTINEL TOK_INVALID

typedef struct {
    size_t current;
    const TokenList *tokens;
} Parser;

typedef void (*AstCallback)(AstNode *node, int depth, void *args);

Token *parser_get_current_token(const Parser *p);
void parser_advance            (Parser *p);
bool parser_match_tokenkinds   (const Parser *p, ...);
void parser_expect_token       (const Parser *p, TokenKind tokenkind, const char *expected);
void parser_throw_error        (const Parser *p, const char *msg);
bool parser_is_at_end          (const Parser *p);
void parser_traverse_ast       (AstNode *root, AstCallback callback, bool top_down, void *args);
void parser_print_ast(AstNode  *root, int spacing);
void parser_free_ast           (AstNode *root);

AstNode *parser_parse(const TokenList *tokens);



#endif // _PARSER_H
