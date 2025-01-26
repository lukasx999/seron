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



#if 0
// TODO: do sth with this
static void parser_print_error_cool(Parser *p) {

    Token tok = parser_get_current_token(p);
    throw_cool_error(p->filename, &tok, "you messed up!");

    // TODO: wrapper for dealing with this mess
    int linecounter = 1;
    for (size_t i=0; i < strlen(p->src); ++i) {
        char c = p->src[i];

        if (linecounter == tok.pos_line) {
            if (i == tok.pos_absolute) {
                fprintf(stderr, "%s%.*s%s", COLOR_RED, (int) tok.length, p->src+i, COLOR_END);
                i += tok.length - 1;
            }
            fprintf(stderr, "%c", c);
        }

        if (c == '\n')
            linecounter++;

    }

    exit(1);

}
#endif

















typedef struct {
    size_t size, capacity;
    Hashtable *items;
} Symboltable; // Symboltable is a stack of hashtables


static Symboltable symboltable_new(void) {
    Symboltable st = {
        .size     = 0,
        .capacity = 5,
        .items    = NULL,
    };

    st.items = malloc(st.capacity * sizeof(Hashtable));

    return st;
}

static void symboltable_destroy(Symboltable *st) {
    free(st->items);
    st->items = NULL;
}

static void symboltable_push(Symboltable *st) {
    if (st->size == st->capacity) {
        st->capacity *= 2;
        st->items = realloc(st->items, st->capacity * sizeof(Hashtable));
    }

    st->items[st->size++] = hashtable_new();
}

static void symboltable_pop(Symboltable *st) {
    Hashtable *ht = &st->items[st->size-1];
    hashtable_destroy(ht);
    st->size--;
}

static int symboltable_insert(Symboltable *st, const char *key, HashtableValue value) {
    Hashtable *ht = &st->items[st->size-1];
    return hashtable_insert(ht, key, value);
}

static HashtableValue *symboltable_get(const Symboltable *st, const char *key) {

    for (size_t i=0; i < st->size; ++i) {
        size_t rev = st->size - 1 - i;
        Hashtable *ht = &st->items[rev];

        HashtableValue *value = hashtable_get(ht, key);
        if (value != NULL)
            return value;
    }

    return NULL;
}

static void symboltable_override(Symboltable *st, const char *key, HashtableValue value) {
    Hashtable *ht = &st->items[st->size-1];
    assert(hashtable_insert(ht, key, value) == -1);
    *hashtable_get(ht, key) = value;
}

static void symboltable_print(const Symboltable *st) {
    printf("== symboltable(start) ==\n");
    for (size_t i=0; i < st->size; ++i) {
        printf("\n");
        hashtable_print(&st->items[i]);
        printf("\n");
    }
    printf("== symboltable(end) ==\n\n");
}








// returns the location of the evaluated expression in memory
static size_t traverse_ast(
    AstNode       *node,
    CodeGenerator *codegen,
    Symboltable   *symboltable
) {

    switch (node->kind) {

        case ASTNODE_BLOCK: {
            symboltable_push(symboltable);

            AstNodeList list = node->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, symboltable);

            symboltable_pop(symboltable);
        } break;

        case ASTNODE_CALL: {
            ExprCall *call = &node->expr_call;

            AstNodeList list = call->args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen, symboltable);

            // HACK: call into address instead of identifier
            gen_call(codegen, call->callee->expr_literal.op.value);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(node->expr_grouping.expr, codegen, symboltable);
            break;

        case ASTNODE_FUNC: {
            StmtFunc *func = &node->stmt_func;
            gen_func_start(codegen, func->identifier.value);
            traverse_ast(func->body, codegen, symboltable);
            gen_func_end(codegen);
        } break;

        case ASTNODE_INLINEASM: {
            StmtInlineAsm *inlineasm = &node->stmt_inlineasm;
            gen_inlineasm(codegen, inlineasm->src.value);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &node->stmt_vardecl;
            const char *variable = vardecl->identifier.value;

            size_t addr = traverse_ast(vardecl->value, codegen, symboltable);
            int ret = symboltable_insert(symboltable, variable, addr);

            if (ret == -1) {
                // TODO: add shadowing feature
                symboltable_override(symboltable, variable, addr);
                throw_warning(
                    codegen->filename_src,
                    &vardecl->identifier,
                    "Variable `%s` already exists",
                    variable
                );
            }
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp *binop = &node->expr_binop;
            size_t addr_lhs = traverse_ast(binop->lhs, codegen, symboltable);
            size_t addr_rhs = traverse_ast(binop->rhs, codegen, symboltable);

            switch (binop->op.kind) {
                case TOK_PLUS: {
                    // TODO: get type
                    gen_addition(codegen, addr_lhs, addr_rhs);
                } break;
                case TOK_MINUS: {
                    // TODO:
                    assert(!"TODO");
                } break;
                default:
                    assert(!"Unimplemented");
                    break;
            }

        } break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp *unaryop = &node->expr_unaryop;
            size_t addr = traverse_ast(unaryop->node, codegen, symboltable);

            switch (unaryop->op.kind) {
                case TOK_MINUS: {
                    // TODO:
                    assert(!"TODO");
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
                    const char *str = literal->op.value;
                    int64_t num = atoll(str);
                    gen_store_value(codegen, num, INTTYPE_INT);
                } break;

                case TOK_IDENTIFIER: {
                    const char *variable = literal->op.value;
                    HashtableValue *addr = symboltable_get(symboltable, variable);

                    if (addr == NULL)
                        throw_error(
                            codegen->filename_src,
                            &literal->op,
                            "Variable `%s` does not exist",
                            variable
                        );

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

void generate_code(
    AstNode    *root,
    const char *filename_asm,
    bool        print_comments,
    const char *filename_src
) {

    CodeGenerator codegen = gen_new(filename_asm, print_comments, filename_src);
    Symboltable symboltable = symboltable_new();

    gen_prelude(&codegen);
    traverse_ast(root, &codegen, &symboltable);
    gen_postlude(&codegen);

    assert(symboltable.size == 0);

    symboltable_destroy(&symboltable);
    gen_destroy(&codegen);
}
