#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"
#include "util.h"
#include "main.h"



static void throw_thing(
    Token       tok,
    bool        is_error, // else warning
    const char *fmt,
    va_list     va
) {

    const char *color = is_error ? COLOR_RED : COLOR_YELLOW;
    const char *str   = is_error ? "ERROR" : "WARNING";

    fprintf(
        stderr,
        "%s:%d:%d: %s%s%s%s: ",
        compiler_context.filename.raw, tok.pos_line, tok.pos_column,
        COLOR_BOLD, color, str, COLOR_END
    );

    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    if (is_error)
        exit(EXIT_FAILURE);
}

void throw_error(
    Token tok,
    const char *fmt, ...
) {
    va_list va;
    va_start(va, fmt);
    throw_thing(tok, true, fmt, va);
    va_end(va);
}

void throw_warning(
    Token tok,
    const char *fmt, ...
) {
    va_list va;
    va_start(va, fmt);
    throw_thing(tok, false, fmt, va);
    va_end(va);
}

static void throw_thing_simle(bool is_error, const char *fmt, va_list va) {
    const char *str = is_error ? "ERROR" : "WARNING";
    const char *color = is_error ? COLOR_RED : COLOR_YELLOW;

    fprintf(stderr, "%s%s%s: %s", COLOR_BOLD, color, str, COLOR_END);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    if (is_error)
        exit(EXIT_FAILURE);
}

void throw_error_simple(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    throw_thing_simle(true, fmt, va);
    va_end(va);
}

void throw_warning_simple(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    throw_thing_simle(false, fmt, va);
    va_end(va);
}
