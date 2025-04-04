#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "parser.h"


void generate_code(AstNode *root);
void emit(AstNode *root);

#endif // _CODEGEN_H
