#ifndef _ASM_H
#define _ASM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lexer.h"
#include "codegen.h"



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


extern CodeGenerator gen_new           (const char *filename_asm, bool print_comments, const char *filename_src);
extern void          gen_destroy       (CodeGenerator *c);
extern void          gen_comment       (CodeGenerator *c, const char *fmt, ...);
extern void          gen_prelude       (CodeGenerator *c);
extern void          gen_postlude      (CodeGenerator *c);


extern size_t        gen_addition      (CodeGenerator *c, size_t rbp_offset1, size_t rbp_offset2);
extern size_t        gen_store_value   (CodeGenerator *c, int64_t value, IntegerType type);
extern void          gen_inlineasm     (CodeGenerator *c, const char *src, const size_t *addrs, size_t addrs_len);
extern void          gen_func_start    (CodeGenerator *c, const char *identifier);
extern void          gen_func_end      (CodeGenerator *c);
extern void          gen_call          (CodeGenerator *c, const char *identifier);






#endif // _ASM_H
