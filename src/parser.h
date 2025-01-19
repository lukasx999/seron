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



typedef struct {
    Token op;
} Literal;

typedef struct {
    AstNode *lhs, *rhs;
    Token op;
} BinOp;

typedef struct {
    AstNodeList stmts;
} Block;

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
    } kind;
    union {
        Literal literal;
        BinOp binop;
    };
};

extern AstNode *parse(const TokenList *tokens);



#endif // _PARSER_H
