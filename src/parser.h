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
    AstNode *expr;
} ExprGrouping;

typedef struct {
    AstNode *lhs, *rhs;
    Token op;
} ExprBinOp;

typedef struct {
    AstNodeList stmts;
    bool global; // true if block represents the global scope
} Block;

typedef struct {
    Token op, identifier;
    AstNode *body; // NULL if declaration
} StmtFunc;

typedef struct {
    Token op, identifier;
    AstNode *value;
} StmtVarDecl;

struct AstNode {
    enum {
        ASTNODE_LITERAL,
        ASTNODE_GROUPING,
        ASTNODE_BINOP,
        ASTNODE_BLOCK,
        ASTNODE_FUNC,
        ASTNODE_VARDECL,
    } kind;
    union {
        ExprLiteral  expr_literal;
        ExprGrouping expr_grouping;
        ExprBinOp    expr_binop;
        Block        block;
        StmtFunc     stmt_func;
        StmtVarDecl  stmt_vardecl;
    };
};

typedef void (*AstCallback)(AstNode *node, int depth);

extern AstNode *parser_parse(const TokenList *tokens);
// top_down == true: callback will be called for root node first, and for leaf nodes last
// top_down == false: callback will be called for leaf nodes first, and for root node last
extern void parser_traverse_ast(AstNode *root, AstCallback callback, bool top_down);
extern void parser_print_ast(AstNode *root);
extern void parser_free_ast(AstNode *root);



#endif // _PARSER_H
