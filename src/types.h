#ifndef _TYPECHECKING_H
#define _TYPECHECKING_H

#include "lexer.h"

typedef struct AstNode AstNode;


#define INTLITERAL TYPE_INT


typedef struct Type Type;

typedef struct {
    Type *type;
    const char *ident;
} Param;

#define MAX_ARG_COUNT 255

typedef struct {
    Param params[MAX_ARG_COUNT];
    size_t params_count;
    Type *returntype;
} ProcSignature;

typedef enum {
    TYPE_INVALID, // used for error checking

    TYPE_VOID,
    TYPE_BYTE,
    TYPE_INT,
    TYPE_SIZE,
    TYPE_FUNCTION,
    TYPE_POINTER,
} TypeKind;

struct Type {
    TypeKind kind;
    /*
     * This union contains additional information for complex types,
     * such as functions, pointers and user-defined types
     */
    union {
        ProcSignature type_signature; // used only for procedures
        /*Type type_pointee; // TODO: pointers*/
    };
};



/*
 TODO:
 have a type: TYPE_WHATEVER_INT be returned from integer literals, which can be
 coerced into any other integer type
*/

TypeKind typekind_from_tokenkind(TokenKind kind);
const char *typekind_to_string(TypeKind type);
size_t typekind_get_size(TypeKind type);
const char *typekind_get_size_operand(TypeKind type);
const char *typekind_get_register_rax(TypeKind type);
const char *typekind_get_register_rdi(TypeKind type);

void check_types(AstNode *root);



#endif // _TYPECHECKING_H
