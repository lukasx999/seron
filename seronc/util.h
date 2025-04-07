#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>
#include <stdarg.h>

#include "lexer.h"


typedef enum {
    MSG_ERROR,
    MSG_WARNING,
    MSG_INFO,
} MessageKind;


void compiler_message (MessageKind kind, const char *fmt, ...);
void throw_error      (Token tok, const char *fmt, ...);
void throw_warning    (Token tok, const char *fmt, ...);


#endif // _UTIL_H
