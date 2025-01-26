#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"
#include "util.h"





static void throw_cool_thing(
    const char  *filename,
    const Token *tok,
    bool         is_error, // else warning
    const char  *fmt,
    va_list      va
) {

    const char *color = is_error ? COLOR_RED : COLOR_GREEN;
    const char *str   = is_error ? "ERROR" : "WARNING";

    fprintf(
        stderr,
        "%s:%d:%d: %s%s%s%s: ",
        filename, tok->pos_line, tok->pos_column,
        COLOR_BOLD, color, str, COLOR_END
    );

    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    if (is_error)
        exit(EXIT_FAILURE);
}

void throw_cool_error(
    const char *filename,
    const Token *tok,
    const char *fmt, ...
) {
    va_list va;
    va_start(va, fmt);
    throw_cool_thing(filename, tok, true, fmt, va);
    va_end(va);
}

// TODO: throw_cool_warning()



void throw_error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "%s%sERROR: %s", COLOR_BOLD, COLOR_RED, COLOR_END);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
    va_end(va);
    exit(EXIT_FAILURE);
}

void throw_warning(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "%s%sWARNING: %s", COLOR_BOLD, COLOR_YELLOW, COLOR_END);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");
    va_end(va);
}
