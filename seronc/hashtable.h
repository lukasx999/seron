#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib/util.h"




typedef struct AstNode AstNode;



typedef struct Type Type;

typedef struct {
    Type *type; // heap-allocated
    const char *ident;
} Param;

#define MAX_ARG_COUNT 255

// TODO: maybe reuse for structs
typedef struct {
    Param params[MAX_ARG_COUNT]; // TODO: uses shit ton of memory
    size_t params_count;
    Type *returntype; // heap-allocated
} ProcSignature;

typedef enum {
    TYPE_INVALID, // used for error checking
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_FUNCTION,
    TYPE_POINTER,
} TypeKind;

struct Type {
    TypeKind kind;
    bool mutable;
    // This union contains additional information for complex types,
    // such as functions, pointers and user-defined types
    union {
        ProcSignature type_signature;
        Type *type_pointee;
    };
};

static inline const char *typekind_to_string(TypeKind type) {
    switch (type) {
        case TYPE_CHAR:     return "char";    break;
        case TYPE_INT:      return "int";     break;
        case TYPE_POINTER:  return "pointer"; break;
        case TYPE_VOID:     return "void";    break;
        case TYPE_FUNCTION: return "proc";    break;
        default:            PANIC("unknown type");
    }
}









typedef int Symbol;

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

void hashtable_init(Hashtable *ht, size_t size);
void hashtable_destroy(Hashtable *ht);
/* returns -1 if key already exists, else 0 */
int hashtable_insert(Hashtable *ht, const char *key, Symbol value);
/* returns NULL if the key does not exist */
Symbol *hashtable_get(const Hashtable *ht, const char *key);
void hashtable_print(const Hashtable *ht);


#endif // _HASHTABLE_H
