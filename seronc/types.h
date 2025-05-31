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
    TYPE_OBJECT,
} TypeKind;

typedef struct Type Type;
typedef struct Table Table;
typedef struct ProcSignature ProcSignature;

struct Type {
    TypeKind kind;
    union {
        ProcSignature *signature;
        Type *pointee;
        Table *table;
        char object_name[MAX_IDENT_LEN];
    };
};

NO_DISCARD int type_primitive_size(TypeKind type);
// aligns i to a 16 byte boundary
void align_16(int *i);

typedef struct {
    Type type;
    char ident[MAX_IDENT_LEN+1]; // account for nullbyte
    int offset;
} Param;
// TODO: create separate field struct without offset

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
