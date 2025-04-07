#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/wait.h>

#define ARENA_IMPL
#include "lib/arena.h"

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "codegen.h"
#include "symboltable.h"
#include "main.h"
#include "lib/util.h"


#define FILE_EXTENSION "srn"


struct CompilerConfig compiler_config = { 0 };


static void check_fileextension(const char *filename) {

    if (strlen(filename) <= strlen(FILE_EXTENSION) + 1) { // eg: `.___`
        compiler_message(MSG_ERROR, "Invalid filename `%s`", filename);
        exit(1);
    }

    size_t dot_offset = strlen(filename) - 1 - strlen(FILE_EXTENSION);

    if (filename[dot_offset] != '.') {
        compiler_message(MSG_ERROR, "File extension missing");
        exit(1);
    }

    if (strncmp(filename + dot_offset + 1, FILE_EXTENSION, strlen(FILE_EXTENSION))) {
        compiler_message(MSG_ERROR, "File extension must be `.%s`", FILE_EXTENSION);
        exit(1);
    }

}

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        compiler_message(MSG_ERROR, "Source file `%s` does not exist", filename);
        exit(EXIT_FAILURE);
    }

    struct stat statbuf = { 0 };
    stat(filename, &statbuf);
    size_t size = statbuf.st_size;

    char *buf = calloc(size + 1, sizeof(char));
    fread(buf, sizeof(char), size, file);

    fclose(file);
    return buf;
}

static int run_cmd_sync(char *const argv[]) {
    int status;

    if (!fork()) {
        execvp(argv[0], argv);
        exit(1); // Failed to exec
    }

    wait(&status);

    return WIFEXITED(status)
    ? WEXITSTATUS(status)
    : 1;
}

static void assemble(void) {
    char *asm_ = compiler_config.filename.asm_;
    char *obj  = compiler_config.filename.obj;

    // Assemble
    int ret = run_cmd_sync((char*[]) {
        "nasm",
        asm_,
        "-felf64",
        "-o", obj,
        "-gdwarf",
        NULL
    });

    if (ret) {
        compiler_message(MSG_ERROR, "Failed to assemble via `nasm` (is nasm installed?)");
        exit(EXIT_FAILURE);
    }

}

static void link_cc(void) {
    char *obj       = compiler_config.filename.obj;
    const char *bin = compiler_config.filename.stripped;

    // Link
    int ret = run_cmd_sync((char*[]) {
        "cc",
        "-no-pie", // TODO: possibly insecure?
        "-lc",
        obj,
        "-o", (char*) bin,
        NULL
    });

    if (ret) {
        compiler_message(MSG_ERROR, "Failed to link via `cc`");
        exit(EXIT_FAILURE);
    }

}

static void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "\t-S                               generate assembly, don't assemble/link\n"
            "\t-c                               compile and assemble, don't link\n"
            "\t-v, --verbose                    show info messages\n"
            "\t--asmdoc                         add debug comments into the generated assembly\n"
            "\t--dump-ast\n"
            "\t--dump-tokens\n"
            "\t--dump-symboltable\n"
            );
    exit(EXIT_FAILURE);
}

static void set_filenames(const char *raw) {
    compiler_config.filename.raw = raw;

    size_t dot_offset = strlen(raw) - 1 - strlen(FILE_EXTENSION);

    char *stripped = compiler_config.filename.stripped;
    strncpy(stripped, raw, dot_offset);

    char *asm = compiler_config.filename.asm_;
    strncpy(asm, stripped, NAME_MAX);
    strcat(asm, ".s");

    char *obj = compiler_config.filename.obj;
    strncpy(obj, stripped, NAME_MAX);
    strcat(obj, ".o");
}

static void parse_args(int argc, char **argv) {

    int opt_index = 0;
    struct option opts[] = {
        { "dump-ast",         no_argument, &compiler_config.opts.dump_ast,         1 },
        { "dump-tokens",      no_argument, &compiler_config.opts.dump_tokens,      1 },
        { "dump-symboltable", no_argument, &compiler_config.opts.dump_symboltable, 1 },
        { "asmdoc",           no_argument, &compiler_config.opts.debug_asm,        1 },
        { "verbose",          no_argument, &compiler_config.opts.verbose,          1 },
        { NULL, 0, NULL, 0 },
    };

    while (1) {
        int c = getopt_long(argc, argv, "Scv", opts, &opt_index);

        if (c == -1)
            break;

        switch (c) {
            case 0:
                /* long option */
                break;

            case 'v': compiler_config.opts.verbose              = 1; break;
            case 'S': compiler_config.opts.compile_only         = 1; break;
            case 'c': compiler_config.opts.compile_and_assemble = 1; break;

            default:
                compiler_message(MSG_ERROR, "Unknown option");
                exit(EXIT_FAILURE);
        }

    }

    if (optind >= argc)
        print_usage(argv);

    const char *filename = argv[optind];
    check_fileextension(filename);
    set_filenames(filename);
}



// TODO: semcheck
//  - using return in procedure
//  - no ifs/whiles in global scope
// TODO: synchronizing parser / typechecker
// TODO: change Token in ast to Token*
// TODO: pointers (and addrof)
// TODO: show compilation time diff
// TODO: merge parser and grammar
// TODO: ABI: spill arguments onto stack
// TODO: char literal
// TODO: id into tokenlist instead of pointer for ast
// TODO: replace ast traversals with parser_query_ast
// TODO: parser_map_ast() designated initializer array: map from enum to function pointer
// TODO: rework errors/warnings with tokens
// TODO: resolve codegen grouping conflict with push() like chibicc
// TODO: resolve variables in identifier primary rule, so no symboltable is needed for codegen
// TODO: linked list as symboltable


void test_lexer(void) {

    const char *src = "return 1+2+_foo123*/ \"str\" = == proc if else while";
    Token *tok = lexer_collect_tokens(src);
    lexer_print_tokens(src);
    Token *tmp = tok;

    Token *cmp = (Token[]) {
        (Token) { .kind = TOK_KW_RETURN },
        (Token) { .kind = TOK_NUMBER, .value = "1" },
        (Token) { .kind = TOK_PLUS },
        (Token) { .kind = TOK_NUMBER, .value = "2" },
        (Token) { .kind = TOK_PLUS },
        (Token) { .kind = TOK_IDENTIFIER, .value = "_foo123" },
        (Token) { .kind = TOK_ASTERISK },
        (Token) { .kind = TOK_SLASH },
        (Token) { .kind = TOK_STRING, .value = "str" },
        (Token) { .kind = TOK_ASSIGN },
        (Token) { .kind = TOK_EQUALS },
        (Token) { .kind = TOK_KW_FUNCTION },
        (Token) { .kind = TOK_KW_IF },
        (Token) { .kind = TOK_KW_ELSE },
        (Token) { .kind = TOK_KW_WHILE },
    };

    while (tmp->kind != TOK_EOF) {
        // TODO: only compares first two struct fields
        // compare all fields, after refactoring token positions
        assert(!memcmp(tmp, cmp, offsetof(Token, value)));
        cmp++;
        tmp++;
    }

    free(tok);
}

void test_parser(void) {
    Arena arena = { 0 };
    arena_init(&arena);

    AstNode *root = parse("proc main() { 1+2; }", &arena);
    parser_print_ast(root, 2);

    assert(root->kind == ASTNODE_BLOCK);
    AstNode *func = *root->block.stmts.items;
    assert(func->kind == ASTNODE_PROCEDURE);

    AstNode *plus = *func->stmt_proc.body->block.stmts.items;
    assert(plus->kind == ASTNODE_BINOP);

    AstNode *lhs = plus->expr_binop.lhs;
    AstNode *rhs = plus->expr_binop.rhs;

    assert(lhs->kind == ASTNODE_LITERAL);
    assert(rhs->kind == ASTNODE_LITERAL);

    arena_free(&arena);
}

void test(void) {
    test_lexer();
    test_parser();
}

int main(int argc, char **argv) {

    parse_args(argc, argv);

    char *file = read_file(compiler_config.filename.raw);

    if (compiler_config.opts.dump_tokens)
        lexer_print_tokens(file);

    Arena parser_arena = { 0 };
    arena_init(&parser_arena);

    AstNode *node_root = parse(file, &parser_arena);

    if (compiler_config.opts.dump_ast)
        parser_print_ast(node_root, 2);


    // SymboltableList symboltable = symboltable_list_construct(node_root, 5);
    // if (compiler_config.opts.dump_symboltable)
    //     symboltable_list_print(&symboltable);

    // check_types(node_root);
    codegen(node_root);

    if (!compiler_config.opts.compile_only) {
        assemble();

        if (!compiler_config.opts.compile_and_assemble)
            link_cc();

    }

    // symboltable_list_destroy(&symboltable);
    arena_free(&parser_arena);
    free(file);

    return EXIT_SUCCESS;
}
