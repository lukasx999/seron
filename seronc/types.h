#ifndef _TYPECHECKING_H
#define _TYPECHECKING_H

#include "lexer.h"

typedef struct AstNode AstNode;



typedef struct Type Type;

typedef struct {
    Type *type; // heap-allocated
    const char *ident;
} Param;

#define MAX_ARG_COUNT 255

// TODO: maybe reuse for structs
typedef struct {
    Param params[MAX_ARG_COUNT]; // TODO: uses shit ton of memory
    size_t params_count;
    Type *returntype; // heap-allocated
} ProcSignature;

typedef enum {
    TYPE_INVALID, // used for error checking
    TYPE_VOID,
    TYPE_CHAR,
    TYPE_INT,
    TYPE_LONG,
    TYPE_PROCEDURE,
    TYPE_POINTER,
} TypeKind;

struct Type {
    TypeKind kind;
    // This union contains additional information for complex types,
    // such as functions, pointers and user-defined types
    union {
        ProcSignature signature;
        Type *pointee;
    };
};

const char *typekind_to_string(TypeKind type);



#endif // _TYPECHECKING_H
