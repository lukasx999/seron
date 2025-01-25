#ifndef _HASHTABLE_H
#define _HASHTABLE_H

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
extern void      hashtable_destroy(Hashtable *ht);
// returns -1 if key already exists
extern int       hashtable_insert (Hashtable *ht, const char *key, HashtableValue value);
// returns NULL if the key does not exist
extern HashtableValue    *hashtable_get    (const Hashtable *ht, const char *key);
extern void      hashtable_print  (const Hashtable *ht);


#endif // _HASHTABLE_H
