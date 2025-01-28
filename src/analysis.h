#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include "ast.h"
#include "symboltable.h"


/* Construct a Symboltable from a given AST */
extern Symboltable symboltable_construct(AstNode *root, size_t table_size);


#endif // _ANALYSIS_H
