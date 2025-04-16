#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/util.h"
#include "lib/arena.h"



#define MAX_IDENT_LEN 64



typedef struct Type Type;

typedef struct {
    Type *type; // heap-allocated
    char ident[MAX_IDENT_LEN];
} Param;


#define MAX_PARAM_COUNT 255


typedef struct {
    Param params[MAX_PARAM_COUNT];
    size_t params_count;
    Type *returntype; // heap-allocated
} ProcSignature;

// TODO: put this somewhere else
typedef enum {
    TYPE_INVALID,

    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_LONG,
    TYPE_POINTER,
    TYPE_FUNCTION,
} TypeKind;

struct Type {
    TypeKind kind;
    union {
        ProcSignature type_signature;
        Type *type_pointee;
    };
};

const char *stringify_typekind(TypeKind type);
// TODO: recursively stringify type



typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_PARAMETER,
    SYMBOL_PROCEDURE,
} SymbolKind;

typedef struct {
    SymbolKind kind;
    Type type;
    union {
        int offset; // var/param
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
