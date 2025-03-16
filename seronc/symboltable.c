#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "types.h"
#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "gen.h"
#include "ast.h"
#include "symboltable.h"



static size_t hash(size_t size, const char *key) {
    // sum up the ascii representation of the string
    size_t sum = 0;

    for (size_t i=0; i < strlen(key); ++i)
        sum += (size_t) key[i] * i;

    return sum % size;
}

static HashtableEntry *new_hashtable_entry(const char *key, Symbol value) {
    HashtableEntry *entry = malloc(sizeof(HashtableEntry));
    entry->key   = key;
    entry->value = value;
    entry->next  = NULL;

    return entry;
}

void symboltable_init(Symboltable *ht, size_t size) {
    *ht = (Symboltable) {
        .size     = size,
        .buckets  = NULL,
        .parent   = NULL,
    };

    ht->buckets = malloc(ht->size * sizeof(HashtableEntry*));

    for (size_t i=0; i < ht->size; ++i)
        ht->buckets[i] = NULL;
}

void symboltable_destroy(Symboltable *ht) {
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

int symboltable_insert(Symboltable *ht, const char *key, Symbol value) {
    assert(ht != NULL);

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

Symbol *symboltable_get(const Symboltable *ht, const char *key) {
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

Symbol *symboltable_list_lookup(const Symboltable *ht, const char *key) {
    assert(ht != NULL);
    const Symboltable *current = ht;

    while (current != NULL) {
        Symbol *value = symboltable_get(current, key);

        if (value != NULL)
            return value;

        current = current->parent;
    }

    return NULL;
}

void symboltable_print(const Symboltable *ht) {
    for (size_t i=0; i < ht->size; ++i) {
        HashtableEntry *entry = ht->buckets[i];

        if (entry == NULL)
            continue;

        while (entry != NULL) {
            printf(
                "%s%s%s%s",
                COLOR_BOLD,
                COLOR_RED,
                entry->key,
                COLOR_END
            );

            Symbol *sym = &entry->value;

            switch (sym->kind) {
                case SYMBOL_PARAMETER:
                case SYMBOL_VARIABLE:
                    printf(": %lu", sym->stack_addr);
                    break;
                default: break;
            }

            // TODO: show procedure signature

            printf("\n");
            entry = entry->next;
        }

    }
}




/* Symboltable */
void symboltable_list_init(SymboltableList *s, size_t capacity, size_t table_size) {
    *s = (SymboltableList) {
        .capacity   = capacity,
        .size       = 0,
        .items      = NULL,
    };

    s->items = malloc(s->capacity * sizeof(Symboltable));

    for (size_t i=0; i < s->capacity; ++i)
        symboltable_init(&s->items[i], table_size);
}

void symboltable_list_destroy(SymboltableList *s) {
    for (size_t i=0; i < s->capacity; ++i)
        symboltable_destroy(&s->items[i]);

    free(s->items);
    s->items = NULL;
}

Symboltable *symboltable_list_append(SymboltableList *s, Symboltable *parent) {
    assert(s->size < s->capacity);
    s->items[s->size++].parent = parent;
    return &s->items[s->size-1];
}

void symboltable_list_print(const SymboltableList *st) {
    printf("\n");
    const char *divider = "-----------------------";

    for (size_t i=0; i < st->size; ++i) {
        Symboltable *ht = &st->items[i];
        ptrdiff_t parent = ht->parent - st->items;

        printf("%s%s%s ", COLOR_GRAY, divider, COLOR_END);
        printf("%lu", i);
        if (ht->parent == NULL)
            printf("\n");
        else
            printf(" -> %ld\n", parent);

        symboltable_print(ht);
    }

    printf("%s%s%s\n\n", COLOR_GRAY, divider, COLOR_END);
}






//
// Symboltable construction:
//
// build up st, which is an array of symboltables, that all hold a pointer
// to their parent table
// each block node gets a pointer to the corresponding symboltable
// This makes hierarchical lookup pretty simple
//
// Example:
// [AST]             |       [Symboltable array]
//
// proc                         +------+
// - ident: "main"   /------->  |  #1  |
// - block ---------/           +------+
//                                ^  ^
//   - if                         |  |------|
//     - cond: 1                  |         |
//     - block -----\           +------+    |
//       - ...       \--------> |  #2  |    |
//                              +------+    |
//   - while                                |
//     - cond: 1                    /-------|
//     - block ----\               |
//       - ...      \           +------+
//                   \--------> |  #3  |
//                              +------+
//

typedef struct {
    Symboltable     *scope; // current parent symboltable
    SymboltableList *st;    // array of all symboltables
} TraversalContext;

static void traverse_ast(AstNode *root, TraversalContext *ctx);



static void ast_block(Block *block, TraversalContext *ctx) {
    AstNodeList list = block->stmts;

    block->symboltable = symboltable_list_append(ctx->st, ctx->scope);
    assert(block->symboltable != NULL);

    for (size_t i=0; i < list.size; ++i) {
        ctx->scope = block->symboltable;
        traverse_ast(list.items[i], ctx);
    }
}

static void ast_procedure(StmtProcedure *proc, TraversalContext *ctx) {
    const char *ident = proc->identifier.value;

    Symbol sym = {
        .kind  = SYMBOL_PROCEDURE,
        .type  = proc->type,
        .label = ident,
    };

    int ret = symboltable_insert(ctx->scope, ident, sym);
    if (ret) {
        compiler_message(MSG_ERROR, "Procedure `%s` already exists", ident);
        exit(1);
    }

    AstNode *body = proc->body;
    if (body != NULL)
        traverse_ast(body, ctx);
}

static void ast_vardecl(StmtVarDecl *vardecl, TraversalContext *ctx) {
    const char *ident = vardecl->identifier.value;

    Symbol sym = {
        .kind = SYMBOL_VARIABLE,
        .type = vardecl->type,
    };

    int ret = symboltable_insert(ctx->scope, ident, sym);
    if (ret) {
        compiler_message(MSG_ERROR, "Variable `%s` already exists", ident);
        exit(1);
    }

    if (vardecl->value != NULL)
        traverse_ast(vardecl->value, ctx);
}

static void traverse_ast(AstNode *root, TraversalContext *ctx) {
    assert(root != NULL);

    switch (root->kind) {
        case ASTNODE_BLOCK:
            ast_block(&root->block, ctx);
            break;

        case ASTNODE_PROCEDURE:
            ast_procedure(&root->stmt_procedure, ctx);
            break;

        case ASTNODE_VARDECL:
            ast_vardecl(&root->stmt_vardecl, ctx);
            break;

        // All Astnodes with blocks must be traversed here, so that
        // block->symboltable is initialized, and not NULL

        case ASTNODE_WHILE: {
            StmtWhile *while_ = &root->stmt_while;
            traverse_ast(while_->body, ctx);
        } break;

        case ASTNODE_IF: {
            StmtIf *if_ = &root->stmt_if;
            traverse_ast(if_->then_body, ctx);
            if (if_->else_body != NULL)
                traverse_ast(if_->else_body, ctx);
        } break;

        default:
            break;
    }

}

static void scope_count_callback(AstNode *_node, int _depth, void *args) {
    (void) _node, (void) _depth;
    int *block_count = args;
    ++*block_count;
}

//
// Insert procedure parameters after all symboltables have been created
//
// Doing this in the big recursive function above would be annoying, since
// when reaching a proc node, the body member hasn't been initialized yet.
//
// Therefore, you'd have to save the signature and, when reaching the next
// block node, insert the parameters from said signature into the
// symboltable of the block, which results in not very nice looking code. (imo)
//
static void proc_insert_params_callback(AstNode *node, int _depth, void *_args) {
    (void) _depth, (void) _args;

    StmtProcedure *proc = &node->stmt_procedure;
    ProcSignature *sig = &proc->type.type_signature;

    // Declaration
    if (proc->body == NULL)
        return;

    assert(proc->body->kind == ASTNODE_BLOCK);
    Block *block = &proc->body->block;

    for (size_t i=0; i < sig->params_count; ++i) {
        Param *param = &sig->params[i];
        Symbol sym = {
            .kind  = SYMBOL_PARAMETER,
            .type  = *param->type,
        };

        int ret = symboltable_insert(block->symboltable, param->ident, sym);
        if (ret != 0) {
            compiler_message(MSG_ERROR, "Parameter named `%s` already exists", param->ident);
            exit(EXIT_FAILURE);
        }

    }

}

static void precompute_stack_layout(AstNode *node, int _depth, void *_args) {
    (void) _depth, (void) _args;

    StmtProcedure *proc = &node->stmt_procedure;
    ProcSignature *sig = &proc->type.type_signature;
    assert(proc->stack_size == 0);

    if (proc->body == NULL)
        return;


    assert(proc->body->kind == ASTNODE_BLOCK);
    Block *body = &proc->body->block;

    for (size_t i=0; i < sig->params_count; ++i) {
        Param *param = &sig->params[i];
        proc->stack_size += typekind_get_size(param->type->kind);

        Symbol *sym = symboltable_get(body->symboltable, param->ident);
        assert(sym != NULL);
        sym->stack_addr = proc->stack_size;
    }


    AstNodeList *list = &body->stmts;

    for (size_t i=0; i < list->size; ++i) {
        AstNode *item = list->items[i];

        if (item->kind == ASTNODE_VARDECL) {
            StmtVarDecl *vardecl = &item->stmt_vardecl;
            proc->stack_size += typekind_get_size(vardecl->type.kind);

            Symbol *sym = symboltable_get(body->symboltable, vardecl->identifier.value);
            assert(sym != NULL);
            sym->stack_addr = proc->stack_size;
        }

    }

}

SymboltableList symboltable_list_construct(AstNode *root, size_t table_size) {

    int block_count = 0;
    parser_query_ast(root, scope_count_callback, ASTNODE_BLOCK, (void*) &block_count);

    SymboltableList st = { 0 };
    symboltable_list_init(&st, block_count, table_size);

    TraversalContext ctx = {
        .scope = NULL,
        .st    = &st,
    };

    traverse_ast(root, &ctx);
    parser_query_ast(root, proc_insert_params_callback, ASTNODE_PROCEDURE, NULL);
    parser_query_ast(root, precompute_stack_layout, ASTNODE_PROCEDURE, NULL);

    return st;
}
