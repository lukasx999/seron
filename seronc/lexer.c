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
#define LITERAL_SUFFIX_CHAR 'B'
#define LITERAL_SUFFIX_INT  'I'

const char *stringify_tokenkind(TokenKind tok) {
    switch (tok) {
        case TOK_INVALID:        return "invalid";
        case TOK_SENTINEL:       return "sentinel";
        case TOK_EOF:            return "eof";
        case TOK_KW_FOR:         return "for";
        case TOK_LITERAL_NUMBER: return "num";
        case TOK_PIPE:           return "pipe";
        case TOK_LITERAL_STRING: return "string";
        case TOK_LOG_AND:        return "and";
        case TOK_LT:             return "lt";
        case TOK_LT_EQ:          return "lt-eq";
        case TOK_GT:             return "gt";
        case TOK_KW_TABLE:       return "table";
        case TOK_GT_EQ:          return "gt-eq";
        case TOK_PLUS:           return "plus";
        case TOK_LOG_OR:         return "or";
        case TOK_MINUS:          return "minus";
        case TOK_ASTERISK:       return "asterisk";
        case TOK_SLASH:          return "slash";
        case TOK_SEMICOLON:      return "semicolon";
        case TOK_COMMA:          return "comma";
        case TOK_COLON:          return "colon";
        case TOK_LITERAL_IDENT:  return "ident";
        case TOK_ASSIGN:         return "assign";
        case TOK_EQ:             return "eq";
        case TOK_NEQ:            return "neq";
        case TOK_BANG:           return "bang";
        case TOK_AMPERSAND:      return "ampersand";
        case TOK_LPAREN:         return "lparen";
        case TOK_RPAREN:         return "rparen";
        case TOK_LBRACE:         return "lbrace";
        case TOK_RBRACE:         return "rbrace";
        case TOK_LBRACKET:       return "lbracket";
        case TOK_RBRACKET:       return "rbracket";
        case TOK_KW_PROC:        return "proc";
        case TOK_KW_VARDECL:     return "let";
        case TOK_KW_IF:          return "if";
        case TOK_KW_ELSE:        return "else";
        case TOK_KW_ELSIF:       return "elsif";
        case TOK_KW_WHILE:       return "while";
        case TOK_KW_RETURN:      return "return";
        case TOK_KW_TYPE_VOID:   return "void";
        case TOK_KW_TYPE_CHAR:   return "char";
        case TOK_KW_TYPE_INT:    return "int";
        case TOK_KW_TYPE_LONG:   return "long";
    }
    UNREACHABLE();
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
    match_kw(str, "for")    ? TOK_KW_FOR         :
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

static void tokenize_string(Lexer *lex) {

    Token *tok = &lex->tok;

    tok->kind = TOK_LITERAL_STRING;
    const char *start = lex->src + 1;

    while (*++lex->src != '"') {
        if (*lex->src == '\0') {
            diagnostic(DIAG_ERROR, "unterminated string literal: `%s`", start);
            exit(EXIT_FAILURE);
        }
    }

    size_t len = lex->src - start;
    tok->len = len + 2; // account for quotes surrounding the string
    strncpy(tok->value, start, MIN(len, ARRAY_LEN(tok->value)));
    lex->src++; // make src point to the next char, instead of `"`

}

static void tokenize_char(Lexer *lex) {

    Token *tok = &lex->tok;

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

static void tokenize_number(Lexer *lex) {

    Token *tok = &lex->tok;

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

static void tokenize_ident(Lexer *lex) {

    Token *tok = &lex->tok;

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

static void tokenize_single(Lexer *lex, TokenKind kind) {
    Token *tok = &lex->tok;
    tok->kind = kind;
    lex->src++;
}

static void tokenize_double(Lexer *lex, TokenKind first, char cond, TokenKind second) {

    Token *tok = &lex->tok;

    tok->kind = first;
    if (*++lex->src == cond) {
        tok->kind = second;
        tok->len = 2;
        lex->src++;
    }
}

void lexer_init(Lexer *lex, const char *src) {
    lex->src = src;
    lex->position = 0;
}

Token lexer_next(Lexer *lex) {

    lex->tok = (Token) {
        .kind     = TOK_INVALID,
        .position = lex->position,
        .len      = 1,
    };

    Token *tok = &lex->tok;

    if (*lex->src == '\0') {
        tok->kind = TOK_EOF;
        return *tok;
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

        case '+':  tokenize_single(lex, TOK_PLUS);                        break;
        case '-':  tokenize_single(lex, TOK_MINUS);                       break;
        case '*':  tokenize_single(lex, TOK_ASTERISK);                    break;
        case '/':  tokenize_single(lex, TOK_SLASH);                       break;
        case '(':  tokenize_single(lex, TOK_LPAREN);                      break;
        case ')':  tokenize_single(lex, TOK_RPAREN);                      break;
        case '{':  tokenize_single(lex, TOK_LBRACE);                      break;
        case '}':  tokenize_single(lex, TOK_RBRACE);                      break;
        case '[':  tokenize_single(lex, TOK_LBRACKET);                    break;
        case ']':  tokenize_single(lex, TOK_RBRACKET);                    break;
        case ';':  tokenize_single(lex, TOK_SEMICOLON);                   break;
        case ',':  tokenize_single(lex, TOK_COMMA);                       break;
        case ':':  tokenize_single(lex, TOK_COLON);                       break;
        case '<':  tokenize_double(lex, TOK_LT,        '=', TOK_LT_EQ);   break;
        case '>':  tokenize_double(lex, TOK_GT,        '=', TOK_GT_EQ);   break;
        case '!':  tokenize_double(lex, TOK_BANG,      '=', TOK_NEQ);     break;
        case '=':  tokenize_double(lex, TOK_ASSIGN,    '=', TOK_EQ);      break;
        case '|':  tokenize_double(lex, TOK_PIPE,      '|', TOK_LOG_OR);  break;
        case '&':  tokenize_double(lex, TOK_AMPERSAND, '&', TOK_LOG_AND); break;
        case '\'': tokenize_char(lex);                                    break;
        case '"':  tokenize_string(lex);                                  break;

        default: {

            if (isdigit(*lex->src)) {
                tokenize_number(lex);

            } else if (is_ident_head(*lex->src)) {
                tokenize_ident(lex);

            } else {
                diagnostic(DIAG_ERROR, "unknown token `%c`", *lex->src);
                exit(EXIT_FAILURE);
            }

        } break;

    }

    lex->position += tok->len;

    assert(tok->kind != TOK_INVALID);
    return *tok;

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
