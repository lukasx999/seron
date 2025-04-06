#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <alloca.h>

#include "types.h"
#include "util.h"
#include "lexer.h"
#include "symboltable.h"
#include "main.h"
#include "parser.h"



size_t typekind_get_size(TypeKind type) {
    switch (type) {
        case TYPE_CHAR: return 1;
        case TYPE_INT:  return 4;
        default: assert(!"unknown type");
    }
}

static const char *typekind_get_size_operand(TypeKind type) {
    switch (type) {
        case TYPE_CHAR: return "byte";
        case TYPE_INT:  return "dword";
        default: assert(!"unknown type");
    }
}

static const char *typekind_get_subregister(Register reg, TypeKind type) {
    switch (reg) {

        case REG_RAX: switch (type) {
            case TYPE_INT:  return "eax";
            case TYPE_CHAR: return "al";
            default: assert(!"unknown type");
        } break;

        case REG_RDI: switch (type) {
            case TYPE_INT:  return "edi";
            case TYPE_CHAR: return "dil";
            default: assert(!"unknown type");
        } break;

        case REG_RSI: switch (type) {
            case TYPE_INT:  return "esi";
            case TYPE_CHAR: return "sil";
            default: assert(!"unknown type");
        } break;

        case REG_RDX: switch (type) {
            case TYPE_INT:  return "edx";
            case TYPE_CHAR: return "dl";
            default: assert(!"unknown type");
        } break;

        case REG_RCX: switch (type) {
            case TYPE_INT:  return "ecx";
            case TYPE_CHAR: return "cl";
            default: assert(!"unknown type");
        } break;

        case REG_R8: switch (type) {
            case TYPE_INT:  return "r8d";
            case TYPE_CHAR: return "r8b";
            default: assert(!"unknown type");
        } break;

        case REG_R9: switch (type) {
            case TYPE_INT:  return "r9d";
            case TYPE_CHAR: return "r9b";
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





struct Gen {
    FILE *file;
    int label_count;
} gen = { 0 };

static void gen_init(const char *out_file) {
    gen.file = fopen(out_file, "w");
    if (gen.file == NULL) {
        compiler_message(MSG_ERROR, "Failed to open output file %s", out_file);
        exit(EXIT_FAILURE);
    }
}

static void gen_destroy(void) {
    fclose(gen.file);
}

static void gen_write(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(gen.file, fmt, va);
    fprintf(gen.file, "\n");
    va_end(va);
}



static void emit(AstNode *node);

static void procedure(const StmtProcedure *proc) {
    const char *ident  = proc->identifier.value;
    const ProcSignature *sig = &proc->type.type_signature;
    (void) sig;
    // TODO: ABI

    if (proc->body == NULL) {
        gen_write("extern %s", ident);
        return;
    }

    gen_write("global %s", ident);
    gen_write("%s:", ident);
    gen_write("push rbp");
    gen_write("mov rbp, rsp");
    gen_write("sub rsp, %lu", proc->stack_size);

    emit(proc->body);

    gen_write("mov rsp, rbp");
    gen_write("pop rbp");
    gen_write("ret");

}

static void return_(const StmtReturn *ret) {
    // TODO: early return
    emit(ret->expr);
}

static void block(const Block *block) {

    AstNodeList list = block->stmts;

    for (size_t i=0; i < list.size; ++i)
        emit(list.items[i]);
}

static void unaryop(const ExprUnaryOp *unaryop) {
    emit(unaryop->node);

    switch (unaryop->kind) {
        case UNARYOP_MINUS: gen_write("imul rax, -1"); break;
        default:            assert(!"unimplemented");
    }

}

static void binop(const ExprBinOp *binop) {

    emit(binop->rhs);
    gen_write("push rax");

    emit(binop->lhs);
    gen_write("pop rdi");

    switch (binop->kind) {
        case BINOP_ADD: gen_write("add rax, rdi"); break;
        case BINOP_SUB: gen_write("sub rax, rdi"); break;
        case BINOP_MUL: gen_write("imul rdi");     break;
        case BINOP_DIV: gen_write("idiv rdi");     break;
        default:        assert(!"unimplemented");
    }

}

static void literal(const ExprLiteral *literal) {
    switch (literal->op.kind) {

        case TOK_NUMBER: {
            int num = atoll(literal->op.value);
            typekind_get_subregister(REG_RAX, TYPE_INT);
            gen_write("mov rax, %d", num);
        } break;

        default: assert(!"unimplemented");
    }
}

static void grouping(const ExprGrouping *grouping) {
    emit(grouping->expr);
}

static void cond(const StmtIf *cond) {

    int lbl = gen.label_count++;
    emit(cond->condition);

    // IF
    gen_write("cmp rax, 0");
    gen_write("je .else_%d", lbl);

    // THEN
    emit(cond->then_body);

    // ELSE
    gen_write("jmp .end_%d", lbl);
    gen_write(".else_%d:", lbl);

    if (cond->else_body != NULL)
        emit(cond->else_body);

    // END
    gen_write(".end_%d:", lbl);

}

static void emit(AstNode *node) {
    assert(node != NULL);

    switch (node->kind) {
        case ASTNODE_BLOCK:     block     (&node->block);          break;
        case ASTNODE_GROUPING:  grouping  (&node->expr_grouping);  break;
        case ASTNODE_PROCEDURE: procedure (&node->stmt_procedure); break;
        case ASTNODE_RETURN:    return_   (&node->stmt_return);    break;
        case ASTNODE_IF:        cond      (&node->stmt_if);        break;
        case ASTNODE_BINOP:     binop     (&node->expr_binop);     break;
        case ASTNODE_UNARYOP:   unaryop   (&node->expr_unaryop);   break;
        case ASTNODE_LITERAL:   literal   (&node->expr_literal);   break;
        default: assert(!"unexpected node kind");
    }

}

void codegen(AstNode *root) {
    gen_init(compiler_config.filename.asm_);
    emit(root);
    gen_destroy();
}
