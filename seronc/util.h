#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"


#define ARRAY_LEN(arr) (sizeof((arr)) / sizeof(*(arr)))

#define COLOR_RED           "\33[31m"
#define COLOR_BLUE          "\33[34m"
#define COLOR_PURPLE        "\33[35m"
#define COLOR_GRAY          "\33[90m"
#define COLOR_CYAN          "\33[36m"
#define COLOR_YELLOW        "\33[33m"
#define COLOR_WHITE         "\33[97m"
#define COLOR_GREEN         "\33[32m"
#define COLOR_BLUE_HIGH     "\33[94m"
#define COLOR_YELLOW_HIGH   "\33[93m"
#define COLOR_PURPLE_HIGH   "\33[95m"
#define COLOR_BOLD          "\33[1m"
#define COLOR_UNDERLINE     "\33[4m"
#define COLOR_ITALIC        "\33[3m"
#define COLOR_STRIKETHROUGH "\33[9m"
#define COLOR_END           "\33[0m"


typedef enum {
    MSG_ERROR,
    MSG_WARNING,
    MSG_INFO,
} MessageKind;


void compiler_message (MessageKind kind, const char *fmt, ...);
void throw_error      (Token tok, const char *fmt, ...);
void throw_warning    (Token tok, const char *fmt, ...);


#endif // _UTIL_H
