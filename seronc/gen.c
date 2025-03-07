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


size_t typekind_get_size(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return 1; break;
        case TYPE_INT:  return 4; break;
        case TYPE_SIZE: return 8; break;
        default:
            assert(!"Unknown Type");
            break;
    }
}

static const char *typekind_get_size_operand(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "byte";  break;
        case TYPE_INT:  return "dword"; break;
        case TYPE_SIZE: return "qword"; break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *typekind_get_register_rax(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "al";  break;
        case TYPE_INT:  return "eax"; break;
        case TYPE_SIZE: return "rax"; break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *typekind_get_register_rdi(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "dil"; break;
        case TYPE_INT:  return "edi"; break;
        case TYPE_SIZE: return "rdi"; break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *typekind_get_register_rsi(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "sil"; break;
        case TYPE_INT:  return "esi"; break;
        case TYPE_SIZE: return "rsi"; break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *typekind_get_register_rdx(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "dl";  break;
        case TYPE_INT:  return "edx"; break;
        case TYPE_SIZE: return "rdx"; break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *typekind_get_register_rcx(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "cl";  break;
        case TYPE_INT:  return "ecx"; break;
        case TYPE_SIZE: return "rcx"; break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *typekind_get_register_r8(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "r8b"; break;
        case TYPE_INT:  return "r8d"; break;
        case TYPE_SIZE: return "r8";  break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *typekind_get_register_r9(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "r9b"; break;
        case TYPE_INT:  return "r9d"; break;
        case TYPE_SIZE: return "r9";  break;
        default:
            assert(!"Unknown type");
            break;
    }
}

static const char *abi_get_register(int arg_n, TypeKind type) {
    assert(arg_n != 0);

    /* x86_64-linux ABI */
    /* first 6 arguments are stored in registers, the rest goes onto the stack */

    if (arg_n > 6)
        return NULL;

    const char *registers[] = {
        [1] = typekind_get_register_rdi(type),
        [2] = typekind_get_register_rsi(type),
        [3] = typekind_get_register_rdx(type),
        [4] = typekind_get_register_rcx(type),
        [5] = typekind_get_register_r8(type),
        [6] = typekind_get_register_r9(type),
    };

    return registers[arg_n];
}


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
        .label_count    = 0,
        .print_comments = print_comments,
    };

    return 0;
}

void gen_destroy(CodeGenerator *gen) {
    fclose(gen->file);
}

void gen_prelude(CodeGenerator *gen) {
    // TODO: different sections
    gen_addinstr(gen, "section .text");
}

gen_ctx gen_if_then(CodeGenerator *gen, Symbol cond) {
    gen_ctx ctx = gen->label_count;

    gen_comment(gen, "START: if");
    gen_addinstr(
        gen,
        "cmp %s [rbp-%lu], 0",
        typekind_get_size_operand(cond.type.kind),
        cond.stack_addr
    );
    gen_addinstr(gen, "je .else_%lu", ctx);

    gen->label_count++;
    return ctx;
}

void gen_if_else(CodeGenerator *gen, gen_ctx ctx) {
    gen_addinstr(gen, "jmp .end_%lu", ctx);
    gen_addinstr(gen, ".else_%lu:", ctx);
}

void gen_if_end(CodeGenerator *gen, gen_ctx ctx) {
    gen_addinstr(gen, ".end_%lu:", ctx);
    gen_comment(gen, "END: if\n");
}

gen_ctx gen_while_start(CodeGenerator *gen) {
    gen_ctx ctx = gen->label_count;

    gen_comment(gen, "START: while");
    gen_addinstr(gen, "jmp .cond_%lu", ctx);
    gen_addinstr(gen, ".while_%lu:", ctx);

    gen->label_count++;
    return ctx;
}

void gen_while_end(CodeGenerator *gen, Symbol cond, gen_ctx ctx) {
    gen_addinstr(gen, ".cond_%lu:", ctx);
    gen_addinstr(
        gen,
        "cmp %s [rbp-%lu], 0",
        typekind_get_size_operand(cond.type.kind),
        cond.stack_addr
    );
    gen_addinstr(gen, "jne .while_%lu", ctx);
    gen_comment(gen, "END: while\n");
}

Symbol gen_binop(CodeGenerator *gen, Symbol a, Symbol b, BinOpKind kind) {
    assert(a.type.kind == a.type.kind);
    Type type = a.type;
    gen_comment(gen, "START: binop");

    gen->rbp_offset += typekind_get_size(type.kind);
    size_t rbp_offset_a = a.stack_addr;
    size_t rbp_offset_b = b.stack_addr;
    const char *rax = typekind_get_register_rax(type.kind);
    const char *rdi = typekind_get_register_rdi(type.kind);

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

    gen_addinstr(gen, "sub rsp, %lu", typekind_get_size(type.kind));
    gen_addinstr(gen, "mov %s [rbp-%lu], %s", typekind_get_size_operand(type.kind), gen->rbp_offset, rax);

    gen_comment(gen, "END: binop\n");
    return (Symbol) {
        .stack_addr = gen->rbp_offset,
        .type       = type,
        .label      = NULL,
    };
}

Symbol gen_store_literal(CodeGenerator *gen, int64_t value, TypeKind type) {
    gen->rbp_offset += typekind_get_size(type);
    gen_comment(gen, "START: store");

    gen_addinstr(gen, "sub rsp, %lu", typekind_get_size(type));
    gen_addinstr(
        gen,
        "mov %s [rbp-%lu], %lu",
        typekind_get_size_operand(type),
        gen->rbp_offset,
        value
    );

    gen_comment(gen, "END: store\n");
    return (Symbol) {
        .stack_addr = gen->rbp_offset,
        .type       = (Type) { .kind = type },
        .label       = NULL,
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

void gen_procedure_extern(CodeGenerator *gen, const char *identifier) {
    gen_comment(gen, "START: extern proc");
    gen_addinstr(gen, "extern %s", identifier);
    gen_comment(gen, "END: extern proc");
}

void gen_procedure_start(
    CodeGenerator       *gen,
    const char          *identifier,
    const ProcSignature *sig,
    const Symboltable     *scope
) {
    gen->rbp_offset = 0;

    gen_comment(gen, "START: proc");
    gen_comment(gen, "START: proc_prelude");

    gen_addinstr(gen, "");
    gen_addinstr(gen, "global %s", identifier);
    gen_addinstr(gen, "%s:", identifier);
    gen_addinstr(gen, "push rbp");
    gen_addinstr(gen, "mov rbp, rsp");


    gen_comment(gen, "START: abi");

    for (size_t i=0; i < sig->params_count; ++i) {
        const Param *param = &sig->params[i];
        const char *reg = abi_get_register(i+1, param->type->kind);

        assert(reg != NULL && "TODO");

        size_t offset = typekind_get_size(param->type->kind);
        gen->rbp_offset += offset;
        gen_addinstr(gen, "sub rsp, %lu", offset);
        gen_addinstr(
            gen,
            "mov %s [rbp-%lu], %s",
            typekind_get_size_operand(param->type->kind),
            gen->rbp_offset,
            reg
        );

        // fill in address of params
        Symbol *sym = symboltable_list_lookup(scope, param->ident);
        assert(sym != NULL);
        sym->stack_addr = gen->rbp_offset;
    }

    gen_comment(gen, "END: abi");
    gen_comment(gen, "END: proc_prelude\n");
}

void gen_procedure_end(CodeGenerator *gen) {
    gen_comment(gen, "START: proc_postlude");

    gen_addinstr(gen, "mov rsp, rbp");
    gen_addinstr(gen, "pop rbp");
    gen_addinstr(gen, "ret");

    gen_comment(gen, "END: proc_postlude");
    gen_comment(gen, "END: proc\n");
}

void gen_return(CodeGenerator *gen, Symbol value) {
    gen_comment(gen, "START: return");

    gen_addinstr(
        gen,
        "mov %s, [rbp-%lu]",
        typekind_get_register_rax(value.type.kind),
        value.stack_addr
    );

    gen_comment(gen, "END: return\n");
}

Symbol gen_call(
    CodeGenerator *gen,
    Symbol         callee,
    const Symbol  *args,
    size_t         args_len
) {

    ProcSignature *sig = &callee.type.type_signature;

    gen_comment(gen, "START: call");
    gen_comment(gen, "START: abi");

    for (size_t i=0; i < args_len; ++i) {
        const Symbol *arg = &args[i];
        size_t argnum = i+1;

        // TODO: spill the rest of arguments onto stack

        const char *reg = abi_get_register(argnum, arg->type.kind);

        assert(reg != NULL && "TODO");
        gen_addinstr(gen, "mov %s, [rbp-%lu]", reg, arg->stack_addr);

    }
    gen_comment(gen, "END: abi");


    gen_addinstr(gen, "call %s", callee.label);

    TypeKind returntype = sig->returntype->kind;
    if (returntype == TYPE_VOID)
        return (Symbol) { .kind = SYMBOL_NONE };

    gen_addinstr(gen, "sub rsp, %lu", typekind_get_size(returntype));
    gen->rbp_offset += typekind_get_size(returntype);
    gen_addinstr(
        gen,
        "mov %s [rbp-%lu], %s",
        typekind_get_size_operand(returntype),
        gen->rbp_offset,
        typekind_get_register_rax(returntype)
    );
    gen_comment(gen, "END: call\n");

    return (Symbol) {
        .stack_addr = gen->rbp_offset,
        .type       = (Type) { .kind = returntype },
        .kind       = SYMBOL_VARIABLE,
    };
}

void gen_assign(CodeGenerator *gen, Symbol assignee, Symbol value) {
    assert(assignee.type.kind == value.type.kind);

    Type type = assignee.type;
    const char *rax = typekind_get_register_rax(type.kind);

    gen_comment(gen, "START: assignment");
    gen_addinstr(gen, "mov %s, [rbp-%lu]", rax, value.stack_addr);
    gen_addinstr(
        gen,
        "mov %s [rbp-%lu], %s", typekind_get_size_operand(type.kind),
        assignee.stack_addr,
        rax
    );
    gen_comment(gen, "END: assignment");
}
