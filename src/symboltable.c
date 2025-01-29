#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

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

int hashtable_set(Hashtable *ht, const char *key, Symbol value) {
    Symbol *v = hashtable_get(ht, key);
    if (v == NULL)
        return -1;

    *v = value;
    return 0;
}

Symbol *symboltable_lookup(const Hashtable *ht, const char *key) {
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
                "%s%s%s%s %s|%s ",
                COLOR_BOLD,
                COLOR_RED,
                entry->key,
                COLOR_END,
                COLOR_GRAY,
                COLOR_END
            );

            switch (entry->value.kind) {
                case SYMBOLKIND_ADDRESS:
                    printf("%lu", entry->value.stack_addr);
                    break;
                case SYMBOLKIND_LABEL:
                    printf("%s", entry->value.label);
                    break;
                default:
                    assert(false);
                    break;
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





static void traverse_ast(AstNode *root, Hashtable *parent, Symboltable *st) {

    switch (root->kind) {
        case ASTNODE_BLOCK: {
            Block *block     = &root->block;
            AstNodeList list = block->stmts;

            symboltable_append(st, parent);
            Hashtable *last = symboltable_get_last(st);
            assert(last != NULL);
            block->symboltable = last;

            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], last, st);

        } break;

        case ASTNODE_FUNC: {
            const StmtFunc *func = &root->stmt_func;
            const char *ident = func->identifier.value;

            Symbol sym = {
                .kind  = SYMBOLKIND_LABEL,
                .label = ident,
            };

            int ret = hashtable_insert(parent, ident, sym);
            if (ret != 0)
                throw_error_simple("Procedure `%s` already exists", ident);

            traverse_ast(func->body, parent, st);
        } break;

        case ASTNODE_VARDECL: {
            const StmtVarDecl *vardecl = &root->stmt_vardecl;
            const char *ident = vardecl->identifier.value;

            Symbol sym = {
                .kind       = SYMBOLKIND_ADDRESS,
                .stack_addr = 0,
            };

            int ret = hashtable_insert(parent, ident, sym);
            if (ret == -1)
                throw_warning_simple("Variable `%s` already exists", ident);

            traverse_ast(vardecl->value, parent, st);
        } break;

        case ASTNODE_CALL: {
            ExprCall call = root->expr_call;
            traverse_ast(call.callee, parent, st);

            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], parent, st);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(root->expr_grouping.expr, parent, st);
            break;

        case ASTNODE_BINOP: {
            const ExprBinOp *binop = &root->expr_binop;
            traverse_ast(binop->lhs, parent, st);
            traverse_ast(binop->rhs, parent, st);
        } break;

        case ASTNODE_UNARYOP: {
            const ExprUnaryOp *unaryop = &root->expr_unaryop;
            traverse_ast(unaryop->node, parent, st);
        } break;

        case ASTNODE_LITERAL: {
            const ExprLiteral* literal = &root->expr_literal;

            if (literal->op.kind == TOK_IDENTIFIER) {
                const char *variable = literal->op.value;

                /* Ignore builtin functions */
                if (string_to_builtinfunc(variable) != BUILTINFUNC_NONE)
                    break;

                assert(parent != NULL);
                Symbol *sym = symboltable_lookup(parent, variable);

                if (sym == NULL)
                    throw_error_simple("Symbol `%s` does not exist", variable);
            }

        } break;

        default:
            assert(!"Unexpected Node Kind");
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
    int count = 0;
    parser_traverse_ast(root, scope_count_callback, true, &count);

    Symboltable symboltable = { 0 };
    symboltable_init(&symboltable, count, table_size);

    traverse_ast(root, NULL, &symboltable);

    return symboltable;
}
