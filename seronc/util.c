#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "lexer.h"
#include "util.h"
#include "main.h"
#include "lib/util.h"


static void compiler_message_core(MessageKind kind, const char *fmt, va_list va) {
    if (kind == MSG_INFO && !compiler_config.opts.verbose)
        return;

    NON_NULL(fmt);

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

        default: PANIC("unknown message type");
    }

    fprintf(stderr, "%s%s%s: %s", COLOR_BOLD, color, str, COLOR_END);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    va_end(va);
}

void compiler_message_tok(MessageKind kind, Token tok, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    char buf[NAME_MAX] = { 0 };
    snprintf(
        buf,
        ARRAY_LEN(buf),
        "%s:%d:%d: %s",
        compiler_config.filename.raw,
        tok.pos_line,
        tok.pos_column,
        fmt
    );

    compiler_message_core(kind, buf, va);

    va_end(va);
}

void compiler_message(MessageKind kind, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    compiler_message_core(kind, fmt, va);

    va_end(va);
}
