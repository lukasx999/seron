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



void block_pre(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    Block *block = &node->block;
    block->symboltable = symboltable_push(st);
}

void block_post(UNUSED AstNode *_node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    symboltable_pop(st);
}

void vardecl(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    StmtVarDecl *vardecl = &node->stmt_vardecl;
    symboltable_insert(st, vardecl->ident.value, vardecl->type);
}

void proc_pre(UNUSED AstNode *_node, UNUSED int _depth, void *args) {
    // TODO: insert proc params
    Symboltable *st = args;
    st->stack = 0;
}

void proc_post(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    StmtProc *proc = &node->stmt_proc;
    proc->stack_size = st->stack;
}

void symboltable_build(AstNode *root, Arena *arena) {

    Symboltable st = { 0 };
    symboltable_init(&st, arena);

    AstDispatchEntry table[] = {
        { ASTNODE_BLOCK,     block_pre, block_post },
        { ASTNODE_VARDECL,   vardecl,   NULL       },
        { ASTNODE_PROCEDURE, proc_pre,  proc_post  },
    };

    parser_dispatch_ast(root, table, ARRAY_LEN(table), (void*) &st);

}
