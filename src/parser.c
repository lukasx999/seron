#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "asm.h"





#define SENTINEL TOK_INVALID



AstNodeList astnodelist_new(void) {
    AstNodeList list = {
        .capacity = 5,
        .size     = 0,
        .items    = NULL,
    };
    list.items = malloc(list.capacity * sizeof(AstNode*));
    return list;
}

void astnodelist_append(AstNodeList *list, AstNode *node) {
    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(AstNode*));
    }
    list->items[list->size++] = node;
}

void astnodelist_destroy(AstNodeList *list) {
    free(list->items);
    list->items = NULL;
}






typedef struct {
    size_t current;
    const TokenList *tokens;
    const char *filename;
} Parser;

static inline Token parser_get_current_token(const Parser *p) {
    const Token *tok = tokenlist_get(p->tokens, p->current);
    assert(tok != NULL);
    return *tok;
}

static inline void parser_advance(Parser *p) {
    p->current++;
}

// Checks if the current token is of one of the supplied kinds
// Last variadic argument should be `TOK_INVALID`
static bool parser_match_tokenkinds(const Parser *p, ...) {
    va_list va;
    va_start(va, p);

    while (1) {
        TokenKind tok = va_arg(va, TokenKind);
        bool matched = parser_get_current_token(p).kind == tok;
        if (matched || tok == SENTINEL) {
            va_end(va);
            return matched;
        }
    }

    assert(false);
}

static inline void parser_expect_token(
    const Parser *p,
    TokenKind tokenkind,
    const char *expected
) {
    if (!parser_match_tokenkinds(p, tokenkind, SENTINEL)) {
        Token tok = parser_get_current_token(p);
        throw_error(p->filename, &tok, "Expected `%s`", expected);
    }
}

static inline void parser_throw_error(const Parser *p, const char *msg) {
    Token tok = parser_get_current_token(p);
    throw_error(p->filename, &tok, "%s", msg);
}

static inline bool parser_is_at_end(const Parser *p) {
    return parser_match_tokenkinds(p, TOK_EOF, SENTINEL);
}

void parser_traverse_ast(AstNode *root, AstCallback callback, bool top_down, void *args) {
    static int depth = 0;
    if (top_down)
        callback(root, depth, args);

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            depth++;
            AstNodeList list = root->block.stmts;

            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback, top_down, args);

            depth--;
        } break;

        case ASTNODE_CALL: {
            depth++;
            ExprCall call = root->expr_call;

            parser_traverse_ast(call.callee, callback, top_down, args);

            AstNodeList list = call.args;
            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback, top_down, args);

            depth--;
        } break;

        case ASTNODE_GROUPING:
            depth++;
            parser_traverse_ast(root->expr_grouping.expr, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_FUNC:
            depth++;
            parser_traverse_ast(root->stmt_func.body, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_VARDECL:
            depth++;
            parser_traverse_ast(root->stmt_vardecl.value, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_BINOP: {
            depth++;
            ExprBinOp binop = root->expr_binop;
            parser_traverse_ast(binop.lhs, callback, top_down, args);
            parser_traverse_ast(binop.rhs, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_UNARYOP: {
            depth++;
            ExprUnaryOp unaryop = root->expr_unaryop;
            parser_traverse_ast(unaryop.node, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_INLINEASM:
        case ASTNODE_LITERAL:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    if (!top_down)
        callback(root, depth, args);

}

// Prints a value in the following format: `<str>: <arg>`
// <arg> is omitted if arg == NULL
static void print_ast_value(const char *str, const char *color, const char *value, const char *opt) {
    printf("%s%s%s", color, str, COLOR_END);
    if (value != NULL)
        printf("%s: %s%s", COLOR_GRAY, value, COLOR_END);
    if (opt != NULL)
        printf(" %s(%s)%s", COLOR_GRAY, opt, COLOR_END);
    puts("");
}

void parser_print_ast_callback(AstNode *root, int depth, void *_args) {
    (void) _args;

    const int spacing = 3; // TODO: pass this in through void* argument
    for (int _=0; _ < depth * spacing; ++_)
        printf("%s⋅%s", COLOR_GRAY, COLOR_END);

    switch (root->kind) {
        case ASTNODE_BLOCK: {
            Block *block = &root->block;
            print_ast_value("block", COLOR_BLUE, block->global ? "global" : NULL, NULL);
        } break;

        case ASTNODE_GROUPING: {
            print_ast_value("grouping", COLOR_BLUE, NULL, NULL);
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp *binop = &root->expr_binop;
            print_ast_value(tokenkind_to_string(binop->op.kind), COLOR_PURPLE, NULL, NULL);
        } break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp *unaryop = &root->expr_unaryop;
            print_ast_value(tokenkind_to_string(unaryop->op.kind), COLOR_PURPLE, NULL, NULL);
        } break;

        case ASTNODE_CALL: {
            print_ast_value("call", COLOR_BLUE, NULL, NULL);
        } break;

        case ASTNODE_FUNC: {
            StmtFunc *func = &root->stmt_func;
            print_ast_value(tokenkind_to_string(func->op.kind), COLOR_RED, func->identifier.value, NULL);
        } break;

        case ASTNODE_INLINEASM: {
            StmtInlineAsm *inlineasm = &root->stmt_inlineasm;
            print_ast_value(tokenkind_to_string(inlineasm->op.kind), COLOR_RED, NULL, NULL);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &root->stmt_vardecl;
            print_ast_value(
                tokenkind_to_string(vardecl->op.kind),
                COLOR_RED,
                vardecl->identifier.value,
                tokenkind_to_string(vardecl->type.kind)
            );
        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral *literal = &root->expr_literal;
            Token *tok = &literal->op;
            print_ast_value(tokenkind_to_string(tok->kind), COLOR_RED, tok->value, NULL);
        } break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }
}

void parser_print_ast(AstNode *root) {
    parser_traverse_ast(root, parser_print_ast_callback, true, NULL);
}

static void parser_free_ast_callback(AstNode *node, int _depth, void *_args) {
    (void) _depth;
    (void) _args;

    switch (node->kind) {
        case ASTNODE_BLOCK:
            astnodelist_destroy(&node->block.stmts);
            break;

        case ASTNODE_CALL:
            astnodelist_destroy(&node->expr_call.args);
            break;

        case ASTNODE_GROUPING:
        case ASTNODE_LITERAL:
        case ASTNODE_FUNC:
        case ASTNODE_VARDECL:
        case ASTNODE_BINOP:
        case ASTNODE_UNARYOP:
        case ASTNODE_INLINEASM:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }
    free(node);
}

void parser_free_ast(AstNode *root) {
    parser_traverse_ast(root, parser_free_ast_callback, false, NULL);
}






// forward-declaration because some grammar rules have circular dependencies
static AstNode *rule_primary    (Parser *p);
static AstNode *rule_term       (Parser *p);
static AstNode *rule_expression (Parser *p);
static AstNode *rule_exprstmt   (Parser *p);
static AstNode *rule_function   (Parser *p);
static AstNode *rule_builtin_asm(Parser *p);
static AstNode *rule_vardecl    (Parser *p);
static AstNode *rule_stmt       (Parser *p);
static AstNode *rule_block      (Parser *p);
static AstNode *rule_program    (Parser *p);
static AstNode *rule_call       (Parser *p);
static AstNode *rule_unary      (Parser *p);
static AstNode *rule_factor     (Parser *p);



static AstNode *rule_primary(Parser *p) {
    // primary ::= NUMBER | IDENTIFIER | STRING | "(" <expression> ")"

    AstNode *astnode = malloc(sizeof(AstNode));

    if (parser_match_tokenkinds(p, TOK_NUMBER, TOK_IDENTIFIER, TOK_STRING, SENTINEL)) {
        astnode->kind         = ASTNODE_LITERAL;
        astnode->expr_literal = (ExprLiteral) {
            .op = parser_get_current_token(p)
        };
        parser_advance(p);

    } else if (parser_match_tokenkinds(p, TOK_LPAREN, SENTINEL)) {
        parser_advance(p);

        if (parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL))
            parser_throw_error(p, "Don't write functional code!");

        astnode->kind          = ASTNODE_GROUPING;
        astnode->expr_grouping = (ExprGrouping) {
            .expr = rule_expression(p)
        };

        parser_expect_token(p, TOK_RPAREN, ")");
        parser_advance(p);

    } else
        assert(!"Unexpected Token");

    return astnode;
}

static AstNode *rule_call(Parser *p) {
    // call ::= primary "(" ( <expr> ("," <expr>)* )? ")"

    AstNode *node = rule_primary(p);

    if (parser_match_tokenkinds(p, TOK_LPAREN, SENTINEL)) {
        parser_advance(p);

        AstNode *call   = malloc(sizeof(AstNode));
        call->kind      = ASTNODE_CALL;
        call->expr_call = (ExprCall) {
            .callee = node,
            .args   = astnodelist_new(),
        };

        while (!parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL)) {
            astnodelist_append(&call->expr_call.args, rule_expression(p));

            if (parser_match_tokenkinds(p, TOK_COMMA, SENTINEL)) {
                parser_advance(p);

                if (parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL))
                    parser_throw_error(p,"Extraneous `,`");
            }
        }

        parser_expect_token(p, TOK_RPAREN, ")");
        parser_advance(p);
        node = call;
    }

    return node;
}

static AstNode *rule_unary(Parser *p) {
    // unary ::= ("!" | "-") unary | call

    if (parser_match_tokenkinds(p, TOK_MINUS, TOK_BANG, SENTINEL)) {
        Token op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *node = malloc(sizeof(AstNode));

        node->kind = ASTNODE_UNARYOP;
        node->expr_unaryop = (ExprUnaryOp) {
            .op   = op,
            .node = rule_unary(p),
        };

        return node;

    } else {
        return rule_call(p);
    }

}

static AstNode *rule_factor(Parser *p) {
    // factor ::= unary (("/" | "*") unary)*
    return rule_unary(p);
}

static AstNode *rule_term(Parser *p) {
    // term ::= factor (("+" | "-") factor)*

    AstNode *lhs = rule_factor(p);

    while (parser_match_tokenkinds(p, TOK_PLUS, TOK_MINUS, SENTINEL)) {
        Token op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *rhs = rule_factor(p);

        AstNode *astnode = malloc(sizeof(AstNode));
        astnode->kind    = ASTNODE_BINOP;
        astnode->expr_binop   = (ExprBinOp) {
            .lhs = lhs,
            .op  = op,
            .rhs = rhs,
        };

        lhs = astnode;
    }

    return lhs;
}

static AstNode *rule_vardecl(Parser *p) {
    // vardecl ::= "val" <identifier> "'" TYPE "=" <expression> ";"

    assert(parser_match_tokenkinds(p, TOK_KW_VARDECL, SENTINEL));
    Token op = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_IDENTIFIER, "identifier");
    Token identifier = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_TICK, "type annotation");
    parser_advance(p);

    Token type = parser_get_current_token(p);
    if (!tokenkind_is_type(type.kind))
        throw_error(p->filename, &type, "Unknown type `%s`", type.value);

    parser_advance(p);

    parser_expect_token(p, TOK_ASSIGN, "=");
    parser_advance(p);

    AstNode *vardecl      = malloc(sizeof(AstNode));
    vardecl->kind         = ASTNODE_VARDECL;
    vardecl->stmt_vardecl = (StmtVarDecl) {
        .op         = op,
        .identifier = identifier,
        .type       = type,
        .value      = rule_expression(p),
    };

    parser_expect_token(p, TOK_SEMICOLON, ";");
    parser_advance(p);

    return vardecl;
}

static AstNode *rule_builtin_asm(Parser *p) {
    // asm ::= "asm" STRING ";"

    assert(parser_match_tokenkinds(p, TOK_KW_ASM, SENTINEL));
    Token op = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_STRING, "string");
    Token src = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_SEMICOLON, ";");
    parser_advance(p);

    AstNode *inlineasm   = malloc(sizeof(AstNode));
    inlineasm->kind      = ASTNODE_INLINEASM;
    inlineasm->stmt_inlineasm= (StmtInlineAsm) {
        .op  = op,
        .src = src,
    };

    return inlineasm;
}

static AstNode *rule_function(Parser *p) {
    // function ::= "proc" <identifier> "(" ")" <block>

    // TODO: paramlist
    // TODO: returntype

    assert(parser_match_tokenkinds(p, TOK_KW_FUNCTION, SENTINEL));
    Token op = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_IDENTIFIER, "identifier");

    Token identifier = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_LPAREN, "(");
    parser_advance(p);

    parser_expect_token(p, TOK_RPAREN, ")");
    parser_advance(p);

    AstNode *function   = malloc(sizeof(AstNode));
    function->kind      = ASTNODE_FUNC;
    function->stmt_func = (StmtFunc) {
        .op         = op,
        .body       = rule_block(p),
        .identifier = identifier,
    };

    return function;
}

static AstNode *rule_block(Parser *p) {
    // block ::= "{" statement* "}"

    parser_expect_token(p, TOK_LBRACE, "{");
    parser_advance(p);

    AstNode *node = malloc(sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts  = astnodelist_new(),
        .global = false
    };

    while (!parser_match_tokenkinds(p, TOK_RBRACE, SENTINEL)) {
        if (parser_is_at_end(p))
            throw_error_simple("Unmatching brace");

        astnodelist_append(&node->block.stmts, rule_stmt(p));
    }

    parser_advance(p);
    return node;
}

static AstNode *rule_expression(Parser *p) {
    return rule_term(p);
}

static AstNode *rule_exprstmt(Parser *p) {
    // exprstmt ::= expr ";"

    AstNode *node = rule_expression(p);
    parser_expect_token(p, TOK_SEMICOLON,";");

    parser_advance(p);
    return node;
}

static AstNode *rule_stmt(Parser *p) {
    // statement ::= block | function | if | while | expr ";"

    if (parser_match_tokenkinds(p, TOK_LBRACE, SENTINEL))
        return rule_block(p);

    else if (parser_match_tokenkinds(p, TOK_KW_FUNCTION, SENTINEL))
        return rule_function(p);

    else if (parser_match_tokenkinds(p, TOK_KW_ASM, SENTINEL))
        return rule_builtin_asm(p);

    else if (parser_match_tokenkinds(p, TOK_KW_VARDECL, SENTINEL))
        return rule_vardecl(p);

    else
        return rule_exprstmt(p);

}

static AstNode *rule_program(Parser *p) {
    // program ::= statement*

    AstNode *node = malloc(sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts  = astnodelist_new(),
        .global = true
    };

    while (!parser_is_at_end(p))
        astnodelist_append(&node->block.stmts, rule_stmt(p));

    return node;
}



AstNode *parser_parse(const TokenList *tokens, const char *filename) {
    Parser parser = {
        .current  = 0,
        .tokens   = tokens,
        .filename = filename,
    };
    AstNode *root = rule_program(&parser);
    return root;
}
