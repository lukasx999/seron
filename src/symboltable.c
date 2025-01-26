#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hashtable.h"
#include "symboltable.h"



Symboltable symboltable_new(void) {
    Symboltable st = {
        .size     = 0,
        .capacity = 5,
        .items    = NULL,
    };

    st.items = malloc(st.capacity * sizeof(Hashtable));

    return st;
}

void symboltable_destroy(Symboltable *st) {
    free(st->items);
    st->items = NULL;
}

void symboltable_push(Symboltable *st) {
    if (st->size == st->capacity) {
        st->capacity *= 2;
        st->items = realloc(st->items, st->capacity * sizeof(Hashtable));
    }

    st->items[st->size++] = hashtable_new();
}

void symboltable_pop(Symboltable *st) {
    Hashtable *ht = &st->items[st->size-1];
    hashtable_destroy(ht);
    st->size--;
}

int symboltable_insert(Symboltable *st, const char *key, HashtableValue value) {
    Hashtable *ht = &st->items[st->size-1];
    return hashtable_insert(ht, key, value);
}

HashtableValue *symboltable_get(const Symboltable *st, const char *key) {

    for (size_t i=0; i < st->size; ++i) {
        size_t rev = st->size - 1 - i;
        Hashtable *ht = &st->items[rev];

        HashtableValue *value = hashtable_get(ht, key);
        if (value != NULL)
            return value;
    }

    return NULL;
}

void symboltable_override(Symboltable *st, const char *key, HashtableValue value) {
    Hashtable *ht = &st->items[st->size-1];
    assert(hashtable_insert(ht, key, value) == -1);
    *hashtable_get(ht, key) = value;
}

void symboltable_print(const Symboltable *st) {
    printf("== symboltable(start) ==\n");
    for (size_t i=0; i < st->size; ++i) {
        printf("\n");
        hashtable_print(&st->items[i]);
        printf("\n");
    }
    printf("== symboltable(end) ==\n\n");
}
