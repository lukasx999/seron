#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symboltable.h"




/* Hashtable */

static size_t hash(size_t size, const char *key) {
    size_t sum = 0;

    for (size_t i=0; i < strlen(key); ++i)
        sum += (size_t) key[i];

    return sum % size;
}

static HashtableEntry *new_hashtable_entry(const char *key, HashtableValue value) {
    HashtableEntry *entry = malloc(sizeof(HashtableEntry));
    entry->key   = key;
    entry->value = value;
    entry->next  = NULL;

    return entry;
}

void hashtable_init(Hashtable *ht, size_t size) {
    *ht = (Hashtable) {
        .size     = size,
        .buckets  = NULL,
    };

    ht->buckets = malloc(ht->size * sizeof(HashtableEntry*));

    for (size_t i=0; i < ht->size; ++i)
        ht->buckets[i] = NULL;
}

void hashtable_destroy(Hashtable *ht) {
    for (size_t i=0; i < ht->size; ++i) {
        HashtableEntry *entry = ht->buckets[i];

        while (entry != NULL) {
            HashtableEntry *next = entry->next;
            free(entry);
            entry = next;
        }

    }

    free(ht->buckets);
    ht->buckets = NULL;
}

int hashtable_insert(Hashtable *ht, const char *key, HashtableValue value) {
    size_t index = hash(ht->size, key);
    HashtableEntry *current = ht->buckets[index];

    if (current == NULL) {
        ht->buckets[index] = new_hashtable_entry(key, value);
        return 0;
    }

    while (current != NULL) {
        if (!strcmp(current->key, key))
            return -1;

        if (current->next == NULL) {
            current->next = new_hashtable_entry(key, value);
            return 0;
        }

        current = current->next;
    }

    assert(!"unreachable");
}

HashtableValue *hashtable_get(const Hashtable *ht, const char *key) {
    size_t index = hash(ht->size, key);
    HashtableEntry *current = ht->buckets[index];

    if (current == NULL)
        return NULL;

    while (current != NULL) {
        if (!strcmp(current->key, key))
            return &current->value;

        current = current->next;
    }

    return NULL;

}

void hashtable_print(const Hashtable *ht) {
    for (size_t i=0; i < ht->size; ++i) {
        HashtableEntry *entry = ht->buckets[i];
        printf("|%lu| ", i);

        if (entry == NULL) {
            puts("(empty)");
            continue;
        }

        while (entry != NULL) {
            printf("(%s: %lu)", entry->key, entry->value);
            printf(entry->next == NULL ? "\n" : " -> ");
            entry = entry->next;
        }

    }
}



/* Symboltable */

void symboltable_init(Symboltable *s, size_t table_size) {
    *s = (Symboltable) {
        .capacity   = 5,
        .size       = 0,
        .items      = NULL,
        .table_size = table_size,
    };

    s->items = malloc(s->capacity * sizeof(Hashtable));

    for (size_t i=0; i < s->capacity; ++i)
        hashtable_init(&s->items[i], s->table_size);
}

void symboltable_destroy(Symboltable *s) {
    for (size_t i=0; i < s->capacity; ++i)
        hashtable_destroy(&s->items[i]);

    free(s->items);
    s->items = NULL;
}

void symboltable_append(Symboltable *s, Hashtable *parent) {
    if (s->size == s->capacity) {
        s->capacity *= 2;
        s->items = realloc(s->items, s->capacity * sizeof(Hashtable));

        for (size_t i=s->capacity/2; i < s->capacity; ++i)
            hashtable_init(&s->items[i], s->table_size);
    }

    s->items[s->size++].parent = parent;
}

Hashtable *symboltable_get_last(const Symboltable *s) {
    return &s->items[s->size-1];
}

void symboltable_print(const Symboltable *s) {
    for (size_t i=0; i < s->size; ++i) {
        Hashtable *ht = &s->items[i];
        printf("\n");
        hashtable_print(ht);
        // printf("addr: %p\n", (void*) ht);
        // printf("parent: %p\n", (void*) ht->parent);
        printf("\n");
    }
}
