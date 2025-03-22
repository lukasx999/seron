#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "ast.h"
#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "lib/arena.h"


#define SENTINEL TOK_INVALID

typedef struct {
    size_t current;
    const TokenList *tokens;
    Arena *arena;
} Parser;

Token *parser_get_current_token(const Parser *p) {
    Token *tok = tokenlist_get(p->tokens, p->current);
    assert(tok != NULL);
    return tok;
}

// Convenience function that allows us to change the allocator easily
void *parser_alloc(Parser *p, size_t size) {
    return arena_alloc(p->arena, size);
}

void parser_advance(Parser *p) {
    p->current++;
}

// Checks if the current token is of one of the supplied kinds
// Last variadic argument should be `TOK_INVALID`
bool parser_match_tokenkinds(const Parser *p, ...) {
    va_list va;
    va_start(va, p);

    while (1) {
        TokenKind tok = va_arg(va, TokenKind);
        bool matched = parser_get_current_token(p)->kind == tok;
        if (matched || tok == SENTINEL) {
            va_end(va);
            return matched;
        }
    }

    assert(!"unreachable");
}

void parser_expect_token(
    const Parser *p,
    TokenKind tokenkind,
    const char *expected
) {
    if (!parser_match_tokenkinds(p, tokenkind, SENTINEL)) {
        Token *tok = parser_get_current_token(p);
        throw_error(*tok, "Expected `%s`", expected);
    }
}

void parser_throw_error(const Parser *p, const char *msg) {
    Token *tok = parser_get_current_token(p);
    throw_error(*tok, "%s", msg);
}

bool parser_is_at_end(const Parser *p) {
    return parser_match_tokenkinds(p, TOK_EOF, SENTINEL);
}

void parser_traverse_ast(AstNode *root, AstCallback callback, bool top_down, void *args) {
    assert(root != NULL);

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

        case ASTNODE_ASSIGN: {
            depth++;
            parser_traverse_ast(root->expr_assign.value, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_CALL: {
            depth++;
            ExprCall *call = &root->expr_call;

            if (call->builtin == BUILTIN_NONE)
                parser_traverse_ast(call->callee, callback, top_down, args);

            AstNodeList list = call->args;
            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback, top_down, args);

            depth--;
        } break;

        case ASTNODE_WHILE: {
            depth++;
            StmtWhile *while_ = &root->stmt_while;
            parser_traverse_ast(while_->condition, callback, top_down, args);
            parser_traverse_ast(while_->body, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_RETURN: {
            depth++;
            StmtReturn *return_ = &root->stmt_return;
            parser_traverse_ast(return_->expr, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_IF: {
            depth++;
            StmtIf if_ = root->stmt_if;
            parser_traverse_ast(if_.condition, callback, top_down, args);
            parser_traverse_ast(if_.then_body, callback, top_down, args);
            if (if_.else_body != NULL)
                parser_traverse_ast(if_.else_body, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_GROUPING:
            depth++;
            parser_traverse_ast(root->expr_grouping.expr, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_PROCEDURE:
            depth++;
            AstNode *body = root->stmt_procedure.body;
            if (body != NULL)
                parser_traverse_ast(body, callback, top_down, args);
            depth--;
            break;

        case ASTNODE_VARDECL: {
            depth++;
            StmtVarDecl *vardecl = &root->stmt_vardecl;
            if (vardecl->value != NULL)
                parser_traverse_ast(vardecl->value, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_BINOP: {
            depth++;
            ExprBinOp *binop = &root->expr_binop;
            parser_traverse_ast(binop->lhs, callback, top_down, args);
            parser_traverse_ast(binop->rhs, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_UNARYOP: {
            depth++;
            ExprUnaryOp *unaryop = &root->expr_unaryop;
            parser_traverse_ast(unaryop->node, callback, top_down, args);
            depth--;
        } break;

        case ASTNODE_LITERAL:
            break;

        default:
            assert(!"Unexpected Node Kind");
            break;
    }

    if (!top_down)
        callback(root, depth, args);

}

typedef struct {
    AstCallback callback;
    AstNodeKind kind;
    void *args;
} Query;

static void query_callback(AstNode *node, int depth, void *args) {
    Query *q = (Query*) args;

    if (node->kind == q->kind)
        q->callback(node, depth, q->args);
}

void parser_query_ast(AstNode *root, AstCallback callback, AstNodeKind kind, void *args) {
    Query q = {
        .args     = args,
        .callback = callback,
        .kind     = kind,
    };

    parser_traverse_ast(root, query_callback, true, (void*) &q);
}

// Prints a value in the following format: `<str>: <arg>`
// <arg> is omitted if arg == NULL
static void print_ast_value(
    const char *str,
    const char *color,
    const char *value,
    const char *opt
) {
    printf("%s%s%s", color, str, COLOR_END);
    if (value != NULL)
        printf("%s: %s%s", COLOR_GRAY, value, COLOR_END);
    if (opt != NULL)
        printf(" %s(%s)%s", COLOR_GRAY, opt, COLOR_END);
    puts("");
}

static void parser_print_ast_callback(AstNode *root, int depth, void *args) {
    assert(root != NULL);
    int spacing = *(int*) args;

    for (int _=0; _ < depth * spacing; ++_)
        printf("%s⋅%s", COLOR_GRAY, COLOR_END);

    switch (root->kind) {
        case ASTNODE_BLOCK: {
            print_ast_value(
                "block",
                COLOR_BLUE,
                NULL,
                NULL
            );
        } break;

        case ASTNODE_GROUPING:
            print_ast_value("grouping", COLOR_BLUE, NULL, NULL);
            break;

        case ASTNODE_ASSIGN:
            print_ast_value("assign", COLOR_RED, root->expr_assign.identifier.value, NULL);
            break;

        case ASTNODE_IF:
            print_ast_value("if", COLOR_RED, NULL, NULL);
            break;

        case ASTNODE_WHILE:
            print_ast_value("while", COLOR_RED, NULL, NULL);
            break;

        case ASTNODE_RETURN:
            print_ast_value("return", COLOR_RED, NULL, NULL);
            break;

        case ASTNODE_BINOP: {
            ExprBinOp *binop = &root->expr_binop;
            print_ast_value(tokenkind_to_string(binop->op.kind), COLOR_PURPLE, NULL, NULL);
        } break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp *unaryop = &root->expr_unaryop;
            print_ast_value(tokenkind_to_string(unaryop->op.kind), COLOR_PURPLE, NULL, NULL);
        } break;

        case ASTNODE_CALL: {
            ExprCall *call = &root->expr_call;
            print_ast_value(
                "call",
                COLOR_BLUE,
                NULL,
                call->builtin != BUILTIN_NONE ? "builtin" : NULL
            );
        } break;

        case ASTNODE_PROCEDURE: {
            StmtProcedure *func = &root->stmt_procedure;
            print_ast_value(tokenkind_to_string(func->op.kind), COLOR_RED, func->identifier.value, NULL);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &root->stmt_vardecl;
            print_ast_value(
                tokenkind_to_string(vardecl->op.kind),
                COLOR_RED,
                vardecl->identifier.value,
                typekind_to_string(vardecl->type.kind)
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

void parser_print_ast(AstNode *root, int spacing) {
    printf("\n");
    parser_traverse_ast(root, parser_print_ast_callback, true, &spacing);
    printf("\n");
}

AstNode *rule_program(Parser *p);

AstNode *parse(const TokenList *tokens, Arena *arena) {
    Parser parser = {
        .current  = 0,
        .tokens   = tokens,
        .arena    = arena,
    };

    return rule_program(&parser);
}



// Grammar rules

// forward-declarations, as some rules have cyclic dependencies
AstNode *rule_block(Parser *p);
AstNode *rule_expression(Parser *p);
AstNode *rule_stmt(Parser *p);

TypeKind rule_util_type(Parser *p) {
    // <type> ::= TYPE

    Token *type_tok = parser_get_current_token(p);
    TypeKind type = typekind_from_tokenkind(type_tok->kind);

    if (type == TYPE_INVALID)
        throw_error(*type_tok, "Unknown type `%s`", type_tok->value);

    parser_advance(p);
    return type;
}

AstNodeList rule_util_arglist(Parser *p) {
    // <arglist> ::= "(" ( <expr> ("," <expr>)* )? ")"

    parser_expect_token(p, TOK_LPAREN, "(");
    parser_advance(p);

    AstNodeList args = { 0 };
    astnodelist_init(&args, p->arena);

    while (!parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL)) {
        astnodelist_append(&args, rule_expression(p));

        if (parser_match_tokenkinds(p, TOK_COMMA, SENTINEL)) {
            parser_advance(p);

            if (parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL))
                parser_throw_error(p, "Extraneous `,`");
        }

    }

    parser_expect_token(p, TOK_RPAREN, ")");
    parser_advance(p);

    return args;
}

/*
 * the max size of out_params is assumed to be MAX_ARG_COUNT
 * returns paramlist param count
 */
size_t rule_util_paramlist(Parser *p, Param *out_params) {
    // <paramlist> ::= "(" (IDENTIFIER <type> ("," IDENTIFIER <type>)* )? ")"

    size_t i = 0;

    parser_expect_token(p, TOK_LPAREN, "(");
    parser_advance(p);

    while (!parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL)) {

        parser_expect_token(p, TOK_IDENTIFIER, "identifier");
        Token *tok = parser_get_current_token(p);
        const char *ident = tok->value;
        parser_advance(p);

        Type *type = parser_alloc(p, sizeof(Type));
        type->kind = rule_util_type(p);

        if (i >= MAX_ARG_COUNT)
            throw_error(*tok, "Procedures may not have more than %lu arguments", MAX_ARG_COUNT);

        out_params[i++] = (Param) {
            .ident = ident,
            .type  = type,
        };

        if (parser_match_tokenkinds(p, TOK_COMMA, SENTINEL)) {
            parser_advance(p);

            if (parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL))
                parser_throw_error(p, "Extraneous `,`");
        }

    }

    parser_expect_token(p, TOK_RPAREN, ")");
    parser_advance(p);

    return i;
}

AstNode *rule_primary(Parser *p) {
    // <primary> ::= NUMBER | IDENTIFIER | STRING | "(" <expression> ")"

    AstNode *astnode = parser_alloc(p, sizeof(AstNode));

    if (parser_match_tokenkinds(p, TOK_NUMBER, TOK_IDENTIFIER, TOK_STRING, SENTINEL)) {
        astnode->kind = ASTNODE_LITERAL;
        astnode->expr_literal = (ExprLiteral) {
            .op = *parser_get_current_token(p)
        };
        parser_advance(p);

    } else if (parser_match_tokenkinds(p, TOK_LPAREN, SENTINEL)) {
        parser_advance(p);

        if (parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL))
            parser_throw_error(p, "Don't write functional code!");

        astnode->kind = ASTNODE_GROUPING;
        astnode->expr_grouping = (ExprGrouping) {
            .expr = rule_expression(p)
        };

        parser_expect_token(p, TOK_RPAREN, ")");
        parser_advance(p);

    } else {
        parser_throw_error(p, "Unexpected Token");
    }

    return astnode;
}

AstNode *rule_call(Parser *p) {
    // <call> ::= ( <primary> | BUILTIN ) <argumentlist>

    Token *callee = parser_get_current_token(p);
    BuiltinFunction builtin = builtin_from_tokenkind(callee->kind);

    AstNode *node = NULL;
    if (builtin == BUILTIN_NONE) {
        node = rule_primary(p);

        if (!parser_match_tokenkinds(p, TOK_LPAREN, SENTINEL))
            return node;

    } else {
        parser_advance(p);
    }

    Token *op = parser_get_current_token(p);

    AstNode *call   = parser_alloc(p, sizeof(AstNode));
    call->kind      = ASTNODE_CALL;
    call->expr_call = (ExprCall) {
        .op      = *op,
        .callee  = node,
        .args    = rule_util_arglist(p),
        .builtin = builtin,
    };

    return call;
}

AstNode *rule_unary(Parser *p) {
    // <unary> ::= ("!" | "-") <unary> | <call>

    if (parser_match_tokenkinds(p, TOK_MINUS, TOK_BANG, SENTINEL)) {
        Token *op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *node = parser_alloc(p, sizeof(AstNode));

        node->kind = ASTNODE_UNARYOP;
        node->expr_unaryop = (ExprUnaryOp) {
            .op   = *op,
            .node = rule_unary(p),
        };

        return node;

    } else {
        return rule_call(p);
    }

}

AstNode *rule_addressof(Parser *p) {
    // <addressof> ::= "&" <expression>
    (void) p;
    // TODO: addrof
    assert(!"TODO");
    return NULL;
}

AstNode *rule_factor(Parser *p) {
    // <factor> ::= <unary> (("/" | "*") <unary>)*

    AstNode *lhs = rule_unary(p);

    while (parser_match_tokenkinds(p, TOK_SLASH, TOK_ASTERISK, SENTINEL)) {
        Token *op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *rhs = rule_unary(p);

        AstNode *astnode    = parser_alloc(p, sizeof(AstNode));
        astnode->kind       = ASTNODE_BINOP;
        astnode->expr_binop = (ExprBinOp) {
            .lhs  = lhs,
            .op   = *op,
            .rhs  = rhs,
            .kind = binopkind_from_tokenkind(op->kind),
        };

        lhs = astnode;
    }

    return lhs;

}

AstNode *rule_term(Parser *p) {
    // <term> ::= <factor> (("+" | "-") <factor>)*

    AstNode *lhs = rule_factor(p);

    while (parser_match_tokenkinds(p, TOK_PLUS, TOK_MINUS, SENTINEL)) {
        Token *op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *rhs = rule_factor(p);

        AstNode *astnode    = parser_alloc(p, sizeof(AstNode));
        astnode->kind       = ASTNODE_BINOP;
        astnode->expr_binop = (ExprBinOp) {
            .lhs  = lhs,
            .op   = *op,
            .rhs  = rhs,
            .kind = binopkind_from_tokenkind(op->kind),
        };

        lhs = astnode;
    }

    return lhs;
}

AstNode *rule_vardecl(Parser *p) {
    // <vardecl> ::= "val" <identifier> <type> ("=" <expression>)? ";"

    assert(parser_match_tokenkinds(p, TOK_KW_VARDECL, SENTINEL));
    Token *op = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_IDENTIFIER, "identifier");
    Token *identifier = parser_get_current_token(p);
    parser_advance(p);

    Type type = { .kind = rule_util_type(p) };


    AstNode *value = NULL;

    if (!parser_match_tokenkinds(p, TOK_SEMICOLON, SENTINEL)) {
        parser_expect_token(p, TOK_ASSIGN, "=");
        parser_advance(p);

        value = rule_expression(p);
    }

    parser_expect_token(p, TOK_SEMICOLON, ";");
    parser_advance(p);

    AstNode *vardecl      = parser_alloc(p, sizeof(AstNode));
    vardecl->kind         = ASTNODE_VARDECL;
    vardecl->stmt_vardecl = (StmtVarDecl) {
        .op         = *op,
        .identifier = *identifier,
        .type       = type,
        .value      = value,
    };

    return vardecl;
}

AstNode *rule_while(Parser *p) {
    // <while> ::= "while" <expression> <block>

    assert(parser_match_tokenkinds(p, TOK_KW_WHILE, SENTINEL));
    Token *op = parser_get_current_token(p);
    parser_advance(p);

    AstNode *cond  = rule_expression(p);
    AstNode *body  = rule_block(p);

    AstNode *node    = parser_alloc(p, sizeof(AstNode));
    node->kind       = ASTNODE_WHILE;
    node->stmt_while = (StmtWhile) {
        .op        = *op,
        .condition = cond,
        .body      = body,
    };

    return node;
}

AstNode *rule_if(Parser *p) {
    // <if> ::= "if" <expression> <block> ("else" <block>)?

    assert(parser_match_tokenkinds(p, TOK_KW_IF, SENTINEL));
    Token *op = parser_get_current_token(p);
    parser_advance(p);

    AstNode *cond  = rule_expression(p);
    AstNode *then  = rule_block(p);
    AstNode *else_ = NULL;

    if (parser_match_tokenkinds(p, TOK_KW_ELSE, SENTINEL)) {
        parser_advance(p);
        else_ = rule_block(p);
    }

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind    = ASTNODE_IF;
    node->stmt_if = (StmtIf) {
        .op        = *op,
        .condition = cond,
        .then_body = then,
        .else_body = else_,
    };

    return node;
}

AstNode *rule_return(Parser *p) {
    // <return> ::= "return" <expression> ";"

    assert(parser_match_tokenkinds(p, TOK_KW_RETURN, SENTINEL));
    Token *op = parser_get_current_token(p);
    parser_advance(p);

    AstNode *expr = rule_expression(p);

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind = ASTNODE_RETURN;
    node->stmt_return = (StmtReturn) {
        .op   = *op,
        .expr = expr,
    };

    parser_expect_token(p, TOK_SEMICOLON, ";");
    parser_advance(p);

    return node;
}

AstNode *rule_procedure(Parser *p) {
    // <procedure> ::= "proc" IDENTIFIER <paramlist> <type> <block>

    assert(parser_match_tokenkinds(p, TOK_KW_FUNCTION, SENTINEL));
    Token *op = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_IDENTIFIER, "identifier");
    Token *identifier = parser_get_current_token(p);
    parser_advance(p);

    ProcSignature sig = { 0 };
    sig.params_count = rule_util_paramlist(p, sig.params);

    sig.returntype = parser_alloc(p, sizeof(Type));
    /* Returntype is void if not specified */
    sig.returntype->kind = parser_match_tokenkinds(p, TOK_LBRACE, TOK_SEMICOLON, SENTINEL)
        ? TYPE_VOID
        : rule_util_type(p);

    Type type = {
        .kind = TYPE_FUNCTION,
        .type_signature = sig,
    };

    AstNode *body = parser_match_tokenkinds(p, TOK_SEMICOLON, SENTINEL)
        ? parser_advance(p), NULL // bet you didn't know about this one
        : rule_block(p);

    AstNode *proc = parser_alloc(p, sizeof(AstNode));
    proc->kind = ASTNODE_PROCEDURE;
    proc->stmt_procedure = (StmtProcedure) {
        .op         = *op,
        .body       = body,
        .identifier = *identifier,
        .type       = type,
    };

    return proc;
}

AstNode *rule_block(Parser *p) {
    // <block> ::= "{" <statement>* "}"

    Token *brace = parser_get_current_token(p);
    parser_expect_token(p, TOK_LBRACE, "{");
    parser_advance(p);

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts       = { 0 },
        .symboltable = NULL,
    };
    astnodelist_init(&node->block.stmts, p->arena);

    while (!parser_match_tokenkinds(p, TOK_RBRACE, SENTINEL)) {

        if (parser_is_at_end(p)) {
            throw_error(*brace, "Unmatching brace");
        }

        astnodelist_append(&node->block.stmts, rule_stmt(p));
    }

    parser_advance(p);
    return node;
}

AstNode *rule_assignment(Parser *p) {
    // <assignment> ::= IDENTIFIER "=" <assignment> | <term>

    AstNode *expr = rule_term(p);

    if (parser_match_tokenkinds(p, TOK_ASSIGN, SENTINEL)) {
        Token *op = parser_get_current_token(p);
        parser_advance(p);

        bool is_literal = expr->kind == ASTNODE_LITERAL;
        bool is_ident   = expr->expr_literal.op.kind == TOK_IDENTIFIER;
        if (!(is_literal && is_ident))
            parser_throw_error(p, "Invalid assignment target");

        // AstNode is not needed anymore, since we know its an identifier
        Token ident = expr->expr_literal.op;

        AstNode *value = rule_assignment(p);

        AstNode *node     = parser_alloc(p, sizeof(AstNode));
        node->kind        = ASTNODE_ASSIGN;
        node->expr_assign = (ExprAssignment) {
            .op         = *op,
            .value      = value,
            .identifier = ident,
        };

        return node;

    } else
        return expr;

}

AstNode *rule_expression(Parser *p) {
    // <expression> ::= <assignment>
    return rule_assignment(p);
}

AstNode *rule_exprstmt(Parser *p) {
    // <exprstmt> ::= <expr> ";"

    AstNode *node = rule_expression(p);
    parser_expect_token(p, TOK_SEMICOLON, ";");

    parser_advance(p);
    return node;
}

AstNode *rule_stmt(Parser *p) {
    // <statement> ::= <block> | <procedure> | <vardeclaration> | <if> | <while> | <return> | <expr> ";"

    if (parser_match_tokenkinds(p, TOK_LBRACE, SENTINEL))
        return rule_block(p);

    else if (parser_match_tokenkinds(p, TOK_KW_FUNCTION, SENTINEL))
        return rule_procedure(p);

    else if (parser_match_tokenkinds(p, TOK_KW_VARDECL, SENTINEL))
        return rule_vardecl(p);

    else if (parser_match_tokenkinds(p, TOK_KW_IF, SENTINEL))
        return rule_if(p);

    else if (parser_match_tokenkinds(p, TOK_KW_WHILE, SENTINEL))
        return rule_while(p);

    else if (parser_match_tokenkinds(p, TOK_KW_RETURN, SENTINEL))
        return rule_return(p);

    else
        return rule_exprstmt(p);

}

AstNode *rule_program(Parser *p) {
    // <program> ::= <statement>*

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts       = { 0 },
        .symboltable = NULL,
    };
    astnodelist_init(&node->block.stmts, p->arena);

    while (!parser_is_at_end(p))
        astnodelist_append(&node->block.stmts, rule_stmt(p));

    return node;
}
