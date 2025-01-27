#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "analysis.h"
#include "symboltable.h"


typedef struct {
    size_t size, capacity;
    Hashtable *items;
} Hashtables;

static Hashtables hashtables_new(void) {
    Hashtables hts = {
        .size     = 0,
        .capacity = 5,
        .items    = NULL,
    };
    hts.items = malloc(sizeof(Hashtable) * hts.capacity);
    return hts;
}

static void hashtables_append(Hashtables *hts, Hashtable *parent) {
    if (hts->size == hts->capacity) {
        hts->capacity *= 2;
        hts->items = realloc(hts->items, sizeof(Hashtable) * hts->capacity);
    }

    hts->items[hts->size++] = hashtable_new();
}

typedef struct {
    Hashtable **items;
} Stack;

static void stack_view_top(const Stack *stack) {
}

static void stack_push(const Stack *stack) {
}


/*
2 datastructures:
- symboltable (dynarray of hashmaps)
- stack of pointers to hashmaps

when entering a scope:
- append hashtable to dynarray
  - parent of that hashtable is whatever address is on top of the stack
- put a pointer to that hashtable in the astnode of the block
- push the address of said table to the stack

when leaving a scope:
- pop from the stack


now we can look up a symbol be traversing the symboltable upwards
by following pointers around

*/





// TODO:

#if 0

void traverse_ast(const AstNode *root, Hashtables *hts, Stack *stack) {

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = root->block.stmts;

            void *ht = hashtables_append(hts, stack_view_top(stack));
            stack_push(ht);

            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], hts);

            stack_pop();

        } break;

        case ASTNODE_CALL: {
            ExprCall call = root->expr_call;

            traverse_ast(call.callee, hts);

            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], hts);

        } break;

        case ASTNODE_GROUPING:
            traverse_ast(root->expr_grouping.expr, hts);
            break;

        case ASTNODE_FUNC:
            traverse_ast(root->stmt_func.body, hts);
            break;

        case ASTNODE_VARDECL:
            traverse_ast(root->stmt_vardecl.value, hts);
            break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = root->expr_binop;
            traverse_ast(binop.lhs, hts);
            traverse_ast(binop.rhs, hts);
        } break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp unaryop = root->expr_unaryop;
            traverse_ast(unaryop.node, hts);
        } break;

        case ASTNODE_LITERAL:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }


}
#endif


void build_symboltable(const AstNode *root) {
    Hashtables hts = hashtables_new();

    // traverse_ast(root, &hts);

}
