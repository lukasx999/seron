#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>

#include "gen.h"
#include "symboltable.h"
#include "types.h"








static void gen_comment(CodeGenerator *c, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    if (c->print_comments) {
        fprintf(c->file, "; ");
        vfprintf(c->file, fmt, va);
        fprintf(c->file, "\n");
    }

    va_end(va);
}

int gen_init(CodeGenerator *gen, const char *filename_asm, bool print_comments) {
    FILE *file = fopen(filename_asm, "w");

    if (file == NULL)
        return -1;

    *gen = (CodeGenerator) {
        .file           = file,
        .rbp_offset     = 0,
        .print_comments = print_comments,
    };

    return 0;
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

Symbol gen_binop(CodeGenerator *gen, Symbol a, Symbol b, BinOpKind kind) {
    assert(a.kind != SYMBOLKIND_LABEL && b.kind != SYMBOLKIND_LABEL);
    assert(a.type == a.type);
    Type type = a.type;
    gen_comment(gen, "START: binop");

    gen->rbp_offset += type_get_size(type);
    size_t rbp_offset_a = a.stack_addr;
    size_t rbp_offset_b = b.stack_addr;
    const char *rax = type_get_register_rax(type);
    const char *rdi = type_get_register_rdi(type);

    FILE *f = gen->file;
    fprintf(f, "mov %s, [rbp-%lu]\n", rax, rbp_offset_a);
    fprintf(f, "mov %s, [rbp-%lu]\n", rdi, rbp_offset_b);

    switch (kind) {
        case BINOP_ADD:
            fprintf(f, "add %s, %s\n", rax, rdi);
            break;
        case BINOP_SUB:
            fprintf(f, "sub %s, %s\n", rax, rdi);
            break;
        case BINOP_MUL:
            fprintf(f, "imul %s\n", rdi);
            break;
        case BINOP_DIV:
            fprintf(f, "idiv %s\n", rdi);
            break;
        default:
            assert(!"Unimplemented");
            break;
    }

    fprintf(f, "sub rsp, %lu\n", type_get_size(type));
    fprintf(f, "mov %s [rbp-%lu], %s\n", type_get_size_operand(type), gen->rbp_offset, rax);

    gen_comment(gen, "END: binop\n");
    return (Symbol) {
        .kind       = SYMBOLKIND_ADDRESS,
        .stack_addr = gen->rbp_offset,
        .type       = type,
    };
}

Symbol gen_store_literal(CodeGenerator *gen, int64_t value, Type type) {
    gen->rbp_offset += type_get_size(type);
    gen_comment(gen, "START: store");

    FILE *f = gen->file;
    fprintf(f, "sub rsp, %lu\n", type_get_size(type));
    fprintf(
        f,
        "mov %s [rbp-%lu], %lu\n",
        type_get_size_operand(type),
        gen->rbp_offset,
        value
    );

    gen_comment(gen, "END: store\n");
    return (Symbol) {
        .kind       = SYMBOLKIND_ADDRESS,
        .stack_addr = gen->rbp_offset,
        .type       = type,
    };
}

// addrs is an array containing the addresses (rbp-offsets) of the arguments
void gen_inlineasm(
    CodeGenerator *gen,
    const char    *src,
    const Symbol  *symbols,
    size_t         symbols_len
) {
    gen_comment(gen, "START: inline");

    size_t arg_index = 0;
    for (size_t i=0; i < strlen(src); ++i) {

        if (src[i] == '{' && src[i+1] == '}') {
            Symbol sym = symbols[arg_index];
            assert(sym.kind != SYMBOLKIND_LABEL);
            fprintf(gen->file, "[rbp-%lu]", sym.stack_addr);
            i++;
            arg_index++;

        } else
            fprintf(gen->file, "%c", src[i]);

    }

    fprintf(gen->file, "\n");

    assert(arg_index == symbols_len && "argument count insufficent");

    gen_comment(gen, "END: inline\n");
}

void gen_func_start(CodeGenerator *c, const char *identifier) {
    gen_comment(c, "START: function");
    gen_comment(c, "START: function_prelude");

    FILE *f = c->file;
    fprintf(f, "global %s\n", identifier);
    fprintf(f, "%s:\n",       identifier);
    fprintf(f, "push rbp\n"             );
    fprintf(f, "mov rbp, rsp\n"         );

    gen_comment(c, "END: function_prelude\n");
}

void gen_func_end(CodeGenerator *c) {
    gen_comment(c, "START: function_postlude");

    FILE *f = c->file;
    fprintf(f, "mov rsp, rbp\n");
    fprintf(f, "pop rbp\n");
    fprintf(f, "ret\n"    );

    gen_comment(c, "END: function_postlude");
    gen_comment(c, "END: function\n");
}

// TODO: call into address
void gen_call(CodeGenerator *c, const char *identifier) {
    gen_comment(c, "START: call");

    fprintf(
        c->file,
        "call %s\n", identifier
    );

    gen_comment(c, "END: call\n");
}
