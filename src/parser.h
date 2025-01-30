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

extern Token parser_get_current_token (const Parser *p);
extern void  parser_advance           (Parser *p);
extern bool  parser_match_tokenkinds  (const Parser *p, ...);
extern void  parser_expect_token      (const Parser *p, TokenKind tokenkind, const char *expected);
extern void  parser_throw_error       (const Parser *p, const char *msg);
extern bool  parser_is_at_end         (const Parser *p);
extern void  parser_traverse_ast      (AstNode *root, AstCallback callback, bool top_down, void *args);
extern void  parser_print_ast         (AstNode *root);
extern void  parser_free_ast          (AstNode *root);

extern AstNode *parser_parse(const TokenList *tokens);



#endif // _PARSER_H
