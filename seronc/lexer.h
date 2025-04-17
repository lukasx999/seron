#ifndef _LEXER_H
#define _LEXER_H

#include <stdio.h>
#include <stddef.h>
#include <stddef.h>
#include <stdbool.h>



typedef enum {
    TOK_INVALID, // Used only for error checking and as a sentinel value

    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,

    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTERISK,
    TOK_SLASH,
    TOK_BANG,

    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_COLON,
    TOK_ASSIGN,
    TOK_EQUALS,
    TOK_AMPERSAND,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,

    TOK_KW_PROC,
    TOK_KW_VARDECL,
    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_ELSIF,
    TOK_KW_WHILE,
    TOK_KW_RETURN,

    TOK_BUILTIN_ASM,

    TOK_TYPE_VOID,
    TOK_TYPE_CHAR,
    TOK_TYPE_INT,
    TOK_TYPE_LONG,

    TOK_EOF,

    TOKENKIND_COUNT,
} TokenKind;

const char *tokenkind_to_string(TokenKind tok);

// TODO: given the source string, calculate column position lazily based on start position
typedef struct {
    TokenKind kind;
    char value[BUFSIZ]; // only holds value for literals, otherwise: ""
    int pos_line, pos_column;
    size_t pos_absolute, length;
} Token;

typedef struct {
    const char *src;
} LexerState;

Token lexer_next(LexerState *s);

// these functions are very performance heavy, and are only used for
// debugging and testing purposes, the actual parser should be streaming
// the tokens from lexer_next()
Token *lexer_collect_tokens(const char *src); // lexer unit tests
void lexer_print_tokens(const char *src);     // lexer debugging



#endif // _LEXER_H
