#ifndef _LEXER_H
#define _LEXER_H

#include <stdio.h>
#include <stddef.h>



typedef enum {
    TOK_INVALID,
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTERISK,
    TOK_SLASH,
    TOK_SEMICOLON,
    TOK_IDENTIFIER,
    TOK_ASSIGN,
    TOKENKIND_COUNT,
} TokenKind;

typedef struct {
    TokenKind kind;
    char value[BUFSIZ]; // only holds value for literals, otherwise: ""
} Token;

typedef struct {
    size_t capacity;
    size_t size;
    Token *items;
} TokenList;

extern TokenList    tokenlist_new    (void);
extern void         tokenlist_append (TokenList *tokens, Token item);
extern const Token *tokenlist_get    (const TokenList *tokens, size_t index);
extern void         tokenlist_print  (const TokenList *tokens);
extern void         tokenlist_destroy(TokenList *tokens);

extern TokenList tokenize(const char *src);



#endif // _LEXER_H
