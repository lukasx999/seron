#ifndef _ASM_H
#define _ASM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "symboltable.h"

/* context needed for if/while */
/* remembers the current label count, to make nested if/while work */
typedef size_t gen_ctx_t;

typedef struct {
    FILE       *file;
    uint64_t    label_count;
    bool        print_comments;
    const char *filename_src;
} CodeGenerator;


size_t typekind_get_size(TypeKind type);


int  gen_init    (CodeGenerator *gen, const char *filename_asm, bool print_comments);
void gen_destroy (CodeGenerator *c);
void gen_prelude (CodeGenerator *c);


Symbol    gen_binop            (CodeGenerator *gen, const Symbol  *lhs, const Symbol  *rhs, BinOpKind      kind);
Symbol    gen_store_literal    (CodeGenerator *gen, int64_t value, TypeKind type);
void      gen_procedure_start  (CodeGenerator *gen, const char *identifier, uint64_t stack_size, const ProcSignature *sig, const Symboltable *scope);
void      gen_procedure_end    (CodeGenerator *gen);
void      gen_procedure_extern (CodeGenerator *gen, const char *identifier);
void      gen_inlineasm        (CodeGenerator *gen,   const char *src, const Symbol *symbols, size_t symbols_len);
Symbol    gen_call             (CodeGenerator *gen, Symbol callee,   const Symbol *args,    size_t args_len);
gen_ctx_t gen_if_then          (CodeGenerator *gen, Symbol cond);
void      gen_if_else          (CodeGenerator *gen, gen_ctx_t ctx);
void      gen_if_end           (CodeGenerator *gen, gen_ctx_t ctx);
gen_ctx_t gen_while_start      (CodeGenerator *gen);
void      gen_while_end        (CodeGenerator *gen, Symbol cond,     gen_ctx_t ctx);
void      gen_assign           (CodeGenerator *gen, Symbol assignee, Symbol value);
void      gen_return           (CodeGenerator *gen, const Symbol *value);
void      gen_var_init         (CodeGenerator *gen, const Symbol *var, const Symbol *init);


#endif // _ASM_H
