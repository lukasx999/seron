#include "hashtable.h"
#include <ver.h>


const char *stringify_typekind(TypeKind type) {
    switch (type) {
        case TYPE_CHAR:      return "char";    break;
        case TYPE_INT:       return "int";     break;
        case TYPE_POINTER:   return "pointer"; break;
        case TYPE_VOID:      return "void";    break;
        case TYPE_PROCEDURE: return "proc";    break;
        default:             PANIC("unknown type");
    }
}


static size_t hash(size_t size, const char *key) {
    // sum up the ascii representation of the string
    size_t sum = 0;

    for (size_t i=0; i < strlen(key); ++i)
        sum += (size_t) key[i] * i;

    return sum % size;
}

static HashtableEntry *new_entry(Arena *arena, const char *key, Symbol value) {

    HashtableEntry *entry = NON_NULL(arena_alloc(arena, sizeof(HashtableEntry)));
    *entry = (HashtableEntry) {
        .value = value,
        .next  = NULL,
    };
    strncpy(entry->key, key, ARRAY_LEN(entry->key));

    return entry;
}

void hashtable_init(Hashtable *ht, size_t size, Arena *arena) {
    *ht = (Hashtable) {
        .size     = size,
        .buckets  = NULL,
        .parent   = NULL,
        .arena    = arena,
    };

    ht->buckets = NON_NULL(arena_alloc(ht->arena, ht->size * sizeof(HashtableEntry*)));

    for (size_t i=0; i < ht->size; ++i)
        ht->buckets[i] = NULL;
}

int hashtable_insert(Hashtable *ht, const char *key, Symbol value) {

    NON_NULL(ht);

    size_t index = hash(ht->size, key);
    HashtableEntry *current = ht->buckets[index];

    if (current == NULL) {
        ht->buckets[index] = new_entry(ht->arena, key, value);
        return 0;
    }

    while (current != NULL) {
        if (!strcmp(current->key, key))
            return -1;

        if (current->next == NULL) {
            current->next = new_entry(ht->arena, key, value);
            return 0;
        }

        current = current->next;
    }

    UNREACHABLE();
}

Symbol *hashtable_get(const Hashtable *ht, const char *key) {

    NON_NULL(ht);

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
