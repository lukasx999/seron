#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"




const char *tokenkind_to_string(TokenKind tok) {
    const char *repr[] = {
        [TOK_INVALID]     = "invalid",
        [TOK_NUMBER]      = "number",
        [TOK_STRING]      = "string",
        [TOK_PLUS]        = "plus",
        [TOK_MINUS]       = "minus",
        [TOK_ASTERISK]    = "asterisk",
        [TOK_SLASH]       = "slash",
        [TOK_SEMICOLON]   = "semicolon",
        [TOK_COMMA]       = "comma",
        [TOK_COLON]       = "colon",
        [TOK_IDENTIFIER]  = "identifier",
        [TOK_ASSIGN]      = "assign",
        [TOK_EQUALS]      = "equals",
        [TOK_LPAREN]      = "lparen",
        [TOK_RPAREN]      = "rparen",
        [TOK_LBRACE]      = "lbrace",
        [TOK_RBRACE]      = "rbrace",
        [TOK_KW_FUNCTION] = "func",
        [TOK_KW_VARDECL]  = "val",
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






// returns the tokenkind to the corresponding keyword for str
// returns TOK_INVALID if no keyword was matched
// len is needed, as str must not be nullbyte terminated
static TokenKind match_keywords(const char *str, size_t len) {

    int keywords_tokens[] = {
        TOK_KW_FUNCTION,
        TOK_KW_VARDECL,
        TOK_KW_IF,
        TOK_KW_ELSE,
        TOK_KW_WHILE,
    };

    const char *keywords[] = {
        "proc",
        "val",
        "if",
        "else",
        "while",
    };

    assert(ARRAY_LEN(keywords_tokens) == ARRAY_LEN(keywords));

    for (size_t i=0; i < ARRAY_LEN(keywords); ++i)
        if (!strncmp(str, keywords[i], len))
            return keywords_tokens[i];

    return TOK_INVALID;
}


TokenList tokenize(const char *src) {
    TokenList tokens = tokenlist_new();

    for (size_t i=0; i < strlen(src); ++i) {
        char c    = src[i];
        Token tok = {
            .kind  = TOK_INVALID,
            .value = { 0 }
        };

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
            case '#':
                if (src[i+1] == '#') { // multi line comments
                    ++i;
                    while (true) {
                        c = src[++i];
                        if (c == '\0')
                            throw_error("Unterminated multi-line comment\n");
                        if (c == '#' && src[i+1] == '#')
                            break;
                    }
                    ++i;
                } else { // inline comments
                    while ((c = src[++i]) != '\n' && c != '\0');
                }
                continue;
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
                // TODO: refactor
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
            case ':':
                tok.kind = TOK_COLON;
                break;
            case '\"': {
                tok.kind = TOK_STRING;

                size_t start = i + 1;
                while ((c = src[++i]) != '\"' && c != '\0');
                if (c == '\0')
                    throw_error("Unterminated string literal");

                size_t len = i - start;
                strncpy(tok.value, &src[start], len);
            } break;

            default: {
                size_t start = i;

                if (isdigit(c)) {
                    tok.kind = TOK_NUMBER;
                    while (isdigit((c = src[++i])));

                } else if (isalpha(c) || c == '_') {
                    tok.kind = TOK_IDENTIFIER;
                    while (isalpha(c) || c == '_' || isdigit(c))
                        c = src[++i];

                } else
                    throw_error("Unknown token: `%c`\n", c);

                --i; // move back, as i gets incremented by the for loop
                size_t len = i - start + 1;

                TokenKind kw = match_keywords(src+start, len);
                if (kw == TOK_INVALID) // not a keyword, must be an identifier
                    strncpy(tok.value, &src[start], len);
                else
                    tok.kind = kw;

            } break;
        }

        assert(tok.kind != TOK_INVALID);
        tokenlist_append(&tokens, tok);

    }

    return tokens;
}
