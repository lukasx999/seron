#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"
#include "hashtable.h"
#include "lib/util.h"
#include "lib/arena.h"

// typedef struct {
// } Symboltable;

Hashtable *head = NULL;

static void symboltable_insert(const char *key, Symbol value) {
    hashtable_insert(head, key, value);
}

static void symboltable_push(Hashtable *ht) {
    ht->parent = head;
    head = ht;
}

static void symboltable_pop(void) {
    head = head->parent;
}


static void vardecl(AstNode *root, UNUSED int _depth, UNUSED void *_args) {
    static int i = 0;

    StmtVarDecl *vardecl = &root->stmt_vardecl;
    const char *ident = vardecl->identifier.value;
    symboltable_insert(ident, i++);

}

static void block(AstNode *root, UNUSED int _depth, UNUSED void *_args) {

    Block *block = &root->block;

    // TODO: use arena for this
    Hashtable *ht = malloc(sizeof(Hashtable));
    block->symboltable = ht;

    symboltable_push(ht);
    // TODO: post traversal callback for pop operation
    // OR: do this in the parser

}


static void block_post(UNUSED AstNode *_root, UNUSED int _depth, UNUSED void *_args) {
    symboltable_pop();
}




void symboltable(AstNode *root) {

    AstDispatchEntry table[] = {
        { ASTNODE_BLOCK,   block,   block_post },
        { ASTNODE_VARDECL, vardecl, NULL       },
    };

    parser_dispatch_ast(root, table, ARRAY_LEN(table), NULL);

}
