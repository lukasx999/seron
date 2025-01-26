#ifndef _ASM_H
#define _ASM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lexer.h"



// Enum value is size of type in bytes
typedef enum {
    INTTYPE_CHAR  = 1,
    INTTYPE_SHORT = 2,
    INTTYPE_INT   = 4,
    INTTYPE_SIZE  = 8,
} IntegerType;


extern IntegerType inttype_from_astnode (TokenKind type);
extern const char *inttype_reg_rax      (IntegerType type);
extern const char *inttype_asm_operand  (IntegerType type);



typedef struct {
    FILE *file;
    size_t rbp_offset;
    bool print_comments;
} CodeGenerator;

extern CodeGenerator gen_new           (const char *filename, bool print_comments);
extern void          gen_destroy       (CodeGenerator *c);
extern void          gen_comment       (CodeGenerator *c, const char *fmt, ...);
extern void          gen_prelude       (CodeGenerator *c);
extern void          gen_postlude      (CodeGenerator *c);
extern void          gen_addition      (CodeGenerator *c, size_t rbp_offset1, size_t rbp_offset2);
extern void          gen_copy_value    (CodeGenerator *c, size_t addr, IntegerType type);
extern void          gen_store_value   (CodeGenerator *c, int64_t value, IntegerType type);
extern void          gen_inlineasm     (CodeGenerator *c, const char *src);
extern void          gen_func_start    (CodeGenerator *c, const char *identifier);
extern void          gen_func_end      (CodeGenerator *c);
extern void          gen_call          (CodeGenerator *c, const char *identifier);



#endif // _ASM_H
