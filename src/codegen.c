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
    fprintf(c->file, "section .text\n\n");
}

static void asm_postlude(Codegen *c) {
}

static void asm_addition(Codegen *c, size_t rbp_offset1, size_t rbp_offset2) {
    fprintf(c->file, "; addition(start)\n");
    fprintf(
        c->file,
        "mov rax, [rbp-%lu]\n"
        "add qword [rbp-%lu], rax\n"
        "mov rax, [rbp-%lu]\n"
        "mov qword [rbp-%lu], rax\n",
        rbp_offset1, rbp_offset2, rbp_offset2, c->rbp_offset
    );
    fprintf(c->file, "; addition(end)\n\n");
    c->rbp_offset += 4;
}

static void asm_store_value_int(Codegen *c, int value) {
    fprintf(c->file, "mov dword [rbp-%lu], %d\n", c->rbp_offset, value);
    c->rbp_offset += 4;
}

static void asm_inlineasm(Codegen *c, const char *src) {
    fprintf(c->file, "; inline(start)\n");
    fprintf(c->file, "%s\n", src);
    fprintf(c->file, "; inline(end)\n\n");
}

static void asm_function_start(Codegen *c, const char *identifier) {
    fprintf(c->file, "; function(start)\n");
    fprintf(c->file, "; function_prelude(start)\n");
    fprintf(
        c->file,
        "global %s\n"
        "%s:\n"
    "push rbp\n"
    "mov rbp, rsp\n", identifier, identifier
    );
    fprintf(c->file, "; function_prelude(end)\n\n");
}

static void asm_function_end(Codegen *c) {
    fprintf(c->file, "\n; function_postlude(start)\n");
    fprintf(
        c->file,
        "pop rbp\n"
        "ret\n"
    );
    fprintf(c->file, "; function_postlude(end)\n");
    fprintf(c->file, "; function(end)\n\n");
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

        case ASTNODE_INLINEASM: {
            StmtInlineAsm inlineasm = root->stmt_inlineasm;
            asm_inlineasm(codegen, inlineasm.src.value);
        } break;

        case ASTNODE_VARDECL:
            traverse_ast(root->stmt_vardecl.value, codegen);
            break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = root->expr_binop;
            traverse_ast(binop.lhs, codegen);
            traverse_ast(binop.rhs, codegen);

            switch (binop.op.kind) {
                case TOK_PLUS: {
                    asm_addition(codegen, codegen->rbp_offset-4, codegen->rbp_offset-8);
                } break;
                default:
                    assert(!"Unimplemented");
                    break;
            }

        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral literal = root->expr_literal;

            switch (literal.op.kind) {
                case TOK_NUMBER: {
                    int num = atoi(literal.op.value);
                    asm_store_value_int(codegen, num);
                } break;
                // TODO: ident literal symbol table lookup
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




void generate_code(AstNode *root, const char *filename) {
    Codegen codegen = asm_new(filename);
    asm_prelude(&codegen);
    traverse_ast(root, &codegen);
    asm_destroy(&codegen);
}
