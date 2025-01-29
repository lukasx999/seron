#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "ast.h"
#include "symboltable.h"


/*
 * Construct a Symboltable from a given AST
 * This procedure only constructs the hierarchical layout,
 * filling in the addresses of the symbols is done in the backend
 */
extern Symboltable symboltable_construct(AstNode *root, size_t table_size);


#endif // _ANALYSIS_H
