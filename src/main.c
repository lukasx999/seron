#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "./util.h"
#include "./lexer.h"



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



int main(void) {

    char *file = read_file("example.spx");
    const char *src = "foo() = 123;";

    printf("Tokenizing: `%s`\n", src);

    TokenList tokens = tokenize(src);
    tokenlist_print(&tokens);

    tokenlist_destroy(&tokens);
    free(file);


    return EXIT_SUCCESS;
}
