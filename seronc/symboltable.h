#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ast.h"
#include "types.h"
#include "backend.h"


// TODO: static variables
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

typedef struct HashtableEntry {
    const char *key;
    struct HashtableEntry *next;
    Symbol value;
} HashtableEntry;

// separate-chaining hashtable
typedef struct Symboltable {
    size_t size;
    HashtableEntry **buckets;
    struct Symboltable *parent;
} Symboltable;

void    symboltable_init    (Symboltable *ht, size_t size);
void    symboltable_destroy (Symboltable *ht);
/* returns -1 if key already exists, else 0 */
int     symboltable_insert  (Symboltable *ht, const char *key, Symbol value);
/* does a lookup in the current hashtable */
/* returns NULL if the key does not exist */
Symbol *symboltable_get     (const Symboltable *ht, const char *key);
void    symboltable_print   (const Symboltable *ht);



typedef struct {
    size_t size, capacity;
    Symboltable *items;
} SymboltableList;

void            symboltable_list_init      (SymboltableList *s, size_t capacity, size_t table_size);
void            symboltable_list_destroy   (SymboltableList *s);
// Returns the just appended table
Symboltable    *symboltable_list_append    (SymboltableList *s, Symboltable *parent);
/* does a lookup in the current and parent hashtables */
Symbol         *symboltable_list_lookup    (const Symboltable *ht, const char *key);
/* addresses are filled in at code generation */
SymboltableList symboltable_list_construct (AstNode *root, size_t table_size);
void            symboltable_list_print     (const SymboltableList *st);




#endif // _SYMBOLTABLE_H
