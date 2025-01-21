#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"


typedef struct {
    FILE *file;
    size_t rbp_offset;
} Codegen;

static Codegen asm_new(const char *filename) {
    FILE *file = fopen(filename, "w");
    return (Codegen) {
        .file       = file,
        .rbp_offset = 0,
    };
}

static void asm_destroy(Codegen *c) {
    fclose(c->file);
}

static void asm_prelude(Codegen *c) {
    fprintf(c->file, "section .text\n");
}

static void asm_postlude(Codegen *c) {
}

static void asm_store_value_int(Codegen *c, int value) {
    fprintf(c->file, "mov dword [rbp-%lu], %d\n", c->rbp_offset, value);
    c->rbp_offset += 4;
}

static void asm_function_start(Codegen *c, const char *identifier) {
    fprintf(
        c->file,
        "global %s\n"
        "%s:\n"
    "push rbp\n"
    "mov rbp, rsp\n", identifier, identifier
    );
}

static void asm_function_end(Codegen *c) {
    fprintf(
        c->file,
        "pop rbp\n"
        "ret\n"
    );
}



static void traverse_ast(AstNode *root, Codegen *codegen) {

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = root->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(root->expr_grouping.expr, codegen);
            break;

        case ASTNODE_FUNC: {
            StmtFunc func = root->stmt_func;
            asm_function_start(codegen, func.identifier.value);
            traverse_ast(func.body, codegen);
            asm_function_end(codegen);
        } break;

        case ASTNODE_VARDECL:
            traverse_ast(root->stmt_vardecl.value, codegen);
            break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = root->expr_binop;
            traverse_ast(binop.lhs, codegen);
            traverse_ast(binop.rhs, codegen);
        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral literal = root->expr_literal;

            switch (literal.op.kind) {
                case TOK_NUMBER: {
                    int num = atoi(literal.op.value);
                    asm_store_value_int(codegen, num);
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

}




void generate_code(AstNode *root) {
    Codegen codegen = asm_new("out.s");
    asm_prelude(&codegen);
    traverse_ast(root, &codegen);
    asm_destroy(&codegen);
}
