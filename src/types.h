#ifndef _TYPECHECKING_H
#define _TYPECHECKING_H

#include "lexer.h"

typedef struct AstNode AstNode;


typedef enum {
    TYPE_INVALID, // used for error checking
    TYPE_BYTE,
    TYPE_INT,
    TYPE_SIZE,
    TYPE_FUNCTION,
    TYPE_POINTER,
} Type;

extern Type type_from_tokenkind(TokenKind kind);
extern const char *type_to_string(Type type);
// TODO: register from type

extern void check_types(AstNode *root);



#endif // _TYPECHECKING_H
