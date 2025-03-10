#ifndef _ARENA_H
#define _ARENA_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


// Arena Allocator Implementation
//
// The purpose of this allocator is to let the parser allocate a bunch
// of nodes on the AST, and then free all of them at once, without having
// to chase pointers from malloc() around.
//
// This only makes sense because all of the allocated memory is part of
// the same data structure


typedef struct {
    size_t size, cap;
    void **items;
} Arena;

void  arena_init(Arena *a);
void *arena_alloc(Arena *a, size_t size);
void  arena_free(Arena *a);


#endif // _ARENA_H
