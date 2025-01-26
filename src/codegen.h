#ifndef _CODEGEN_H
#define _CODEGEN_H



#include "lexer.h"
#include "parser.h"


typedef struct {
    FILE *file;
    size_t rbp_offset;
    bool print_comments;
    const char *filename_src;
} CodeGenerator;

extern void generate_code(AstNode *root, const char *filename_asm, bool print_comments, const char *filename_src);



#endif // _CODEGEN_H
