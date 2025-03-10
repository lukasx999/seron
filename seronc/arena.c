#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "arena.h"



void arena_init(Arena *a) {
    *a = (Arena) {
        .cap   = 5,
        .size  = 0,
        .items = NULL,
    };

    a->items = malloc(a->cap * sizeof(void*));
}

void *arena_alloc(Arena *a, size_t size) {
    void *ptr = malloc(size);
    assert(ptr != NULL);

    if (a->size == a->cap) {
        a->cap *= 2;
        a->items = realloc(a->items, a->cap * sizeof(void*));
    }

    a->items[a->size++] = ptr;

    return ptr;
}

void arena_free(Arena *a) {
    for (size_t i=0; i < a->size; ++i)
        free(a->items[i]);

    free(a->items);
    a->items = NULL;
}
