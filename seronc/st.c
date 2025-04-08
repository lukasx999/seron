#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"
#include "lib/util.h"






static void blockcount_callback(UNUSED AstNode *_node, UNUSED int _depth, void *args) {
    ++*(int*) args;
}

UNUSED static void vardecl(AstNode *root, int depth, void *args) {
    DISCARD(root);
    DISCARD(depth);
    DISCARD(args);
    printf("vardecl!\n");
}

UNUSED static void block(AstNode *root, int depth, void *args) {
    DISCARD(root);
    DISCARD(depth);
    DISCARD(args);
    printf("block!\n");
}

void symboltable(AstNode *root) {
    int block_count = 0;
    parser_query_ast(root, blockcount_callback, ASTNODE_BLOCK, (void*) &block_count);

    AstCallback table[] = {
        [ASTNODE_VARDECL] = vardecl,
        [ASTNODE_BLOCK] = block,
    };

    parser_dispatch_ast(root, table, ARRAY_LEN(table), NULL);


}
