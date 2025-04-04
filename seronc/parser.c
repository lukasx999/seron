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
#include "lib/arena.h"


#define SENTINEL TOK_INVALID




void astnodelist_init(AstNodeList *list, Arena *arena) {
     *list = (AstNodeList) {
        .cap   = 5,
        .size  = 0,
        .items = NULL,
        .arena = arena,
    };

    list->items = arena_alloc(arena, list->cap * sizeof(AstNode*));
}

void astnodelist_append(AstNodeList *list, AstNode *node) {

    if (list->size == list->cap) {
        list->cap *= 2;
        list->items = arena_realloc(list->arena, list->items, list->cap * sizeof(AstNode*));
    }

    list->items[list->size++] = node;
}

BinOpKind binopkind_from_tokenkind(TokenKind kind) {
    switch (kind) {
        case TOK_PLUS:     return BINOP_ADD;
        case TOK_MINUS:    return BINOP_SUB;
        case TOK_SLASH:    return BINOP_DIV;
        case TOK_ASTERISK: return BINOP_MUL;
        default:           assert(!"unknown tokenkind");
    }
}

BuiltinFunction builtin_from_tokenkind(TokenKind kind) {
    switch (kind) {
        case TOK_BUILTIN_ASM: return BUILTIN_ASM;
        default:              return BUILTIN_NONE;
    }
}



typedef struct {
    size_t current;
    Arena *arena;
    Token tok;
    LexerState lexer;
} Parser;

static inline Token parser_tok(const Parser *p) {
    return p->tok;
}

// returns the old token
static inline Token parser_advance(Parser *p) {
    Token old = p->tok;
    p->tok = lexer_next(&p->lexer);
    return old;
}

// Convenience function that allows us to change the allocator easily
static inline void *parser_alloc(Parser *p, size_t size) {
    return arena_alloc(p->arena, size);
}

// checks if the current token is of one of the supplied kinds
// last variadic argument should be `TOK_INVALID` aka `SENTINEL`
static bool parser_match_tokenkinds(const Parser *p, ...) {
    va_list va;
    va_start(va, p);

    while (1) {
        TokenKind tok = va_arg(va, TokenKind);
        bool matched = parser_tok(p).kind == tok;
        if (matched || tok == SENTINEL) {
            va_end(va);
            return matched;
        }
    }

    assert(!"unreachable");
}

// checks if the current token is of the supplied kind
static inline bool parser_match_tokenkind(const Parser *p, TokenKind kind) {
    return parser_match_tokenkinds(p, kind, SENTINEL);
}

static inline void parser_expect_token(const Parser *p, TokenKind tokenkind) {
    if (!parser_match_tokenkind(p, tokenkind)) {
        Token tok = parser_tok(p);
        throw_error(tok, "Expected %s", tokenkind_to_string(tokenkind));
    }
}

static inline void parser_throw_error(const Parser *p, const char *msg) {
    throw_error(parser_tok(p), "%s", msg);
}

static inline bool parser_is_at_end(const Parser *p) {
    return parser_match_tokenkind(p, TOK_EOF);
}

void parser_traverse_ast(
    AstNode *root,
    AstCallback callback,
    bool top_down,
    void *args
) {
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
static inline void print_ast_value(
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

static AstNode *rule_program(Parser *p);

AstNode *parse(const char *src, Arena *arena) {
    Parser parser = {
        .current  = 0,
        .arena    = arena,
        .lexer    = { .src = src },
    };

    parser_advance(&parser); // get first token

    return rule_program(&parser);
}



// forward-declarations, as some rules have cyclic dependencies
static AstNode *rule_expression(Parser *p);
static AstNode *rule_stmt(Parser *p);

static TypeKind rule_util_type(Parser *p) {
    // <type> ::= TYPE

    Token type_tok = parser_tok(p);
    TypeKind type = typekind_from_tokenkind(type_tok.kind);

    if (type == TYPE_INVALID)
        throw_error(type_tok, "Unknown type `%s`", type_tok.value);

    parser_advance(p);
    return type;
}

static AstNodeList rule_util_arglist(Parser *p) {
    // <arglist> ::= "(" ( <expr> ("," <expr>)* )? ")"

    parser_expect_token(p, TOK_LPAREN);
    parser_advance(p);

    AstNodeList args = { 0 };
    astnodelist_init(&args, p->arena);

    while (!parser_match_tokenkind(p, TOK_RPAREN)) {
        astnodelist_append(&args, rule_expression(p));

        if (parser_match_tokenkind(p, TOK_COMMA)) {
            parser_advance(p);

            if (parser_match_tokenkind(p, TOK_RPAREN))
                parser_throw_error(p, "Extraneous `,`");
        }

    }

    parser_expect_token(p, TOK_RPAREN);
    parser_advance(p);

    return args;
}


/*
 * the max size of out_params is assumed to be MAX_ARG_COUNT
 * returns paramlist param count
 */
static size_t rule_util_paramlist(Parser *p, Param *out_params) {
    // <paramlist> ::= "(" (IDENTIFIER <type> ("," IDENTIFIER <type>)* )? ")"

    size_t i = 0;

    parser_expect_token(p, TOK_LPAREN);
    parser_advance(p);

    while (!parser_match_tokenkind(p, TOK_RPAREN)) {

        parser_expect_token(p, TOK_IDENTIFIER);
        Token tok = parser_advance(p);

        Type *type = parser_alloc(p, sizeof(Type));
        type->kind = rule_util_type(p);

        if (i >= MAX_ARG_COUNT)
            throw_error(tok, "Procedures may not have more than %lu arguments", MAX_ARG_COUNT);

        out_params[i++] = (Param) {
            .ident = tok.value,
            .type  = type,
        };

        if (parser_match_tokenkind(p, TOK_COMMA)) {
            parser_advance(p);

            if (parser_match_tokenkind(p, TOK_RPAREN))
                parser_throw_error(p, "Extraneous `,`");
        }

    }

    parser_expect_token(p, TOK_RPAREN);
    parser_advance(p);

    return i;
}

static AstNode *rule_primary(Parser *p) {
    // <primary> ::= NUMBER | IDENTIFIER | STRING | "(" <expression> ")"

    AstNode *astnode = parser_alloc(p, sizeof(AstNode));

    if (parser_match_tokenkinds(p, TOK_NUMBER, TOK_IDENTIFIER, TOK_STRING, SENTINEL)) {
        astnode->kind = ASTNODE_LITERAL;
        astnode->expr_literal = (ExprLiteral) {
            .op = parser_tok(p)
        };
        parser_advance(p);

    } else if (parser_match_tokenkind(p, TOK_LPAREN)) {
        parser_advance(p);

        if (parser_match_tokenkind(p, TOK_RPAREN))
            parser_throw_error(p, "Don't write functional code!");

        astnode->kind = ASTNODE_GROUPING;
        astnode->expr_grouping = (ExprGrouping) {
            .expr = rule_expression(p)
        };

        parser_expect_token(p, TOK_RPAREN);
        parser_advance(p);

    } else {
        parser_throw_error(p, "Unexpected Token");
    }

    return astnode;
}

static AstNode *rule_call(Parser *p) {
    // <call> ::= ( <primary> | BUILTIN ) <argumentlist>

    Token callee = parser_tok(p);
    BuiltinFunction builtin = builtin_from_tokenkind(callee.kind);

    AstNode *node = NULL;
    if (builtin == BUILTIN_NONE) {
        node = rule_primary(p);

        if (!parser_match_tokenkind(p, TOK_LPAREN))
            return node;

    } else {
        parser_advance(p);
    }

    Token op = parser_tok(p);

    AstNode *call   = parser_alloc(p, sizeof(AstNode));
    call->kind      = ASTNODE_CALL;
    call->expr_call = (ExprCall) {
        .op      = op,
        .callee  = node,
        .args    = rule_util_arglist(p),
        .builtin = builtin,
    };

    return call;
}

static AstNode *rule_unary(Parser *p) {
    // <unary> ::= ("!" | "-") <unary> | <call>

    if (parser_match_tokenkinds(p, TOK_MINUS, TOK_BANG, SENTINEL)) {
        Token op = parser_advance(p);

        AstNode *node = parser_alloc(p, sizeof(AstNode));

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
    // <factor> ::= <unary> (("/" | "*") <unary>)*

    AstNode *lhs = rule_unary(p);

    while (parser_match_tokenkinds(p, TOK_SLASH, TOK_ASTERISK, SENTINEL)) {
        Token op = parser_advance(p);

        AstNode *rhs = rule_unary(p);

        AstNode *astnode    = parser_alloc(p, sizeof(AstNode));
        astnode->kind       = ASTNODE_BINOP;
        astnode->expr_binop = (ExprBinOp) {
            .lhs  = lhs,
            .op   = op,
            .rhs  = rhs,
            .kind = binopkind_from_tokenkind(op.kind),
        };

        lhs = astnode;
    }

    return lhs;

}

static AstNode *rule_term(Parser *p) {
    // <term> ::= <factor> (("+" | "-") <factor>)*

    AstNode *lhs = rule_factor(p);

    while (parser_match_tokenkinds(p, TOK_PLUS, TOK_MINUS, SENTINEL)) {
        Token op = parser_advance(p);
        AstNode *rhs = rule_factor(p);

        AstNode *astnode    = parser_alloc(p, sizeof(AstNode));
        astnode->kind       = ASTNODE_BINOP;
        astnode->expr_binop = (ExprBinOp) {
            .lhs  = lhs,
            .op   = op,
            .rhs  = rhs,
            .kind = binopkind_from_tokenkind(op.kind),
        };

        lhs = astnode;
    }

    return lhs;
}

static AstNode *rule_assignment(Parser *p) {
    // <assignment> ::= IDENTIFIER "=" <assignment> | <term>

    AstNode *expr = rule_term(p);

    if (parser_match_tokenkind(p, TOK_ASSIGN)) {
        Token op = parser_advance(p);

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
            .op         = op,
            .value      = value,
            .identifier = ident,
        };

        return node;

    } else {
        return expr;
    }

}

static AstNode *rule_expression(Parser *p) {
    // <expression> ::= <assignment>
    return rule_assignment(p);
}

static AstNode *rule_exprstmt(Parser *p) {
    // TODO: make expr optional to allow empty statements
    // <exprstmt> ::= <expr> ";"

    AstNode *node = rule_expression(p);
    parser_expect_token(p, TOK_SEMICOLON);
    parser_advance(p);
    return node;
}

static AstNode *rule_vardecl(Parser *p) {
    // <vardecl> ::= "let" <identifier> <type> ("=" <expression>)? ";"

    assert(parser_match_tokenkind(p, TOK_KW_VARDECL));
    Token op = parser_advance(p);

    parser_expect_token(p, TOK_IDENTIFIER);
    Token identifier = parser_advance(p);

    Type type = {
        .kind = rule_util_type(p)
    };

    AstNode *value = NULL;

    if (!parser_match_tokenkind(p, TOK_SEMICOLON)) {
        parser_expect_token(p, TOK_ASSIGN);
        parser_advance(p);

        value = rule_expression(p);
    }

    parser_expect_token(p, TOK_SEMICOLON);
    parser_advance(p);

    AstNode *vardecl      = parser_alloc(p, sizeof(AstNode));
    vardecl->kind         = ASTNODE_VARDECL;
    vardecl->stmt_vardecl = (StmtVarDecl) {
        .op         = op,
        .identifier = identifier,
        .type       = type,
        .value      = value,
    };

    return vardecl;
}

static AstNode *rule_block(Parser *p) {
    // <block> ::= "{" <statement>* "}"

    parser_expect_token(p, TOK_LBRACE);
    Token brace = parser_advance(p);

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts       = { 0 },
        .symboltable = NULL,
    };
    astnodelist_init(&node->block.stmts, p->arena);

    while (!parser_match_tokenkind(p, TOK_RBRACE)) {

        if (parser_is_at_end(p))
            throw_error(brace, "Unmatching brace");

        astnodelist_append(&node->block.stmts, rule_stmt(p));
    }

    parser_advance(p);
    return node;
}

static AstNode *rule_while(Parser *p) {
    // <while> ::= "while" <expression> <block>

    assert(parser_match_tokenkind(p, TOK_KW_WHILE));
    Token op = parser_advance(p);

    AstNode *cond  = rule_expression(p);
    AstNode *body  = rule_block(p);

    AstNode *node    = parser_alloc(p, sizeof(AstNode));
    node->kind       = ASTNODE_WHILE;
    node->stmt_while = (StmtWhile) {
        .op        = op,
        .condition = cond,
        .body      = body,
    };

    return node;
}

static AstNode *rule_if(Parser *p) {
    // <if> ::= "if" <expression> <block> ("else" <block>)?

    assert(parser_match_tokenkind(p, TOK_KW_IF));
    Token op = parser_advance(p);

    AstNode *cond  = rule_expression(p);
    AstNode *then  = rule_block(p);
    AstNode *else_ = NULL;

    if (parser_match_tokenkind(p, TOK_KW_ELSE)) {
        parser_advance(p);
        else_ = rule_block(p);
    }

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind    = ASTNODE_IF;
    node->stmt_if = (StmtIf) {
        .op        = op,
        .condition = cond,
        .then_body = then,
        .else_body = else_,
    };

    return node;
}

static AstNode *rule_return(Parser *p) {
    // <return> ::= "return" <expression> ";"

    assert(parser_match_tokenkind(p, TOK_KW_RETURN));
    Token op = parser_advance(p);

    AstNode *expr = rule_expression(p);

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind = ASTNODE_RETURN;
    node->stmt_return = (StmtReturn) {
        .op   = op,
        .expr = expr,
    };

    parser_expect_token(p, TOK_SEMICOLON);
    parser_advance(p);

    return node;
}

static AstNode *rule_stmt(Parser *p) {
    // <statement> ::= <block> | <procedure> | <vardeclaration> | <if> | <while> | <return> | <expr> ";"

    return
        parser_match_tokenkind(p, TOK_LBRACE)      ? rule_block  (p) :
        parser_match_tokenkind(p, TOK_KW_VARDECL)  ? rule_vardecl(p) :
        parser_match_tokenkind(p, TOK_KW_IF)       ? rule_if     (p) :
        parser_match_tokenkind(p, TOK_KW_WHILE)    ? rule_while  (p) :
        parser_match_tokenkind(p, TOK_KW_RETURN)   ? rule_return (p) :
    rule_exprstmt(p);
}

static AstNode *rule_procedure(Parser *p) {
    // <procedure> ::= "proc" IDENTIFIER <paramlist> <type> <block>

    assert(parser_match_tokenkind(p, TOK_KW_FUNCTION));
    Token op = parser_advance(p);

    parser_expect_token(p, TOK_IDENTIFIER);
    Token identifier = parser_advance(p);

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

    AstNode *body = parser_match_tokenkind(p, TOK_SEMICOLON)
        ? parser_advance(p), NULL // bet you didn't know about this one
        : rule_block(p);

    AstNode *proc = parser_alloc(p, sizeof(AstNode));
    proc->kind = ASTNODE_PROCEDURE;
    proc->stmt_procedure = (StmtProcedure) {
        .op         = op,
        .body       = body,
        .identifier = identifier,
        .type       = type,
    };

    return proc;
}

static AstNode *rule_declaration(Parser *p) {
    // <declaration> ::= <procedure> | <vardecl>

    return
        parser_match_tokenkind(p, TOK_KW_FUNCTION) ? rule_procedure(p) :
        parser_match_tokenkind(p, TOK_KW_VARDECL)  ? rule_vardecl  (p) :
    (compiler_message(MSG_ERROR, "Expected declaration"), exit(EXIT_FAILURE), NULL);
    // TODO: synchronize parser
}

static AstNode *rule_program(Parser *p) {
    // <program> ::= <declaration>*

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts       = { 0 },
        .symboltable = NULL,
    };
    astnodelist_init(&node->block.stmts, p->arena);

    while (!parser_is_at_end(p))
        astnodelist_append(&node->block.stmts, rule_declaration(p));

    return node;
}
