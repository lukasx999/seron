#ifndef _PARSER_H
#define _PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "lexer.h"
#include "lib/arena.h"
#include "types.h"
#include "symboltable.h"



typedef struct AstNode AstNode;

typedef struct {
    Arena *arena;
    size_t size, cap;
    AstNode **items;
} AstNodeList;

void astnodelist_init  (AstNodeList *list, Arena *arena);
void astnodelist_append(AstNodeList *list, AstNode *node);


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
    AstNode *node;
    Token op;
} ExprUnaryOp;

typedef enum {
    BUILTIN_ASM,
    BUILTIN_NONE,
} BuiltinFunction;

BuiltinFunction builtin_from_tokenkind(TokenKind kind);

typedef struct {
    Token           op;
    AstNode        *callee; // NULL if builtin
    AstNodeList     args;
    BuiltinFunction builtin; // BUILTIN_NONE if no builtin
} ExprCall;

// TODO: make assignee an expression, to allow for struct member assignments
typedef struct {
    Token op, identifier;
    AstNode *value;
} ExprAssignment;

typedef struct {
    AstNodeList stmts;
    Symboltable *symboltable;
} Block;

typedef struct {
    Token op, identifier;
    AstNode *body; // NULL if declaration
    // TODO: why not just use ProcSignature
    Type type; // type is holding function signature
    uint64_t stack_size;
} StmtProcedure;

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
    AstNode *expr; // TODO: NULL if no expr
} StmtReturn;

typedef struct {
    Token op, identifier;
    AstNode *value; // NULL if declaration
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




// Allocate an AST into the given arena
AstNode *parse(const char *src, Arena *arena);

typedef void (*AstCallback)(AstNode *node, int depth, void *args);

// Call the given callback function for every node in the AST
void parser_traverse_ast(AstNode *root, AstCallback callback, bool top_down, void *args);
// Call the given callback function for every node of kind `kind` in the AST
void parser_query_ast(AstNode *root, AstCallback callback, AstNodeKind kind, void *args);
void parser_print_ast(AstNode *root, int spacing);



#endif // _PARSER_H
