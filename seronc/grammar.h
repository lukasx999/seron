#ifndef _GRAMMAR_H
#define _GRAMMAR_H

#include "parser.h"


/*
Purpose of this header:
- usage by parser.c
- cross-referencing rules
*/

TypeKind    rule_util_type      (Parser *p);
AstNodeList rule_util_arglist   (Parser *p);
size_t      rule_util_paramlist (Parser *p, Param *out_params);

AstNode *rule_primary    (Parser *p);
AstNode *rule_call       (Parser *p);
AstNode *rule_unary      (Parser *p);
AstNode *rule_addressof  (Parser *p);
AstNode *rule_factor     (Parser *p);
AstNode *rule_term       (Parser *p);
AstNode *rule_vardecl    (Parser *p);
AstNode *rule_while      (Parser *p);
AstNode *rule_if         (Parser *p);
AstNode *rule_return     (Parser *p);
AstNode *rule_procedure  (Parser *p);
AstNode *rule_block      (Parser *p);
AstNode *rule_assignment (Parser *p);
AstNode *rule_expression (Parser *p);
AstNode *rule_exprstmt   (Parser *p);
AstNode *rule_stmt       (Parser *p);
AstNode *rule_program    (Parser *p);

#endif // _GRAMMAR_H
