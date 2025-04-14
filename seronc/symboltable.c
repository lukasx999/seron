#include "symboltable.h"
#include "codegen.h"

void symboltable_init(Symboltable *st, Arena *arena) {

    *st = (Symboltable) {
        .head  = NULL,
        .arena = arena,
    };

}

Symbol *symboltable_lookup(Hashtable *current, const char *key) {

    NON_NULL(current);

    while (current != NULL) {
        Symbol *sym = hashtable_get(current, key);
        if (sym != NULL) return sym;
        current = current->parent;
    }

    return NULL;
}

void symboltable_proc(Symboltable *st) {
    st->stack = 0;
}

int symboltable_insert(Symboltable *st, const char *key, Type type) {

    st->stack += get_type_size(type.kind);

    Symbol sym = {
        .offset = st->stack,
    };

    return hashtable_insert(st->head, key, sym);
}

// returns the newly allocated hashtable
NO_DISCARD Hashtable *symboltable_push(Symboltable *st) {

    Hashtable *ht = NON_NULL(arena_alloc(st->arena, sizeof(Hashtable)));
    hashtable_init(ht, 50, st->arena); // TODO: get rid of magic
    ht->parent = st->head;
    st->head = ht;

    return ht;
}

void symboltable_pop(Symboltable *st) {
    st->head = st->head->parent;
}
