#ifndef _AST_H
#define _AST_H

#include <stdio.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "lexer.h"
#include <stddef.h>
#include <stdbool.h>

#include "lexer.h"



typedef struct AstNode AstNode;

typedef struct {
    size_t capacity, size;
    AstNode **items;
} AstNodeList;

extern AstNodeList astnodelist_new    (void);
extern void        astnodelist_append (AstNodeList *list, AstNode *node);
extern void        astnodelist_destroy(AstNodeList *list);


typedef enum {
    BUILTINFUNC_NONE, // call is not a builtin
    BUILTINFUNC_ASM,
    // TODO: sizeof
} BuiltinFunc;




typedef struct {
    Token op;
    // TODO: add type information
} ExprLiteral;

typedef struct {
    AstNode *expr;
} ExprGrouping;

typedef struct {
    AstNode *lhs, *rhs;
    Token op;
} ExprBinOp;

typedef struct {
    Token op;
    AstNode *callee;
    AstNodeList args;
    BuiltinFunc builtin; // None, if no builtin
} ExprCall;

typedef struct {
    AstNode *node;
    Token op;
} ExprUnaryOp;

typedef struct {
    AstNodeList stmts;
    bool global; // true if block represents the global scope
} Block;

typedef struct {
    Token op, identifier;
    AstNode *body; // NULL if declaration
} StmtFunc;

typedef struct {
    Token op, identifier, type;
    AstNode *value;
} StmtVarDecl;

typedef enum {
    ASTNODE_LITERAL,
    ASTNODE_GROUPING,
    ASTNODE_BINOP,
    ASTNODE_UNARYOP,
    ASTNODE_CALL,
    ASTNODE_BLOCK,
    ASTNODE_FUNC,
    ASTNODE_VARDECL,
} AstNodeKind;

struct AstNode {
    AstNodeKind kind;
    union {
        ExprLiteral   expr_literal;
        ExprGrouping  expr_grouping;
        ExprBinOp     expr_binop;
        ExprUnaryOp   expr_unaryop;
        ExprCall      expr_call;
        Block         block;
        StmtFunc      stmt_func;
        StmtVarDecl   stmt_vardecl;
    };
};



#endif // _AST_H
