#ifndef _ASM_H
#define _ASM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lexer.h"
#include "symboltable.h"


// Enum value is size of type in bytes
typedef enum {
    INTTYPE_CHAR  = 1,
    INTTYPE_INT   = 4,
    INTTYPE_SIZE  = 8,
} IntegerType;


extern IntegerType inttype_from_astnode (TokenKind type);
extern const char *inttype_reg_rax      (IntegerType type);
extern const char *inttype_asm_operand  (IntegerType type);


typedef struct {
    FILE       *file;
    size_t      rbp_offset;
    bool        print_comments;
    const char *filename_src;
} CodeGenerator;


extern int           gen_init          (CodeGenerator *gen, const char *filename_asm, bool print_comments, const char *filename_src);
extern void          gen_destroy       (CodeGenerator *c);
extern void          gen_prelude       (CodeGenerator *c);
extern void          gen_postlude      (CodeGenerator *c);

extern Symbol gen_addition(CodeGenerator *gen, Symbol a, Symbol b);
extern Symbol gen_store_value(CodeGenerator *gen, int64_t value, IntegerType type);
extern void          gen_func_start    (CodeGenerator *c, const char *identifier);
extern void          gen_func_end      (CodeGenerator *c);
extern void          gen_call          (CodeGenerator *c, const char *identifier);
extern void gen_inlineasm(CodeGenerator *c, const char *src, const Symbol *symbols, size_t symbols_len);





#endif // _ASM_H
