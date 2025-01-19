#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./util.h"
#include "./lexer.h"



// TODO:
static void read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    fclose(file);
}



int main(void) {

    const char *src = "foo = 123;";
    TokenList tokens = tokenize(src);
    tokenlist_print(&tokens);
    tokenlist_destroy(&tokens);


    return EXIT_SUCCESS;
}
