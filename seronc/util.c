#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

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
        compiler_config.filename.raw, tok.pos_line, tok.pos_column,
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

void compiler_message(MessageKind kind, const char *fmt, ...) {
    if (kind == MSG_INFO && !compiler_config.opts.verbose)
        return;

    assert(fmt != NULL);

    va_list va;
    va_start(va, fmt);

    const char *str, *color = NULL;
    switch (kind) {
        case MSG_INFO:
            str = "INFO";
            color = COLOR_BLUE;
            break;
        case MSG_WARNING:
            str = "WARNING";
            color = COLOR_YELLOW;
            break;
        case MSG_ERROR:
            str = "ERROR";
            color = COLOR_RED;
            break;
        default:
            assert(!"wtf");
            break;
    }

    fprintf(stderr, "%s%s%s: %s", COLOR_BOLD, color, str, COLOR_END);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    va_end(va);
}
