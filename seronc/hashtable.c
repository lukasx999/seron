#include "hashtable.h"
#include "lib/util.h"



static size_t hash(size_t size, const char *key) {
    // sum up the ascii representation of the string
    size_t sum = 0;

    for (size_t i=0; i < strlen(key); ++i)
        sum += (size_t) key[i] * i;

    return sum % size;
}

static HashtableEntry *new_entry(const char *key, Symbol value) {
    HashtableEntry *entry = NON_NULL(malloc(sizeof(HashtableEntry)));
    entry->key   = key;
    entry->value = value;
    entry->next  = NULL;

    return entry;
}

void hashtable_init(Hashtable *ht, size_t size) {
    *ht = (Hashtable) {
        .size     = size,
        .buckets  = NULL,
        .parent   = NULL,
    };

    ht->buckets = NON_NULL(malloc(ht->size * sizeof(HashtableEntry*)));

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

int hashtable_insert(Hashtable *ht, const char *key, Symbol value) {
    assert(ht != NULL);

    size_t index = hash(ht->size, key);
    HashtableEntry *current = ht->buckets[index];

    if (current == NULL) {
        ht->buckets[index] = new_entry(key, value);
        return 0;
    }

    while (current != NULL) {
        if (!strcmp(current->key, key))
            return -1;

        if (current->next == NULL) {
            current->next = new_entry(key, value);
            return 0;
        }

        current = current->next;
    }

    UNREACHABLE();
}

Symbol *hashtable_get(const Hashtable *ht, const char *key) {
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

static inline void stringify_symbol(const Symbol *sym, char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "%d", *sym);
}

void hashtable_print(const Hashtable *ht) {

    for (size_t i=0; i < ht->size; ++i) {
        HashtableEntry *current = ht->buckets[i];

        while (current != NULL) {
            char buf[512] = { 0 };
            stringify_symbol(&current->value, buf, ARRAY_LEN(buf));
            printf("[%s]: %s\n", current->key, buf);
            current = current->next;
        }

    }


}
