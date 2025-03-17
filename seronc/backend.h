#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "ast.h"

typedef enum {
    REG_RAX, // operand 1
    REG_RDI, // operand 2
    REG_RSI, // out 1
    REG_RDX, // out 2
    REG_RCX,
    REG_R8,
    REG_R9,
} Register;

extern void generate_code(AstNode *root);

#endif // _CODEGEN_H
