#ifndef _AST_H
#define _AST_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include "lexer.h"
#include "types.h"

typedef struct Hashtable Hashtable;





typedef struct AstNode AstNode;

typedef struct {
    size_t capacity, size;
    AstNode **items;
} AstNodeList;

extern AstNodeList astnodelist_new(void);
extern void astnodelist_append (AstNodeList *list, AstNode *node);
extern void astnodelist_destroy(AstNodeList *list);


typedef enum {
    BUILTINFUNC_NONE, // call is not a builtin
    BUILTINFUNC_ASM,
    // TODO: sizeof
} BuiltinFunc;

extern BuiltinFunc string_to_builtinfunc(const char *str);

// Token is included in AstNode for printing source location on error/warning

typedef struct {
    Token op;
} ExprLiteral;

typedef struct {
    AstNode *expr;
} ExprGrouping;

typedef enum {
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MUL,
    BINOP_DIV,
} BinOpKind;

extern BinOpKind binopkind_from_tokenkind(TokenKind kind);

typedef struct {
    AstNode *lhs, *rhs;
    Token op;
    BinOpKind kind;
} ExprBinOp;

typedef struct {
    Token op;
    AstNode *callee;
    AstNodeList args;
    BuiltinFunc builtin; // `BUILTINFUNC_NONE`, if no builtin
} ExprCall;

typedef struct {
    AstNode *node;
    Token op;
} ExprUnaryOp;

typedef struct {
    AstNodeList stmts;
    Hashtable *symboltable;
} Block;

typedef struct {
    Token op, identifier;
    AstNode *body; // NULL if declaration
    // TODO:
    // Type returntype;
} StmtFunc;

typedef struct {
    Token op;
    AstNode *condition, *then_body, *else_body; // else_body is NULL if there is none
} StmtIf;

typedef struct {
    Token op;
    AstNode *condition, *body;
} StmtWhile;

typedef struct {
    Token op, identifier;
    AstNode *value;
    Type type;
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
    ASTNODE_IF,
    ASTNODE_WHILE,
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
        StmtIf        stmt_if;
        StmtWhile     stmt_while;
    };
};



#endif // _AST_H
