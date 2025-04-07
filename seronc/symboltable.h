#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "types.h"

typedef enum {
    REG_RAX,
    REG_RDI,
    REG_RSI,
    REG_RDX,
    REG_RCX,
    REG_R8,
    REG_R9,
} Register;


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

typedef struct HashtableEntry {
    const char *key;
    struct HashtableEntry *next;
    Symbol value;
} HashtableEntry;

// separate-chaining hashtable
typedef struct Hashtable {
    size_t size;
    HashtableEntry **buckets;
    struct Hashtable *parent;
} Hashtable;

void    hashtable_init    (Hashtable *ht, size_t size);
void    hashtable_destroy (Hashtable *ht);
/* returns -1 if key already exists, else 0 */
int     hashtable_insert  (Hashtable *ht, const char *key, Symbol value);
/* does a lookup in the current hashtable */
/* returns NULL if the key does not exist */
Symbol *hashtable_get     (const Hashtable *ht, const char *key);
void    hashtable_print   (const Hashtable *ht);



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
