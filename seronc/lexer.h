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
    TOK_INVALID, // invalid token, should never appear, but can be used for enforcing invariants
    TOK_SENTINEL, // a sentinel value, used by the parser
    TOK_EOF, // end-of-file, should only appear once at the end of the tokenstream

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
    TOK_NOT_EQUALS,
    TOK_AMPERSAND,
    TOK_GREATER_THAN,
    TOK_GREATER_THAN_EQ,
    TOK_LESS_THAN,
    TOK_LESS_THAN_EQ,

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

    TOKENKIND_COUNT,
} TokenKind;

const char *stringify_tokenkind(TokenKind tok);

// TODO: change to `Type`?
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

    size_t position;
    size_t len;
} Token;

// required for diagnostics
typedef struct {
    int line;
    int column;
    int start; // index of the start of the line the token is located on
} TokenLocation;

// expensive computation, use lazily
TokenLocation get_token_location(const Token *tok, const char *src);

typedef struct {
    const char *src;
    size_t position;
} Lexer;

void lexer_init(Lexer *state, const char *src);
Token lexer_next(Lexer *s);

// these functions are very performance heavy, and are only used for
// debugging and testing purposes, the actual parser should be streaming
// the tokens from lexer_next()
Token *lexer_collect_tokens(const char *src); // lexer unit tests
void lexer_print_tokens(const char *src);     // lexer debugging



#endif // _LEXER_H
