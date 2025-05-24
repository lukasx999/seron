#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <setjmp.h>

#include <arena.h>
#include <ver.h>

#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "colors.h"



#define SENTINEL TOK_SENTINEL



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

static TypeKind type_from_token_keyword(TokenKind kind) {
    switch (kind) {
        case TOK_KW_TYPE_INT:  return TYPE_INT;
        case TOK_KW_TYPE_LONG: return TYPE_LONG;
        case TOK_KW_TYPE_CHAR: return TYPE_CHAR;
        case TOK_KW_TYPE_VOID: return TYPE_VOID;
        default:               return TYPE_INVALID;
    }
    UNREACHABLE();
}

static LiteralKind literal_from_token(TokenKind kind) {
    switch (kind) {
        case TOK_LITERAL_IDENT:  return LITERAL_IDENT;
        case TOK_LITERAL_NUMBER: return LITERAL_NUMBER;
        case TOK_LITERAL_STRING: return LITERAL_STRING;
        default:                 PANIC("unknown tokenkind");
    }
    UNREACHABLE();
}

static BinOpKind binop_from_token(TokenKind kind) {
    switch (kind) {
        case TOK_PLUS:      return BINOP_ADD;
        case TOK_MINUS:     return BINOP_SUB;
        case TOK_SLASH:     return BINOP_DIV;
        case TOK_ASTERISK:  return BINOP_MUL;
        case TOK_EQ:        return BINOP_EQ;
        case TOK_NEQ:       return BINOP_NEQ;
        case TOK_LT:        return BINOP_LT;
        case TOK_LT_EQ:     return BINOP_LT_EQ;
        case TOK_GT:        return BINOP_GT;
        case TOK_GT_EQ:     return BINOP_GT_EQ;
        case TOK_PIPE:      return BINOP_BITWISE_OR;
        case TOK_AMPERSAND: return BINOP_BITWISE_AND;
        case TOK_LOG_OR:    return BINOP_LOG_OR;
        case TOK_LOG_AND:   return BINOP_LOG_AND;
        default: PANIC("unknown tokenkind");
    }
    UNREACHABLE();
}

static UnaryOpKind unaryop_from_token(TokenKind kind) {
    switch (kind) {
        case TOK_MINUS:     return UNARYOP_MINUS;
        case TOK_BANG:      return UNARYOP_NEG;
        case TOK_AMPERSAND: return UNARYOP_ADDROF;
        case TOK_ASTERISK:  return UNARYOP_DEREF;
        default: PANIC("unknown tokenkind");
    }
    UNREACHABLE();
}

static const char *stringify_unaryop(UnaryOpKind op) {
    switch (op) {
        case UNARYOP_MINUS:  return "minus";
        case UNARYOP_NEG:    return "neg";
        case UNARYOP_ADDROF: return "addrof";
        case UNARYOP_DEREF:  return "deref";
    }
    UNREACHABLE();
}

static const char *stringify_binop(BinOpKind op) {
    switch (op) {
        case BINOP_ADD:         return "add";
        case BINOP_SUB:         return "sub";
        case BINOP_MUL:         return "mul";
        case BINOP_DIV:         return "div";
        case BINOP_EQ:          return "eq";
        case BINOP_NEQ:         return "neq";
        case BINOP_GT_EQ:       return "gt-eq";
        case BINOP_GT:          return "gt";
        case BINOP_LT:          return "lt";
        case BINOP_LT_EQ:       return "lt-eq";
        case BINOP_BITWISE_OR:  return "bitwise-or";
        case BINOP_BITWISE_AND: return "bitwise-and";
        case BINOP_LOG_OR:      return "log-or";
        case BINOP_LOG_AND:     return "log-and";
    }
    UNREACHABLE();
}



// we always have to know if we're in a statement or declaration, so we know
// where to jump to in case of an error
typedef enum {
    CONTEXT_STMT,
    CONTEXT_DECL,
} ParserContext;

typedef struct {
    Arena *arena;
    Token tok;
    Lexer lexer;
    ParserContext ctx;
    int errcount;
    // this is the sanest way to implement error recovery for recursive descent parsing
    // otherwise, we'd have to propagate errors all the way up the call stack, which would be
    // very verbose, and annoying.
    jmp_buf ctx_stmt;
    jmp_buf ctx_decl;
} Parser;

// returns the current token
static inline const Token *parser_peek(const Parser *p) {
    return &p->tok;
}

static inline AstNode *parser_new_node(Parser *p) {
    return NON_NULL(arena_alloc(p->arena, sizeof(AstNode)));
}

static bool parser_match_tokens_va(const Parser *p, va_list va) {
    while (1) {
        TokenKind tok = va_arg(va, TokenKind);
        bool matched = parser_peek(p)->kind == tok;
        if (matched || tok == SENTINEL) {
            return matched;
        }
    }

    UNREACHABLE();
}

// checks if the current token is of one of the supplied kinds
// last variadic argument should be `TOK_INVALID` aka `SENTINEL`
static bool parser_match_tokens(const Parser *p, ...) {
    va_list va;
    va_start(va, p);
    bool match = parser_match_tokens_va(p, va);
    va_end(va);
    return match;
}

// checks if the current token is of the supplied kind
static inline bool parser_match_token(const Parser *p, TokenKind kind) {
    return parser_match_tokens(p, kind, SENTINEL);
}

static inline bool parser_is_at_end(const Parser *p) {
    return parser_match_token(p, TOK_EOF);
}

// moves one token ahead, returning the token before
static inline Token parser_advance(Parser *p) {

    if (parser_is_at_end(p)) {
        diagnostic(DIAG_ERROR, "Unexpected end of file. (please finish your code)");
        // there's no point in trying to synchronize anything as there are no more tokens
        exit(EXIT_FAILURE);
    }

    Token old = p->tok;
    p->tok = lexer_next(&p->lexer);
    return old;
}

// move to the next statement
static void parser_recover_stmt(Parser *p) {
    while (1) {
        if (parser_match_token(p, TOK_SEMICOLON))
            break;
        parser_advance(p);
    }
    parser_advance(p);
}

// move to the next declaration
static void parser_recover_decl(Parser *p) {
    while (1) {
        if (parser_match_tokens(p, TOK_KW_PROC, TOK_KW_TABLE, SENTINEL))
            break;
        parser_advance(p);
    }
}

// jump to the next valid token in the current context ("panic mode")
static inline void parser_sync(Parser *p) {
    // TODO: check for EOF
    p->errcount++;
    switch (p->ctx) {
        case CONTEXT_DECL:
            parser_recover_decl(p);
            longjmp(p->ctx_decl, 1);
            break;
        case CONTEXT_STMT:
            parser_recover_stmt(p);
            longjmp(p->ctx_stmt, 1);
            break;
    }
    UNREACHABLE();
}

static inline void parser_expect_token(Parser *p, TokenKind tokenkind) {
    if (!parser_match_token(p, tokenkind)) {
        diagnostic_loc(DIAG_ERROR, parser_peek(p), "Expected `%s`", stringify_tokenkind(tokenkind));
        parser_sync(p);
    }
}

static inline bool parser_token_is_type(const Parser *p) {
    // TODO: find a better way of doing this
    return parser_match_tokens(p, TOK_KW_TYPE_INT, TOK_KW_TYPE_CHAR, TOK_KW_TYPE_LONG, TOK_KW_TYPE_VOID, TOK_ASTERISK, SENTINEL);
}

// advance, enforcing that the current token has the specified kind
static inline Token parser_consume(Parser *p, TokenKind kind) {
    // can't use parser_expect_token() on an EOF token, as its gonna try to synchronize
    if (!parser_is_at_end(p))
        parser_expect_token(p, kind);
    return parser_advance(p);
}




void parser_traverse_ast(AstNode *root, AstCallback fn_pre, AstCallback fn_post, void *args) {
    NON_NULL(root);

    static int depth = 0;

    if (fn_pre != NULL)
        fn_pre(root, depth, args);

    switch (root->kind) {

        case ASTNODE_BLOCK: {
            depth++;
            AstNodeList list = root->block.stmts;

            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], fn_pre, fn_post, args);

            depth--;
        } break;

        case ASTNODE_ASSIGN: {
            depth++;
            parser_traverse_ast(root->expr_assign.value, fn_pre, fn_post, args);
            depth--;
        } break;

        case ASTNODE_CALL: {
            depth++;

            ExprCall *call = &root->expr_call;
            parser_traverse_ast(call->callee, fn_pre, fn_post, args);

            AstNodeList list = call->args;
            for (size_t i=0; i < list.size; ++i)
                parser_traverse_ast(list.items[i], fn_pre, fn_post, args);

            depth--;
        } break;

        case ASTNODE_WHILE: {
            depth++;
            StmtWhile *while_ = &root->stmt_while;
            parser_traverse_ast(while_->condition, fn_pre, fn_post, args);
            parser_traverse_ast(while_->body, fn_pre, fn_post, args);
            depth--;
        } break;

        case ASTNODE_RETURN: {
            depth++;
            StmtReturn *return_ = &root->stmt_return;
            if (return_->expr != NULL)
                parser_traverse_ast(return_->expr, fn_pre, fn_post, args);
            depth--;
        } break;

        case ASTNODE_IF: {
            depth++;

            StmtIf *if_ = &root->stmt_if;
            parser_traverse_ast(if_->condition, fn_pre, fn_post, args);
            parser_traverse_ast(if_->then_body, fn_pre, fn_post, args);

            if (if_->else_body != NULL)
                parser_traverse_ast(if_->else_body, fn_pre, fn_post, args);

            depth--;
        } break;

        case ASTNODE_GROUPING:
            depth++;
            parser_traverse_ast(root->expr_grouping.expr, fn_pre, fn_post, args);
            depth--;
            break;

        case ASTNODE_PROC:
            depth++;

            AstNode *body = root->stmt_proc.body;
            if (body != NULL)
                parser_traverse_ast(body, fn_pre, fn_post, args);

            depth--;
            break;

        case ASTNODE_VARDECL: {
            depth++;
            StmtVarDecl *vardecl = &root->stmt_vardecl;
            if (vardecl->init != NULL)
                parser_traverse_ast(vardecl->init, fn_pre, fn_post, args);
            depth--;
        } break;

        case ASTNODE_BINOP: {
            depth++;
            ExprBinOp *binop = &root->expr_binop;
            parser_traverse_ast(binop->lhs, fn_pre, fn_post, args);
            parser_traverse_ast(binop->rhs, fn_pre, fn_post, args);
            depth--;
        } break;

        case ASTNODE_UNARYOP: {
            depth++;
            ExprUnaryOp *unaryop = &root->expr_unaryop;
            parser_traverse_ast(unaryop->node, fn_pre, fn_post, args);
            depth--;
        } break;

        case ASTNODE_TABLE:
        case ASTNODE_LITERAL:
            NOP()
            break;

        default: PANIC("unexpected node kind");
    }

    if (fn_post != NULL)
        fn_post(root, depth, args);

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

void parser_dispatch_ast(AstNode *root, AstDispatchEntry *table, size_t table_size, void *args) {
    DispatchTable dt = {
        .table = table,
        .size = table_size,
        .user_args = args,
    };
    parser_traverse_ast(root, dispatch_callback_pre, dispatch_callback_post, &dt);
}

static void print_colored(const char *color, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    printf("%s", color);
    vprintf(fmt, va);
    printf(COLOR_END);
    va_end(va);
}

// TODO: refactor
#define AST_COLOR_KEYWORD   COLOR_RED
#define AST_COLOR_SEMANTIC  COLOR_BLUE
#define AST_COLOR_OPERATION COLOR_PURPLE
#define AST_COLOR_IDENT     COLOR_PURPLE

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

        case ASTNODE_RETURN: {
            StmtReturn *ret = &root->stmt_return;
            print_colored(AST_COLOR_KEYWORD, "return");

            if (ret->expr == NULL)
                print_colored(AST_COLOR_IDENT, " (no-expr)");

            printf("\n");
        } break;

        case ASTNODE_BINOP: {
            ExprBinOp *binop = &root->expr_binop;

            const char *op = stringify_binop(binop->kind);
            print_colored(AST_COLOR_OPERATION, "%s\n", NON_NULL(op));

        } break;

        case ASTNODE_CALL:
            print_colored(AST_COLOR_OPERATION, "call\n");
            break;

        case ASTNODE_UNARYOP: {
            ExprUnaryOp *unaryop = &root->expr_unaryop;

            const char *op = stringify_unaryop(unaryop->kind);
            print_colored(AST_COLOR_OPERATION, "%s\n", NON_NULL(op));

        } break;

        case ASTNODE_ASSIGN: {
            print_colored(AST_COLOR_OPERATION, "assign\n");
        } break;

        case ASTNODE_PROC: {
            DeclProc *proc = &root->stmt_proc;
            print_colored(AST_COLOR_KEYWORD, "proc: ");
            print_colored(AST_COLOR_IDENT, "%s", proc->ident.value);

            print_colored(AST_COLOR_IDENT, "(");

            ProcSignature *sig = proc->type.signature;
            for (size_t i=0; i < sig->params_count; ++i) {
                const char *sep = i == sig->params_count-1 ? "" : ", ";
                print_colored(AST_COLOR_IDENT, "%s%s", sig->params[i].ident, sep);
            }

            print_colored(AST_COLOR_IDENT, ")");

            if (proc->body == NULL)
                print_colored(AST_COLOR_IDENT, " (no-body)");

            printf("\n");

            // TODO: print param types
        } break;

        case ASTNODE_VARDECL: {
            StmtVarDecl *vardecl = &root->stmt_vardecl;
            print_colored(AST_COLOR_KEYWORD, "vardecl: ");
            print_colored(AST_COLOR_IDENT, "%s\n", vardecl->ident.value);
            // TODO: print type information

        } break;

        case ASTNODE_LITERAL: {
            ExprLiteral *literal = &root->expr_literal;
            Token *tok = &literal->op;

            switch (literal->kind) {
                case LITERAL_STRING:
                    print_colored(AST_COLOR_KEYWORD, "string: ");
                    print_colored(AST_COLOR_IDENT, "%s\n", tok->value);
                    break;

                case LITERAL_IDENT:
                    print_colored(AST_COLOR_KEYWORD, "ident: ");
                    print_colored(AST_COLOR_IDENT, "%s\n", tok->value);
                    break;

                case LITERAL_NUMBER: {
                    print_colored(AST_COLOR_KEYWORD, "number: ");
                    print_colored(AST_COLOR_IDENT, "%d ", tok->number);

                    const char *type = NULL;
                    switch (tok->number_type) {
                        case NUMBER_CHAR: type = "char"; break;
                        case NUMBER_INT:  type = "int";  break;
                        case NUMBER_LONG: type = "long"; break;
                        case NUMBER_ANY:  NOP()          break;
                    }

                    if (type != NULL)
                        print_colored(AST_COLOR_IDENT, "(%s)", type);
                    printf("\n");


                } break;
                default: PANIC("unknown unaryop");
            }

        } break;

        default: PANIC("unexpected node kind");
    }
}

void parser_print_ast(AstNode *root, int spacing) {
    parser_traverse_ast(root, parser_print_ast_callback, NULL, &spacing);
}

static AstNode *rule_program(Parser *p);

AstNode *parse(const char *src, Arena *arena) {

    Parser parser = {
        .arena = arena,
    };

    lexer_init(&parser.lexer, src);
    // get the first token
    parser.tok = lexer_next(&parser.lexer);

    AstNode *root = rule_program(&parser);

    if (parser.errcount) {
        diagnostic(DIAG_ERROR, "Parsing failed with %d errors", parser.errcount);
        exit(EXIT_FAILURE);
    }

    return root;
}



// forward-declarations, as some rules have cyclic dependencies
static AstNode *rule_expr(Parser *p);
static AstNode *rule_stmt(Parser *p);
static Type rule_util_type(Parser *p);

static AstNodeList rule_util_arglist(Parser *p) {
    // <arglist> ::= "(" ( <expr> ("," <expr>)* )? ")"

    parser_consume(p, TOK_LPAREN);

    AstNodeList args = { 0 };
    astnodelist_init(&args, p->arena);

    while (!parser_match_token(p, TOK_RPAREN)) {
        astnodelist_append(&args, rule_expr(p));

        if (parser_match_token(p, TOK_COMMA))
            parser_consume(p, TOK_COMMA);

    }

    parser_consume(p, TOK_RPAREN);

    return args;
}

static Param rule_util_param(Parser *p) {
    // <param> ::= IDENTIFIER ":" <type>

    Token tok = parser_consume(p, TOK_LITERAL_IDENT);
    parser_consume(p, TOK_COLON);

    Param param = {
        .ident = { 0 },
        .type  = rule_util_type(p),
    };

    strncpy(param.ident, tok.value, ARRAY_LEN(param.ident) - 1);
    return param;

}

static void rule_util_paramlist(Parser *p, ProcSignature *sig) {
    // <paramlist> ::= "(" (<param> ("," <param>)* )? ")"

    parser_consume(p, TOK_LPAREN);

    while (!parser_match_token(p, TOK_RPAREN)) {

        sig->params[sig->params_count++] = rule_util_param(p);

        const Token *tok = parser_peek(p);
        if (sig->params_count >= MAX_PARAM_COUNT) {
            diagnostic_loc(DIAG_ERROR, tok, "Procedures may not have more than %d parameters!", MAX_PARAM_COUNT);
            parser_sync(p);
        }

        if (parser_match_token(p, TOK_COMMA))
            parser_consume(p, TOK_COMMA);
    }

    parser_consume(p, TOK_RPAREN);

}

static void rule_util_fieldlist(Parser *p, Table *table) {
    // <fieldlist> ::= "{" (<param> ("," <param>)* )? "}"

    parser_consume(p, TOK_LBRACE);

    while (!parser_match_token(p, TOK_RBRACE)) {

        table->fields[table->field_count++] = rule_util_param(p);

        if (parser_match_token(p, TOK_COMMA))
            parser_consume(p, TOK_COMMA);

    }

    parser_consume(p, TOK_RBRACE);

}

// if `out_ident` is not NULL, the rule will check for a procedure name, and write it to the pointer
// if `out_ident` is NULL, the rule will parser an anonymous procedure
// the operator token is returned when `out_op` is non NULL
// this utility rule exists, so procedure type parsing may be reused for type annotations and lambdas
static Type rule_util_proc_type(Parser *p, Token *out_ident, Token *out_op) {
    // <util_proc> ::= "proc" IDENTIFIER? <paramlist> <type>?

    Token op = parser_consume(p, TOK_KW_PROC);
    if (out_op != NULL)
        *out_op = op;

    if (out_ident != NULL)
        *out_ident = parser_consume(p, TOK_LITERAL_IDENT);

    ProcSignature *sig = arena_alloc(p->arena, sizeof(ProcSignature));
    sig->params_count = 0;
    rule_util_paramlist(p, sig);

    // returntype is void if not specified
    if (parser_token_is_type(p))
        sig->returntype = rule_util_type(p);
    else
        sig->returntype.kind = TYPE_VOID;

    Type type = {
        .kind = TYPE_PROCEDURE,
        .signature = sig,
    };

    return type;
}

static Type rule_util_type(Parser *p) {
    // <type> ::=
    //        | "*" <type>
    //        | "int"
    //        | "void"
    //        | "char"
    //        | <proc-type>

    Type ty = { .kind = TYPE_INVALID };
    const Token *tok = parser_peek(p);

    if (parser_match_token(p, TOK_ASTERISK)) {
        parser_advance(p);

        ty.kind = TYPE_POINTER;
        ty.pointee = arena_alloc(p->arena, sizeof(Type));
        *ty.pointee = rule_util_type(p);

    } else if (parser_match_token(p, TOK_KW_PROC)) {
        ty = rule_util_proc_type(p, NULL, NULL);

    } else if (parser_match_token(p, TOK_LITERAL_IDENT)) {
        // TODO:
        ty.kind = TYPE_TABLE;

    } else if (parser_token_is_type(p)) {
        Token tok = parser_advance(p);
        ty.kind = type_from_token_keyword(tok.kind);

    } else {
        diagnostic_loc(DIAG_ERROR, tok, "Unknown type `%s`", stringify_tokenkind(tok->kind));
        parser_sync(p);
    }

    assert(ty.kind != TYPE_INVALID);
    return ty;
}

static AstNode *rule_grouping(Parser *p) {
    // <grouping> ::= "(" <expression> ")"

    parser_consume(p, TOK_LPAREN);

    if (parser_match_token(p, TOK_RPAREN)) {
        diagnostic_loc(DIAG_ERROR, parser_peek(p), "Don't write functional code!");
        exit(EXIT_FAILURE);
    }

    AstNode *astnode       = parser_new_node(p);
    astnode->kind          = ASTNODE_GROUPING;
    astnode->expr_grouping = (ExprGrouping) {
        .expr = rule_expr(p)
    };

    parser_consume(p, TOK_RPAREN);

    return astnode;
}

static AstNode *rule_primary(Parser *p) {
    // <primary> ::=
    //           | NUMBER
    //           | CHAR
    //           | IDENTIFIER
    //           | STRING
    //           | <grouping>

    if (parser_match_tokens(p, TOK_LITERAL_NUMBER, TOK_LITERAL_IDENT, TOK_LITERAL_STRING, SENTINEL)) {

        const Token *tok = parser_peek(p);

        AstNode *astnode      = parser_new_node(p);
        astnode->kind         = ASTNODE_LITERAL;
        astnode->expr_literal = (ExprLiteral) {
            .op = *tok,
            .kind = literal_from_token(tok->kind),
        };

        parser_advance(p);
        return astnode;

    } else if (parser_match_token(p, TOK_LPAREN)) {
        return rule_grouping(p);

    } else {
        const Token *tok = parser_peek(p);
        diagnostic_loc(DIAG_ERROR, tok, "unexpected token `%s`, expected expression", stringify_tokenkind(tok->kind));
        parser_sync(p);
    }

    UNREACHABLE();

}

static AstNode *rule_call(Parser *p) {
    // <call> ::= <primary> <arglist>

    AstNode *node = rule_primary(p);

    if (!parser_match_token(p, TOK_LPAREN))
        return node;

    const Token *op = parser_peek(p);

    AstNode *call   = parser_new_node(p);
    call->kind      = ASTNODE_CALL;
    call->expr_call = (ExprCall) {
        .op      = *op,
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
    AstNode *node      = parser_new_node(p);
    node->kind         = ASTNODE_UNARYOP;
    node->expr_unaryop = (ExprUnaryOp) {
        .op   = op,
        .kind = unaryop_from_token(op.kind),
        .node = rule_unary(p),
    };

    return node;

}

typedef AstNode*(*GrammarRule)(Parser *p);

// template for a binop rule
static AstNode *templ_binop(Parser *p, GrammarRule rule, ...) {
    va_list va;
    va_start(va, rule);

    AstNode *lhs = rule(p);

    // va list gets exhausted, so it needs to be copied for every check
    va_list va_new;

    while (1) {
        va_copy(va_new, va);
        if (!parser_match_tokens_va(p, va_new))
            break;

        Token op = parser_advance(p);
        AstNode *rhs = rule(p);

        AstNode *node    = parser_new_node(p);
        node->kind       = ASTNODE_BINOP;
        node->expr_binop = (ExprBinOp) {
            .lhs  = lhs,
            .op   = op,
            .rhs  = rhs,
            .kind = binop_from_token(op.kind),
        };

        lhs = node;
    }

    va_end(va);
    return lhs;
}

static AstNode *rule_factor(Parser *p) {
    // <factor> ::= <unary> (( "/" | "*" ) <unary>)*
    return templ_binop(p, rule_unary, TOK_SLASH, TOK_ASTERISK, SENTINEL);
}

static AstNode *rule_term(Parser *p) {
    // <term> ::= <factor> (("+" | "-") <factor>)*
    return templ_binop(p, rule_factor, TOK_PLUS, TOK_MINUS, SENTINEL);
}

static AstNode *rule_comparison(Parser *p) {
    // <comparison> ::= <term> ((">" | ">=" | "<" | "<=") <term>)*
    return templ_binop(p, rule_term, TOK_LT, TOK_LT_EQ, TOK_GT, TOK_GT_EQ, SENTINEL);
}

static AstNode *rule_equality(Parser *p) {
    // <equality> ::= <comparison> (("!=" | "==") <comparison>)*
    return templ_binop(p, rule_comparison, TOK_EQ, TOK_NEQ, SENTINEL);
}

static AstNode *rule_bitwise_and(Parser *p) {
    // <bitwise-and> ::= <equality> ("&" <equality>)*
    return templ_binop(p, rule_equality, TOK_AMPERSAND, SENTINEL);
}

static AstNode *rule_bitwise_or(Parser *p) {
    // <bitwise-or> ::= <bitwise-and> ("|" <bitwise-and>)*
    return templ_binop(p, rule_bitwise_and, TOK_PIPE, SENTINEL);
}

static AstNode *rule_log_and(Parser *p) {
    // <log-and> ::= <bitwise-or> ("&&" <bitwise-or>)*
    return templ_binop(p, rule_bitwise_or, TOK_LOG_AND, SENTINEL);
}

static AstNode *rule_log_or(Parser *p) {
    // <log-or> ::= <log-and> ("||" <log-and>)*
    return templ_binop(p, rule_log_and, TOK_LOG_OR, SENTINEL);
}

static AstNode *rule_assign(Parser *p) {
    // <assign> ::= <log-or> "=" <assign> | <log-or>

    AstNode *expr = rule_log_or(p);

    if (!parser_match_token(p, TOK_ASSIGN))
        return expr;

    Token op = parser_advance(p);

    AstNode *value = rule_assign(p);

    AstNode *node     = parser_new_node(p);
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

    parser_consume(p, TOK_SEMICOLON);
    return node;
}

static AstNode *rule_vardecl(Parser *p) {
    // <vardecl> ::= "let" <identifier> ":" <type> ("=" <expression>)? ";"

    Token op = parser_consume(p, TOK_KW_VARDECL);

    Token ident = parser_consume(p, TOK_LITERAL_IDENT);
    parser_consume(p, TOK_COLON);

    Type type = rule_util_type(p);
    AstNode *value = NULL;

    if (!parser_match_token(p, TOK_SEMICOLON)) {
        parser_consume(p, TOK_ASSIGN);

        value = rule_expr(p);
    }

    parser_consume(p, TOK_SEMICOLON);

    AstNode *vardecl      = parser_new_node(p);
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

    Token brace = parser_consume(p, TOK_LBRACE);

    AstNode *node = parser_new_node(p);
    node->kind  = ASTNODE_BLOCK;
    node->block = (Block) { 0 };

    astnodelist_init(&node->block.stmts, p->arena);

    while (1) {

        // we cant put the call to parser_match_token() in the condition,
        // as it has to be checked after we've jumped
        // we also can't put the setjmp() call at the end of the loop
        // as that means, that the stack frame of rule_stmt() is already gone,
        // making setjmp() segfault
        setjmp(p->ctx_stmt);
        if (parser_match_token(p, TOK_RBRACE))
            break;

        if (parser_is_at_end(p)) {
            diagnostic_loc(DIAG_ERROR, &brace, "Unmatching brace. did you forget the closing brace?");
            // TODO: sync
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

    Token op = parser_consume(p, TOK_KW_WHILE);

    AstNode *cond  = rule_expr(p);
    AstNode *body  = rule_block(p);

    AstNode *node    = parser_new_node(p);
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

    Token op = parser_consume(p, TOK_KW_IF);

    AstNode *cond  = rule_expr(p);
    AstNode *then  = rule_block(p);
    AstNode *else_ = NULL;

    if (parser_match_token(p, TOK_KW_ELSE)) {
        parser_advance(p);
        else_ = rule_block(p);
    }

    AstNode *node = parser_new_node(p);
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

    Token op = parser_consume(p, TOK_KW_RETURN);

    AstNode *expr = parser_match_token(p, TOK_SEMICOLON)
        ? NULL
        : rule_expr(p);

    AstNode *node = arena_alloc(p->arena, sizeof(AstNode));
    node->kind = ASTNODE_RETURN;
    node->stmt_return = (StmtReturn) {
        .op   = op,
        .expr = expr,
    };

    parser_consume(p, TOK_SEMICOLON);

    return node;
}

// returns NULL on empty statement
static AstNode *rule_stmt(Parser *p) {
    // <statement> ::=
    //             | <block>
    //             | <vardecl>
    //             | <if>
    //             | <while>
    //             | <return>
    //             | <exprstmt>

    p->ctx = CONTEXT_STMT;

    return
        parser_match_token(p, TOK_LBRACE)     ? rule_block  (p) :
        parser_match_token(p, TOK_KW_VARDECL) ? rule_vardecl(p) :
        parser_match_token(p, TOK_KW_IF)      ? rule_if     (p) :
        parser_match_token(p, TOK_KW_WHILE)   ? rule_while  (p) :
        parser_match_token(p, TOK_KW_RETURN)  ? rule_return (p) :
    rule_exprstmt(p);
}

static AstNode *rule_proc(Parser *p) {
    // <procedure> ::= <proc-type> <block>?

    Token ident, op;
    Type ty = rule_util_proc_type(p, &ident, &op);

    AstNode *body = parser_match_token(p, TOK_SEMICOLON)
        ? parser_advance(p), NULL
        : rule_block(p);

    AstNode *proc = parser_new_node(p);
    proc->kind = ASTNODE_PROC;
    proc->stmt_proc = (DeclProc) {
        .op         = op,
        .body       = body,
        .ident      = ident,
        .type       = ty,
    };

    return proc;
}

static AstNode *rule_table(Parser *p) {
    // <table> ::= "table" IDENTIFIER <fieldlist>

    Token op = parser_consume(p, TOK_KW_TABLE);
    Token ident = parser_consume(p, TOK_LITERAL_IDENT);

    Table *table = arena_alloc(p->arena, sizeof(Table));
    rule_util_fieldlist(p, table);

    Type type = {
        .kind  = TYPE_TABLE,
        .table = table,
    };

    AstNode *node = parser_new_node(p);
    node->kind    = ASTNODE_TABLE;
    node->table   = (DeclTable) {
        .ident = ident,
        .op    = op,
        .type  = type,
    };

    return node;
}

static AstNode *rule_decl(Parser *p) {
    // <declaration> ::= <proc> | <vardecl>

    p->ctx = CONTEXT_DECL;

    return
        parser_match_token(p, TOK_KW_PROC)    ? rule_proc(p)    :
        parser_match_token(p, TOK_KW_TABLE)   ? rule_table(p)   :
    (diagnostic_loc(DIAG_ERROR, parser_peek(p), "Expected declaration"),
        parser_sync(p),
        NULL);
}

static AstNode *rule_program(Parser *p) {
    // <program> ::= <declaration>*

    AstNode *node = parser_new_node(p);
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) { 0 };

    astnodelist_init(&node->block.stmts, p->arena);

    while (!parser_is_at_end(p)) {
        setjmp(p->ctx_decl);
        astnodelist_append(&node->block.stmts, rule_decl(p));
    }

    return node;
}
