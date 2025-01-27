#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "symboltable.h"





static size_t hash(size_t size, const char *key) {
    size_t sum = 0;

    for (size_t i=0; i < strlen(key); ++i)
        sum += key[i];

    return sum % size;
}

static HashtableEntry *new_hashtable_entry(const char *key, HashtableValue value) {
    HashtableEntry *entry = malloc(sizeof(HashtableEntry));
    entry->key   = key;
    entry->value = value;
    entry->next  = NULL;

    return entry;
}

Hashtable hashtable_new(void) {
    Hashtable ht = {
        .size     = 5,
        .buckets  = NULL,
    };

    ht.buckets = malloc(ht.size * sizeof(HashtableEntry*));

    for (size_t i=0; i < ht.size; ++i)
        ht.buckets[i] = NULL;

    return ht;
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
            printf("(%s : %lu)", entry->key, entry->value);
            printf(entry->next == NULL ? "\n" : " -> ");
            entry = entry->next;
        }

    }
}




















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
