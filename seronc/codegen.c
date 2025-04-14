#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <alloca.h>

#include "util.h"
#include "lexer.h"
#include "main.h"
#include "parser.h"
#include "symboltable.h"
#include "lib/util.h"



typedef enum {
    REG_RAX,
    REG_RDI,
    REG_RSI,
    REG_RDX,
    REG_RCX,
    REG_R8,
    REG_R9,
} Register;



size_t get_type_size(TypeKind type) {
    switch (type) {
        case TYPE_CHAR: return 1;
        case TYPE_INT:  return 8;
        default:        PANIC("unknown type");
    }
}

UNUSED static const char *typekind_get_size_operand(TypeKind type) {
    switch (type) {
        case TYPE_CHAR: return "byte";
        case TYPE_INT:  return "qword";
        default:        PANIC("unknown type");
    }
}

static const char *typekind_get_subregister(Register reg, TypeKind type) {
    switch (reg) {

        case REG_RAX: switch (type) {
            case TYPE_INT:  return "eax";
            case TYPE_CHAR: return "al";
            default: PANIC("unknown type");
        } break;

        case REG_RDI: switch (type) {
            case TYPE_INT:  return "edi";
            case TYPE_CHAR: return "dil";
            default: PANIC("unknown type");
        } break;

        case REG_RSI: switch (type) {
            case TYPE_INT:  return "esi";
            case TYPE_CHAR: return "sil";
            default: PANIC("unknown type");
        } break;

        case REG_RDX: switch (type) {
            case TYPE_INT:  return "edx";
            case TYPE_CHAR: return "dl";
            default: PANIC("unknown type");
        } break;

        case REG_RCX: switch (type) {
            case TYPE_INT:  return "ecx";
            case TYPE_CHAR: return "cl";
            default: PANIC("unknown type");
        } break;

        case REG_R8: switch (type) {
            case TYPE_INT:  return "r8d";
            case TYPE_CHAR: return "r8b";
            default: PANIC("unknown type");
        } break;

        case REG_R9: switch (type) {
            case TYPE_INT:  return "r9d";
            case TYPE_CHAR: return "r9b";
            default: PANIC("unknown type");
        } break;

        default: PANIC("unknown register");
    }
}

UNUSED static const char *abi_get_register(int arg_n, TypeKind type) {
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
    Hashtable *scope;
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

static void proc(const StmtProc *proc) {
    const char *ident  = proc->ident.value;
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
    if (ret->expr != NULL) {
        emit(ret->expr);
    }
}

static void block(const Block *block) {
    Hashtable *old_scope = gen.scope;
    gen.scope = block->symboltable;

    AstNodeList list = block->stmts;
    for (size_t i=0; i < list.size; ++i)
        emit(list.items[i]);

    gen.scope = old_scope;
}

static void unaryop(const ExprUnaryOp *unaryop) {
    emit(unaryop->node);

    switch (unaryop->kind) {
        case UNARYOP_MINUS: gen_write("imul rax, -1"); break;
        default:            PANIC("unknown operation");
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
        default:        PANIC("unknown operation");
    }

}

static void literal(const ExprLiteral *literal) {

    const char *str = literal->op.value;

    switch (literal->kind) {

        case LITERAL_NUMBER: {
            int num = atoll(str);
            typekind_get_subregister(REG_RAX, TYPE_INT);
            gen_write("mov rax, %d", num);
        } break;

        case LITERAL_IDENT: {
            Symbol *sym = symboltable_lookup(gen.scope, str);
            if (sym == NULL) {
                compiler_message(MSG_ERROR, "Variable `%s` does not exist in the current scope", str);
                exit(EXIT_FAILURE);
            }
            gen_write("mov rax, [rbp-%d]", sym->offset);
        } break;

        default: PANIC("unknown operation");
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

static void while_(const StmtWhile *loop) {

    int lbl = gen.label_count++;

    // WHILE
    gen_write("jmp .cond_%d", lbl);
    gen_write(".while_%d:", lbl);

    // DO
    emit(loop->body);

    // END
    emit(loop->condition);
    gen_write(".cond_%lu:", lbl);
    gen_write("cmp rax, 0");
    gen_write("jne .while_%lu", lbl);

}

static void vardecl(const StmtVarDecl *decl) {

    if (decl->init == NULL) return;

    emit(decl->init);
    // TODO: maybe include symbol info in astnode itself?
    Symbol *sym = NON_NULL(symboltable_lookup(gen.scope, decl->ident.value));
    gen_write("mov [rbp-%d], rax", sym->offset);

}

static void emit(AstNode *node) {
    NON_NULL(node);

    switch (node->kind) {
        case ASTNODE_BLOCK:     block    (&node->block);          break;
        case ASTNODE_GROUPING:  grouping (&node->expr_grouping);  break;
        case ASTNODE_PROCEDURE: proc     (&node->stmt_proc);      break;
        case ASTNODE_RETURN:    return_  (&node->stmt_return);    break;
        case ASTNODE_VARDECL:   vardecl  (&node->stmt_vardecl);   break;
        case ASTNODE_IF:        cond     (&node->stmt_if);        break;
        case ASTNODE_WHILE:     while_   (&node->stmt_while);     break;
        case ASTNODE_BINOP:     binop    (&node->expr_binop);     break;
        case ASTNODE_UNARYOP:   unaryop  (&node->expr_unaryop);   break;
        case ASTNODE_LITERAL:   literal  (&node->expr_literal);   break;
        default:                PANIC("unexpected node kind");
    }

}

void codegen(AstNode *root) {
    gen_init(compiler_config.filename.asm_);
    emit(root);
    gen_destroy();
}
