#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "lib/arena.h"
#define UTIL_COLORS
#include "lib/util.h"

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "hashtable.h"


#define SENTINEL TOK_INVALID




static void astnodelist_init(AstNodeList *list, Arena *arena) {
     *list = (AstNodeList) {
        .cap   = 5,
        .size  = 0,
        .items = NULL,
        .arena = arena,
    };

    list->items = arena_alloc(arena, list->cap * sizeof(AstNode*));
}

static void astnodelist_append(AstNodeList *list, AstNode *node) {

    if (list->size == list->cap) {
        list->cap *= 2;
        list->items = arena_realloc(list->arena, list->items, list->cap * sizeof(AstNode*));
    }

    list->items[list->size++] = node;
}

static TypeKind type_from_token(TokenKind kind) {
    switch (kind) {
        case TOK_TYPE_INT:  return TYPE_INT;
        case TOK_TYPE_LONG: return TYPE_LONG;
        case TOK_TYPE_CHAR: return TYPE_CHAR;
        case TOK_TYPE_VOID: return TYPE_VOID;
        default:            return TYPE_INVALID;
    }
}

static LiteralKind literal_from_token(TokenKind kind) {
    switch (kind) {
        case TOK_NUMBER: return LITERAL_NUMBER;
        case TOK_CHAR:   return LITERAL_CHAR;
        case TOK_IDENT:  return LITERAL_IDENT;
        case TOK_STRING: return LITERAL_STRING;
        default:         PANIC("unknown tokenkind");
    }
}

static BinOpKind binop_from_token(TokenKind kind) {
    switch (kind) {
        case TOK_PLUS:     return BINOP_ADD;
        case TOK_MINUS:    return BINOP_SUB;
        case TOK_SLASH:    return BINOP_DIV;
        case TOK_ASTERISK: return BINOP_MUL;
        default:           PANIC("unknown tokenkind");
    }
}

static UnaryOpKind unaryop_from_token(TokenKind kind) {
    switch (kind) {
        case TOK_MINUS:     return UNARYOP_MINUS;
        case TOK_BANG:      return UNARYOP_NEG;
        case TOK_AMPERSAND: return UNARYOP_ADDROF;
        case TOK_ASTERISK:  return UNARYOP_DEREF;
        default:            PANIC("unknown tokenkind");
    }
}

typedef struct {
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

// TODO: use this instead of parser_alloc()
static inline void *parser_new_node(Parser *p) {
    return NON_NULL(arena_alloc(p->arena, sizeof(AstNode)));
}

// Convenience function that allows us to change the allocator easily
static inline void *parser_alloc(Parser *p, size_t size) {
    return arena_alloc(p->arena, size);
}

// checks if the current token is of one of the supplied kinds
// last variadic argument should be `TOK_INVALID` aka `SENTINEL`
static bool parser_match_tokens(const Parser *p, ...) {
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

    UNREACHABLE();
}

// checks if the current token is of the supplied kind
static inline bool parser_match_token(const Parser *p, TokenKind kind) {
    return parser_match_tokens(p, kind, SENTINEL);
}

static inline void parser_expect_token(const Parser *p, TokenKind tokenkind) {
    if (!parser_match_token(p, tokenkind)) {
        compiler_message_tok(
            MSG_ERROR,
            parser_tok(p),
            "Expected %s",
            tokenkind_to_string(tokenkind)
        );
        exit(EXIT_FAILURE);
    }
}

static inline bool parser_is_at_end(const Parser *p) {
    return parser_match_token(p, TOK_EOF);
}

void parser_traverse_ast(
    AstNode    *root,
    AstCallback callback_pre,
    AstCallback callback_post,
    void       *args
) {
    NON_NULL(root);

    static int depth = 0;

    if (callback_pre != NULL)
        callback_pre(root, depth, args);

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            depth++;
            AstNodeList list = root->block.stmts;

            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback_pre, callback_post, args);

            depth--;
        } break;

        case ASTNODE_ASSIGN: {
            depth++;
            parser_traverse_ast(root->expr_assign.value, callback_pre, callback_post, args);
            depth--;
        } break;

        case ASTNODE_CALL: {
            depth++;

            ExprCall *call = &root->expr_call;
            parser_traverse_ast(call->callee, callback_pre, callback_post, args);

            AstNodeList list = call->args;
            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], callback_pre, callback_post, args);

            depth--;
        } break;

        case ASTNODE_WHILE: {
            depth++;
            StmtWhile *while_ = &root->stmt_while;
            parser_traverse_ast(while_->condition, callback_pre, callback_post, args);
            parser_traverse_ast(while_->body, callback_pre, callback_post, args);
            depth--;
        } break;

        case ASTNODE_RETURN: {
            depth++;
            StmtReturn *return_ = &root->stmt_return;
            if (return_->expr != NULL)
                parser_traverse_ast(return_->expr, callback_pre, callback_post, args);
            depth--;
        } break;

        case ASTNODE_IF: {
            depth++;

            StmtIf *if_ = &root->stmt_if;
            parser_traverse_ast(if_->condition, callback_pre, callback_post, args);
            parser_traverse_ast(if_->then_body, callback_pre, callback_post, args);

            if (if_->else_body != NULL)
                parser_traverse_ast(if_->else_body, callback_pre, callback_post, args);

            depth--;
        } break;

        case ASTNODE_GROUPING:
            depth++;
            parser_traverse_ast(root->expr_grouping.expr, callback_pre, callback_post, args);
            depth--;
            break;

        case ASTNODE_PROC:
            depth++;

            AstNode *body = root->stmt_proc.body;
            if (body != NULL)
                parser_traverse_ast(body, callback_pre, callback_post, args);

            depth--;
            break;

        case ASTNODE_VARDECL: {
            depth++;
            StmtVarDecl *vardecl = &root->stmt_vardecl;
            if (vardecl->init != NULL)
                parser_traverse_ast(vardecl->init, callback_pre, callback_post, args);
            depth--;
        } break;

        case ASTNODE_BINOP: {
            depth++;
            ExprBinOp *binop = &root->expr_binop;
            parser_traverse_ast(binop->lhs, callback_pre, callback_post, args);
            parser_traverse_ast(binop->rhs, callback_pre, callback_post, args);
            depth--;
        } break;

        case ASTNODE_UNARYOP: {
            depth++;
            ExprUnaryOp *unaryop = &root->expr_unaryop;
            parser_traverse_ast(unaryop->node, callback_pre, callback_post, args);
            depth--;
        } break;

        case ASTNODE_LITERAL: break;

        default: PANIC("unexpected node kind");
    }

    if (callback_post != NULL)
        callback_post(root, depth, args);

}




typedef struct {
    AstDispatchEntry *table;
    size_t size;
    void *user_args;
} DispatchTable;

static AstDispatchEntry *get_dispatch_entry(AstNode *node, DispatchTable *dt) {

    for (size_t i=0; i < dt->size; ++i) {
        AstDispatchEntry *entry = &dt->table[i];
        if (node->kind == entry->kind)
            return entry;
    }

    return NULL;

}

static void dispatch_callback_pre(AstNode *root, int depth, void *args) {
    DispatchTable *dt = args;

    AstDispatchEntry *entry = get_dispatch_entry(root, dt);
    if (entry == NULL) return;

    AstCallback fn = entry->fn_pre;
    if (fn != NULL)
        fn(root, depth, dt->user_args);

}

static void dispatch_callback_post(AstNode *root, int depth, void *args) {
    DispatchTable *dt = args;

    AstDispatchEntry *entry = get_dispatch_entry(root, dt);
    if (entry == NULL) return;

    AstCallback fn = entry->fn_post;
    if (fn != NULL)
        fn(root, depth, dt->user_args);

}

void parser_dispatch_ast(
    AstNode          *root,
    AstDispatchEntry *table,
    size_t            table_size,
    void             *args
) {

    DispatchTable dt = { table, table_size, args };
    parser_traverse_ast(root, dispatch_callback_pre, dispatch_callback_post, (void*) &dt);

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

static void print_colored(const char *color, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    printf("%s", color);
    vprintf(fmt, va);
    printf(COLOR_END);
    va_end(va);
}

#define AST_COLOR_KEYWORD COLOR_RED
#define AST_COLOR_SEMANTIC COLOR_BLUE
#define AST_COLOR_OPERATION COLOR_PURPLE

static void parser_print_ast_callback(AstNode *root, int depth, void *args) {

    NON_NULL(root);
    NON_NULL(args);

    int spacing = *(int*) args;

    for (int _=0; _ < depth * spacing; ++_)
        printf("%s⋅%s", COLOR_GRAY, COLOR_END);

    switch (root->kind) {

        case ASTNODE_BLOCK:
            print_colored(AST_COLOR_SEMANTIC, "block\n");
            break;

        case ASTNODE_GROUPING:
            print_colored(AST_COLOR_SEMANTIC, "grouping\n");
            break;

        case ASTNODE_IF:
            print_colored(AST_COLOR_KEYWORD, "if\n");
            break;

        case ASTNODE_WHILE:
            print_colored(AST_COLOR_KEYWORD, "while\n");
            break;

        case ASTNODE_RETURN:
            print_colored(AST_COLOR_KEYWORD, "return\n");
            break;

        case ASTNODE_BINOP: {
            ExprBinOp *binop = &root->expr_binop;

            const char *op = NULL;
            switch (binop->kind) {
                case BINOP_ADD: op = "add"; break;
                case BINOP_SUB: op = "sub"; break;
                case BINOP_MUL: op = "mul"; break;
                case BINOP_DIV: op = "div"; break;
                default:        PANIC("unknown binop kind");
            }

            print_colored(AST_COLOR_OPERATION, "%s\n", NON_NULL(op));

        } break;

        case ASTNODE_CALL:
            print_colored(AST_COLOR_OPERATION, "call\n");
            break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp *unaryop = &root->expr_unaryop;

            const char *op = NULL;
            switch (unaryop->kind) {
                case UNARYOP_MINUS:  op = "minus";  break;
                case UNARYOP_NEG:    op = "neg";    break;
                case UNARYOP_ADDROF: op = "addrof"; break;
                case UNARYOP_DEREF:  op = "deref";  break;
                default:            PANIC("unknown unaryop kind");
            }

            print_colored(AST_COLOR_OPERATION, "%s\n", NON_NULL(op));

        } break;

        case ASTNODE_ASSIGN: {
            // ExprAssign *assign = &root->expr_assign;

            print_colored(AST_COLOR_OPERATION, "assign");

        } break;

        // TODO: refactor to print_colored()
        case ASTNODE_PROC: {
            StmtProc *func = &root->stmt_proc;
            print_ast_value(tokenkind_to_string(func->op.kind), COLOR_RED, func->ident.value, NULL);
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &root->stmt_vardecl;

            print_colored(AST_COLOR_KEYWORD, "vardecl");
            print_colored(
                COLOR_GRAY,
                " (%s: %s)\n",
                vardecl->ident.value,
                stringify_typekind(vardecl->type.kind)
            );

        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral *literal = &root->expr_literal;
            Token *tok = &literal->op;
            print_ast_value(tokenkind_to_string(tok->kind), COLOR_RED, tok->value, NULL);
        } break;

        default: PANIC("unexpected node kind");
    }
}

void parser_print_ast(AstNode *root, int spacing) {
    parser_traverse_ast(root, parser_print_ast_callback, NULL, (void*) &spacing);
}

static AstNode *rule_program(Parser *p);

AstNode *parse(const char *src, Arena *arena) {

    Parser parser = {
        .arena    = arena,
        .lexer    = { .src = src },
    };

    parser_advance(&parser); // get first token
    return rule_program(&parser);
}



// forward-declarations, as some rules have cyclic dependencies
static AstNode *rule_expr(Parser *p);
static AstNode *rule_stmt(Parser *p);
static Type rule_util_type(Parser *p);

static AstNodeList rule_util_arglist(Parser *p) {
    // <arglist> ::= "(" ( <expr> ("," <expr>)* )? ")"

    parser_expect_token(p, TOK_LPAREN);
    parser_advance(p);

    AstNodeList args = { 0 };
    astnodelist_init(&args, p->arena);

    while (!parser_match_token(p, TOK_RPAREN)) {
        astnodelist_append(&args, rule_expr(p));

        if (parser_match_token(p, TOK_COMMA)) {
            parser_advance(p);

            if (parser_match_token(p, TOK_RPAREN)) {
                compiler_message_tok(MSG_ERROR, parser_tok(p),  "Extraneous `,`");
                exit(EXIT_FAILURE);
            }
        }

    }

    parser_expect_token(p, TOK_RPAREN);
    parser_advance(p);

    return args;
}

static void rule_util_paramlist(Parser *p, ProcSignature *sig) {
    // <paramlist> ::= "(" (IDENTIFIER <type> ("," IDENTIFIER <type>)* )? ")"

    parser_expect_token(p, TOK_LPAREN);
    parser_advance(p);

    while (!parser_match_token(p, TOK_RPAREN)) {

        parser_expect_token(p, TOK_IDENT);
        Token tok = parser_advance(p);

        Type *type = parser_alloc(p, sizeof(Type));
        *type = rule_util_type(p);

        if (sig->params_count >= MAX_PARAM_COUNT) {
            compiler_message_tok(MSG_ERROR, tok, "Procedures may not have more than %d parameters!", MAX_PARAM_COUNT);
            exit(EXIT_FAILURE);
        }

        Param param = {
            .ident = { 0 },
            .type  = type,
        };

        if (strlen(tok.value) > MAX_IDENT_LEN) {
            compiler_message_tok(MSG_ERROR, tok, "Identifiers cannot be longer than %d characters!", MAX_IDENT_LEN);
            exit(EXIT_FAILURE);
        }

        strncpy(param.ident, tok.value, ARRAY_LEN(param.ident));

        sig->params[sig->params_count++] = param;

        if (parser_match_token(p, TOK_COMMA)) {
            parser_advance(p);

            if (parser_match_token(p, TOK_RPAREN)) {
                compiler_message_tok(MSG_ERROR, parser_tok(p), "Extraneous `,`"),
                exit(EXIT_FAILURE);
            }
        }

    }

    parser_expect_token(p, TOK_RPAREN);
    parser_advance(p);

}

static Type rule_util_type(Parser *p) {
    // <type> ::=
    //        | "*" <type>
    //        | "int"
    //        | "void"
    //        | "char"
    //        | "proc" <paramlist> <type>

    Type ty = { .kind = TYPE_INVALID };

    if (parser_match_token(p, TOK_ASTERISK)) {
        parser_advance(p);

        ty.kind = TYPE_POINTER;
        ty.pointee = parser_alloc(p, sizeof(Type));
        *ty.pointee = rule_util_type(p);

    } else if (parser_match_token(p, TOK_KW_PROC)) {
        parser_advance(p);

        // TODO: exract this common behaviour
        ty.kind = TYPE_PROCEDURE;
        rule_util_paramlist(p, &ty.signature);

        ty.signature.returntype = parser_alloc(p, sizeof(Type));
        *ty.signature.returntype = rule_util_type(p);

    } else {
        Token tok = parser_advance(p);
        ty.kind = type_from_token(tok.kind);
    }


    // TODO:
    // if (ty.kind == TYPE_INVALID) {
    //     compiler_message_tok(MSG_ERROR, tok, "Unknown type `%s`", tok.value);
    //     exit(EXIT_FAILURE);
    // }

    return ty;
}



static AstNode *rule_grouping(Parser *p) {
    // <grouping> ::= "(" <expression> ")"

    assert(parser_match_token(p, TOK_LPAREN));
    parser_advance(p);

    if (parser_match_token(p, TOK_RPAREN)) {
        compiler_message_tok(MSG_ERROR, parser_tok(p), "Don't write functional code!");
        exit(EXIT_FAILURE);
    }

    AstNode *astnode       = parser_alloc(p, sizeof(AstNode));
    astnode->kind          = ASTNODE_GROUPING;
    astnode->expr_grouping = (ExprGrouping) {
        .expr = rule_expr(p)
    };

    parser_expect_token(p, TOK_RPAREN);
    parser_advance(p);

    return astnode;
}

static AstNode *rule_primary(Parser *p) {
    // <primary> ::=
    //           | NUMBER
    //           | CHAR
    //           | IDENTIFIER
    //           | STRING
    //           | <grouping>


    if (parser_match_tokens(p, TOK_NUMBER, TOK_CHAR, TOK_IDENT, TOK_STRING, SENTINEL)) {

        Token tok = parser_tok(p);

        AstNode *astnode      = parser_alloc(p, sizeof(AstNode));
        astnode->kind         = ASTNODE_LITERAL;
        astnode->expr_literal = (ExprLiteral) {
            .kind = literal_from_token(tok.kind),
            .op   = tok,
        };

        parser_advance(p);
        return astnode;

    } else if (parser_match_token(p, TOK_LPAREN)) {
        return rule_grouping(p);

    } else {
        compiler_message_tok(MSG_ERROR, parser_tok(p), "Unexpected Token");
        exit(EXIT_FAILURE);
    }

    UNREACHABLE();

}

static AstNode *rule_call(Parser *p) {
    // <call> ::= <primary> <arglist>

    AstNode *node = rule_primary(p);

    if (!parser_match_token(p, TOK_LPAREN))
        return node;

    Token op = parser_tok(p);

    AstNode *call   = parser_alloc(p, sizeof(AstNode));
    call->kind      = ASTNODE_CALL;
    call->expr_call = (ExprCall) {
        .op      = op,
        .callee  = node,
        .args    = rule_util_arglist(p),
    };

    return call;
}

static AstNode *rule_unary(Parser *p) {
    // <unary> ::= ( "&" | "*" | "!" | "-" ) <unary> | <call>

    if (!parser_match_tokens(p, TOK_MINUS, TOK_BANG, TOK_AMPERSAND, TOK_ASTERISK, SENTINEL))
        return rule_call(p);

    Token op           = parser_advance(p);
    AstNode *node      = parser_alloc(p, sizeof(AstNode));
    node->kind         = ASTNODE_UNARYOP;
    node->expr_unaryop = (ExprUnaryOp) {
        .op   = op,
        .kind = unaryop_from_token(op.kind),
        .node = rule_unary(p),
    };

    return node;

}

static AstNode *rule_factor(Parser *p) {
    // <factor> ::= <unary> (( "/" | "*" ) <unary>)*

    AstNode *lhs = rule_unary(p);

    while (parser_match_tokens(p, TOK_SLASH, TOK_ASTERISK, SENTINEL)) {
        Token op = parser_advance(p);

        AstNode *rhs = rule_unary(p);

        AstNode *astnode    = parser_alloc(p, sizeof(AstNode));
        astnode->kind       = ASTNODE_BINOP;
        astnode->expr_binop = (ExprBinOp) {
            .lhs  = lhs,
            .op   = op,
            .rhs  = rhs,
            .kind = binop_from_token(op.kind),
        };

        lhs = astnode;
    }

    return lhs;

}

static AstNode *rule_term(Parser *p) {
    // <term> ::= <factor> (("+" | "-") <factor>)*

    AstNode *lhs = rule_factor(p);

    while (parser_match_tokens(p, TOK_PLUS, TOK_MINUS, SENTINEL)) {
        Token op = parser_advance(p);
        AstNode *rhs = rule_factor(p);

        AstNode *node    = parser_alloc(p, sizeof(AstNode));
        node->kind       = ASTNODE_BINOP;
        node->expr_binop = (ExprBinOp) {
            .lhs  = lhs,
            .op   = op,
            .rhs  = rhs,
            .kind = binop_from_token(op.kind),
        };

        lhs = node;
    }

    return lhs;
}

static AstNode *rule_assign(Parser *p) {
    // <assign> ::= <expr> "=" <assign> | <term>

    AstNode *expr = rule_term(p);

    if (!parser_match_token(p, TOK_ASSIGN))
        return expr;

    Token op = parser_advance(p);

    AstNode *value = rule_assign(p);

    AstNode *node     = parser_alloc(p, sizeof(AstNode));
    node->kind        = ASTNODE_ASSIGN;
    node->expr_assign = (ExprAssign) {
        .op     = op,
        .target = expr,
        .value  = value,
    };

    return node;

}

static AstNode *rule_expr(Parser *p) {
    // <expression> ::= <assignment>
    return rule_assign(p);
}


// returns NULL on empty statement
static AstNode *rule_exprstmt(Parser *p) {
    // <exprstmt> ::= <expr>? ";"

    AstNode *node = parser_match_token(p, TOK_SEMICOLON)
        ? NULL
        : rule_expr(p);
    parser_expect_token(p, TOK_SEMICOLON);
    parser_advance(p);
    return node;
}

static AstNode *rule_vardecl(Parser *p) {
    // <vardecl> ::= "let" <identifier> <type> ("=" <expression>)? ";"

    assert(parser_match_token(p, TOK_KW_VARDECL));
    Token op = parser_advance(p);

    parser_expect_token(p, TOK_IDENT);
    Token ident = parser_advance(p);

    Type type = rule_util_type(p);
    AstNode *value = NULL;

    if (!parser_match_token(p, TOK_SEMICOLON)) {
        parser_expect_token(p, TOK_ASSIGN);
        parser_advance(p);

        value = rule_expr(p);
    }

    parser_expect_token(p, TOK_SEMICOLON);
    parser_advance(p);

    AstNode *vardecl      = parser_alloc(p, sizeof(AstNode));
    vardecl->kind         = ASTNODE_VARDECL;
    vardecl->stmt_vardecl = (StmtVarDecl) {
        .op    = op,
        .ident = ident,
        .type  = type,
        .init  = value,
    };

    return vardecl;
}

static AstNode *rule_block(Parser *p) {
    // <block> ::= "{" <statement>* "}"

    parser_expect_token(p, TOK_LBRACE);
    Token brace = parser_advance(p);

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind  = ASTNODE_BLOCK;
    node->block = (Block) { 0 };

    astnodelist_init(&node->block.stmts, p->arena);

    while (!parser_match_token(p, TOK_RBRACE)) {

        if (parser_is_at_end(p)) {
            compiler_message_tok(MSG_ERROR, brace, "Unmatching brace");
            exit(EXIT_FAILURE);
        }

        AstNode *stmt = rule_stmt(p);
        if (stmt != NULL)
            astnodelist_append(&node->block.stmts, stmt);
    }

    parser_advance(p);
    return node;
}

static AstNode *rule_while(Parser *p) {
    // <while> ::= "while" <expression> <block>

    assert(parser_match_token(p, TOK_KW_WHILE));
    Token op = parser_advance(p);

    AstNode *cond  = rule_expr(p);
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

    assert(parser_match_token(p, TOK_KW_IF));
    Token op = parser_advance(p);

    AstNode *cond  = rule_expr(p);
    AstNode *then  = rule_block(p);
    AstNode *else_ = NULL;

    if (parser_match_token(p, TOK_KW_ELSE)) {
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
    // <return> ::= "return" <expression>? ";"

    assert(parser_match_token(p, TOK_KW_RETURN));
    Token op = parser_advance(p);

    AstNode *expr = parser_match_token(p, TOK_SEMICOLON)
        ? NULL
        : rule_expr(p);

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

// returns NULL on empty statement
static AstNode *rule_stmt(Parser *p) {
    // <statement> ::=
    //             | <block>
    //             | <procedure>
    //             | <vardecl>
    //             | <if>
    //             | <while>
    //             | <return>
    //             | <exprstmt>

    return
        parser_match_token(p, TOK_LBRACE)     ? rule_block  (p) :
        parser_match_token(p, TOK_KW_VARDECL) ? rule_vardecl(p) :
        parser_match_token(p, TOK_KW_IF)      ? rule_if     (p) :
        parser_match_token(p, TOK_KW_WHILE)   ? rule_while  (p) :
        parser_match_token(p, TOK_KW_RETURN)  ? rule_return (p) :
    rule_exprstmt(p);
}

static AstNode *rule_proc(Parser *p) {
    // <procedure> ::= "proc" IDENTIFIER <paramlist> <type> <block>

    assert(parser_match_token(p, TOK_KW_PROC));
    Token op = parser_advance(p);

    parser_expect_token(p, TOK_IDENT);
    Token ident = parser_advance(p);

    ProcSignature sig = { 0 };
    rule_util_paramlist(p, &sig);

    sig.returntype = parser_alloc(p, sizeof(Type));

    // Returntype is void if not specified
    if (parser_match_tokens(p, TOK_LBRACE, TOK_SEMICOLON, SENTINEL))
        sig.returntype->kind = TYPE_VOID;
    else
        *sig.returntype = rule_util_type(p);

    Type type = {
        .kind = TYPE_PROCEDURE,
        .signature = sig,
    };

    AstNode *body = parser_match_token(p, TOK_SEMICOLON)
        ? parser_advance(p), NULL
        : rule_block(p);

    AstNode *proc = parser_alloc(p, sizeof(AstNode));
    proc->kind = ASTNODE_PROC;
    proc->stmt_proc = (StmtProc) {
        .op         = op,
        .body       = body,
        .ident      = ident,
        .type       = type,
    };

    return proc;
}

static AstNode *rule_declaration(Parser *p) {
    // <declaration> ::= <procedure> | <vardecl>

    return
        parser_match_token(p, TOK_KW_PROC) ? rule_proc(p) :
        parser_match_token(p, TOK_KW_VARDECL)  ? rule_vardecl  (p) :
    (compiler_message(MSG_ERROR, "Expected declaration"), exit(EXIT_FAILURE), NULL);
    // TODO: synchronize parser
}

static AstNode *rule_program(Parser *p) {
    // <program> ::= <declaration>*

    AstNode *node = parser_alloc(p, sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) { 0 };

    astnodelist_init(&node->block.stmts, p->arena);

    while (!parser_is_at_end(p))
        astnodelist_append(&node->block.stmts, rule_declaration(p));

    return node;
}
