#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "parser.h"

size_t typekind_get_size(TypeKind type);

void codegen(AstNode *root);

#endif // _CODEGEN_H
