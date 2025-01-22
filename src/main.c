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

static void build_binary(char *filename_asm, char *filename_bin, bool link_with_libc) {
    // TODO: ensure nasm and ld are installed
    // TODO: handle filenames

    // Assemble
    run_cmd((char*[]) { "nasm", "-felf64", filename_asm, NULL });

    // Link
    run_cmd(
        link_with_libc
        ? (char*[]) { "ld", "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2", "-lc", "out.o", "-o", filename_bin, NULL }
        : (char*[]) { "ld", "out.o", "-o", filename_bin, NULL }
    );

}



// TODO: remove assertions -> central error handling
// TODO: seperate TU for grammar rules, astnodelist and printing
// TODO: unit tests for parser
// TODO: lexer track token position
// TODO: type checking + semantic analysis
// TODO: symbol table
// TODO: inlineasm arguments
// TODO: asm grouping bug

int main(void) {

    const char *filename = "example.spx";
    char *file = read_file(filename);

    TokenList tokens = tokenize(file);
    tokenlist_print(&tokens);

    AstNode *root = parser_parse(&tokens);
    parser_print_ast(root);


    size_t bufsize = strlen(filename);

    char *filename_bin = alloca(bufsize);
    memset(filename_bin, 0, bufsize);
    strncpy(filename_bin, filename, bufsize);
    filename_bin[strcspn(filename_bin, ".")] = '\0';

    char *filename_asm = alloca(bufsize);
    memset(filename_asm, 0, bufsize);
    snprintf(filename_asm, bufsize, "%s.s", filename_bin);


    generate_code(root,filename_asm, true);


    build_binary(filename_asm,filename_bin, false);



    parser_free_ast(root);
    tokenlist_destroy(&tokens);
    free(file);

    return EXIT_SUCCESS;
}
