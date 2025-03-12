#ifndef _PARSER_H
#define _PARSER_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "lexer.h"
#include "ast.h"
#include "lib/arena.h"



#define SENTINEL TOK_INVALID

typedef struct {
    size_t current;
    const TokenList *tokens;
    Arena *arena;
} Parser;


Token *parser_get_current_token (const Parser *p);
void   parser_advance           (Parser *p);
void  *parser_alloc             (Parser *p, size_t size);
bool   parser_match_tokenkinds  (const Parser *p, ...);
void   parser_expect_token      (const Parser *p, TokenKind tokenkind, const char *expected);
void   parser_throw_error       (const Parser *p, const char *msg);
bool   parser_is_at_end         (const Parser *p);


/* Public API */

// Allocate an AST into the given arena
AstNode *parser_parse             (const TokenList *tokens, Arena *arena);

typedef void (*AstCallback) (AstNode *node, int depth, void *args);

// Call the given callback function for every node in the AST
void     parser_traverse_ast      (AstNode *root, AstCallback callback, bool top_down, void *args);

// Call the given callback function for every node of kind `kind` in the AST
void     parser_query_ast         (AstNode *root, AstCallback callback, AstNodeKind kind, void *args);

// TODO:
// Given a map of AstNodeKind enums to function pointers, call each function
// for the corresponding AstNode.
// map: array of ast callbacks, where the index is the AstNodeKind enum as a
// designated initializer. Unused indexes should hold NULL as a value.
/*void     parser_map_ast           (AstNode *root, AstCallback callback, AstCallback *table, size_t table_len, void *args);*/

void     parser_print_ast         (AstNode  *root, int spacing);


/* ---------- */



#endif // _PARSER_H
