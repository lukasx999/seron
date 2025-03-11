#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "ast.h"
#include "lexer.h"
#include "lib/arena.h"



void astnodelist_init(AstNodeList *l, Arena *arena) {
     *l = (AstNodeList) {
        .cap   = 5,
        .size  = 0,
        .items = NULL,
        .arena = arena,
    };

    l->items = arena_alloc(arena, l->cap * sizeof(AstNode*));
}

void astnodelist_append(AstNodeList *l, AstNode *node) {

    if (l->size == l->cap) {
        l->cap *= 2;
        l->items = arena_realloc(l->arena, l->items, l->cap * sizeof(AstNode*));
    }

    l->items[l->size++] = node;
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

BuiltinFunction builtin_from_tokenkind(TokenKind kind) {
    switch (kind) {
        case TOK_BUILTIN_ASM:
            return BUILTIN_ASM;
            break;
        default:
            return BUILTIN_NONE;
            break;
    }
}
