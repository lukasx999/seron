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
    bool print_comments;
} Codegen;

static void asm_write_comment(Codegen *c, const char *str) {
    if (c->print_comments)
        fprintf(c->file, "; %s\n", str);
}

static Codegen asm_new(const char *filename, bool print_comments) {
    FILE *file = fopen(filename, "w");
    return (Codegen) {
        .file           = file,
        .rbp_offset     = 0,
        .print_comments = print_comments,
    };
}

static void asm_destroy(Codegen *c) {
    fclose(c->file);
}

static void asm_prelude(Codegen *c) {
    // TODO: different sections
    fprintf(c->file, "section .text\n\n");
}

static void asm_postlude(Codegen *c) {
}

static void asm_addition(Codegen *c, size_t rbp_offset1, size_t rbp_offset2) {
    asm_write_comment(c, "addition(start)");
    c->rbp_offset += 4;

    fprintf(
        c->file,
        "mov rax, [rbp-%lu]\n"
        "add qword [rbp-%lu], rax\n"
        "mov rax, [rbp-%lu]\n"
        "sub rsp, 4\n"
        "mov qword [rbp-%lu], rax\n",
        rbp_offset1, rbp_offset2, rbp_offset2, c->rbp_offset
    );

    asm_write_comment(c, "addition(end)\n");
}

static void asm_store_value_int(Codegen *c, int value) {
    asm_write_comment(c, "store(start)");
    c->rbp_offset += 4;

    fprintf(
        c->file,
        "sub rsp, 4\n"
        "mov dword [rbp-%lu], %d\n",
        c->rbp_offset, value
    );

    asm_write_comment(c, "store(end)\n");
}

static void asm_inlineasm(Codegen *c, const char *src) {
    asm_write_comment(c, "inline(start)");

    fprintf(c->file, "%s\n", src);

    asm_write_comment(c, "inline(end)\n");
}

static void asm_function_start(Codegen *c, const char *identifier) {
    asm_write_comment(c, "function(start)");
    asm_write_comment(c, "function_prelude(start)");

    fprintf(
        c->file,
        "global %s\n"
        "%s:\n"
    "push rbp\n"
    "mov rbp, rsp\n", identifier, identifier
    );

    asm_write_comment(c, "function_prelude(end)\n");
}

static void asm_function_end(Codegen *c) {
    asm_write_comment(c, "function_postlude(start)");

    fprintf(
        c->file,
        "pop rbp\n"
        "ret\n"
    );

    asm_write_comment(c, "function_postlude(end)");
    asm_write_comment(c, "function(end)\n");
}



static void traverse_ast(AstNode *node, Codegen *codegen) {

    switch (node->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = node->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen);
        } break;

        case ASTNODE_GROUPING:
            traverse_ast(node->expr_grouping.expr, codegen);
            break;

        case ASTNODE_FUNC: {
            StmtFunc func = node->stmt_func;
            asm_function_start(codegen, func.identifier.value);
            traverse_ast(func.body, codegen);
            asm_function_end(codegen);
        } break;

        case ASTNODE_INLINEASM: {
            StmtInlineAsm inlineasm = node->stmt_inlineasm;
            asm_inlineasm(codegen, inlineasm.src.value);
        } break;

        case ASTNODE_VARDECL:
            traverse_ast(node->stmt_vardecl.value, codegen);
            break;

        case ASTNODE_BINOP: {
            ExprBinOp binop = node->expr_binop;
            traverse_ast(binop.lhs, codegen);
            traverse_ast(binop.rhs, codegen);

            switch (binop.op.kind) {
                case TOK_PLUS: {
                    asm_addition(codegen, codegen->rbp_offset-4, codegen->rbp_offset);
                } break;
                default:
                    assert(!"Unimplemented");
                    break;
            }

        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral literal = node->expr_literal;

            switch (literal.op.kind) {
                case TOK_NUMBER: {
                    int num = atoi(literal.op.value);
                    assert(num != 0);
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

void generate_code(AstNode *root, const char *filename, bool print_comments) {
    Codegen codegen = asm_new(filename, print_comments);

    asm_prelude(&codegen);
    traverse_ast(root, &codegen);

    asm_destroy(&codegen);
}
