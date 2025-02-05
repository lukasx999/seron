#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "grammar.h"
#include "types.h"



AstNodeList rule_util_arglist(Parser *p) {
    // <arglist> ::= "(" ( <expr> ("," <expr>)* )? ")"

    parser_expect_token(p, TOK_LPAREN, "(");
    parser_advance(p);

    AstNodeList args = astnodelist_new();

    while (!parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL)) {
        astnodelist_append(&args, rule_expression(p));

        if (parser_match_tokenkinds(p, TOK_COMMA, SENTINEL)) {
            parser_advance(p);

            if (parser_match_tokenkinds(p, TOK_RPAREN, SENTINEL))
                parser_throw_error(p,"Extraneous `,`");
        }

    }

    parser_expect_token(p, TOK_RPAREN, ")");
    parser_advance(p);

    return args;
}

Type rule_util_type(Parser *p) {
    // <type> ::= "'" TYPE

    parser_expect_token(p, TOK_TICK, "type annotation");
    parser_advance(p);

    Token type_tok = parser_get_current_token(p);
    Type type = type_from_tokenkind(type_tok.kind);

    if (type == TYPE_INVALID)
        throw_error(type_tok, "Unknown type `%s`", type_tok.value);

    parser_advance(p);
    return type;
}

AstNode *rule_primary(Parser *p) {
    // <primary> ::= NUMBER | IDENTIFIER | STRING | "(" <expression> ")"

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


AstNode *rule_call(Parser *p) {
    // <call> ::= (<primary> | "asm") "(" ( <expr> ("," <expr>)* )? ")"

    bool is_ident = parser_match_tokenkinds(p, TOK_IDENTIFIER, SENTINEL);
    AstNode *node = rule_primary(p);

    if (!parser_match_tokenkinds(p, TOK_LPAREN, SENTINEL))
        return node;

    BuiltinFunc builtin = BUILTINFUNC_NONE;
    if (is_ident) {
        const char *value = node->expr_literal.op.value;
        builtin = string_to_builtinfunc(value);
    }

    Token op = parser_get_current_token(p);

    AstNode *call   = malloc(sizeof(AstNode));
    call->kind      = ASTNODE_CALL;
    call->expr_call = (ExprCall) {
        .op      = op,
        .callee  = node,
        .args    = rule_util_arglist(p),
        .builtin = builtin,
    };

    return call;
}

AstNode *rule_unary(Parser *p) {
    // <unary> ::= ("!" | "-") <unary> | <call>

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

AstNode *rule_factor(Parser *p) {
    // <factor> ::= <unary> (("/" | "*") <unary>)*

    AstNode *lhs = rule_unary(p);

    while (parser_match_tokenkinds(p, TOK_SLASH, TOK_ASTERISK, SENTINEL)) {
        Token op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *rhs = rule_unary(p);

        AstNode *astnode    = malloc(sizeof(AstNode));
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

AstNode *rule_term(Parser *p) {
    // <term> ::= <factor> (("+" | "-") <factor>)*

    AstNode *lhs = rule_factor(p);

    while (parser_match_tokenkinds(p, TOK_PLUS, TOK_MINUS, SENTINEL)) {
        Token op = parser_get_current_token(p);
        parser_advance(p);

        AstNode *rhs = rule_factor(p);

        AstNode *astnode    = malloc(sizeof(AstNode));
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

AstNode *rule_vardecl(Parser *p) {
    // <vardecl> ::= "val" <identifier> <type> "=" <expression> ";"

    assert(parser_match_tokenkinds(p, TOK_KW_VARDECL, SENTINEL));
    Token op = parser_get_current_token(p);
    parser_advance(p);

    parser_expect_token(p, TOK_IDENTIFIER, "identifier");
    Token identifier = parser_get_current_token(p);
    parser_advance(p);

    Type type = rule_util_type(p);

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

AstNode *rule_while(Parser *p) {
    // <while> ::= "while" <expression> <block>

    assert(parser_match_tokenkinds(p, TOK_KW_WHILE, SENTINEL));
    Token op = parser_get_current_token(p);
    parser_advance(p);

    AstNode *cond  = rule_expression(p);
    AstNode *body  = rule_block(p);

    AstNode *node    = malloc(sizeof(AstNode));
    node->kind       = ASTNODE_WHILE;
    node->stmt_while = (StmtWhile) {
        .op        = op,
        .condition = cond,
        .body      = body,
    };

    return node;
}

AstNode *rule_if(Parser *p) {
    // <if> ::= "if" <expression> <block> ("else" <block>)?

    assert(parser_match_tokenkinds(p, TOK_KW_IF, SENTINEL));
    Token op = parser_get_current_token(p);
    parser_advance(p);

    AstNode *cond  = rule_expression(p);
    AstNode *then  = rule_block(p);
    AstNode *else_ = NULL;

    if (parser_match_tokenkinds(p, TOK_KW_ELSE, SENTINEL)) {
        parser_advance(p);
        else_ = rule_block(p);
    }

    AstNode *node = malloc(sizeof(AstNode));
    node->kind    = ASTNODE_IF;
    node->stmt_if = (StmtIf) {
        .op        = op,
        .condition = cond,
        .then_body = then,
        .else_body = else_,
    };

    return node;
}

AstNode *rule_function(Parser *p) {
    // <function> ::= "proc" IDENTIFIER "(" ")" <type> <block>

    // TODO: paramlist

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

    /* Returntype is void if not specified */
    Type returntype = parser_match_tokenkinds(p, TOK_LBRACE, SENTINEL)
        ? TYPE_VOID
        : rule_util_type(p);

    AstNode *body = rule_block(p);

    AstNode *function   = malloc(sizeof(AstNode));
    function->kind      = ASTNODE_FUNC;
    function->stmt_func = (StmtFunc) {
        .op         = op,
        .body       = body,
        .identifier = identifier,
        .returntype = returntype,
    };

    return function;
}

AstNode *rule_block(Parser *p) {
    // <block> ::= "{" <statement>* "}"

    parser_expect_token(p, TOK_LBRACE, "{");
    parser_advance(p);

    AstNode *node = malloc(sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts       = astnodelist_new(),
        .symboltable = NULL,
    };

    while (!parser_match_tokenkinds(p, TOK_RBRACE, SENTINEL)) {
        if (parser_is_at_end(p))
            throw_error_simple("Unmatching brace");

        astnodelist_append(&node->block.stmts, rule_stmt(p));
    }

    parser_advance(p);
    return node;
}

AstNode *rule_assignment(Parser *p) {
    // <assignment> ::= IDENTIFIER "=" <assignment> | <term>

    AstNode *expr = rule_term(p);

    if (parser_match_tokenkinds(p, TOK_ASSIGN, SENTINEL)) {
        Token op = parser_get_current_token(p);
        parser_advance(p);

        bool is_literal = expr->kind == ASTNODE_LITERAL;
        bool is_ident   = expr->expr_literal.op.kind == TOK_IDENTIFIER;
        if (!(is_literal && is_ident))
            parser_throw_error(p, "Invalid assignment target");

        /* AstNode is not needed anymore, since we know its an identifier */
        Token ident = expr->expr_literal.op;
        free(expr);

        AstNode *value = rule_assignment(p);

        AstNode *node     = malloc(sizeof(AstNode));
        node->kind        = ASTNODE_ASSIGN;
        node->expr_assign = (ExprAssignment) {
            .op         = op,
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
    parser_expect_token(p, TOK_SEMICOLON,";");

    parser_advance(p);
    return node;
}

AstNode *rule_stmt(Parser *p) {
    // <statement> ::= <block> | <function> | <vardeclaration> | <if> | <while> | <expr> ";"

    if (parser_match_tokenkinds(p, TOK_LBRACE, SENTINEL))
        return rule_block(p);

    else if (parser_match_tokenkinds(p, TOK_KW_FUNCTION, SENTINEL))
        return rule_function(p);

    else if (parser_match_tokenkinds(p, TOK_KW_VARDECL, SENTINEL))
        return rule_vardecl(p);

    else if (parser_match_tokenkinds(p, TOK_KW_IF, SENTINEL))
        return rule_if(p);

    else if (parser_match_tokenkinds(p, TOK_KW_WHILE, SENTINEL))
        return rule_while(p);

    else
        return rule_exprstmt(p);

}

AstNode *rule_program(Parser *p) {
    // <program> ::= <statement>*

    AstNode *node = malloc(sizeof(AstNode));
    node->kind    = ASTNODE_BLOCK;
    node->block   = (Block) {
        .stmts       = astnodelist_new(),
        .symboltable = NULL,
    };

    while (!parser_is_at_end(p))
        astnodelist_append(&node->block.stmts, rule_stmt(p));

    return node;
}
