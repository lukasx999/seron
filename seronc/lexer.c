#define _DEFAULT_SOURCE // isascii()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#include <ver.h>

#include "diagnostics.h"
#include "lexer.h"
#include "colors.h"

#define LITERAL_SUFFIX_LONG 'L'
#define LITERAL_SUFFIX_CHAR 'C'
#define LITERAL_SUFFIX_INT  'I'

const char *stringify_tokenkind(TokenKind tok) {
    const char *repr[] = {
        [TOK_INVALID]        = "invalid",
        [TOK_LITERAL_NUMBER] = "num",
        [TOK_LITERAL_STRING] = "string",
        [TOK_PLUS]           = "plus",
        [TOK_MINUS]          = "minus",
        [TOK_ASTERISK]       = "asterisk",
        [TOK_SLASH]          = "slash",
        [TOK_SEMICOLON]      = "semicolon",
        [TOK_COMMA]          = "comma",
        [TOK_COLON]          = "colon",
        [TOK_LITERAL_IDENT]  = "ident",
        [TOK_ASSIGN]         = "assign",
        [TOK_EQUALS]         = "equals",
        [TOK_BANG]           = "bang",
        [TOK_AMPERSAND]      = "ampersand",
        [TOK_LPAREN]         = "lparen",
        [TOK_RPAREN]         = "rparen",
        [TOK_LBRACE]         = "lbrace",
        [TOK_RBRACE]         = "rbrace",
        [TOK_KW_PROC]        = "proc",
        [TOK_KW_VARDECL]     = "let",
        [TOK_KW_IF]          = "if",
        [TOK_KW_ELSE]        = "else",
        [TOK_KW_ELSIF]       = "elsif",
        [TOK_KW_WHILE]       = "while",
        [TOK_KW_RETURN]      = "return",
        [TOK_KW_TYPE_VOID]   = "void",
        [TOK_KW_TYPE_CHAR]   = "char",
        [TOK_KW_TYPE_INT]    = "int",
        [TOK_KW_TYPE_LONG]   = "long",
        [TOK_EOF]            = "eof",
    };
    assert(ARRAY_LEN(repr) == TOKENKIND_COUNT);
    return repr[tok];
}


// identifier may not have leading digits
static inline bool is_ident_head(char c) {
    return isalpha(c) || c == '_';
}

static inline bool is_ident_tail(char c) {
    return is_ident_head(c) || isdigit(c);
}

static inline int match_kw(const char *str, const char *kw) {
    return !strncmp(str, kw, strlen(kw));
}

// str is a slice into the source string, and is therefore not NUL-terminated
// therefore must not compare more chars than the length of the keyword
static TokenKind get_keyword(const char *str) {
    return

    match_kw(str, "proc")   ? TOK_KW_PROC        :
    match_kw(str, "let")    ? TOK_KW_VARDECL     :
    match_kw(str, "if")     ? TOK_KW_IF          :
    match_kw(str, "else")   ? TOK_KW_ELSE        :
    match_kw(str, "elsif")  ? TOK_KW_ELSIF       :
    match_kw(str, "while")  ? TOK_KW_WHILE       :
    match_kw(str, "table")  ? TOK_KW_TABLE       :
    match_kw(str, "return") ? TOK_KW_RETURN      :
    match_kw(str, "void")   ? TOK_KW_TYPE_VOID   :
    match_kw(str, "char")   ? TOK_KW_TYPE_CHAR   :
    match_kw(str, "int")    ? TOK_KW_TYPE_INT    :
    match_kw(str, "long")   ? TOK_KW_TYPE_LONG   :

    TOK_LITERAL_IDENT; // no keyword found? must be an identifier!
}

static void tokenize_string(Lexer *s, Token *tok) {

    tok->kind = TOK_LITERAL_STRING;
    const char *start = s->src + 1;

    while (*++s->src != '"') {
        if (*s->src == '\0') {
            diagnostic(DIAG_ERROR, "unterminated string literal: `%s`", start);
            exit(EXIT_FAILURE);
        }
    }

    size_t len = s->src - start;
    tok->len = len + 2; // account for quotes surrounding the string
    strncpy(tok->value, start, MIN(len, ARRAY_LEN(tok->value)));
    s->src++; // make src point to the next char, instead of `"`

}

static void tokenize_char(Lexer *lex, Token *tok) {

    tok->kind = TOK_LITERAL_NUMBER;
    tok->number_type = NUMBER_CHAR;
    lex->src++;

    // TODO: escape codes
    char c = *lex->src++;

    if (!isascii(c)) {
        diagnostic(DIAG_ERROR, "invalid character literal");
        exit(EXIT_FAILURE);
    }

    // chars are converted to numbers
    tok->number = c;
    tok->len = 3; // account for quotes

    if (*lex->src++ != '\'') {
        diagnostic(DIAG_ERROR, "unterminated character literal");
        exit(EXIT_FAILURE);
    }

}

static void tokenize_number(Lexer *lex, Token *tok) {

    tok->kind = TOK_LITERAL_NUMBER;
    tok->number_type = NUMBER_ANY;
    const char *start = lex->src;

    while (isdigit(*++lex->src));

    switch (*lex->src) {

        case LITERAL_SUFFIX_LONG:
            tok->number_type = NUMBER_LONG;
            lex->src++;
            break;

        case LITERAL_SUFFIX_CHAR:
            tok->number_type = NUMBER_CHAR;
            lex->src++;
            break;

        case LITERAL_SUFFIX_INT:
            tok->number_type = NUMBER_INT;
            lex->src++;
            break;

        default: break;
    }

    tok->len = lex->src - start;

    if (tok->len > MAX_NUMBER_LITERAL_LEN) {
        diagnostic_loc(
            DIAG_ERROR,
            tok,
            "Integer literals may not be longer than %d characters. This one has: %d",
            MAX_NUMBER_LITERAL_LEN,
            tok->len
        );
        exit(EXIT_FAILURE);
    }

    char buf[MAX_NUMBER_LITERAL_LEN] = { 0 };
    strncpy(buf, start, tok->len);

    tok->number = atoll(buf);
    // TODO: proper error handling

}

static void tokenize_ident(Lexer *lex, Token *tok) {

    const char *start = lex->src;
    while (is_ident_tail(*++lex->src));

    tok->len = lex->src - start;

    if (tok->len > MAX_IDENT_LEN) {
        diagnostic_loc(
            DIAG_ERROR,
            tok,
            "Identifiers may not be longer than %d characters. This one has: %d",
            MAX_IDENT_LEN,
            tok->len
        );
        exit(EXIT_FAILURE);
    }

    if ((tok->kind = get_keyword(start)) == TOK_LITERAL_IDENT) {
        strncpy(tok->value, start, tok->len);
    }
}

void lexer_init(Lexer *lex, const char *src) {
    lex->src = src;
    lex->position = 0;
}

Token lexer_next(Lexer *lex) {

    Token tok = {
        .kind     = TOK_INVALID,
        .position = lex->position,
        .len      = 1,
    };

    if (*lex->src == '\0') {
        tok.kind = TOK_EOF;
        return tok;
    }

    // TODO: multi-line comments

    switch (*lex->src) {

        case '\t':
        case '\r':
        case ' ':
        case '\n':
            lex->src++;
            lex->position++;
            return lexer_next(lex);
            break;

        case '#':
            while (*++lex->src != '\n');
            lex->src++;
            return lexer_next(lex);
            break;

        case '+': tok.kind = TOK_PLUS;
            lex->src++;
            break;

        case '-':
            tok.kind = TOK_MINUS;
            lex->src++;
            break;

        case '*':
            tok.kind = TOK_ASTERISK;
            lex->src++;
            break;

        case '/':
            tok.kind = TOK_SLASH;
            lex->src++;
            break;

        case '!':
            tok.kind = TOK_BANG;
            lex->src++;
            break;

        case '&':
            tok.kind = TOK_AMPERSAND;
            lex->src++;
            break;

        case '(':
            tok.kind = TOK_LPAREN;
            lex->src++;
            break;

        case ')':
            tok.kind = TOK_RPAREN;
            lex->src++;
            break;

        case '{':
            tok.kind = TOK_LBRACE;
            lex->src++;
            break;

        case '}':
            tok.kind = TOK_RBRACE;
            lex->src++;
            break;

        case ';':
            tok.kind = TOK_SEMICOLON;
            lex->src++;
            break;

        case ',':
            tok.kind = TOK_COMMA;
            lex->src++;
            break;

        case ':':
            tok.kind = TOK_COLON;
            lex->src++;
            break;

        case '=':
            tok.kind = TOK_ASSIGN;
            if (*++lex->src == '=') {
                tok.kind = TOK_EQUALS;
                tok.len = 2;
                lex->src++;
            }
            break;

        case '\'':
            tokenize_char(lex, &tok);
            break;

        case '"':
            tokenize_string(lex, &tok);
            break;

        default: {

            if (isdigit(*lex->src)) {
                tokenize_number(lex, &tok);

            } else if (is_ident_head(*lex->src)) {
                tokenize_ident(lex, &tok);

            } else {
                diagnostic(DIAG_ERROR, "unknown token `%c`", *lex->src);
                exit(EXIT_FAILURE);
            }


        } break;

    }

    lex->position += tok.len;

    assert(tok.kind != TOK_INVALID);
    return tok;

}

TokenLocation get_token_location(const Token *tok, const char *src) {

    int linecount = 1;
    int column = 1;
    int start = 0;
    // TODO: dont recompute src len every single call
    for (size_t i=0; i < strlen(src); ++i) {


        if (i == tok->position)
            break;

        column++;

        if (src[i] == '\n') {
            linecount++;
            start = i+1;
            column = 1;
        }

    }

    return (TokenLocation) {
        .line   = linecount,
        .column = column,
        .start  = start,
    };

}

void lexer_print_tokens(const char *src) {

    Token *tokens = lexer_collect_tokens(src);
    Token *tok = tokens;

    printf("\n");

    while (1) {
        const char *kind = stringify_tokenkind(tok->kind);

        TokenLocation loc = get_token_location(tok, src);
        printf(
            "pos: %lu, len: %lu, line: %d, col: %d | ",
            tok->position,
            tok->len,
            loc.line,
            loc.column
        );

        printf("%s", kind);
        if (strcmp(tok->value, ""))
            printf("%s(%s)%s", COLOR_GRAY, tok->value, COLOR_END);

        printf("\n");
        if (tok++->kind == TOK_EOF) break;
    }

    printf("\n");
    free(tokens);
}

static size_t get_tokencount(const char *src) {
    Lexer s = { .src = src };
    size_t tokencount = 0;

    Token tok;
    while (tok.kind != TOK_EOF) {
        tok = lexer_next(&s);
        tokencount++;
    }

    return tokencount;
}

Token *lexer_collect_tokens(const char *src) {

    // precompute the amount of tokens there are, so we don't have to bother
    // with dynamic arrays (this is for debugging/testing purposes so perf
    // doesn't matter
    size_t count = get_tokencount(src);
    Token *tokens = malloc(count * sizeof(Token));
    size_t i = 0;

    Lexer s = { .src = src };
    Token tok;
    while (tok.kind != TOK_EOF) {
        tok = lexer_next(&s);
        tokens[i++] = tok;
    }

    return tokens;

}
