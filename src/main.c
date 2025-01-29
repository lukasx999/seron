#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <alloca.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "symboltable.h"



static void check_fileextension(const char *filename, const char *extension) {

    size_t dot_offset = strcspn(filename, ".");
    if (dot_offset == strlen(filename))
        throw_error_simple("File extension missing");

    if (strncmp(filename + dot_offset + 1, extension, strlen(extension)))
        throw_error_simple("File extension must be `.%s`", extension);
}

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        throw_error_simple("Source file `%s` does not exist", filename);

    struct stat statbuf = { 0 };
    stat(filename, &statbuf);
    size_t size = statbuf.st_size;

    char *buf = calloc(size + 1, sizeof(char));
    fread(buf, sizeof(char), size, file);

    fclose(file);
    return buf;
}

static void run_cmd(char *const argv[]) {
    if (fork() == 0)
        execvp(argv[0], argv);
    wait(NULL);
}

static void build_binary(
    char *filename_asm,
    char *filename_obj,
    char *filename_bin,
    bool link_with_libc
) {
    // TODO: ensure nasm and ld are installed

    // Assemble
    run_cmd((char*[]) {
        "nasm",
        filename_asm,
        "-felf64",
        "-o", filename_obj,
        "-gdwarf",
        NULL
    });

    // Link
    run_cmd(
        link_with_libc
        ? (char*[]) {
            "ld",
            filename_obj,
            "-lc",
            "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2",
            "-o", filename_bin,
            NULL
        }
        : (char*[]) {
            "ld",
            filename_obj,
            "-o", filename_bin,
            NULL
        }
    );

}




// TODO: metaprogramming
// TODO: cmdline args
// TODO: type checking + semantic analysis
// TODO: synchronizing parser
// TODO: refactoring lexer
// TODO: ast optimization pass
// TODO: asm gets generated even if compiler fails -> semantic analysis
// TODO: expect-style testing
// TODO: unit tests for parser + symboltable
// TODO: ast traversal context struct
// TODO: compiler state (global) struct

// TODO:
/*
cmdline args:
-W, -S, -c, --dump-ast, --dump-tokens, -lc, --run
*/

int main(void) {

    const char *filename = "example/main.spx";
    check_fileextension(filename, "spx");

    char *file = read_file(filename);

    TokenList tokens = tokenize(file);
    // tokenlist_print(&tokens);

    AstNode *root = parser_parse(&tokens, filename);
    parser_print_ast(root);

    Symboltable symboltable = symboltable_construct(root, 5);


    // TODO: refactor
    size_t bufsize = strlen(filename);

    char *filename_bin = alloca(bufsize);
    memset(filename_bin, 0, bufsize);
    strncpy(filename_bin, filename, bufsize);
    filename_bin[strcspn(filename_bin, ".")] = '\0';

    char *filename_asm = alloca(bufsize);
    memset(filename_asm, 0, bufsize);
    snprintf(filename_asm, bufsize, "%s.s", filename_bin);

    char *filename_obj = alloca(bufsize);
    memset(filename_obj, 0, bufsize);
    snprintf(filename_obj, bufsize, "%s.o", filename_bin);

    generate_code(root, filename, filename_asm, false);
#if 1
    build_binary(filename_asm, filename_obj, filename_bin, false);
#endif

    symboltable_print(&symboltable);
    symboltable_destroy(&symboltable);



    parser_free_ast(root);
    tokenlist_destroy(&tokens);
    free(file);

    return EXIT_SUCCESS;
}
