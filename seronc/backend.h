#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "ast.h"

typedef enum {
    REG_RAX,
    REG_RDI,
    REG_RSI,
    REG_RDX,
    REG_RCX,
    REG_R8,
    REG_R9,
} Register;

extern void generate_code(AstNode *root);

#endif // _CODEGEN_H
