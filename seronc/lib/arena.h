#ifndef _ARENA_H
#define _ARENA_H

#include <stdlib.h>


typedef struct {
    size_t size, cap;
    void **items;
} Arena;

void  arena_init    (Arena *a);
void *arena_alloc   (Arena *a, size_t size);
void *arena_realloc (Arena *a, void *ptr, size_t size);
void  arena_free    (Arena *a);



#ifdef ARENA_IMPL

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

    if (a->size == a->cap) {
        a->cap *= 2;
        a->items = realloc(a->items, a->cap * sizeof(void*));
    }

    a->items[a->size++] = ptr;

    return ptr;
}

void *arena_realloc(Arena *a, void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);

    if (a->size == a->cap) {
        a->cap *= 2;
        a->items = realloc(a->items, a->cap * sizeof(void*));
    }

    a->items[a->size++] = new_ptr;

    // Remove old (invalid) pointer from list
    // Instead of removing the element, we can just set it to NULL, as free()
    // ignores NULL. Otherwise we'd have to move over all of the other items,
    // which would be slow and unnecessary.
    for (size_t i=0; i < a->size; ++i)
        if (a->items[i] == ptr)
            a->items[i] = NULL;

    return new_ptr;
}

void arena_free(Arena *a) {
    for (size_t i=0; i < a->size; ++i)
        free(a->items[i]);

    free(a->items);
    a->items = NULL;
}

#endif // ARENA_IMPL

#endif // _ARENA_H
