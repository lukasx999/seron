#ifndef _TYPES_H
#define _TYPES_H

#include <stddef.h>
#include <stdint.h>

#include <ver.h>

#include "lexer.h"

#define MAX_PARAM_COUNT 255





typedef enum {
    TYPE_INVALID,

    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_LONG,
    TYPE_POINTER,
    TYPE_PROCEDURE,
    TYPE_TABLE,
} TypeKind;

typedef struct Type Type;
typedef struct Table Table;
typedef struct ProcSignature ProcSignature;

struct Type {
    TypeKind kind;
    union {
        ProcSignature *signature;
        Type *pointee;
        Table *table; // TODO: should be string
    };
};

NO_DISCARD size_t get_type_size(TypeKind type);

typedef struct {
    Type type;
    char ident[MAX_IDENT_LEN+1]; // account for nullbyte
    int offset;
} Param;

struct ProcSignature {
    Param params[MAX_PARAM_COUNT];
    size_t params_count;
    Type returntype;
};

typedef Param Field;

struct Table {
    Field fields[MAX_PARAM_COUNT];
    size_t field_count;
};

// TODO: recursively stringify type
const char *stringify_typekind(TypeKind type);






#endif // _TYPES_H
