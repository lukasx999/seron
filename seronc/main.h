#ifndef _MAIN_H
#define _MAIN_H

#include <stddef.h>
#include <stdbool.h>


#define MAX_FILENAME_SIZE 256

/*
This structure is global as it would be very annoying to pass
the compiler context to every single recursive function call when traversing the AST,
hence using a global variable makes code cleaner (imo)
*/
struct CompilerContext {

    struct {
        const char *raw; // filename with extension
        char stripped[MAX_FILENAME_SIZE]; // stripped of suffix
        char asm_[MAX_FILENAME_SIZE];
        char obj[MAX_FILENAME_SIZE];
    } filename;

    // options are ints, because `struct option` only accept int pointers
    struct {
        int debug_asm;
        int dump_ast;
        int dump_tokens;
        int dump_symboltable;
        int verbose; // Show info messages
        int compile_only;
        int compile_and_assemble;
    } opts;

};

extern struct CompilerContext compiler_context;



#endif // _MAIN_H
