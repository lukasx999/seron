#ifndef _ASM_H
#define _ASM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "symboltable.h"


typedef struct {
    FILE       *file;
    size_t      rbp_offset;
    size_t      label_count;
    bool        print_comments;
    const char *filename_src;
} CodeGenerator;


extern int  gen_init(CodeGenerator *gen, const char *filename_asm, bool print_comments);
extern void gen_destroy(CodeGenerator *c);
extern void gen_prelude(CodeGenerator *c);

extern Symbol gen_binop(CodeGenerator *gen, Symbol a, Symbol b, BinOpKind kind);
extern Symbol gen_store_literal(CodeGenerator *gen, int64_t value, Type type);
extern void gen_func_start(CodeGenerator *c, const char *identifier);
extern void gen_func_end(CodeGenerator *c);
extern void gen_inlineasm(CodeGenerator *c, const char *src, const Symbol *symbols, size_t symbols_len);
extern Symbol gen_call(CodeGenerator *gen, Symbol callee, const Symbol *arguments, size_t arguments_len);


extern void gen_if_then(CodeGenerator *gen, Symbol cond);
extern void gen_if_else(CodeGenerator *gen);
extern void gen_if_end(CodeGenerator *gen);

extern void gen_while_start(CodeGenerator *gen);
extern void gen_while_end(CodeGenerator *gen, Symbol cond);

extern void gen_assign(CodeGenerator *gen, Symbol assignee, Symbol value);



#endif // _ASM_H
