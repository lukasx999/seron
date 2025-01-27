#ifndef _GRAMMAR_H
#define _GRAMMAR_H

#include "parser.h"
#include "ast.h"


extern AstNodeList rule_util_arglist(Parser *p);

extern AstNode *rule_primary    (Parser *p);
extern AstNode *rule_call       (Parser *p);
extern AstNode *rule_unary      (Parser *p);
extern AstNode *rule_factor     (Parser *p);
extern AstNode *rule_term       (Parser *p);
extern AstNode *rule_vardecl    (Parser *p);
extern AstNode *rule_function   (Parser *p);
extern AstNode *rule_block      (Parser *p);
extern AstNode *rule_expression (Parser *p);
extern AstNode *rule_exprstmt   (Parser *p);
extern AstNode *rule_stmt       (Parser *p);
extern AstNode *rule_program    (Parser *p);



#endif // _GRAMMAR_H
