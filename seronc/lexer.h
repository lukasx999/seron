#ifndef _LEXER_H
#define _LEXER_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_IDENT_LEN 64
#define MAX_NUMBER_LITERAL_LEN 64

typedef enum {
    TOK_INVALID, // used only for error checking and as a sentinel value

    TOK_LITERAL_IDENT,
    TOK_LITERAL_NUMBER,
    TOK_LITERAL_STRING,

    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTERISK,
    TOK_SLASH,
    TOK_BANG,

    TOK_SEMICOLON,
    TOK_COLON,
    TOK_COMMA,
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
    TOK_KW_TABLE,

    TOK_KW_TYPE_VOID,
    TOK_KW_TYPE_CHAR,
    TOK_KW_TYPE_INT,
    TOK_KW_TYPE_LONG,

    TOK_EOF,

    TOKENKIND_COUNT,
} TokenKind;

const char *tokenkind_to_string(TokenKind tok);

typedef enum {
    NUMBER_LONG,
    NUMBER_INT,
    NUMBER_CHAR,
    NUMBER_ANY,
} NumberLiteralType;

typedef struct {
    TokenKind kind;
    // TODO: memory usage?
    char value[BUFSIZ]; // for strings and identifiers
    NumberLiteralType number_type;
    uint64_t number;    // for all kinds of numbers

    int pos_line, pos_column;
    size_t pos_absolute, length;
} Token;

typedef struct {
    const char *src;
} Lexer;

void lexer_init(Lexer *state, const char *src);
Token lexer_next(Lexer *s);

// these functions are very performance heavy, and are only used for
// debugging and testing purposes, the actual parser should be streaming
// the tokens from lexer_next()
Token *lexer_collect_tokens(const char *src); // lexer unit tests
void lexer_print_tokens(const char *src);     // lexer debugging



#endif // _LEXER_H
