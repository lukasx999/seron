#ifndef _SYMBOLTABLE_H
#define _SYMBOLTABLE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "hashtable.h"


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
