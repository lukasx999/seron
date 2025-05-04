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
    REG_INVALID,

    REG_RAX,
    REG_RDI,
    REG_RSI,
    REG_RDX,
    REG_RCX,
    REG_R8,
    REG_R9,
} Register;



NO_DISCARD size_t get_type_size(TypeKind type) {
    switch (type) {
        case TYPE_LONG:
        case TYPE_POINTER: return 8;
        case TYPE_INT:     return 4;
        case TYPE_CHAR:    return 1;
        default:        PANIC("unknown type");
    }
}

NO_DISCARD static const char *size_op(TypeKind type) {
    switch (type) {
        case TYPE_LONG: return "qword";
        case TYPE_INT:  return "dword";
        case TYPE_CHAR: return "byte";
        default:        PANIC("unknown type");
    }
}

NO_DISCARD static const char *subregister(Register reg, TypeKind type) {
    switch (reg) {

        case REG_RAX: switch (type) {
            case TYPE_FUNCTION:
            case TYPE_LONG:
            case TYPE_POINTER: return "rax";
            case TYPE_INT:     return "eax";
            case TYPE_CHAR:    return "al";
            default: PANIC("unknown type");
        } break;

        case REG_RDI: switch (type) {
            case TYPE_FUNCTION:
            case TYPE_LONG:
            case TYPE_POINTER: return "rdi";
            case TYPE_INT:     return "edi";
            case TYPE_CHAR:    return "dil";
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

// argnum starts at 1
NO_DISCARD static Register abi_register(int argnum) {
    assert(argnum != 0);

    // x86_64-linux ABI
    // first 6 arguments are stored in registers, the rest goes on the stack

    if (argnum > 6)
        return REG_INVALID;

    Register registers[] = {
        [1] = REG_RDI,
        [2] = REG_RSI,
        [3] = REG_RDX,
        [4] = REG_RCX,
        [5] = REG_R8,
        [6] = REG_R9,
    };

    return registers[argnum];
}

NO_DISCARD static const char *abi_register_str(int argnum, TypeKind type) {
    Register reg = abi_register(argnum);
    if (reg == REG_INVALID) return NULL;
    return subregister(reg, type);
}




static Symbol create_symbol_temp(Type type) {
    return (Symbol) {
        .kind = SYMBOL_NONE,
        .type = type,
    };
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



static void emit_addr(AstNode *node);
static Symbol emit(AstNode *node);



// static Type call(const ExprCall *call) {
//
//     // TODO: emit callee
//     const char *ident  = call->callee->expr_literal.op.value;
//     Symbol *sym        = NON_NULL(symboltable_lookup(gen.scope, ident));
//     ProcSignature *sig = &sym->type.type_signature;
//
//     const AstNodeList *list = &call->args;
//     for (size_t i=0; i < list->size; ++i) {
//
//         TypeKind type = sig->params[i].type->kind;
//         const char *reg = abi_register_str(i+1, type);
//
//         emit(list->items[i]);
//
//         if (reg == NULL) {
//             gen_write("push rax");
//
//         } else {
//             const char *rax = subregister(REG_RAX, type);
//             gen_write("mov %s, %s", reg, rax);
//
//         }
//
//     }
//
//     gen_write("call %s", ident);
//     return *sig->returntype;
// }

static Symbol call(const ExprCall *call) {

    Symbol sym = emit(call->callee);
    gen_write("push rax");
    ProcSignature *sig = &sym.type.type_signature;

    const AstNodeList *list = &call->args;
    for (size_t i=0; i < list->size; ++i) {

        TypeKind type = sig->params[i].type->kind;
        const char *reg = abi_register_str(i+1, type);

        emit(list->items[i]);

        if (reg == NULL) {
            gen_write("push rax");

        } else {
            const char *rax = subregister(REG_RAX, type);
            gen_write("mov %s, %s", reg, rax);

        }

    }

    gen_write("pop rax");
    gen_write("call rax");
    return (Symbol) {
        .kind = SYMBOL_NONE,
        .type = *sig->returntype
    };
}

static void proc(const StmtProc *proc) {
    const char *ident  = proc->ident.value;
    const ProcSignature *sig = &proc->type.type_signature;

    if (proc->body == NULL) {
        gen_write("extern %s", ident);
        return;
    }

    gen_write("global %s", ident);
    gen_write("%s:", ident);
    gen_write("push rbp");
    gen_write("mov rbp, rsp");
    gen_write("sub rsp, %d", proc->stack_size);

    // offset starts at 16 because the old rbp and return address are
    // already on the stack
    int offset = 16;

    for (size_t i=0; i < sig->params_count; ++i) {

        const Param *param = &sig->params[i];
        const char *reg = abi_register_str(i+1, param->type->kind);

        Symbol *sym = NON_NULL(symboltable_lookup(proc->symboltable, param->ident));

        if (reg == NULL) {
            const char *rax = subregister(REG_RAX, param->type->kind);
            gen_write("mov %s, [rbp+%d]", rax, offset);
            gen_write("mov [rbp-%d], %s", sym->offset, rax);
            offset += 8;
        } else {
            gen_write("mov [rbp-%d], %s", sym->offset, reg);
        }

    }

    emit(proc->body);

    gen_write(".return:");
    gen_write("mov rsp, rbp");
    gen_write("pop rbp");
    gen_write("ret");

}

static void return_(const StmtReturn *ret) {
    if (ret->expr != NULL)
        emit(ret->expr);

    gen_write("jmp .return");
}

static void block(const Block *block) {
    Hashtable *old_scope = gen.scope;
    gen.scope = block->symboltable;

    AstNodeList list = block->stmts;
    for (size_t i=0; i < list.size; ++i)
        emit(list.items[i]);

    gen.scope = old_scope;
}

static void unaryop_addr(const ExprUnaryOp *unaryop) {

    emit(unaryop->node);

    switch (unaryop->kind) {

        case UNARYOP_DEREF:
            // address is already in rax, do nothing
            break;

        default: PANIC("unknown operation");
    }

}

static Symbol unaryop(const ExprUnaryOp *unaryop) {
    Symbol node = emit(unaryop->node);

    const char *rax = subregister(REG_RAX, node.type.kind);

    switch (unaryop->kind) {

        case UNARYOP_MINUS:
            gen_write("imul %s, -1", rax);
            break;

        case UNARYOP_DEREF: {
            gen_write("mov %s, [rax]", rax);
        } break;

        case UNARYOP_ADDROF: {
            // TODO: use symbol->kind for this
            bool is_literal = unaryop->node->kind == ASTNODE_LITERAL;
            bool is_ident = unaryop->node->expr_literal.kind == LITERAL_IDENT;
            if (!(is_ident && is_literal)) {
                compiler_message(MSG_ERROR, "Cannot take address of an rvalue");
                exit(EXIT_FAILURE);
            }

            const char *ident = unaryop->node->expr_literal.op.value;
            Symbol *sym = symboltable_lookup(gen.scope, ident);
            if (sym == NULL) {
                compiler_message(MSG_ERROR, "Unknown identifier `%s`", ident);
                exit(EXIT_FAILURE);
            }

            // TODO:
            assert(sym->kind == SYMBOL_VARIABLE || sym->kind == SYMBOL_PARAMETER);

            gen_write("lea rax, [rbp-%d]", sym->offset);

        } break;

        default: PANIC("unknown operation");
    }

    return node;

}

static Symbol binop(const ExprBinOp *binop) {

    Symbol rhs = emit(binop->rhs);
    const char *rdi = subregister(REG_RDI, rhs.type.kind);
    gen_write("push rax");

    Symbol lhs = emit(binop->lhs);
    const char *rax = subregister(REG_RAX, lhs.type.kind);
    gen_write("pop rdi");

    // if (rhs.kind != lhs.kind) {
    //     // TODO: print type names
    //     compiler_message(MSG_ERROR, "Incompatible types");
    //     exit(EXIT_FAILURE);
    // }

    switch (binop->kind) {
        case BINOP_ADD: gen_write("add %s, %s", rax, rdi); break;
        case BINOP_SUB: gen_write("sub %s, %s", rax, rdi); break;
        case BINOP_MUL: gen_write("imul %s", rdi);         break;
        case BINOP_DIV: gen_write("idiv %s", rdi);         break;
        default:        PANIC("unknown operation");
    }

    return rhs;

}

static Symbol literal_ident(const char *str, bool addr) {

    Symbol *sym = symboltable_lookup(gen.scope, str);

    if (sym == NULL) {
        compiler_message(MSG_ERROR, "Symbol `%s` does not exist in the current scope", str);
        exit(EXIT_FAILURE);
    }

    switch (sym->kind) {
        case SYMBOL_VARIABLE:
        case SYMBOL_PARAMETER:
            if (addr)
                gen_write("lea rax, [rbp-%d]", sym->offset);
            else
                gen_write("mov %s, [rbp-%d]", subregister(REG_RAX, sym->type.kind), sym->offset);
            break;

        case SYMBOL_PROCEDURE:
            gen_write("mov rax, %s", str);
            break;

        default: PANIC("invalid symbol");
    }

    return *sym;
}

static void literal_addr(const ExprLiteral *literal) {

    const char *str = literal->op.value;

    switch (literal->kind) {

        case LITERAL_IDENT:
            literal_ident(str, true);
            break;

        default: PANIC("unknown operation");
    }

}

static Symbol literal(const ExprLiteral *literal) {

    const char *str = literal->op.value;

    switch (literal->kind) {
        // TODO: string literal

        case LITERAL_CHAR:
        case LITERAL_NUMBER: {

            // FIXME: find a better way to do this
            bool is_long = str[strlen(str)-1] == 'L';
            bool is_char = literal->kind == LITERAL_CHAR;
            TypeKind type =
                is_long ? TYPE_LONG :
                is_char ? TYPE_CHAR :
                TYPE_INT;

            gen_write("mov %s, %d", subregister(REG_RAX, type), atoll(str));
            return create_symbol_temp((Type) { .kind = type });
        } break;

        case LITERAL_IDENT:
            return literal_ident(str, false);
            break;

        default: PANIC("unknown operation");
    }

    UNREACHABLE();

}

static Symbol grouping(const ExprGrouping *grouping) {
    return emit(grouping->expr);
}

static void cond(const StmtIf *cond) {

    int lbl = gen.label_count++;
    Symbol sym = emit(cond->condition);
    const char *rax = subregister(REG_RAX, sym.type.kind);

    // IF
    gen_write("cmp %s, 0", rax);
    gen_write("je .else%d", lbl);

    // THEN
    emit(cond->then_body);

    // ELSE
    gen_write("jmp .end%d", lbl);
    gen_write(".else%d:", lbl);

    if (cond->else_body != NULL)
        emit(cond->else_body);

    // END
    gen_write(".end%d:", lbl);

}

static void while_(const StmtWhile *loop) {

    int lbl = gen.label_count++;

    // WHILE
    gen_write("jmp .cond%d", lbl);
    gen_write(".while%d:", lbl);

    // DO
    emit(loop->body);

    // END
    gen_write(".cond%lu:", lbl);
    Symbol cond = emit(loop->condition);
    const char *rax = subregister(REG_RAX, cond.type.kind);
    gen_write("cmp %s, 0", rax);
    gen_write("jne .while%lu", lbl);

}

static void vardecl(const StmtVarDecl *decl) {

    if (decl->init == NULL) return;

    Symbol init = emit(decl->init);
    Symbol *sym = NON_NULL(symboltable_lookup(gen.scope, decl->ident.value));
    const char *rax = subregister(REG_RAX, init.type.kind);
    gen_write("mov [rbp-%d], %s", sym->offset, rax);

}


static Symbol assign(const ExprAssign *assign) {

    emit_addr(assign->target);
    gen_write("push rax");
    Symbol value = emit(assign->value);

    gen_write("pop rdi");
    gen_write("mov [rdi], %s", subregister(REG_RAX, value.type.kind));

    return create_symbol_temp(value.type);

}

// get address of lvalue
static void emit_addr(AstNode *node) {
    NON_NULL(node);

    switch (node->kind) {
        case ASTNODE_UNARYOP: unaryop_addr(&node->expr_unaryop); break;
        case ASTNODE_LITERAL: literal_addr(&node->expr_literal); break;
        default:              PANIC("unexpected node kind");
    }


}

static Symbol emit(AstNode *node) {
    NON_NULL(node);

    switch (node->kind) {
        case ASTNODE_BLOCK:     block           (&node->block);         break;
        case ASTNODE_WHILE:     while_          (&node->stmt_while);    break;
        case ASTNODE_PROC:      proc            (&node->stmt_proc);     break;
        case ASTNODE_RETURN:    return_         (&node->stmt_return);   break;
        case ASTNODE_VARDECL:   vardecl         (&node->stmt_vardecl);  break;
        case ASTNODE_IF:        cond            (&node->stmt_if);       break;
        case ASTNODE_GROUPING:  return grouping (&node->expr_grouping); break;
        case ASTNODE_ASSIGN:    return assign   (&node->expr_assign);   break;
        case ASTNODE_BINOP:     return binop    (&node->expr_binop);    break;
        case ASTNODE_CALL:      return call     (&node->expr_call);     break;
        case ASTNODE_UNARYOP:   return unaryop  (&node->expr_unaryop);  break;
        case ASTNODE_LITERAL:   return literal  (&node->expr_literal);  break;
        default:                PANIC("unexpected node kind");
    }

    return (Symbol) { .kind = SYMBOL_INVALID };

}

void codegen(AstNode *root) {
    gen_init(compiler_config.filename.asm_);
    emit(root);
    gen_destroy();
}
