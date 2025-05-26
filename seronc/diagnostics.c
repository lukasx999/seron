#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#include <ver.h>

#include "lexer.h"
#include "diagnostics.h"
#include "main.h"
#include "colors.h"



// returns length of tag string
static size_t print_diag_tag(DiagnosticKind kind) {
    const char *str, *color;

    switch (kind) {

        case DIAG_INFO:
            str = "INFO";
            color = COLOR_BLUE;
            break;

        case DIAG_WARNING:
            str = "WARNING";
            color = COLOR_YELLOW;
            break;

        case DIAG_ERROR:
            str = "ERROR";
            color = COLOR_RED;
            break;

        default: PANIC("unknown message type");
    }

    fprintf(stderr, "%s%s%s%s", COLOR_BOLD, color, str, COLOR_END);
    return strlen(str);

}

static void print_diag_header(DiagnosticKind kind) {
    fprintf(stderr, "---");
    print_diag_tag(kind);
    fprintf(stderr, "---\n");
}

void diagnostic_loc(DiagnosticKind kind, const Token *tok, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    char location_buf[NAME_MAX] = { 0 };

    const char *src = compiler_ctx.src;
    TokenLocation loc = get_token_location(tok, src);
    print_diag_header(kind);

    snprintf(location_buf, ARRAY_LEN(location_buf), "%s:%d:%d", compiler_ctx.filename, loc.line, loc.column);

    fprintf(stderr, "Cause: ");
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    fprintf(stderr, "Location: %s\n", location_buf);
    fprintf(stderr, "\n");

    for (size_t i=0; i < strlen(src); ++i) {
        size_t idx = i + loc.start;

        if (src[idx] == '\n') break;

        if (idx - tok->position < tok->len)
            fprintf(stderr, "%s%s%c%s", COLOR_BOLD, COLOR_RED, src[idx], COLOR_END);
        else
            fprintf(stderr, "%c", src[idx]);
    }

    fprintf(stderr, "\n");

    // TODO: use fancy printf formatting here
    for (int i=0; i < loc.column-1; ++i)
        fprintf(stderr, " ");

    for (size_t i=0; i < tok->len; ++i)
        fprintf(stderr, "%s^%s", COLOR_RED, COLOR_END);

    fprintf(stderr, "\n");

    va_end(va);
}

void diagnostic(DiagnosticKind kind, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    print_diag_header(kind);
    fprintf(stderr, "Cause: ");
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    va_end(va);
}
