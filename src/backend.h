#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "lexer.h"
#include "parser.h"


extern void generate_code(
    AstNode    *root,
    const char *filename_src,
    bool        print_comments
);



#endif // _CODEGEN_H
