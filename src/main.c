#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"



static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");

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

static void build_binary(char *filename, bool link_with_libc) {
    // TODO: ensure nasm and ld are installed
    // TODO: handle filenames

    // Assemble
    run_cmd((char*[]) { "nasm", "-felf64", filename, NULL });

    // Link
    run_cmd(
        link_with_libc
        ? (char*[]) { "ld", "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", "-lc", "out.o", "-o", "out", NULL }
        : (char*[]) { "ld", "out.o", "-o", "out", NULL }
    );

}



// TODO: remove assertions -> central error handling
// TODO: seperate TU for grammar rules, astnodelist and printing
// TODO: unit tests for parser
// TODO: lexer track token position
// TODO: type checking + semantic analysis
// TODO: symbol table
// TODO: inlineasm arguments

int main(void) {

    char *file = read_file("example.spx");

    TokenList tokens = tokenize(file);
    tokenlist_print(&tokens);

    AstNode *root = parser_parse(&tokens);
    parser_print_ast(root);
    // TODO: preserve input filename eg: foo.spx -> foo.s
    generate_code(root, "out.s");


    build_binary("out.s", false);



    parser_free_ast(root);
    tokenlist_destroy(&tokens);
    free(file);

    return EXIT_SUCCESS;
}
