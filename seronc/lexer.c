#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "lib/util.h"



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
        [TOK_BANG]        = "bang",
        [TOK_AMPERSAND]   = "ampersand",
        [TOK_LPAREN]      = "lparen",
        [TOK_RPAREN]      = "rparen",
        [TOK_LBRACE]      = "lbrace",
        [TOK_RBRACE]      = "rbrace",
        [TOK_KW_FUNCTION] = "func",
        [TOK_KW_VARDECL]  = "let",
        [TOK_KW_IF]       = "if",
        [TOK_KW_ELSE]     = "else",
        [TOK_KW_ELSIF]    = "elsif",
        [TOK_KW_WHILE]    = "while",
        [TOK_KW_RETURN]   = "return",
        [TOK_BUILTIN_ASM] = "asm",
        [TOK_TYPE_VOID]   = "void",
        [TOK_TYPE_CHAR]   = "char",
        [TOK_TYPE_INT]    = "int",
        [TOK_EOF]         = "eof",
    };
    assert(ARRAY_LEN(repr) == TOKENKIND_COUNT);
    return repr[tok];
}

static inline int match_kw(const char *str, const char *kw) {
    return !strncmp(str, kw, strlen(kw));
}

// str is a slice into the source string, and is therefore not NUL-terminated
// therefore must not compare more chars than the length of the keyword
static TokenKind get_kw(const char *str) {
    return

    match_kw(str, "proc")   ? TOK_KW_FUNCTION :
    match_kw(str, "let")    ? TOK_KW_VARDECL  :
    match_kw(str, "if")     ? TOK_KW_IF       :
    match_kw(str, "else")   ? TOK_KW_ELSE     :
    match_kw(str, "elsif")  ? TOK_KW_ELSIF    :
    match_kw(str, "while")  ? TOK_KW_WHILE    :
    match_kw(str, "return") ? TOK_KW_RETURN   :
    match_kw(str, "asm")    ? TOK_BUILTIN_ASM :
    match_kw(str, "void")   ? TOK_TYPE_VOID   :
    match_kw(str, "char")   ? TOK_TYPE_CHAR   :
    match_kw(str, "int")    ? TOK_TYPE_INT    :

    TOK_IDENTIFIER; // no keyword found? must be an identifier!
}

static inline void copy_slice_to_buf(char *buf, const char *start, const char *end) {
    strncpy(buf, start, (size_t) (end - start));
}

Token lexer_next(LexerState *s) {

    Token tok = { .kind = TOK_INVALID };

    if (*s->src == '\0') {
        tok.kind = TOK_EOF;
        return tok;
    }

    // TODO: multi-line comments

    switch (*s->src) {

        case '\t':
        case '\r':
        case ' ':
        case '\n':
            s->src++;
            return lexer_next(s);
            break;

        case '#':
            while (*++s->src != '\n');
            s->src++;
            return lexer_next(s);
            break;

        case '+': tok.kind = TOK_PLUS;
            s->src++;
            break;

        case '-':
            tok.kind = TOK_MINUS;
            s->src++;
            break;

        case '*':
            tok.kind = TOK_ASTERISK;
            s->src++;
            break;

        case '/':
            tok.kind = TOK_SLASH;
            s->src++;
            break;

        case '!':
            tok.kind = TOK_BANG;
            s->src++;
            break;

        case '&':
            tok.kind = TOK_AMPERSAND;
            s->src++;
            break;

        case '(':
            tok.kind = TOK_LPAREN;
            s->src++;
            break;

        case ')':
            tok.kind = TOK_RPAREN;
            s->src++;
            break;

        case '{':
            tok.kind = TOK_LBRACE;
            s->src++;
            break;

        case '}':
            tok.kind = TOK_RBRACE;
            s->src++;
            break;

        case ';':
            tok.kind = TOK_SEMICOLON;
            s->src++;
            break;

        case ',':
            tok.kind = TOK_COMMA;
            s->src++;
            break;

        case ':':
            tok.kind = TOK_COLON;
            s->src++;
            break;

        case '=':
            tok.kind = TOK_ASSIGN;
            if (*++s->src == '=') {
                tok.kind = TOK_EQUALS;
                s->src++;
            }
            break;

        case '"':
            tok.kind = TOK_STRING;
            const char *start = s->src + 1;

            while (*++s->src != '"') {
                if (*s->src == '\0') {
                    compiler_message(MSG_ERROR, "unterminated string literal: `%s`", start);
                    exit(EXIT_FAILURE);
                }
            }

            copy_slice_to_buf(tok.value, start, s->src);
            s->src++; // make src point to the next char, instead of `"`
            break;

        default: {

            if (isdigit(*s->src)) {

                tok.kind = TOK_NUMBER;
                const char *start = s->src;
                while (isdigit(*++s->src));
                copy_slice_to_buf(tok.value, start, s->src);

            } else if (isalpha(*s->src) || *s->src == '_') {

                const char *start = s->src;
                while (isalpha(*++s->src) || *s->src == '_' || isdigit(*s->src));

                if ((tok.kind = get_kw(start)) == TOK_IDENTIFIER)
                    copy_slice_to_buf(tok.value, start, s->src);


            } else {
                compiler_message(MSG_ERROR, "unknown token `%c`", *s->src);
                exit(EXIT_FAILURE);
            }


        } break;

    }

    assert(tok.kind != TOK_INVALID);
    return tok;

}

void lexer_print_tokens(const char *src) {

    Token *tokens = lexer_collect_tokens(src);
    Token *tok = tokens;

    printf("\n");

    while (1) {
        const char *kind = tokenkind_to_string(tok->kind);

        printf("%s%d:%d%s ", COLOR_GRAY, tok->pos_line, tok->pos_column, COLOR_END);

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
    LexerState s = { .src = src };
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

    LexerState s = { .src = src };
    Token tok;
    while (tok.kind != TOK_EOF) {
        tok = lexer_next(&s);
        tokens[i++] = tok;
    }

    return tokens;

}
