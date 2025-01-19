#ifndef _PARSER_H
#define _PARSER_H

#include <stdio.h>
#include <stddef.h>

#include "./lexer.h"




typedef struct AstNode AstNode;

typedef struct {
    size_t capacity, size;
    AstNode **items;
} AstNodeList;

extern AstNodeList astnodelist_new    (void);
extern void        astnodelist_append (AstNodeList *list, AstNode *node);
extern void        astnodelist_destroy(AstNodeList *list);




typedef struct {
    Token op;
} ExprLiteral;

typedef struct {
    AstNode *lhs, *rhs;
    Token op;
} ExprBinOp;

typedef struct {
    AstNodeList stmts;
} Block;

typedef struct {
    Token op, identifier;
    AstNode *body; // NULL if declaration
} StmtFunc;

typedef struct {
    Token op;
    AstNode *condition, *then_body, *else_body;
} StmtIf;

typedef struct {
    Token op;
    AstNode *condition, *body;
} StmtWhile;

struct AstNode {
    enum {
        ASTNODE_LITERAL,
        ASTNODE_BINOP,
        ASTNODE_BLOCK,
        ASTNODE_FUNC,
        ASTNODE_IF,
        ASTNODE_WHILE,
    } kind;
    union {
        ExprLiteral expr_literal;
        ExprBinOp   expr_binop;
        Block       block;
        StmtFunc    stmt_func;
        StmtIf      stmt_if;
        StmtWhile   stmt_while;
    };
};

extern AstNode *parse(const TokenList *tokens);



#endif // _PARSER_H
