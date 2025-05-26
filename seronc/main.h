#ifndef _MAIN_H
#define _MAIN_H

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include <linux/limits.h>


typedef enum {
    TARGET_BINARY,
    TARGET_ASSEMBLY,
    TARGET_OBJECT,
    TARGET_RUN,
} CompilationTarget;

/*
This structure is global as it would be very annoying to pass
the compiler context to every single recursive function call when traversing the AST,
hence using a global variable makes code cleaner (imo)
*/
struct CompilerContext {

    const char *src;
    const char *filename;

    // options are ints, because `struct option` only accept int pointers
    struct {
        int dump_ast;
        int dump_tokens;
        int dump_symboltable;
    } opts;

    CompilationTarget target;

};

extern struct CompilerContext compiler_ctx;



#endif // _MAIN_H
