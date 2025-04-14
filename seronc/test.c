#include <stdio.h>

#include "hashtable.h"





int main(void) {

    Hashtable ht = { 0 };
    hashtable_init(&ht, 50);

    hashtable_insert(&ht, "foo", 123);
    Symbol *sym = hashtable_get(&ht, "foo");
    NON_NULL(sym);


    return 0;
}
