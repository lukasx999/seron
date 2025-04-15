#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include "hashtable.h"
#include "parser.h"
#include "lib/arena.h"
#include "lib/util.h"



typedef struct {
    Hashtable *head;
    Arena *arena;
    int stack;
} Symboltable;

void symboltable_init(Symboltable *st, Arena *arena);
// returns the newly allocated hashtable
NO_DISCARD Hashtable *symboltable_push(Symboltable *st);
void symboltable_pop(Symboltable *st);
// returns NULL if key was not found
Symbol *symboltable_lookup(Hashtable *current, const char *key);
void symboltable_build(AstNode *root, Arena *arena);



#endif // _SYMBOLTABLE_H
