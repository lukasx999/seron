#ifndef _ASM_H
#define _ASM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "lexer.h"
#include "symboltable.h"


typedef struct {
    FILE       *file;
    size_t      rbp_offset;
    bool        print_comments;
    const char *filename_src;
} CodeGenerator;


extern int  gen_init(CodeGenerator *gen, const char *filename_asm, bool print_comments);
extern void gen_destroy(CodeGenerator *c);
extern void gen_prelude(CodeGenerator *c);
extern void gen_postlude(CodeGenerator *c);

extern Symbol gen_binop(CodeGenerator *gen, Symbol a, Symbol b, BinOpKind kind);
extern Symbol gen_store_literal(CodeGenerator *gen, int64_t value, Type type);
extern void gen_func_start(CodeGenerator *c, const char *identifier);
extern void gen_func_end(CodeGenerator *c);
extern void gen_call(CodeGenerator *c, const char *identifier);
extern void gen_inlineasm(CodeGenerator *c, const char *src, const Symbol *symbols, size_t symbols_len);





#endif // _ASM_H
