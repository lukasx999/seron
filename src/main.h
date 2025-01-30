#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>



/*
This structure is global because it would be very annoying to pass
the compiler context to every single recursive function call when traversing the AST,
hence using a global variable makes code cleaner (imo)
*/
struct CompilerContext {
    struct {
        const char *raw; // filename with extension
        char stripped[256]; // stripped of suffix
        char asm[256];
        char obj[256];
    } filename;
    bool show_warnings;
    bool debug_asm;
    bool dump_ast, dump_tokens, dump_symboltable;
} extern compiler_context;



#endif // _MAIN_H
