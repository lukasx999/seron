#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "parser.h"


size_t get_type_size(TypeKind type);

void codegen(AstNode *root);

#endif // _CODEGEN_H
