#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#if 0
typedef enum {
    SYMBOLKIND_ADDRESS,
    SYMBOLKIND_LABEL,
} SymbolKind;

typedef struct {
    SymbolKind kind;
    union {
        size_t stack_addr;
        const char *label;
    };
} Symbol;
#endif

typedef size_t HashtableValue;



typedef struct HashtableEntry {
    const char *key;
    struct HashtableEntry *next;
    HashtableValue value;
} HashtableEntry;

typedef struct Hashtable {
    size_t size;
    HashtableEntry **buckets;
    struct Hashtable *parent;
} Hashtable;

extern void hashtable_init    (Hashtable *ht, size_t size);
extern void hashtable_destroy (Hashtable *ht);
/* returns -1 if key already exists, else 0 */
extern int  hashtable_insert  (Hashtable *ht, const char *key, HashtableValue value);
/* does a lookup in the current hashtable */
/* returns NULL if the key does not exist */
extern HashtableValue *hashtable_get   (const Hashtable *ht, const char *key);
/* does a lookup in the current and parent hashtables */
extern HashtableValue *hashtable_lookup(const Hashtable *ht, const char *key);
extern void            hashtable_print (const Hashtable *ht);



typedef struct {
    size_t size, capacity;
    Hashtable *items;
    size_t table_size; // size for each individual table
} Symboltable;

extern void        symboltable_init      (Symboltable *s, size_t table_size);
extern void        symboltable_destroy   (Symboltable *s);
extern void        symboltable_append    (Symboltable *s, Hashtable *parent);
extern Hashtable  *symboltable_get_last  (const Symboltable *s);
extern void        symboltable_print     (const Symboltable *s);


#endif // _SYMBOLTABLE_H
