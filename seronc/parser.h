#ifndef _PARSER_H
#define _PARSER_H

#include <stddef.h>
#include <stdbool.h>

#include "lexer.h"
#include "ast.h"
#include "lib/arena.h"



// Allocate an AST into the given arena
AstNode *parse(Token *tokens, Arena *arena);

typedef void (*AstCallback) (AstNode *node, int depth, void *args);

// Call the given callback function for every node in the AST
void parser_traverse_ast(AstNode *root, AstCallback callback, bool top_down, void *args);
// Call the given callback function for every node of kind `kind` in the AST
void parser_query_ast(AstNode *root, AstCallback callback, AstNodeKind kind, void *args);
void parser_print_ast(AstNode *root, int spacing);



#endif // _PARSER_H
