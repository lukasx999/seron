#ifndef _LEXER_H
#define _LEXER_H

#include <stdio.h>
#include <stddef.h>



typedef enum {
    TOK_INVALID, // Used only for error checking and as a sentinel value

    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,

    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTERISK,
    TOK_SLASH,

    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_COLON,
    TOK_ASSIGN,
    TOK_EQUALS,

    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,

    TOK_KW_FUNCTION,
    TOK_KW_VARDECL,
    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_WHILE,
    TOK_KW_ASM,

    TOKENKIND_COUNT,
} TokenKind;

extern const char *tokenkind_to_string(TokenKind tok);

typedef struct {
    TokenKind kind;
    char value[BUFSIZ]; // only holds value for literals, otherwise: ""
} Token;

typedef struct {
    size_t capacity, size;
    Token *items;
} TokenList;

extern TokenList    tokenlist_new    (void);
extern void         tokenlist_append (TokenList *tokens, Token item);
extern const Token *tokenlist_get    (const TokenList *tokens, size_t index);
extern void         tokenlist_print  (const TokenList *tokens);
extern void         tokenlist_destroy(TokenList *tokens);

extern TokenList tokenize(const char *src);



#endif // _LEXER_H
