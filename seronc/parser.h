#ifndef _PARSER_H
#define _PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <arena.h>

#include "lexer.h"
#include "hashtable.h"



typedef struct AstNode AstNode;

typedef struct {
    Arena *arena;
    size_t size, cap;
    AstNode **items;
} AstNodeList;

typedef enum {
    LITERAL_NUMBER,
    LITERAL_CHAR,
    LITERAL_IDENT,
    LITERAL_STRING,
} LiteralKind;

typedef struct {
    Token op;
    LiteralKind kind;
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

typedef struct {
    AstNode *lhs, *rhs;
    Token op;
    BinOpKind kind;
} ExprBinOp;

typedef enum {
    UNARYOP_MINUS,
    UNARYOP_NEG,
    UNARYOP_ADDROF,
    UNARYOP_DEREF,
} UnaryOpKind;

typedef struct {
    AstNode *node;
    Token op;
    UnaryOpKind kind;
} ExprUnaryOp;

typedef struct {
    Token        op;
    AstNode     *callee;
    AstNodeList  args;
} ExprCall;

typedef struct {
    Token op;
    AstNode *value, *target;
} ExprAssign;

typedef struct {
    AstNodeList stmts;
    Hashtable *symboltable;
} Block;

typedef struct {
    Token      op, ident;
    AstNode   *body;        // NULL if declaration
    Type       type;        // type is holding function signature
    Hashtable *symboltable; // for convenience
    int        stack_size;
} StmtProc;

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

typedef struct {
    Token op, ident;
    AstNode *init; // NULL if declaration
    Type type;
} StmtVarDecl;

typedef enum {
    ASTNODE_LITERAL,
    ASTNODE_GROUPING,
    ASTNODE_BINOP,
    ASTNODE_UNARYOP,
    ASTNODE_CALL,
    ASTNODE_BLOCK,
    ASTNODE_PROC,
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
        ExprAssign expr_assign;
        Block          block;
        StmtProc       stmt_proc;
        StmtVarDecl    stmt_vardecl;
        StmtIf         stmt_if;
        StmtWhile      stmt_while;
        StmtReturn     stmt_return;
    };
};

// Allocate an AST into the given arena
AstNode *parse(const char *src, Arena *arena);

typedef void (*AstCallback)(AstNode *node, int depth, void *args);


// Call the given callback function for every node in the AST
void parser_traverse_ast(AstNode *root, AstCallback callback_pre, AstCallback callback_post, void *args);

typedef struct {
    AstNodeKind kind;
    AstCallback fn_pre;
    AstCallback fn_post;
} AstDispatchEntry;

void parser_dispatch_ast(AstNode *root, AstDispatchEntry *table, size_t table_size, void *args);

void parser_print_ast(AstNode *root, int spacing);



#endif // _PARSER_H
