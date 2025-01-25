#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "hashtable.h"
#include "asm.h"


typedef struct {
    size_t size, capacity;
    Hashtable *items;
} Symboltables;


static Symboltables symboltables_new(void) {
    Symboltables st = {
        .size     = 0,
        .capacity = 5,
        .items    = NULL,
    };

    st.items = malloc(st.capacity * sizeof(Hashtable));

    return st;
}

static void symboltables_destroy(Symboltables *st) {
    // TODO: memory leak - deallocate hashtables
    free(st->items);
    st->items = NULL;
}

static void symboltables_push(Symboltables *st) {
    if (st->size == st->capacity) {
        st->capacity *= 2;
        st->items = realloc(st->items, st->capacity * sizeof(Hashtable));
    }

    st->items[st->size++] = hashtable_new();
}

static void symboltables_pop(Symboltables *st) {
    Hashtable *ht = &st->items[st->size-1];
    hashtable_destroy(ht);
    st->size--;
}

static int symboltables_insert(Symboltables *st, const char *key, HashtableValue value) {
    Hashtable *ht = &st->items[st->size-1];
    return hashtable_insert(ht, key, value);
}

static HashtableValue *symboltables_get(const Symboltables *st, const char *key) {

    for (size_t i=0; i < st->size; ++i) {
        size_t rev = st->size - 1 - i;
        Hashtable *ht = &st->items[rev];

        HashtableValue *value = hashtable_get(ht, key);
        if (value != NULL)
            return value;
    }

    return NULL;
}

static void symboltables_print(const Symboltables *st) {
    for (size_t i=0; i < st->size; ++i) {
        printf("\n");
        hashtable_print(&st->items[i]);
        printf("\n");
    }
}








// returns the location of the evaluated expression in memory
static size_t traverse_ast(AstNode *node, CodeGenerator *codegen, Symboltables *st) {

    switch (node->kind) {

        case ASTNODE_BLOCK: {
            symboltables_push(st);

            AstNodeList list = node->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, st);

            symboltables_print(st);
            symboltables_pop(st);
        } break;

        case ASTNODE_CALL: {
            ExprCall call    = node->expr_call;
            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, st);

            // HACK: call into address instead of identifier
            gen_call(codegen, call.callee->expr_literal.op.value);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(node->expr_grouping.expr, codegen, st);
            break;

        case ASTNODE_FUNC: {
            StmtFunc func = node->stmt_func;
            gen_func_start(codegen, func.identifier.value);
            traverse_ast(func.body, codegen, st);
            gen_func_end(codegen);
        } break;

        case ASTNODE_INLINEASM: {
            StmtInlineAsm inlineasm = node->stmt_inlineasm;
            gen_inlineasm(codegen, inlineasm.src.value);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &node->stmt_vardecl;
            const char *variable = vardecl->identifier.value;

            size_t addr = traverse_ast(vardecl->value, codegen, st);
            int ret = symboltables_insert(st, variable, addr);

            // TODO: allow variable shadowing via hashtable_get()
            if (ret == -1)
                throw_error("Variable `%s` already exists", variable);
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = node->expr_binop;
            size_t addr_lhs = traverse_ast(binop.lhs, codegen, st);
            size_t addr_rhs = traverse_ast(binop.rhs, codegen, st);

            switch (binop.op.kind) {
                case TOK_PLUS: {
                    gen_addition(codegen, addr_lhs, addr_rhs);
                } break;
                default:
                    assert(!"Unimplemented");
                    break;
            }

        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral *literal = &node->expr_literal;

            switch (literal->op.kind) {

                case TOK_NUMBER: {
                    int num = atoi(literal->op.value);
                    assert(num != 0);
                    gen_store_value(codegen, num, INTTYPE_INT);
                } break;

                case TOK_IDENTIFIER: {
                    const char *variable = literal->op.value;
                    HashtableValue *addr = symboltables_get(st, variable);

                    if (addr == NULL)
                        throw_error("Variable `%s` does not exist", variable);

                    // TODO: handle type
                    gen_copy_value(codegen, *addr, INTTYPE_INT);
                } break;

                default:
                    assert(!"Unimplemented");
                    break;
            }
        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    return codegen->rbp_offset;

}

void generate_code(AstNode *root, const char *filename, bool print_comments) {
    CodeGenerator codegen = gen_new(filename, print_comments);

    Symboltables st = symboltables_new();

    gen_prelude(&codegen);
    traverse_ast(root, &codegen, &st);
    gen_postlude(&codegen);

    assert(st.size == 0);

    symboltables_destroy(&st);
    gen_destroy(&codegen);
}
