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

AstNodeList astnodelist_new(void);
void astnodelist_append(AstNodeList *list, AstNode *node);
void astnodelist_destroy(AstNodeList *list);




// Token is included in AstNode for printing source location on error/warning




typedef struct {
    Token op;
} ExprLiteral;

typedef struct {
    AstNode *expr;
} ExprGrouping;

/* Binary Operations */

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


/* Procedure Calls */

typedef enum {
    BUILTIN_ASM,
    BUILTIN_NONE,
} BuiltinFunction;

BuiltinFunction builtin_from_tokenkind(TokenKind kind);

typedef struct {
    Token op;
    AstNode *callee; // NULL if builtin
    AstNodeList args;
    BuiltinFunction builtin; // BUILTIN_NONE if no builtin
} ExprCall;



typedef struct {
    AstNode *node;
    Token op;
} ExprUnaryOp;

// TODO: make assignee an expression, to allow for struct member assignments
typedef struct {
    Token op, identifier;
    AstNode *value;
} ExprAssignment;

typedef struct {
    AstNodeList stmts;
    Hashtable *symboltable;
} Block;


/* Procedures */

typedef struct {
    Token op, identifier;
    AstNode *body; // NULL if declaration
    Type type; // type is holding function signature
    /*bool external;*/ // TODO:
} StmtProcedure;


/* Control Flow */

typedef struct {
    Token op;
    AstNode *condition, *then_body, *else_body; // else_body is NULL if there is none
} StmtIf;

typedef struct {
    Token op;
    AstNode *condition, *body;
} StmtWhile;

typedef struct {
    Token op;
    AstNode *expr;
} StmtReturn;

/* Variable Declarations */

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
    ASTNODE_PROCEDURE,
    ASTNODE_VARDECL,
    ASTNODE_IF,
    ASTNODE_WHILE,
    ASTNODE_ASSIGN,
    ASTNODE_RETURN,
} AstNodeKind;

struct AstNode {
    AstNodeKind kind;
    union {
        ExprLiteral    expr_literal;
        ExprGrouping   expr_grouping;
        ExprBinOp      expr_binop;
        ExprUnaryOp    expr_unaryop;
        ExprCall       expr_call;
        ExprAssignment expr_assign;
        Block          block;
        StmtProcedure  stmt_procedure;
        StmtVarDecl    stmt_vardecl;
        StmtIf         stmt_if;
        StmtWhile      stmt_while;
        StmtReturn     stmt_return;
    };
};



#endif // _AST_H
