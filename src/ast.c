#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "ast.h"



AstNodeList astnodelist_new(void) {
    AstNodeList list = {
        .capacity = 5,
        .size     = 0,
        .items    = NULL,
    };
    list.items = malloc(list.capacity * sizeof(AstNode*));
    return list;
}

void astnodelist_append(AstNodeList *list, AstNode *node) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(AstNode*));
    }
    list->items[list->size++] = node;
}

void astnodelist_destroy(AstNodeList *list) {
    free(list->items);
    list->items = NULL;
}
