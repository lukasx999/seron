#include "symboltable.h"
#include "codegen.h"

void symboltable_init(Symboltable *st, Arena *arena) {
    *st = (Symboltable) {
        .head  = NULL,
        .arena = arena,
    };
}

NO_DISCARD Symbol *symboltable_lookup(Hashtable *current, const char *key) {

    NON_NULL(current);

    while (current != NULL) {
        Symbol *sym = hashtable_get(current, key);
        if (sym != NULL) return sym;
        current = current->parent;
    }

    return NULL;
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

    st->stack_size += get_type_size(vardecl->type.kind);

    Symbol sym = {
        .kind   = SYMBOL_VARIABLE,
        .type   = vardecl->type,
        .offset = st->stack_size,
    };

    hashtable_insert(st->head, vardecl->ident.value, sym);
}

void proc_pre(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    StmtProc *proc = &node->stmt_proc;

    Symbol sym = {
        .kind = SYMBOL_PROCEDURE,
        .type = proc->type,
    };

    hashtable_insert(st->head, proc->ident.value, sym);

    if (proc->body != NULL) {
        st->stack_size = 0;
    }

}

void proc_post(AstNode *node, UNUSED int _depth, void *args) {

    Symboltable *st = args;
    StmtProc *proc = &node->stmt_proc;

    if (proc->body == NULL) return;


    assert(proc->body->kind == ASTNODE_BLOCK);
    proc->symboltable = proc->body->block.symboltable;


    ProcSignature *sig = &proc->type.signature;

    for (size_t i=0; i < sig->params_count; ++i) {
        Param *param = &sig->params[i];

        st->stack_size += get_type_size(param->type->kind);

        Symbol sym = {
            .kind   = SYMBOL_PARAMETER,
            .type   = *param->type,
            .offset = st->stack_size,
        };

        hashtable_insert(proc->symboltable, param->ident, sym);
    }

    proc->stack_size = st->stack_size;

}

void symboltable_build(AstNode *root, Arena *arena) {

    Symboltable st = { 0 };
    symboltable_init(&st, arena);

    AstDispatchEntry table[] = {
        { ASTNODE_BLOCK,   block_pre, block_post },
        { ASTNODE_VARDECL, vardecl,   NULL       },
        { ASTNODE_PROC,    proc_pre,  proc_post  },
    };

    parser_dispatch_ast(root, table, ARRAY_LEN(table), (void*) &st);

}
