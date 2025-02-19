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
#include "ast.h"
#include "symboltable.h"




/* Hashtable */

static size_t hash(size_t size, const char *key) {
    size_t sum = 0;

    for (size_t i=0; i < strlen(key); ++i)
        sum += (size_t) key[i];

    return sum % size;
}

static HashtableEntry *new_hashtable_entry(const char *key, Symbol value) {
    HashtableEntry *entry = malloc(sizeof(HashtableEntry));
    entry->key   = key;
    entry->value = value;
    entry->next  = NULL;

    return entry;
}

void hashtable_init(Hashtable *ht, size_t size) {
    *ht = (Hashtable) {
        .size     = size,
        .buckets  = NULL,
    };

    ht->buckets = malloc(ht->size * sizeof(HashtableEntry*));

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

Symbol *symboltable_lookup(const Hashtable *ht, const char *key) {
    assert(ht != NULL);
    const Hashtable *current = ht;

    while (current != NULL) {
        Symbol *value = hashtable_get(current, key);

        if (value != NULL)
            return value;

        current = current->parent;
    }

    return NULL;
}

void hashtable_print(const Hashtable *ht) {
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

            if (sym->type.kind == TYPE_FUNCTION) {
                // TODO:
#if 0
                printf(" %s(%s", COLOR_GRAY, COLOR_END);

                ProcSignature *sig = &sym->type.type_signature;

                /*
                 * we have to check that count is non-zero before subtracting 1,
                 * as it will otherwise underflow
                 */
                size_t count = sig->params_count;
                for (size_t i=0; i < (count ? count-1 : count); ++i) {
                    printf(
                        "%s%s,%s ",
                        typekind_to_string(sig->params[i].type->kind),
                        COLOR_GRAY,
                        COLOR_END
                    );
                }

                if (count)
                    printf("%s ", typekind_to_string(sig->params[count-1].type->kind));
                else
                    printf("%s ", typekind_to_string(TYPE_VOID));

                printf("%s->%s %s", COLOR_GRAY, COLOR_END, typekind_to_string(sig->returntype->kind));
                printf("%s)%s", COLOR_GRAY, COLOR_END);
#endif
            }



            printf("\n");
            entry = entry->next;
        }

    }
}




/* Symboltable */
void symboltable_init(Symboltable *s, size_t capacity, size_t table_size) {
    *s = (Symboltable) {
        .capacity   = capacity,
        .size       = 0,
        .items      = NULL,
    };

    s->items = malloc(s->capacity * sizeof(Hashtable));

    for (size_t i=0; i < s->capacity; ++i)
        hashtable_init(&s->items[i], table_size);
}

void symboltable_destroy(Symboltable *s) {
    for (size_t i=0; i < s->capacity; ++i)
        hashtable_destroy(&s->items[i]);

    free(s->items);
    s->items = NULL;
}

void symboltable_append(Symboltable *s, Hashtable *parent) {
    assert(s->size < s->capacity);
    s->items[s->size++].parent = parent;
}

Hashtable *symboltable_get_last(const Symboltable *s) {
    return s->size == 0
    ? NULL
    : &s->items[s->size-1];
}

void symboltable_print(const Symboltable *st) {
    printf("\n");
    const char *divider = "-----------------------";

    for (size_t i=0; i < st->size; ++i) {
        Hashtable *ht = &st->items[i];
        ptrdiff_t parent = ht->parent - st->items;

        printf("%s%s%s ", COLOR_GRAY, divider, COLOR_END);
        printf("%lu", i);
        if (ht->parent == NULL)
            printf("\n");
        else
            printf(" -> %ld\n", parent);

        hashtable_print(ht);
    }

    printf("%s%s%s\n\n", COLOR_GRAY, divider, COLOR_END);
}







typedef struct {
    Hashtable *scope;
    Symboltable *st;
    ProcSignature *sig; /* for adding function parameters to function body. NULL if not used */
} TraversalContext;

static void traverse_ast(AstNode *root, TraversalContext *ctx);



static void ast_block(Block *block, TraversalContext *ctx) {
    AstNodeList list = block->stmts;

    symboltable_append(ctx->st, ctx->scope);
    Hashtable *last = symboltable_get_last(ctx->st);
    assert(last != NULL);
    block->symboltable = last;


    /* Insert parameters as variables in the scope of the procedure */
    ProcSignature *sig = ctx->sig;

    // only if parent node is a procedure
    if (sig != NULL) {

        for (size_t i=0; i < sig->params_count; ++i) {
            Param *param = &sig->params[i];
            Symbol sym = {
                .kind  = SYMBOL_VARIABLE,
                .label = param->ident,
                .type  = *param->type,
            };

            int ret = hashtable_insert(block->symboltable, sym.label, sym);
            assert(ret == 0);
        }

        ctx->sig = NULL;
    }


    for (size_t i=0; i < list.size; ++i) {
        ctx->scope = block->symboltable;
        traverse_ast(list.items[i], ctx);
    }

}

static void ast_procedure(StmtProcedure *proc, TraversalContext *ctx) {
    const char *ident = proc->identifier.value;

    ctx->sig = &proc->type.type_signature;

    Symbol sym = {
        .kind  = SYMBOL_PROCEDURE,
        .type  = proc->type,
        .label = ident,
    };

    int ret = hashtable_insert(ctx->scope, ident, sym);
    if (ret == -1)
        throw_error_simple("Procedure `%s` already exists", ident);

    traverse_ast(proc->body, ctx);
}

static void ast_vardecl(StmtVarDecl *vardecl, TraversalContext *ctx) {
    const char *ident = vardecl->identifier.value;

    Symbol sym = {
        .kind = SYMBOL_VARIABLE,
        .type = vardecl->type,
    };

    int ret = hashtable_insert(ctx->scope, ident, sym);
    if (ret == -1)
        throw_warning_simple("Variable `%s` already exists", ident);

    traverse_ast(vardecl->value, ctx);
}

/*
 * `parent` is the symboltable of the current scope
 * `st` is an array of all symboltables
 */
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

static void scope_count_callback(AstNode *node, int _depth, void *args) {
    (void) _depth;
    int *count = args;

    if (node->kind == ASTNODE_BLOCK)
        ++*count;
}

Symboltable symboltable_construct(AstNode *root, size_t table_size) {
    int block_count = 0;
    parser_traverse_ast(root, scope_count_callback, true, &block_count);

    Symboltable symboltable = { 0 };
    symboltable_init(&symboltable, block_count, table_size);

    TraversalContext ctx = {
        .scope = NULL,
        .st    = &symboltable,
        .sig   = NULL,
    };

    traverse_ast(root, &ctx);

    return symboltable;
}
