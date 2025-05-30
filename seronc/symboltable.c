#include "symboltable.h"
#include "diagnostics.h"

void symboltable_init(Symboltable *st, Arena *arena) {
    *st = (Symboltable) {
        .head  = NULL,
        .arena = arena,
    };
}

NO_DISCARD Symbol *symboltable_lookup(const Hashtable *scope, const char *key) {

    NON_NULL(scope);

    while (scope != NULL) {
        Symbol *sym = hashtable_get(scope, key);
        if (sym != NULL) return sym;
        scope = scope->parent;
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



static void block_pre(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    Block *block = &node->block;
    block->symboltable = symboltable_push(st);
}

static void block_post(UNUSED AstNode *_node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    symboltable_pop(st);
}

static int type_complex_size(const Type *type, const Hashtable *ht) {
    int size = 0;

    if (type->kind == TYPE_OBJECT) {
        Symbol *sym = NON_NULL(symboltable_lookup(ht, type->object_name));
        Table *table = sym->type.table;

        for (size_t i=0; i < table->field_count; ++i)
            size += type_primitive_size(table->fields[i].type.kind);

    } else {
        size = type_primitive_size(type->kind);
    }

    return size;
}


static void vardecl(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    StmtVarDecl *vardecl = &node->stmt_vardecl;

    int size = type_primitive_size(vardecl->type.kind);
    st->stack_size += size;

    vardecl->offset = st->stack_size;

    Symbol sym = {
        .kind   = SYMBOL_VARIABLE,
        .type   = vardecl->type,
        .offset = st->stack_size,
    };

    // shadowing is a feature, not a bug
    hashtable_insert(st->head, vardecl->ident.value, sym);
}

static void proc_pre(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    DeclProc *proc = &node->stmt_proc;

    Symbol sym = {
        .kind = SYMBOL_PROCEDURE,
        .type = proc->type,
    };

    hashtable_insert(st->head, proc->ident.value, sym);

    if (proc->body != NULL) {
        st->stack_size = 0;
    }

}

static void proc_post(AstNode *node, UNUSED int _depth, void *args) {

    Symboltable *st = args;
    DeclProc *proc = &node->stmt_proc;

    if (proc->body == NULL) return;

    assert(proc->body->kind == ASTNODE_BLOCK);
    proc->symboltable = proc->body->block.symboltable;

    ProcSignature *sig = proc->type.signature;

    for (size_t i=0; i < sig->params_count; ++i) {
        Param *param = &sig->params[i];

        st->stack_size += type_primitive_size(param->type.kind);
        param->offset = st->stack_size;

        Symbol sym = {
            .kind   = SYMBOL_PARAMETER,
            .type   = param->type,
            .offset = st->stack_size,
        };

        hashtable_insert(proc->symboltable, param->ident, sym);
    }

    proc->stack_size = st->stack_size;

}

static void table_pre(AstNode *node, UNUSED int _depth, void *args) {
    Symboltable *st = args;
    DeclTable *table = &node->table;

    Symbol sym = {
        .kind = SYMBOL_TABLE,
        .type = table->type,
    };

    hashtable_insert(st->head, table->ident.value, sym);
}

void symboltable_build(AstNode *root, Arena *arena) {

    Symboltable st = { 0 };
    symboltable_init(&st, arena);

    AstDispatchEntry table[] = {
        { ASTNODE_BLOCK,   block_pre, block_post },
        { ASTNODE_VARDECL, vardecl,   NULL       },
        { ASTNODE_PROC,    proc_pre,  proc_post  },
        { ASTNODE_TABLE,   table_pre, NULL       },
    };

    parser_dispatch_ast(root, table, ARRAY_LEN(table), &st);
}
