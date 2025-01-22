#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

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




// TODO: remove assertions -> central error handling
// TODO: seperate TU for grammar rules, astnodelist and printing
// TODO: unit tests for parser
// TODO: lexer track token position
// TODO: type checking + semantic analysis
// TODO: symbol table

int main(void) {

    char *file = read_file("example.spx");
    const char *src = file;
    // const char *src = "#foo";

    printf("Source: `%s`\n\n", src);

    TokenList tokens = tokenize(src);
    tokenlist_print(&tokens);
    printf("\n");

    AstNode *root = parser_parse(&tokens);
    parser_print_ast(root);
    printf("\n");

    generate_code(root);


    parser_free_ast(root);
    tokenlist_destroy(&tokens);
    free(file);

    return EXIT_SUCCESS;
}
