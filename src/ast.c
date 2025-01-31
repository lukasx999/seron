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



BuiltinFunc string_to_builtinfunc(const char *str) {
    if (!strcmp(str, "asm"))
        return BUILTINFUNC_ASM;
    else
        return BUILTINFUNC_NONE;
}

BinOpKind binopkind_from_tokenkind(TokenKind kind) {
    switch (kind) {
        case TOK_PLUS:
            return BINOP_ADD;
            break;
        case TOK_MINUS:
            return BINOP_SUB;
            break;
        case TOK_SLASH:
            return BINOP_DIV;
            break;
        case TOK_ASTERISK:
            return BINOP_MUL;
            break;
        default:
            assert(!"Unknown Tokenkind");
            break;
    }
}
