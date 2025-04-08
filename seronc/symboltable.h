#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "types.h"



// TODO: static variables
// TODO: add literal symbol type
typedef enum {
    SYMBOL_NONE,

    SYMBOL_VARIABLE,
    SYMBOL_PARAMETER,
    SYMBOL_PROCEDURE,

    SYMBOL_TEMPORARY,
} SymbolKind;

// A symbol is just a representation of a value stored in future memory
typedef struct {
    SymbolKind kind;
    Type type;

    union {
        const char *label; // procedures
        size_t stack_addr; // variables / parameters
        Register reg;      // temporaries
    };
} Symbol;




typedef struct {
    size_t size, capacity;
    Hashtable *items;
} SymboltableList;

void            symboltable_list_init      (SymboltableList *s, size_t capacity, size_t table_size);
void            symboltable_list_destroy   (SymboltableList *s);
// Returns the just appended table
Hashtable    *symboltable_list_append    (SymboltableList *s, Hashtable *parent);
/* does a lookup in the current and parent hashtables */
Symbol         *symboltable_list_lookup    (const Hashtable *ht, const char *key);
/* addresses are filled in at code generation */
SymboltableList symboltable_list_construct (AstNode *root, size_t table_size);
void            symboltable_list_print     (const SymboltableList *st);




#endif // _SYMBOLTABLE_H
