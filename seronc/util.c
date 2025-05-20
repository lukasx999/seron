#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include "lexer.h"
#include "util.h"
#include "main.h"
#define UTIL_COLORS
#include "lib/util.h"


static void compiler_message_core(MessageKind kind, const char *fmt, va_list va) {
    if (kind == MSG_INFO && !compiler_ctx.opts.verbose)
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

    const char *src = compiler_ctx.src;
    TokenLocation loc = get_token_location(&tok, src);

    for (size_t i=0; i < strlen(src); ++i) {
        const char *s = src + loc.start;
        if (s[i] == '\n') break;
        printf("%c", s[i]);
    }

    printf("\n");

    // TODO: use fancy printf formatting here
    for (int i=0; i < loc.column; ++i)
        printf(" ");

    for (size_t i=0; i < tok.len; ++i)
        printf("^");

    printf("\n");

    snprintf(
        buf,
        ARRAY_LEN(buf),
        "%s:%d:%d: %s",
        compiler_ctx.filename.raw,
        loc.line,
        loc.column,
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
