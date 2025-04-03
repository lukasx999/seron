#ifndef _MAIN_H
#define _MAIN_H

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include <linux/limits.h>



/*
This structure is global as it would be very annoying to pass
the compiler context to every single recursive function call when traversing the AST,
hence using a global variable makes code cleaner (imo)
*/
struct CompilerConfig {

    struct {
        const char *raw;         // main.srn
        char stripped[NAME_MAX]; // main
        char asm_[NAME_MAX];     // main.s
        char obj[NAME_MAX];      // main.o
    } filename;

    // options are ints, because `struct option` only accept int pointers
    struct {
        int verbose;
        int debug_asm;
        int dump_ast;
        int dump_tokens;
        int dump_symboltable;
        int compile_only;
        int compile_and_assemble;
    } opts;

};

extern struct CompilerConfig compiler_config;



#endif // _MAIN_H
