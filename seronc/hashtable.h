#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include <ver.h>
#include <arena.h>

#include "lexer.h"
#include "types.h"




typedef enum {
    SYMBOL_INVALID, // invalid symbol, shouldnt be used
    SYMBOL_NONE,    // temporary value that has a type

    SYMBOL_VARIABLE,
    SYMBOL_PARAMETER,
    SYMBOL_PROCEDURE,
} SymbolKind;

typedef struct {
    SymbolKind kind;
    Type type;
    union {
        int offset; // var / param
    };
} Symbol;

typedef struct HashtableEntry {
    char key[MAX_IDENT_LEN];
    struct HashtableEntry *next;
    Symbol value;
} HashtableEntry;

// separate-chaining hashtable
typedef struct Hashtable {
    size_t size;
    HashtableEntry **buckets;
    struct Hashtable *parent;
    Arena *arena;
} Hashtable;

void hashtable_init(Hashtable *ht, size_t size, Arena *arena);
void hashtable_destroy(Hashtable *ht);
/* returns -1 if key already exists, else 0 */
int hashtable_insert(Hashtable *ht, const char *key, Symbol value);
/* returns NULL if the key does not exist */
Symbol *hashtable_get(const Hashtable *ht, const char *key);


#endif // _HASHTABLE_H
