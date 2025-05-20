#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>
#include <stdarg.h>

#include "lexer.h"


typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_INFO,
} DiagnosticKind;


void diagnostic(DiagnosticKind kind, const char *fmt, ...);
void diagnostic_loc(DiagnosticKind kind, const Token *tok, const char *fmt, ...);

#endif // _UTIL_H
