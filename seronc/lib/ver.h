#ifndef _VER_H
#define _VER_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdnoreturn.h>
#include <stdarg.h>


//
// ver.h - Utility C functions/macros
//



#define __STDC23 __STDC_VERSION__ >= 202000
#define __STDC11 __STDC_VERSION__ == 201112L
#define __STDC99 // TODO:


#ifdef VER_ANSI_COLORS

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

#endif // VER_ANSI_COLORS


#ifdef __STDC11
    #define NORETURN noreturn
#else
    #define NORETURN __attribute((noreturn))
#endif // __STDC11


noreturn static inline void _impl_panic(const char *file, const char *func, int line, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    fprintf(stderr, "Panicked at %s: %s: %d\n", file, func, line);
    vfprintf(stderr, fmt, va);
    fprintf(stderr, "\n");

    va_end(va);
    abort();
}

#define PANIC(msg) \
    ((void) (_impl_panic(__FILE__, __func__, __LINE__, (msg)), 0))


#if __STDC23 || defined(VERSION_I_DONT_CARE)

#define PANIC_FMT(fmt, ...) \
    ((void) (_impl_panic(__FILE__, __func__, __LINE__, (fmt) __VA_OPT__(,) __VA_ARGS__), 0))

#endif


static inline int _impl_must_zero(int value, const char *name, const char *file, const char *func, int line) {
    if (value) _impl_panic(file, func, line, "%s is not 0", name);
    return value;
}

#define MUST_ZERO(value) \
    _impl_must_zero((value), #value, __FILE__, __func__, __LINE__)



static inline void *_impl_non_null(void *ptr, const char *name, const char *file, const char *func, int line) {
    if (ptr == NULL) _impl_panic(file, func, line, "%s is not NULL", name);
    return ptr;
}

#define NON_NULL(ptr) \
    _impl_non_null((void*) (ptr), #ptr, __FILE__, __func__, __LINE__)



#define ARRAY_LEN(xs) \
    (sizeof(xs) / sizeof *(xs))

#define UNREACHABLE() \
    PANIC("unreachable")

#define TODO(msg) \
    PANIC("not implemented yet: " msg)

#define UNIMPLEMENTED(msg) \
    PANIC("not implemented: " msg)

#define UNUSED \
    __attribute__((unused))

#define DISCARD(value) \
    ((void) (value))



#if __STDC23
    #define NO_DISCARD \
        [[nodiscard]]

#else
    #define NO_DISCARD \
        __attribute__((warn_unused_result))

#endif // __STDC23



#define CLAMP(value, min, max) \
    (assert(min <= max),       \
    (value) > (max) ? (max) : (value) < (min) ? (min) : (value))

#define MIN(a, b) \
    ((a) < (b) ? (a) : (b))

#define MAX(a, b) \
    ((a) > (b) ? (a) : (b))

#define LAST(xs) \
    ((xs)[ARRAY_LEN((xs))-1])

#define LASTCHAR(str) \
    ((str)[strlen((str))-1])

#define LERP(a, b, t) \
    ((a) + (t) * ((b) - (a)))

#define NOP()



#ifdef __STDC23

    #define SWAP(x, y)         \
        do {                   \
            typeof(x) tmp = x; \
            x = y;             \
            y = tmp;           \
        } while (0)

#else

    #define SWAP(T, x, y) \
        do {              \
            T tmp = x;    \
            x = y;        \
            y = tmp;      \
        } while (0)

#endif // __STDC23



#ifdef __STDC11

#define PRINT(x) do {                                \
    const char *fmt =                                \
        _Generic((x),                                \
            char:   "%c",                            \
            int:    "%d",                            \
            float:  "%f",                            \
            size_t: "%lu",                           \
            char*:  "%s",                            \
            void*:  "%p"                             \
    );                                               \
    printf("(%s:%d) " #x " = ", __FILE__, __LINE__); \
    printf(fmt, (x));                                \
    printf("\n");                                    \
} while (0)

#endif // __STDC11



#endif // _VER_H
