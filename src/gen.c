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







static void gen_addinstr(CodeGenerator *gen, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(gen->file, fmt, va);
    fprintf(gen->file, "\n");
    va_end(va);
}

static void gen_comment(CodeGenerator *gen, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    if (gen->print_comments) {
        fprintf(gen->file, "; ");
        vfprintf(gen->file, fmt, va);
        fprintf(gen->file, "\n");
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

void gen_destroy(CodeGenerator *gen) {
    fclose(gen->file);
}

void gen_prelude(CodeGenerator *gen) {
    // TODO: different sections
    gen_addinstr(gen, "section .text\n");
}

void gen_if_then(CodeGenerator *gen, Symbol cond) {
    assert(cond.kind != SYMBOLKIND_LABEL);

    FILE *f = gen->file;
    gen_addinstr(
        gen,
        "cmp %s [rbp-%lu], 0",
        type_get_size_operand(cond.type),
        cond.stack_addr
    );
    gen_addinstr(gen, "je .else");
}

void gen_if_else(CodeGenerator *gen) {
    FILE *f = gen->file;
    gen_addinstr(gen, "jmp .end");
    gen_addinstr(gen, ".else:");
}

void gen_if_end(CodeGenerator *gen) {
    FILE *f = gen->file;
    gen_addinstr(gen, ".end:");
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
    gen_addinstr(gen, "mov %s, [rbp-%lu]", rax, rbp_offset_a);
    gen_addinstr(gen, "mov %s, [rbp-%lu]", rdi, rbp_offset_b);

    switch (kind) {
        case BINOP_ADD:
            gen_addinstr(gen, "add %s, %s", rax, rdi);
            break;
        case BINOP_SUB:
            gen_addinstr(gen, "sub %s, %s", rax, rdi);
            break;
        case BINOP_MUL:
            gen_addinstr(gen, "imul %s", rdi);
            break;
        case BINOP_DIV:
            gen_addinstr(gen, "idiv %s", rdi);
            break;
        default:
            assert(!"Unimplemented");
            break;
    }

    gen_addinstr(gen, "sub rsp, %lu", type_get_size(type));
    gen_addinstr(gen, "mov %s [rbp-%lu], %s", type_get_size_operand(type), gen->rbp_offset, rax);

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
    gen_addinstr(gen, "sub rsp, %lu", type_get_size(type));
    gen_addinstr(
        gen,
        "mov %s [rbp-%lu], %lu",
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

void gen_func_start(CodeGenerator *gen, const char *identifier) {
    gen_comment(gen, "START: function");
    gen_comment(gen, "START: function_prelude");

    FILE *f = gen->file;
    gen_addinstr(gen, "global %s", identifier);
    gen_addinstr(gen, "%s:",       identifier);
    gen_addinstr(gen, "push rbp"             );
    gen_addinstr(gen, "mov rbp, rsp"         );

    gen_comment(gen, "END: function_prelude\n");
}

void gen_func_end(CodeGenerator *gen) {
    gen_comment(gen, "START: function_postlude");

    FILE *f = gen->file;
    gen_addinstr(gen, "mov rsp, rbp");
    gen_addinstr(gen, "pop rbp");
    gen_addinstr(gen, "ret");

    gen_comment(gen, "END: function_postlude");
    gen_comment(gen, "END: function\n");
}

// TODO: call into address
void gen_call(CodeGenerator *gen, const char *identifier) {
    gen_comment(gen, "START: call");
    gen_addinstr(gen, "call %s", identifier);
    gen_comment(gen, "END: call\n");
}
