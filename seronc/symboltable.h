#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <arena.h>
#include <util.h>

#include "hashtable.h"
#include "parser.h"



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
NO_DISCARD Symbol *symboltable_lookup(Hashtable *current, const char *key);
void symboltable_build(AstNode *root, Arena *arena);



#endif // _SYMBOLTABLE_H
