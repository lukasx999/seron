#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"
#include "hashtable.h"
#include "lib/util.h"
#include "lib/arena.h"


Hashtable *head = NULL;

static inline void symboltable_push(Hashtable *ht) {
    ht->parent = head;
    head = ht;
}

static inline void symboltable_pop(void) {
    head = head->parent;
}


static void vardecl(AstNode *root, UNUSED int _depth, UNUSED void *_args) {
    static int i = 0;

    StmtVarDecl *vardecl = &root->stmt_vardecl;
    const char *ident = vardecl->identifier.value;
    // hashtable_insert(head, ident, i++);

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

void symboltable(AstNode *root) {

    AstCallback table[] = {
        [ASTNODE_VARDECL] = vardecl,
        [ASTNODE_BLOCK] = block,
    };

    parser_dispatch_ast(root, table, ARRAY_LEN(table), NULL);

}
