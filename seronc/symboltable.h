#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <arena.h>
#include <ver.h>

#include "hashtable.h"
#include "parser.h"



// symboltable is a linked list of hashtables
typedef struct {
    Hashtable *head;
    Arena *arena;
    int stack_size;
} Symboltable;

void symboltable_init(Symboltable *st, Arena *arena);
// returns the newly allocated hashtable
Hashtable *symboltable_push(Symboltable *st);
void symboltable_pop(Symboltable *st);
// returns NULL if key was not found
NO_DISCARD Symbol *symboltable_lookup(const Hashtable *scope, const char *key);
void symboltable_build(AstNode *root, Arena *arena);



#endif // _SYMBOLTABLE_H
