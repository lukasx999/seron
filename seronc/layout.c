#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "symboltable.h"
#include "seronc.h"
#include "gen.h"



typedef struct {
    Symboltable *symboltable;
    size_t layout;
} TraversalContext;



static void ast_vardecl(StmtVarDecl *vardecl) {
    Type type = vardecl->type;
    size_t size = typekind_get_size(type.kind);
    (void) size;
}


static void traverse_ast(AstNode *root) {
    assert(root != NULL);

    switch (root->kind) {
        case ASTNODE_BLOCK:
            break;

        case ASTNODE_PROCEDURE: {
            StmtProcedure *proc = &root->stmt_procedure;
            traverse_ast(proc->body);
        } break;

        case ASTNODE_VARDECL:
            ast_vardecl(&root->stmt_vardecl);
            break;

        default:
            break;
    }

}



void set_stack_layout(AstNode *root) {
    traverse_ast(root);
}
