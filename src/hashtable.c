#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "hashtable.h"


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
            printf("(%s : %d)", entry->key, entry->value);
            printf(entry->next == NULL ? "\n" : " -> ");
            entry = entry->next;
        }

    }
}


int test(void) {

    Hashtable ht = hashtable_new();

    hashtable_insert(&ht, "a", 1);
    hashtable_insert(&ht, "b", 2);
    hashtable_insert(&ht, "c", 3);
    hashtable_insert(&ht, "d", 4);
    hashtable_insert(&ht, "e", 5);
    printf("%d\n", hashtable_insert(&ht, "f", 6));
    printf("%d\n", hashtable_insert(&ht, "f", 3));

    hashtable_print(&ht);

    HashtableValue *a = hashtable_get(&ht, "a");
    HashtableValue *b = hashtable_get(&ht, "b");
    HashtableValue *c = hashtable_get(&ht, "c");

    HashtableValue *f = hashtable_get(&ht, "f");
    printf("%d\n", *f);


    hashtable_destroy(&ht);

    return 0;
}
