#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include <ver.h>

#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "symboltable.h"



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




NO_DISCARD static const char *size_op(TypeKind type) {
    switch (type) {
        case TYPE_POINTER:
        case TYPE_LONG: return "qword";
        case TYPE_INT:  return "dword";
        case TYPE_CHAR: return "byte";
        default: PANIC("invalid type");
    }
    UNREACHABLE();
}

NO_DISCARD static const char *subregister(Register reg, TypeKind type) {
    switch (reg) {

        case REG_RAX: switch (type) {
            case TYPE_PROCEDURE:
            case TYPE_LONG:
            case TYPE_OBJECT:
            case TYPE_POINTER: return "rax";
            case TYPE_INT:     return "eax";
            case TYPE_CHAR:    return "al";
            default: PANIC("invalid type");
        } break;

        case REG_RDI: switch (type) {
            case TYPE_PROCEDURE:
            case TYPE_LONG:
            case TYPE_OBJECT:
            case TYPE_POINTER: return "rdi";
            case TYPE_INT:     return "edi";
            case TYPE_CHAR:    return "dil";
            default: PANIC("invalid type");
        } break;

        case REG_RSI: switch (type) {
            case TYPE_PROCEDURE:
            case TYPE_LONG:
            case TYPE_OBJECT:
            case TYPE_POINTER: return "rsi";
            case TYPE_INT:     return "esi";
            case TYPE_CHAR:    return "sil";
            default: PANIC("invalid type");
        } break;

        case REG_RDX: switch (type) {
            case TYPE_PROCEDURE:
            case TYPE_POINTER:
            case TYPE_OBJECT:
            case TYPE_LONG: return "rdx";
            case TYPE_INT:  return "edx";
            case TYPE_CHAR: return "dl";
            default: PANIC("invalid type");
        } break;

        case REG_RCX: switch (type) {
            case TYPE_PROCEDURE:
            case TYPE_POINTER:
            case TYPE_OBJECT:
            case TYPE_LONG: return "rcx";
            case TYPE_INT:  return "ecx";
            case TYPE_CHAR: return "cl";
            default: PANIC("invalid type");
        } break;

        case REG_R8: switch (type) {
            case TYPE_PROCEDURE:
            case TYPE_POINTER:
            case TYPE_OBJECT:
            case TYPE_LONG: return "r8";
            case TYPE_INT:  return "r8d";
            case TYPE_CHAR: return "r8b";
            default: PANIC("invalid type");
        } break;

        case REG_R9: switch (type) {
            case TYPE_PROCEDURE:
            case TYPE_POINTER:
            case TYPE_OBJECT:
            case TYPE_LONG: return "r9";
            case TYPE_INT:  return "r9d";
            case TYPE_CHAR: return "r9b";
            default: PANIC("invalid type");
        } break;

        case REG_INVALID: PANIC("invalid register");

    }
    UNREACHABLE();
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



typedef struct {
    size_t cap, len;
    char *items;
} Buffer;

static void buffer_init(Buffer *buf) {
    *buf = (Buffer) { 0 };
}

static void buffer_append(Buffer *buf, char c) {

    if (buf->len >= buf->cap) {
        if (buf->cap == 0)
            buf->cap = 50;
        else
            buf->cap *= 2;
        buf->items = NON_NULL(realloc(buf->items, buf->cap));
        // zero out the newly allocated region
        memset(buf->items + buf->len, 0, buf->cap - buf->len);
    }

    buf->items[buf->len++] = c;
}

static void buffer_append_str(Buffer *buf, const char *str) {
    for (const char *c=str; *c; c++)
        buffer_append(buf, *c);
}

static void buffer_destroy(Buffer *buf) {
    free(buf->items);
    buf->items = NULL;
}



struct {
    Buffer buf_data;
    Buffer buf_text;
    int label_count;
    int data_count;
    Hashtable *scope;
} gen = { 0 };

static void gen_init(void) {
    buffer_init(&gen.buf_data);
    buffer_init(&gen.buf_text);
}

static void gen_destroy(void) {
    buffer_destroy(&gen.buf_data);
    buffer_destroy(&gen.buf_text);
}

static void gen_write_to_file(const char *path) {

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        diagnostic(DIAG_ERROR, "Failed to open output file %s", path);
        exit(EXIT_FAILURE);
    }

    if (gen.buf_data.len)
        fprintf(f, "section .data\n%s", gen.buf_data.items);

    if (gen.buf_text.len)
        fprintf(f, "section .text\n%s", gen.buf_text.items);

    fclose(f);
}

static void gen_write(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char buf[1024] = { 0 };
    vsnprintf(buf, ARRAY_LEN(buf), fmt, va);
    strcat(buf, "\n");
    buffer_append_str(&gen.buf_text, buf);
    va_end(va);
}

static void gen_write_data(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    char buf[1024] = { 0 };
    vsnprintf(buf, ARRAY_LEN(buf), fmt, va);
    strcat(buf, "\n");
    buffer_append_str(&gen.buf_data, buf);
    va_end(va);
}

static Type emit_addr(AstNode *node);
static Type emit(AstNode *node);

static Type call(const ExprCall *call) {

    Type ty = emit(call->callee);
    gen_write("push rax");
    ProcSignature *sig = ty.signature;

    const AstNodeList *list = &call->args;
    for (size_t i=0; i < list->size; ++i) {

        TypeKind type = sig->params[i].type.kind;
        const char *reg = abi_register_str(i+1, type);

        emit(list->items[i]);

        if (reg == NULL) {
            gen_write("push rax");

        } else {
            const char *rax = subregister(REG_RAX, type);
            gen_write("mov %s, %s", reg, rax);

        }

    }

    // this weird stuff has to be done in order for function pointers to work
    gen_write("pop rax");
    gen_write("call rax");
    return sig->returntype;
}

static void proc(const DeclProc *proc) {
    const char *ident  = proc->ident.value;
    const ProcSignature *sig = proc->type.signature;

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
        const char *reg = abi_register_str(i+1, param->type.kind);

        if (reg == NULL) {
            const char *rax = subregister(REG_RAX, param->type.kind);
            gen_write("mov %s, [rbp+%d]", rax, offset);
            gen_write("mov [rbp-%d], %s", param->offset, rax);
            offset += 8;
        } else {
            gen_write("mov [rbp-%d], %s", param->offset, reg);
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

static Type unaryop_addr(const ExprUnaryOp *unaryop) {

    switch (unaryop->kind) {

        case UNARYOP_DEREF: {
            // not using emit_addr(), as the operand must already be a pointer
            return emit(unaryop->node);
            // address is already in rax, do nothing.
            break;
        }

        default: PANIC("unknown operation");
    }

    UNREACHABLE();

}

static Type unaryop(const ExprUnaryOp *unaryop) {

    switch (unaryop->kind) {

        case UNARYOP_NEG: {
            Type ty = emit(unaryop->node);
            gen_write("cmp %s, 0", subregister(REG_RAX, ty.kind));
            gen_write("sete %s", subregister(REG_RAX, ty.kind));
            return ty;
        } break;

        case UNARYOP_MINUS: {
            Type ty = emit(unaryop->node);
            gen_write("imul %s, -1", subregister(REG_RAX, ty.kind));
            return ty;
        } break;

        case UNARYOP_DEREF: {
            Type ty = emit(unaryop->node);
            gen_write("mov %s, [rax]", subregister(REG_RAX, ty.kind));
            return *ty.pointee;
        } break;

        case UNARYOP_ADDROF: {
            return emit_addr(unaryop->node);
        } break;

        default: PANIC("unknown operation");
    }

    UNREACHABLE();

}

static Type binop(const ExprBinOp *binop) {

    Type rhs = emit(binop->rhs);
    const char *rdi = subregister(REG_RDI, rhs.kind);
    gen_write("push rax");

    Type lhs = emit(binop->lhs);
    const char *rax = subregister(REG_RAX, lhs.kind);
    gen_write("pop rdi");

    const char *al = subregister(REG_RAX, TYPE_CHAR);

    // LHS: rax
    // RHS: rdi

    // TODO: maybe make this just an ast expansion

    // overload plus operator for pointer arithmetic
    // multiply the index with the size of the type pointed to by the pointer
    if (lhs.kind == TYPE_POINTER && rhs.kind != TYPE_POINTER) {
        gen_write("imul %s, %d", subregister(REG_RDI, rhs.kind), type_primitive_size(lhs.pointee->kind));

    } else if (lhs.kind != TYPE_POINTER && rhs.kind == TYPE_POINTER) {
        gen_write("imul %s, %d", subregister(REG_RAX, lhs.kind), type_primitive_size(rhs.pointee->kind));

    } else {
        // TODO: proper type checking
        assert(rhs.kind == lhs.kind);
    }

    switch (binop->kind) {
        case BINOP_ADD:
            gen_write("add %s, %s", rax, rdi);
            break;

        case BINOP_SUB:
            gen_write("sub %s, %s", rax, rdi);
            break;

        case BINOP_MUL:
            gen_write("imul %s", rdi);
            break;

        case BINOP_DIV:
            gen_write("idiv %s", rdi);
            break;

        case BINOP_EQ:
            gen_write("cmp %s, %s", rax, rdi);
            gen_write("sete %s", al);
            break;

        case BINOP_NEQ:
            gen_write("cmp %s, %s", rax, rdi);
            gen_write("setne %s", al);
            break;

        case BINOP_GT:
            gen_write("cmp %s, %s", rax, rdi);
            gen_write("setg %s", al);
            break;

        case BINOP_GT_EQ:
            gen_write("cmp %s, %s", rax, rdi);
            gen_write("setge %s", al);
            break;

        case BINOP_LT:
            gen_write("cmp %s, %s", rax, rdi);
            gen_write("setl %s", al);
            break;

        case BINOP_LT_EQ:
            gen_write("cmp %s, %s", rax, rdi);
            gen_write("setle %s", al);
            break;

        case BINOP_BITWISE_OR:
            gen_write("or %s, %s", rax, rdi);
            break;

        case BINOP_BITWISE_AND:
            gen_write("and %s, %s", rax, rdi);
            break;

        case BINOP_LOG_OR:
            // do a bitwise or, then convert the resulting number to either 1 or 0
            gen_write("or %s, %s", rax, rdi);
            gen_write("cmp %s, 0", rax);
            gen_write("setne %s", al);
            break;

        case BINOP_LOG_AND:
            // do a bitwise and, then convert the resulting number to either 1 or 0
            gen_write("and %s, %s", rax, rdi);
            gen_write("cmp %s, 0", rax);
            gen_write("setne %s", al);
            break;
    }

    // if pointer arithmetic was done, binop has to evaluate to a pointer
    return rhs.kind == TYPE_POINTER ? rhs : lhs;

}

static Type literal_ident(const ExprLiteral *literal, const char *str, bool addr) {

    Symbol *sym = symboltable_lookup(gen.scope, str);

    if (sym == NULL) {
        diagnostic_loc(DIAG_ERROR, &literal->op, "Symbol `%s` does not exist in the current scope", str);
        exit(EXIT_FAILURE);
    }

    switch (sym->kind) {
        case SYMBOL_TABLE:
            diagnostic_loc(DIAG_ERROR, &literal->op, "Symbol `%s` is a type, not a variable", str);
            exit(EXIT_FAILURE);
            break;

        case SYMBOL_PARAMETER:
        case SYMBOL_VARIABLE:
            if (addr) {
                gen_write("lea rax, [rbp-%d]", sym->offset);
                return (Type) { .kind = TYPE_POINTER, .pointee = &sym->type };

            } else {
                gen_write("mov %s, [rbp-%d]", subregister(REG_RAX, sym->type.kind), sym->offset);
            }
            break;

        case SYMBOL_PROCEDURE:
            gen_write("mov rax, %s", str);
            break;

        case SYMBOL_INVALID:
        case SYMBOL_NONE:
            PANIC("invalid symbol");
    }

    return sym->type;
}

static Type literal_addr(const ExprLiteral *literal) {

    const char *str = literal->op.value;

    switch (literal->kind) {
        case LITERAL_IDENT:
            return literal_ident(literal, str, true);
            break;
        case LITERAL_STRING:
        case LITERAL_NUMBER:
            PANIC("unknown operation");
    }

    UNREACHABLE();

}

static TypeKind type_from_token_literal(NumberLiteralType type) {
    switch (type) {
        case NUMBER_CHAR: return TYPE_CHAR;
        case NUMBER_LONG: return TYPE_LONG;
        case NUMBER_ANY:
        case NUMBER_INT:  return TYPE_INT;
    }
    UNREACHABLE();
}

static Type literal(const ExprLiteral *literal) {

    const char *str = literal->op.value;
    int64_t num = literal->op.number;

    switch (literal->kind) {
        case LITERAL_STRING: {

            gen_write_data("string_%d:", gen.data_count);
            gen_write_data("db \"%s\", 0", str);

            gen_write("mov rax, string_%d", gen.data_count);
            gen.data_count++;

            // TODO: pointee info
            return (Type) { .kind = TYPE_POINTER };
        } break;

        case LITERAL_NUMBER: {

            TypeKind type = type_from_token_literal(literal->op.number_type);
            gen_write("mov %s, %d", subregister(REG_RAX, type), num);
            return (Type) { .kind = type };
        } break;

        case LITERAL_IDENT:
            return literal_ident(literal, str, false);
            break;
    }

    UNREACHABLE();

}

static Type grouping(const ExprGrouping *grouping) {
    return emit(grouping->expr);
}

static void cond(const StmtIf *cond) {

    int lbl = gen.label_count++;
    Type ty = emit(cond->condition);
    const char *rax = subregister(REG_RAX, ty.kind);

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
    Type cond = emit(loop->condition);
    const char *rax = subregister(REG_RAX, cond.kind);
    gen_write("cmp %s, 0", rax);
    gen_write("jne .while%lu", lbl);

}

static void vardecl(const StmtVarDecl *decl) {

    if (decl->init == NULL) return;

    const char *ident = decl->ident.value;
    Type init = emit(decl->init);
    gen_write(
        "mov [rbp-%d], %s ; %s",
        decl->offset,
        subregister(REG_RAX, init.kind),
        ident
    );

}

static Type assign(const ExprAssign *assign) {

    emit_addr(assign->target);
    gen_write("push rax");
    Type ty = emit(assign->value);

    gen_write("pop rdi");
    gen_write("mov [rdi], %s", subregister(REG_RAX, ty.kind));

    return ty;
}

static Type array(const ExprArray *array) {

    AstNodeList list = array->values;
    int elem_size = type_primitive_size(array->type.kind);

    for (size_t i=0; i < list.size; ++i) {
        emit(list.items[i]);

        int offset = array->offset + (list.size - i) * elem_size;
        gen_write("mov [rbp-%d], %s ; array", offset, subregister(REG_RAX, array->type.kind));

    }

    gen_write("lea rax, [rbp-%d]", array->offset + list.size * elem_size);

    // TODO: remove const cast
    return (Type) { .kind = TYPE_POINTER, .pointee = (Type*) &array->type };
}


// get address of lvalue
static Type emit_addr(AstNode *node) {
    NON_NULL(node);

    switch (node->kind) {
        case ASTNODE_UNARYOP: return unaryop_addr(&node->expr_unaryop); break;
        case ASTNODE_LITERAL: return literal_addr(&node->expr_literal); break;

        case ASTNODE_FOR:
        case ASTNODE_INDEX:
            PANIC("syntactic sugar should have been expanded earlier"); break;
        case ASTNODE_BLOCK:
        case ASTNODE_WHILE:
        case ASTNODE_PROC:
        case ASTNODE_RETURN:
        case ASTNODE_ARRAY:
        case ASTNODE_VARDECL:
        case ASTNODE_IF:
        case ASTNODE_GROUPING:
        case ASTNODE_ASSIGN:
        case ASTNODE_BINOP:
        case ASTNODE_CALL:
        case ASTNODE_TABLE:
            PANIC("unknown node kind");
    }

    UNREACHABLE();

}

static Type emit(AstNode *node) {
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
        case ASTNODE_ARRAY:     return array    (&node->expr_array);    break;
        case ASTNODE_INDEX:
        case ASTNODE_FOR:
            PANIC("syntactic sugar should have been expanded earlier"); break;
        case ASTNODE_TABLE:     NOP()                                   break;
    }

    return (Type) { .kind = TYPE_VOID };

}

void codegen(AstNode *root, const char *filename) {
    printf("GEN %s\n", filename);
    gen_init();
    emit(root);
    gen_write_to_file(filename);
    gen_destroy();
}
