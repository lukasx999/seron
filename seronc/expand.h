#ifndef _EXPAND_H
#define _EXPAND_H

#include <arena.h>
#include "parser.h"

// expand various syntax sugars

void expand_ast(AstNode *root, Arena *arena);

#endif // _EXPAND_H
