#ifndef _GRAMMAR_H
#define _GRAMMAR_H

#include "parser.h"
#include "ast.h"


AstNodeList rule_util_arglist(Parser *p);
TypeKind    rule_util_type   (Parser *p);

AstNode *rule_primary    (Parser *p);
AstNode *rule_call       (Parser *p);
AstNode *rule_unary      (Parser *p);
AstNode *rule_factor     (Parser *p);
AstNode *rule_term       (Parser *p);
AstNode *rule_vardecl    (Parser *p);
AstNode *rule_procedure  (Parser *p);
AstNode *rule_block      (Parser *p);
AstNode *rule_expression (Parser *p);
AstNode *rule_exprstmt   (Parser *p);
AstNode *rule_stmt       (Parser *p);
AstNode *rule_program    (Parser *p);



#endif // _GRAMMAR_H
