#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ast.h"
#include "types.h"



// TODO: function params + returntype

typedef enum {
    SYMBOL_NONE,
    SYMBOL_VARIABLE,
    /*SYMBOL_STATIC_VARIABLE,*/
    SYMBOL_PROCEDURE,
} SymbolKind;

typedef struct {
    Type type;
    const char *name;
    size_t stack_addr; /* 0 if symbol is global or procedure */
    ProcSignature sig;
} Symbol;

typedef struct HashtableEntry {
    const char *key;
    struct HashtableEntry *next;
    Symbol value;
} HashtableEntry;

typedef struct Hashtable {
    size_t size;
    HashtableEntry **buckets;
    struct Hashtable *parent;
} Hashtable;

extern void hashtable_init(Hashtable *ht, size_t size);
extern void hashtable_destroy(Hashtable *ht);
/* returns -1 if key already exists, else 0 */
extern int  hashtable_insert(Hashtable *ht, const char *key, Symbol value);
/* does a lookup in the current hashtable */
/* returns NULL if the key does not exist */
extern Symbol *hashtable_get(const Hashtable *ht, const char *key);
extern void hashtable_print(const Hashtable *ht);
/* returns -1 if key does not exist */
extern int hashtable_set(Hashtable *ht, const char *key, Symbol value);



typedef struct {
    size_t size, capacity;
    Hashtable *items;
} Symboltable;

extern void symboltable_init(Symboltable *s, size_t capacity, size_t table_size);
extern void symboltable_destroy(Symboltable *s);
extern void symboltable_append(Symboltable *s, Hashtable *parent);
extern Hashtable *symboltable_get_last(const Symboltable *s);
/* does a lookup in the current and parent hashtables */
extern Symbol *symboltable_lookup(const Hashtable *ht, const char *key);
/* addresses are filled in at code generation */
extern Symboltable symboltable_construct(AstNode *root, size_t table_size);
extern void symboltable_print(const Symboltable *st);




#endif // _SYMBOLTABLE_H
