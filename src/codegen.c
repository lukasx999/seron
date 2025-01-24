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


// Enum value is size of type in bytes
typedef enum {
    TYPE_CHAR  = 1,
    TYPE_SHORT = 2,
    TYPE_INT   = 4,
    TYPE_SIZE  = 8,
} Type;

static const char *get_type_register_rax(Type type) {
    switch (type) {
        case TYPE_CHAR:
            return "al";
            break;
        case TYPE_SHORT:
            return "ax";
            break;
        case TYPE_INT:
            return "eax";
            break;
        case TYPE_SIZE:
            return "rax";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *get_type_asm_keyword(Type type) {
    switch (type) {
        case TYPE_CHAR:
            return "byte";
            break;
        case TYPE_SHORT:
            return "word";
            break;
        case TYPE_INT:
            return "dword";
            break;
        case TYPE_SIZE:
            return "qword";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}


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

    // TODO: size!
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

static void asm_store_value(Codegen *c, size_t value, Type type) {
    asm_write_comment(c, "store(start)");
    c->rbp_offset += type;

    fprintf(
        c->file,
        "sub rsp, %d\n"
        "mov %s [rbp-%lu], %lu\n",
        type, get_type_asm_keyword(type), c->rbp_offset, value
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

static void asm_call(Codegen *c, const char *identifier) {
    asm_write_comment(c, "call(start)");

    fprintf(
        c->file,
        "call %s\n", identifier
    );

    asm_write_comment(c, "call(end)\n");
}



// returns the location of the evaluated expression in memory
static size_t traverse_ast(AstNode *node, Codegen *codegen) {
    // TODO: free registers and keep track of allocated registers

    switch (node->kind) {

        case ASTNODE_BLOCK: {
            AstNodeList list = node->block.stmts;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen);
        } break;

        case ASTNODE_CALL: {
            ExprCall call    = node->expr_call;
            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                traverse_ast(list.items[i], codegen);

            // HACK: call into address instead of identifier
            asm_call(codegen, call.callee->expr_literal.op.value);
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
            size_t addr_lhs = traverse_ast(binop.lhs, codegen);
            size_t addr_rhs = traverse_ast(binop.rhs, codegen);

            switch (binop.op.kind) {
                case TOK_PLUS: {
                    asm_addition(codegen, addr_lhs, addr_rhs);
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
                    asm_store_value(codegen, num, TYPE_INT);
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

    return codegen->rbp_offset;

}

void generate_code(AstNode *root, const char *filename, bool print_comments) {
    Codegen codegen = asm_new(filename, print_comments);

    asm_prelude(&codegen);
    traverse_ast(root, &codegen);
    asm_postlude(&codegen);

    asm_destroy(&codegen);
}
