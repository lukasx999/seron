#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "./util.h"
#include "./lexer.h"




static const char *tokenkind_to_string(TokenKind tok) {
    const char *repr[] = {
        [TOK_INVALID]     = "invalid",
        [TOK_NUMBER]      = "number",
        [TOK_PLUS]        = "plus",
        [TOK_MINUS]       = "minus",
        [TOK_ASTERISK]    = "asterisk",
        [TOK_SLASH]       = "slash",
        [TOK_SEMICOLON]   = "semicolon",
        [TOK_COMMA]       = "comma",
        [TOK_IDENTIFIER]  = "identifier",
        [TOK_ASSIGN]      = "assign",
        [TOK_EQUALS]      = "equals",
        [TOK_LPAREN]      = "lparen",
        [TOK_RPAREN]      = "rparen",
        [TOK_LBRACE]      = "lbrace",
        [TOK_RBRACE]      = "rbrace",
        [TOK_KW_FUNCTION] = "function",
        [TOK_KW_IF]       = "if",
        [TOK_KW_ELSE]     = "else",
        [TOK_KW_WHILE]    = "while",
    };
    assert(ARRAY_LEN(repr) == TOKENKIND_COUNT);
    return repr[tok];
}




TokenList tokenlist_new(void) {
    TokenList tokens = {
        .capacity = 5,
        .size     = 0,
        .items    = NULL,
    };
    tokens.items = malloc(tokens.capacity * sizeof(Token));
    return tokens;
}

void tokenlist_append(TokenList *tokens, Token item) {
    if (tokens->size == tokens->capacity) {
        tokens->capacity *= 2;
        tokens->items = realloc(tokens->items, tokens->capacity * sizeof(Token));
    }
    tokens->items[tokens->size++] = item;
}

const Token *tokenlist_get(const TokenList *tokens, size_t index) {
    return index < tokens->size ? &tokens->items[index] : NULL;
}

void tokenlist_print(const TokenList *tokens) {
    for (size_t i=0; i < tokens->size; ++i) {
        Token tok = tokens->items[i];
        const char *kind = tokenkind_to_string(tok.kind);

        printf("%s", kind);
        if (strcmp(tok.value, ""))
            printf("(%s)", tok.value);
        printf("   ");
    }
    puts("");
}

void tokenlist_destroy(TokenList *tokens) {
    free(tokens->items);
    tokens->items = NULL;
}




TokenList tokenize(const char *src) {
    TokenList tokens = tokenlist_new();

    for (size_t i=0; i < strlen(src); ++i) {
        char c = src[i];
        Token tok = { .kind = TOK_INVALID, .value = { 0 } };

        switch (c) {
            case '\n':
            case '\t':
            case '\r':
            case ' ':
                continue;
                break;
            case '+':
                tok.kind = TOK_PLUS;
                break;
            case '-':
                tok.kind = TOK_MINUS;
                break;
            case '*':
                tok.kind = TOK_ASTERISK;
                break;
            case '/':
                tok.kind = TOK_SLASH;
                break;
            case '(':
                tok.kind = TOK_LPAREN;
                break;
            case ')':
                tok.kind = TOK_RPAREN;
                break;
            case '{':
                tok.kind = TOK_LBRACE;
                break;
            case '}':
                tok.kind = TOK_RBRACE;
                break;
            case '=':
                if (src[i+1] == '=') {
                    tok.kind = TOK_EQUALS;
                    ++i;
                } else
                    tok.kind = TOK_ASSIGN;
                break;
            case ';':
                tok.kind = TOK_SEMICOLON;
                break;
            case ',':
                tok.kind = TOK_COMMA;
                break;

            default: {
                size_t start = i;

                if (isdigit(c)) {
                    tok.kind = TOK_NUMBER;
                    while (isdigit((c = src[++i])));

                } else if (isalpha(c) || c == '_') {
                    tok.kind = TOK_IDENTIFIER;
                    while (isalpha(c) || c == '_' || isdigit(c))
                        c = src[++i];

                } else {
                    // TODO: centralized error handling
                    fprintf(stderr, "ERROR: Unknown token: `%c`\n", c);
                    exit(EXIT_FAILURE);
                }

                --i; // move back, as i gets incremented by the for loop
                size_t len = i - start + 1;

                if (!strncmp(src+start, "proc", len)) {
                    tok.kind = TOK_KW_FUNCTION;

                } else if (!strncmp(src+start, "if", len)) {
                    tok.kind = TOK_KW_IF;

                } else if (!strncmp(src+start, "else", len)) {
                    tok.kind = TOK_KW_ELSE;

                } else if (!strncmp(src+start, "while", len)) {
                    tok.kind = TOK_KW_WHILE;

                } else {
                    strncpy(tok.value, &src[start], len);
                }

            } break;
        }

        assert(tok.kind != TOK_INVALID);
        tokenlist_append(&tokens, tok);

    }

    return tokens;

}
