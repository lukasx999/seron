#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>

#include "lexer.h"
#include "asm.h"



IntegerType inttype_from_astnode(TokenKind type) {
    switch (type) {
        case TOK_TYPE_CHAR:
            return INTTYPE_CHAR;
            break;
        case TOK_TYPE_SHORT:
            return INTTYPE_SHORT;
            break;
        case TOK_TYPE_INT:
            return INTTYPE_INT;
            break;
        case TOK_TYPE_SIZE:
            return INTTYPE_SIZE;
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}

const char *inttype_reg_rax(IntegerType type) {
    switch (type) {
        case INTTYPE_CHAR:
            return "al";
            break;
        case INTTYPE_SHORT:
            return "ax";
            break;
        case INTTYPE_INT:
            return "eax";
            break;
        case INTTYPE_SIZE:
            return "rax";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}

const char *inttype_reg_rdi(IntegerType type) {
    switch (type) {
        case INTTYPE_CHAR:
            return "dil";
            break;
        case INTTYPE_SHORT:
            return "di";
            break;
        case INTTYPE_INT:
            return "edi";
            break;
        case INTTYPE_SIZE:
            return "rdi";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}

const char *inttype_asm_operand(IntegerType type) {
    switch (type) {
        case INTTYPE_CHAR:
            return "byte";
            break;
        case INTTYPE_SHORT:
            return "word";
            break;
        case INTTYPE_INT:
            return "dword";
            break;
        case INTTYPE_SIZE:
            return "qword";
            break;
        default:
            assert(!"Unknown type");
            break;
    }
}





void gen_comment(CodeGenerator *c, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    if (c->print_comments) {
        fprintf(c->file, "; ");
        vfprintf(c->file, fmt, va);
        fprintf(c->file, "\n");
    }

    va_end(va);
}

CodeGenerator gen_new(const char *filename_asm, bool print_comments, const char *filename_src) {
    FILE *file = fopen(filename_asm, "w");
    return (CodeGenerator) {
        .file           = file,
        .rbp_offset     = 0,
        .print_comments = print_comments,
        .filename_src   = filename_src,
    };
}

void gen_destroy(CodeGenerator *c) {
    fclose(c->file);
}

void gen_prelude(CodeGenerator *c) {
    // TODO: different sections
    fprintf(c->file, "section .text\n\n");
}

void gen_postlude(CodeGenerator *c) {
}

// TODO: this should return the address of the result
void gen_addition(CodeGenerator *c, size_t rbp_offset1, size_t rbp_offset2) {
    c->rbp_offset += 4;
    gen_comment(
        c,
        "START: addition([rbp-%lu] + [rbp-%lu] -> [rbp-%lu])",
        rbp_offset1,
        rbp_offset2,
        c->rbp_offset
    );

    fprintf(
        c->file,
        "mov rax, [rbp-%lu]\n"
        "mov rdi, [rbp-%lu]\n"
        "add rax, rdi\n"
        "sub rsp, 4\n"
        "mov qword [rbp-%lu], rax\n",
        rbp_offset1,
        rbp_offset2,
        c->rbp_offset
    );

    gen_comment(c, "END: addition\n");
}

// TODO: not needed anymore?
void gen_copy_value(CodeGenerator *c, size_t addr, IntegerType type) {
    c->rbp_offset += type;
    gen_comment(c, "START: copy([rbp-%lu] -> [rbp-%lu])", addr, c->rbp_offset);

    fprintf(
        c->file,
        "sub rsp, %d\n"
        "mov %s, [rbp-%lu]\n"
        "mov %s [rbp-%lu], %s\n",
        type,
        inttype_reg_rax(type), addr,
        inttype_asm_operand(type), c->rbp_offset, inttype_reg_rax(type)
    );

    gen_comment(c, "END: copy\n");
}

void gen_store_value(CodeGenerator *c, int64_t value, IntegerType type) {
    c->rbp_offset += type;
    gen_comment(c, "START: store(%lu -> [rbp-%lu])", value, c->rbp_offset);

    fprintf(
        c->file,
        "sub rsp, %d\n"
        "mov %s [rbp-%lu], %lu\n",
        type,
        inttype_asm_operand(type), c->rbp_offset, value
    );

    gen_comment(c, "END: store\n");
}

void gen_inlineasm(
    CodeGenerator *c,
    const char    *src,
    const size_t  *addrs,
    size_t         addrs_len
) {
    gen_comment(c, "START: inline");

    size_t arg_index = 0;
    for (size_t i=0; i < strlen(src); ++i) {

        if (src[i] == '{' && src[i+1] == '}') {
            size_t addr = addrs[arg_index];
            fprintf(c->file, "[rbp-%lu]", addr);
            i++;
            arg_index++;

        } else
            fprintf(c->file, "%c", src[i]);

    }

    fprintf(c->file, "\n");

    assert(arg_index == addrs_len && "argument count insufficent");

    gen_comment(c, "END: inline\n");
}

void gen_func_start(CodeGenerator *c, const char *identifier) {
    gen_comment(c, "START: function");
    gen_comment(c, "START: function_prelude");

    fprintf(
        c->file,
        "global %s\n"
        "%s:\n"
        "push rbp\n"
        "mov rbp, rsp\n", identifier, identifier
    );

    gen_comment(c, "END: function_prelude\n");
}

void gen_func_end(CodeGenerator *c) {
    gen_comment(c, "START: function_postlude");

    fprintf(
        c->file,
        "pop rbp\n"
        "ret\n"
    );

    gen_comment(c, "END: function_postlude");
    gen_comment(c, "END: function\n");
}

// TODO: call into address
void gen_call(CodeGenerator *c, const char *identifier) {
    gen_comment(c, "call(start)");

    fprintf(
        c->file,
        "call %s\n", identifier
    );

    gen_comment(c, "call(end)\n");
}
