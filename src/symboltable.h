#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>






typedef size_t HashtableValue;

typedef struct HashtableEntry {
    const char *key;
    HashtableValue value;
    struct HashtableEntry *next;
} HashtableEntry;

typedef struct {
    size_t size;
    HashtableEntry **buckets;
} Hashtable;

extern Hashtable hashtable_new    (void);
extern void hashtable_destroy(Hashtable *ht);
// returns -1 if key already exists, else 0
extern int hashtable_insert (Hashtable *ht, const char *key, HashtableValue value);
// returns NULL if the key does not exist
extern HashtableValue *hashtable_get(const Hashtable *ht, const char *key);
extern void hashtable_print(const Hashtable *ht);





// TODO:
#if 0
typedef enum {
    STKIND_VARIABLE,
    STKIND_FUNCTION,
} SymboltableKind;

typedef struct {
    size_t addr; // runtime address of entry, may not be used during semantic analysis
    // Type type;
    SymboltableKind kind;
} SymboltableEntry;
#endif




// Symboltable is a stack of hashtables
typedef struct {
    size_t size, capacity;
    Hashtable *items;
} Symboltable;

extern Symboltable     symboltable_new      (void);
extern void            symboltable_destroy  (Symboltable *st);
extern void            symboltable_push     (Symboltable *st);
extern void            symboltable_pop      (Symboltable *st);
extern int             symboltable_insert   (Symboltable *st, const char *key, HashtableValue value);
extern HashtableValue *symboltable_get      (const Symboltable *st, const char *key);
extern void            symboltable_override (Symboltable *st, const char *key, HashtableValue value);
extern void            symboltable_print    (const Symboltable *st);


#endif // _SYMBOLTABLE_H
