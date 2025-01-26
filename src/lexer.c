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
        [TOK_TICK]        = "tick",
        [TOK_IDENTIFIER]  = "identifier",
        [TOK_ASSIGN]      = "assign",
        [TOK_EQUALS]      = "equals",
        [TOK_BANG]        = "bang",
        [TOK_LPAREN]      = "lparen",
        [TOK_RPAREN]      = "rparen",
        [TOK_LBRACE]      = "lbrace",
        [TOK_RBRACE]      = "rbrace",
        [TOK_KW_FUNCTION] = "func",
        [TOK_KW_VARDECL]  = "val",
        [TOK_KW_IF]       = "if",
        [TOK_KW_ELSE]     = "else",
        [TOK_KW_WHILE]    = "while",
        [TOK_KW_RETURN]   = "return",
        [TOK_KW_ASM]      = "asm",
        [TOK_TYPE_CHAR]   = "char",
        [TOK_TYPE_INT]    = "int",
        [TOK_EOF]         = "eof",
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

        printf("%lu:%lu ", tok.pos_line, tok.pos_column);

        printf("%s", kind);
        if (strcmp(tok.value, ""))
            printf("(%s)", tok.value);

        printf("\n");
    }
    puts("");
}

void tokenlist_destroy(TokenList *tokens) {
    free(tokens->items);
    tokens->items = NULL;
}






// returns the tokenkind to the corresponding string
// returns TOK_INVALID if no string was matched
// len is needed, as str could possibly not be nullbyte terminated
static TokenKind match_keywords(const char *str, size_t len) {

    TokenKind tokenkinds[] = {
        TOK_KW_FUNCTION,
        TOK_KW_VARDECL,
        TOK_KW_IF,
        TOK_KW_ELSE,
        TOK_KW_WHILE,
        TOK_KW_RETURN,
        TOK_KW_ASM,

        TOK_TYPE_CHAR,
        TOK_TYPE_SHORT,
        TOK_TYPE_INT,
        TOK_TYPE_SIZE,
    };

    const char *strings[] = {
        "proc",
        "val",
        "if",
        "else",
        "while",
        "return",
        "asm",

        "char",
        "short",
        "int",
        "size",
    };

    assert(ARRAY_LEN(tokenkinds) == ARRAY_LEN(strings));

    for (size_t i=0; i < ARRAY_LEN(tokenkinds); ++i) {
        const char *s = strings[i];
        if (!strncmp(str, s, len) && len == strlen(s))
            return tokenkinds[i];
    }

    return TOK_INVALID;
}

bool tokenkind_is_type(TokenKind kind) {

    TokenKind types[] = {
        TOK_TYPE_CHAR,
        TOK_TYPE_SHORT,
        TOK_TYPE_INT,
        TOK_TYPE_SIZE,
    };

    for (size_t i=0; i < ARRAY_LEN(types); ++i)
        if (kind == types[i])
            return true;

    return false;
}

static Token token_new(void) {
    return (Token) {
        .kind       = TOK_INVALID,
        .value      = { 0 },
        .pos_column = 0,
        .pos_line   = 0,
        .length     = 0,
    };
}

TokenList tokenize(const char *src) {
    TokenList tokens = tokenlist_new();
    int linecount   = 1;
    int columncount = 1;

    for (size_t i=0; i < strlen(src); ++i) {
        char c    = src[i];
        Token tok = token_new();

        tok.pos_line     = linecount;
        tok.pos_column   = columncount;
        tok.pos_absolute = i;
        tok.length       = 1;

        switch (c) {
            case '\n':
                linecount++;
                columncount = 1;
                continue;
                break;
            case '\t':
            case '\r':
            case ' ':
                columncount++;
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
            case '!':
                tok.kind = TOK_BANG;
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

                        if (c == '\n') {
                            linecount++;
                            columncount = 1;
                        } else
                            columncount++;
                    }
                    ++i;
                    columncount += 2;
                } else { // inline comments
                    while ((c = src[++i]) != '\n' && c != '\0');
                    linecount++;
                    columncount = 1;
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
                if (src[i+1] == '=') {
                    tok.kind = TOK_EQUALS;
                    ++i;
                    ++columncount;
                    tok.length = 2;
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
            case '\'':
                tok.kind = TOK_TICK;
                break;
            case '\"': {
                tok.kind = TOK_STRING;

                size_t start = i + 1;
                while (true) {
                    c = src[++i];

                    if (c == '\"' || c == '\0')
                        break;

                    if (c == '\n') {
                        linecount++;
                        columncount = 1;
                    } else {
                        columncount++;
                    }

                }

                if (c == '\0')
                    throw_error("Unterminated string literal");

                size_t len = i - start;
                tok.length = len;
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
                columncount += len - 1;
                tok.length = len;

                TokenKind kw = match_keywords(src+start, len);
                if (kw == TOK_INVALID) // not a keyword, must be an identifier
                    strncpy(tok.value, &src[start], len);
                else
                    tok.kind = kw;

            } break;
        }

        assert(tok.kind != TOK_INVALID);
        tokenlist_append(&tokens, tok);

        columncount++;

    }

    Token tok_eof = token_new();
    tok_eof.kind  = TOK_EOF;
    tokenlist_append(&tokens, tok_eof);

    return tokens;
}
