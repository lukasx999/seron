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
        case TYPE_BYTE: return 1;
        case TYPE_INT:  return 4;
        case TYPE_SIZE: return 8;
        default: assert(!"unknown type");
    }
}

static const char *typekind_get_size_operand(TypeKind type) {
    switch (type) {
        case TYPE_BYTE: return "byte";
        case TYPE_INT:  return "dword";
        case TYPE_SIZE: return "qword";
        default: assert(!"unknown type");
    }
}

static const char *typekind_get_subregister(Register reg, TypeKind type) {
    switch (reg) {

        case REG_RAX: switch (type) {
            case TYPE_SIZE: return "rax";
            case TYPE_INT:  return "eax";
            case TYPE_BYTE: return "al";
            default: assert(!"unknown type");
        } break;

        case REG_RDI: switch (type) {
            case TYPE_SIZE: return "rdi";
            case TYPE_INT:  return "edi";
            case TYPE_BYTE: return "dil";
            default: assert(!"unknown type");
        } break;

        case REG_RSI: switch (type) {
            case TYPE_SIZE: return "rsi";
            case TYPE_INT:  return "esi";
            case TYPE_BYTE: return "sil";
            default: assert(!"unknown type");
        } break;

        case REG_RDX: switch (type) {
            case TYPE_SIZE: return "rdx";
            case TYPE_INT:  return "edx";
            case TYPE_BYTE: return "dl";
            default: assert(!"unknown type");
        } break;

        case REG_RCX: switch (type) {
            case TYPE_SIZE: return "rcx";
            case TYPE_INT:  return "ecx";
            case TYPE_BYTE: return "cl";
            default: assert(!"unknown type");
        } break;

        case REG_R8: switch (type) {
            case TYPE_SIZE: return "r8";
            case TYPE_INT:  return "r8d";
            case TYPE_BYTE: return "r8b";
            default: assert(!"unknown type");
        } break;

        case REG_R9: switch (type) {
            case TYPE_SIZE: return "r9";
            case TYPE_INT:  return "r9d";
            case TYPE_BYTE: return "r9b";
            default: assert(!"unknown type");
        } break;

        default: assert(!"unknown register");
    }
}

static const char *abi_get_register(int arg_n, TypeKind type) {
    assert(arg_n != 0);

    /* x86_64-linux ABI */
    /* first 6 arguments are stored in registers, the rest goes onto the stack */

    if (arg_n > 6)
        return NULL;

    const char *registers[] = {
        [1] = typekind_get_subregister(REG_RDI, type),
        [2] = typekind_get_subregister(REG_RSI, type),
        [3] = typekind_get_subregister(REG_RDX, type),
        [4] = typekind_get_subregister(REG_RCX, type),
        [5] = typekind_get_subregister(REG_R8, type),
        [6] = typekind_get_subregister(REG_R9, type),
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

//
// Moves the specified symbol `sym`, regardless of its storage location into
// the specified register `reg`
//
static void gen_move_symbol_into_register(CodeGenerator *gen, Register reg, const Symbol *sym) {
    // TODO: label

    const char *reg_new = typekind_get_subregister(reg, sym->type.kind);

    switch (sym->kind) {

        case SYMBOL_VARIABLE:
        case SYMBOL_PARAMETER:
            gen_addinstr(gen, "mov %s, [rbp-%lu]", reg_new, sym->stack_addr);
            break;

        case SYMBOL_TEMPORARY: {
            const char *reg_old = typekind_get_subregister(sym->reg, sym->type.kind);

            if (reg != sym->reg)
                gen_addinstr(gen, "mov %s, %s", reg_new, reg_old);

        } return;

        default:
            assert(!"unknown symbol kind");
    }
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
        .label_count    = 0,
        .print_comments = print_comments,
    };

    return 0;
}

void gen_destroy(CodeGenerator *gen) {
    fclose(gen->file);
}

void gen_prelude(CodeGenerator *gen) {
    gen_addinstr(gen, "section .text");
}


Symbol gen_binop(
    CodeGenerator *gen,
    const Symbol  *lhs,
    const Symbol  *rhs,
    BinOpKind      kind
) {

    assert(lhs->type.kind == lhs->type.kind);
    gen_comment(gen, "START: binop");

    // TODO: evaluate subtree here!
    gen_move_symbol_into_register(gen, REG_RAX, lhs);
    gen_move_symbol_into_register(gen, REG_RDI, rhs);

    const char *rax = typekind_get_subregister(REG_RAX, lhs->type.kind);
    const char *rdi = typekind_get_subregister(REG_RDI, lhs->type.kind);

    switch (kind) {
        case BINOP_ADD: gen_addinstr(gen, "add %s, %s", rax, rdi); break;
        case BINOP_SUB: gen_addinstr(gen, "sub %s, %s", rax, rdi); break;
        case BINOP_MUL: gen_addinstr(gen, "imul %s", rdi);         break;
        case BINOP_DIV: gen_addinstr(gen, "idiv %s", rdi);         break;
        default: assert(!"unimplemented");
    }

    static int i = 0;
    Register reg = i++ % 2 == 0 ? REG_RSI : REG_RDX;

    const char *out = typekind_get_subregister(reg, lhs->type.kind);
    gen_addinstr(gen, "mov %s, %s", out, rax);

    gen_comment(gen, "END: binop\n");
    return (Symbol) {
        .kind = SYMBOL_TEMPORARY,
        .type = lhs->type,
        .reg  = reg,
    };
}

Symbol gen_store_literal(CodeGenerator *gen, int64_t value, TypeKind type) {
    gen_comment(gen, "START: store");

    static int i = 0;
    Register reg = i++ % 2 == 0 ? REG_RSI : REG_RDX;

    const char *rax = typekind_get_subregister(reg, type);
    gen_addinstr(gen, "mov %s, %lu", rax, value);

    gen_comment(gen, "END: store");
    return (Symbol) {
        .kind = SYMBOL_TEMPORARY,
        .type = (Type) { .kind = type },
        .reg  = reg,
    };
}

void gen_procedure_start(
    CodeGenerator       *gen,
    const char          *identifier,
    uint64_t             stack_size,
    const ProcSignature *sig,
    const Symboltable   *scope
) {

    gen_comment(gen, "START: proc");

    gen_comment(gen, "START: proc_prelude");
    gen_addinstr(gen, "");
    gen_addinstr(gen, "global %s", identifier);
    gen_addinstr(gen, "%s:", identifier);
    gen_addinstr(gen, "push rbp");
    gen_addinstr(gen, "mov rbp, rsp");
    gen_addinstr(gen, "sub rsp, %lu", stack_size);
    gen_comment(gen, "END: proc_prelude\n");

    gen_comment(gen, "START: abi");
    // Move parameters onto stack
    for (size_t i=0; i < sig->params_count; ++i) {
        const Param *param   = &sig->params[i];
        const char  *reg     = abi_get_register(i+1, param->type->kind);
        const char  *size_op = typekind_get_size_operand(param->type->kind);

        assert(reg != NULL && "TODO");

        Symbol *sym = symboltable_list_lookup(scope, param->ident);
        assert(sym != NULL);
        gen_addinstr(gen, "mov %s [rbp-%lu], %s", size_op, sym->stack_addr, reg);

    }
    gen_comment(gen, "END: abi");
}

void gen_procedure_end(CodeGenerator *gen) {
    gen_comment(gen, "START: proc_postlude");

    gen_addinstr(gen, "mov rsp, rbp");
    gen_addinstr(gen, "pop rbp");
    gen_addinstr(gen, "ret");

    gen_comment(gen, "END: proc_postlude");
    gen_comment(gen, "END: proc\n");
}

void gen_return(CodeGenerator *gen, const Symbol *value) {
    gen_comment(gen, "START: return");

    // TODO: label for early return
    gen_move_symbol_into_register(gen, REG_RAX, value);

    gen_comment(gen, "END: return\n");
}

void gen_procedure_extern(CodeGenerator *gen, const char *ident) {
    gen_comment(gen, "START: extern proc");
    gen_addinstr(gen, "extern %s", ident);
    gen_comment(gen, "END: extern proc");
}

void gen_var_init(CodeGenerator *gen, const Symbol *var, const Symbol *init) {
    gen_comment(gen, "START: var init");

    Register reg = REG_RAX;
    gen_move_symbol_into_register(gen, reg, init);
    const char *rax = typekind_get_subregister(reg, init->type.kind);
    const char *size_op = typekind_get_size_operand(init->type.kind);

    gen_addinstr(
        gen,
        "mov %s [rbp-%lu], %s",
        size_op,
        var->stack_addr,
        rax
    );

    gen_comment(gen, "END: var init");
}

gen_ctx_t gen_if_then(CodeGenerator *gen, const Symbol *cond) {
    gen_ctx_t ctx = gen->label_count;
    const char *size_op = typekind_get_size_operand(cond->type.kind);

    gen_comment(gen, "START: if");
    gen_addinstr(gen, "cmp %s [rbp-%lu], 0", size_op, cond->stack_addr);
    gen_addinstr(gen, "je .else_%lu", ctx);

    gen->label_count++;
    return ctx;
}

void gen_if_else(CodeGenerator *gen, gen_ctx_t ctx) {
    gen_addinstr(gen, "jmp .end_%lu", ctx);
    gen_addinstr(gen, ".else_%lu:", ctx);
}

void gen_if_end(CodeGenerator *gen, gen_ctx_t ctx) {
    gen_addinstr(gen, ".end_%lu:", ctx);
    gen_comment(gen, "END: if\n");
}

gen_ctx_t gen_while_start(CodeGenerator *gen) {
    gen_ctx_t ctx = gen->label_count;

    gen_comment(gen, "START: while");
    gen_addinstr(gen, "jmp .cond_%lu", ctx);
    gen_addinstr(gen, ".while_%lu:", ctx);

    gen->label_count++;
    return ctx;
}

void gen_while_end(CodeGenerator *gen, const Symbol *cond, gen_ctx_t ctx) {
    const char *size_op = typekind_get_size_operand(cond->type.kind);

    gen_addinstr(gen, ".cond_%lu:", ctx);
    gen_addinstr(
        gen,
        "cmp %s [rbp-%lu], 0",
        size_op,
        cond->stack_addr
    );
    gen_addinstr(gen, "jne .while_%lu", ctx);
    gen_comment(gen, "END: while\n");
}

void gen_assign(CodeGenerator *gen, const Symbol *assignee, const Symbol *value) {
    assert(assignee->type.kind == value->type.kind);

    // TODO: support static variables at some point
    assert(assignee->kind == SYMBOL_VARIABLE || assignee->kind == SYMBOL_PARAMETER);
    gen_move_symbol_into_register(gen, REG_RAX, value);

    const Type *type    = &assignee->type;
    const char *size_op = typekind_get_size_operand(type->kind);
    const char *rax     = typekind_get_subregister(REG_RAX, type->kind);

    gen_comment(gen, "START: assignment");
    gen_addinstr(gen, "mov %s [rbp-%lu], %s", size_op, assignee->stack_addr, rax);
    gen_comment(gen, "END: assignment");
}








// TODO: REVIEW:


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


Symbol gen_call(CodeGenerator *gen, Symbol callee, const Symbol *args, size_t args_len) { (void) gen, (void) callee, (void) args, (void) args_len; return (Symbol) { 0 }; }


/*
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
        .kind       = SYMBOL_TEMPORARY,
        .type       = (Type) { .kind = returntype },
        .stack_addr = gen->rbp_offset,
    };
}
*/
