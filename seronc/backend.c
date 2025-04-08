#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <alloca.h>

#include "util.h"
#include "lexer.h"
#include "gen.h"
#include "main.h"
#include "parser.h"


CodeGenerator codegen = { 0 };


static void traverse_ast(AstNode *node);


#if 0
static void builtin_inlineasm(ExprCall *call, Symboltable *scope) {

    AstNodeList list = call->args;
    if (list.size == 0)
        throw_error(call->op, "asm(), expects more than 0 arguments");

    AstNode *first = list.items[0];

    bool is_literal = first->kind == ASTNODE_LITERAL;
    bool is_string  = first->expr_literal.op.kind == TOK_STRING;
    if (!(is_literal && is_string))
        throw_error(call->op, "First argument to asm() must be a string");

    // union member access only safe after check
    const char *asm_src = first->expr_literal.op.value;

    // get the count of placeholders to check count of arguments
    size_t expected_args = 0;
    const char *str = asm_src;
    while ((str = strstr(str, "{}")) != NULL) {
        str++;
        expected_args++;
    }

    // `- 1`: first arg is src
    if (list.size - 1 != expected_args) {
        throw_error(
            call->op,
            "Expected %d arguments, got %d",
            expected_args,
            list.size - 1
        );
    }

    size_t addrs_len = list.size - 1;
    Symbol *addrs    = alloca(addrs_len * sizeof(Symbol));
    size_t addrs_i   = 0;

    for (size_t i=1; i < list.size; ++i) {
        Symbol sym = traverse_ast(list.items[i], scope);
        addrs[addrs_i++] = sym;
    }

    gen_inlineasm(&codegen, asm_src, addrs, addrs_len);

}
#endif


// static void ast_vardecl(StmtVarDecl *vardecl, Symboltable *scope) {
//     if (vardecl->value == NULL)
//         return;
//
//     Symbol value = traverse_ast(vardecl->value, scope);
//
//     const char *ident = vardecl->identifier.value;
//     Symbol *sym = symboltable_get(scope, ident);
//     assert(sym != NULL);
//
//     gen_var_init(&codegen, sym, &value);
// }


// static Symbol ast_call(ExprCall *call, Symboltable *scope) {
//
//     if (call->builtin == BUILTIN_ASM) {
//         builtin_inlineasm(call, scope);
//         return (Symbol) {
//             .type = (Type) { .kind = TYPE_INVALID },
//         };
//     }
//     assert(call->builtin == BUILTIN_NONE);
//
//
//     AstNodeList list = call->args;
//
//     size_t symbols_i = 0;
//     Symbol *symbols = alloca(list.size * sizeof(Symbol));
//
//     for (size_t i=0; i < list.size; ++i)
//         symbols[symbols_i++] = traverse_ast(list.items[i], scope);
//
//     assert(symbols_i == list.size);
//
//     Symbol callee = traverse_ast(call->callee, scope);
//     return gen_call(&codegen, callee, symbols, list.size);
// }

// static void ast_if(StmtIf *if_, Symboltable *scope) {
//     Symbol cond = traverse_ast(if_->condition, scope);
//
//     gen_ctx_t if_ctx = gen_if_then(&codegen, &cond);
//     traverse_ast(if_->then_body, scope);
//     gen_if_else(&codegen, if_ctx);
//
//     if (if_->else_body != NULL)
//         traverse_ast(if_->else_body, scope);
//
//     gen_if_end(&codegen, if_ctx);
// }

// static void ast_while(StmtWhile *while_, Symboltable *scope) {
//     Symbol cond = traverse_ast(while_->condition, scope);
//
//     gen_ctx_t while_ctx = gen_while_start(&codegen);
//     traverse_ast(while_->body, scope);
//     gen_while_end(&codegen, &cond, while_ctx);
// }

// static void ast_return(StmtReturn *return_, Symboltable *scope) {
//     Symbol expr = traverse_ast(return_->expr, scope);
//     gen_return(&codegen, &expr);
// }

// static Symbol ast_assign(ExprAssignment *assign, Symboltable *scope) {
//     const char *ident = assign->identifier.value;
//     Symbol value = traverse_ast(assign->value, scope);
//
//     Symbol *assignee = symboltable_list_lookup(scope, ident);
//     assert(assignee != NULL);
//     gen_assign(&codegen, assignee, &value);
//
//     return value;
// }

static void procedure(StmtProc *proc) {
    const char *ident  = proc->identifier.value;
    ProcSignature *sig = &proc->type.type_signature;

    if (proc->body == NULL) {
        gen_procedure_extern(&codegen, ident);
        return;
    }

    assert(proc->body->kind == ASTNODE_BLOCK);
    Hashtable *body = proc->body->block.symboltable;

    gen_procedure_start(&codegen, ident, proc->stack_size, sig, body);
    traverse_ast(proc->body);
    gen_procedure_end(&codegen);
}

static void block(Block *block) {

    AstNodeList list = block->stmts;
    for (size_t i=0; i < list.size; ++i)
        traverse_ast(list.items[i]);
}

static void binop(ExprBinOp *binop) {
    traverse_ast(binop->lhs);
    gen_addinstr(&codegen, "push eax");

    traverse_ast(binop->rhs);
    gen_addinstr(&codegen, "pop edi");

    gen_addinstr(&codegen, "add eax, edi");
    // gen_binop(&codegen, &lhs, &rhs, binop->kind);
}

static void literal(ExprLiteral *literal) {
    switch (literal->op.kind) {

        case TOK_NUMBER: {
            const char *str = literal->op.value;
            gen_store_literal(&codegen, atoll(str), INTLITERAL);
        } break;

        default: assert(!"unimplemented");
    }
}

static void traverse_ast(AstNode *node) {
    assert(node != NULL);

    switch (node->kind) {

        case ASTNODE_BLOCK:
            block(&node->block);
            break;

        case ASTNODE_PROCEDURE:
            procedure(&node->stmt_proc);
            break;

        case ASTNODE_BINOP:
            binop(&node->expr_binop);
            break;

        case ASTNODE_LITERAL:
            literal(&node->expr_literal);
            break;

        // case ASTNODE_CALL:
        //     return ast_call(&node->expr_call, scope);
        //     break;

        // case ASTNODE_GROUPING:
        //     traverse_ast(node->expr_grouping.expr, scope);
        //     break;

        // case ASTNODE_IF:
        //     ast_if(&node->stmt_if, scope);
        //     break;

        // case ASTNODE_WHILE:
        //     ast_while(&node->stmt_while, scope);
        //     break;

        // case ASTNODE_RETURN:
        //     ast_return(&node->stmt_return, scope);
        //     break;

        // case ASTNODE_VARDECL:
        //     ast_vardecl(&node->stmt_vardecl, scope);
        //     break;

        // case ASTNODE_UNARYOP:
        //     assert(!"Unimplemented");
        //     break;

        // case ASTNODE_ASSIGN:
        //     return ast_assign(&node->expr_assign, scope);
        //     break;

        default: assert(!"unexpected node kind");
    }

}

void generate_code(AstNode *root) {

    gen_init(
        &codegen,
        compiler_config.filename.asm_,
        compiler_config.opts.debug_asm
    );

    gen_prelude(&codegen);
    traverse_ast(root);

    gen_destroy(&codegen);
}
