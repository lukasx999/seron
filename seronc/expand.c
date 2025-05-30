#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "symboltable.h"

#include "expand.h"

// convert for loop for while loop
//
// for <vardecl>, <cond>, <assign> {
//    <body>
// }
//
// { # Block
//     <vardecl>
//     while <cond> {
//         <body>
//         <assign>
//     }
// }
static void for_pre(AstNode *node, UNUSED int _depth, void *args) {
    Arena *arena = args;

    StmtFor *for_    = &node->stmt_for;
    AstNode *cond    = for_->condition;
    AstNode *body    = for_->body;
    AstNode *assign  = for_->assign;
    Token    op      = for_->op;

    AstNode *vardecl      = arena_alloc(arena, sizeof(AstNode));
    vardecl->kind         = ASTNODE_VARDECL;
    vardecl->stmt_vardecl = (StmtVarDecl) {
        .ident = for_->var_ident,
        .init  = for_->var_expr,
        .type  = for_->var_type,
        .op    = op,
    };

    AstNode *while_    = arena_alloc(arena, sizeof(AstNode));
    while_->kind       = ASTNODE_WHILE;
    while_->stmt_while = (StmtWhile) {
        .op        = op,
        .condition = cond,
        .body      = body,
    };

    node->kind = ASTNODE_BLOCK;
    Block *block = &node->block;
    astnodelist_init(&block->stmts, arena);
    astnodelist_append(&block->stmts, vardecl);
    astnodelist_append(&block->stmts, while_);

    assert(while_->stmt_while.body->kind == ASTNODE_BLOCK);
    astnodelist_append(&while_->stmt_while.body->block.stmts, assign);

}

// converts index to deref
//
// <expr> [ <index> ];
//
// *(<expr> + <index>);
static void index_pre(AstNode *node, UNUSED int _depth, void *args) {
    Arena *arena = args;
    ExprIndex *index = &node->expr_index;
    Token op = index->op;

    AstNode *binop    = arena_alloc(arena, sizeof(AstNode));
    binop->kind       = ASTNODE_BINOP;
    binop->expr_binop = (ExprBinOp) {
        .kind = BINOP_ADD,
        .op   = op,
        .lhs  = index->expr,
        .rhs  = index->index,
    };

    node->kind = ASTNODE_UNARYOP;
    node->expr_unaryop.kind = UNARYOP_DEREF;
    node->expr_unaryop.op = op;
    node->expr_unaryop.node = binop;
}

void expand_ast(AstNode *root, Arena *arena) {

    AstDispatchEntry table[] = {
        { ASTNODE_FOR,   for_pre,   NULL },
        { ASTNODE_INDEX, index_pre, NULL },
    };

    parser_dispatch_ast(root, table, ARRAY_LEN(table), arena);
}
