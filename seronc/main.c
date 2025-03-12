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
#include "backend.h"
#include "symboltable.h"
#include "main.h"
#include "layout.h"


#define FILE_EXTENSION "srn"


struct CompilerContext compiler_context = { 0 };




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
        exit(1);
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
    char *asm_ = compiler_context.filename.asm_;
    char *obj  = compiler_context.filename.obj;

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
        compiler_message(MSG_ERROR, "Failed to assemble via `nasm`");
        exit(1);
    }

}

static void link_cc(void) {
    char *obj       = compiler_context.filename.obj;
    const char *bin = compiler_context.filename.stripped;

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
        exit(1);
    }

}

static void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "User Flags\n"
            "\t--compile-only, -S\n"
            "\t  Dont assemble/link, only produce assembly\n"
            "Developer Flags:\n"
            "\t--debug-asm\n"
            "\t--dump-ast\n"
            "\t--dump-tokens\n"
            "\t--dump-symboltable\n"
            "\t--verbose, -v\n"
            "\t  Show info messages\n"
            );
    exit(EXIT_FAILURE);
}

static void set_filenames(const char *raw) {
    compiler_context.filename.raw = raw;

    size_t dot_offset = strlen(raw) - 1 - strlen(FILE_EXTENSION);

    char *stripped = compiler_context.filename.stripped;
    strncpy(stripped, raw, dot_offset);

    char *asm = compiler_context.filename.asm_;
    strncpy(asm, stripped, 256);
    strcat(asm, ".s");

    char *obj = compiler_context.filename.obj;
    strncpy(obj, stripped, 256);
    strcat(obj, ".o");
}

static void parse_args(int argc, char *argv[]) {

    int opt_index = 0;
    struct option opts[] = {
        { "dump-ast",         no_argument, &compiler_context.opts.dump_ast,         1 },
        { "dump-tokens",      no_argument, &compiler_context.opts.dump_tokens,      1 },
        { "dump-symboltable", no_argument, &compiler_context.opts.dump_symboltable, 1 },
        { "debug-asm",        no_argument, &compiler_context.opts.debug_asm,        1 },
        { NULL, 0, NULL, 0 },
    };

    while (1) {
        int c = getopt_long(argc, argv, "Sv", opts, &opt_index);

        if (c == -1)
            break;

        switch (c) {

            case 0:
                /* long option */
                break;

            case 'v':
                compiler_context.opts.verbose = true;
                break;

            case 'S':
                compiler_context.opts.compile_only = true;
                break;

            default:
                compiler_message(MSG_ERROR, "Unknown option");
                exit(1);
                break;
        }

    }

    if (optind >= argc)
        print_usage(argv);

    const char *filename = argv[optind];
    check_fileextension(filename);
    set_filenames(filename);

}


// TODO: analysis: dont allow statements in global scope
// TODO: synchronizing parser
// TODO: synchronizing typechecker
// TODO: change Token in ast to Token*
// TODO: pointers (addressof)
// TODO: merge parser and grammar
// TODO: ABI: spill arguments onto stack
// TODO: char literal
// TODO: id into tokenlist instead of pointer for ast
// TODO: precompute stack frame layout + reserve stack space in bulk in prelude
// TODO: replace ast traversals with parser_query_ast
// TODO: parser_map_ast() designated initializer array: map from enum to function pointer
// TODO: rework errors/warnings with tokens

/*
 TODO: semcheck
 - using return in procedure
 - no ifs/whiles in global scope
*/



int main(int argc, char *argv[]) {
    parse_args(argc, argv);

    // TODO: show time
    compiler_message(MSG_INFO, "Starting compilation");

    const char *filename = compiler_context.filename.raw;
    compiler_message(MSG_INFO, "Reading source %s", filename);
    char *file = read_file(filename);

    compiler_message(MSG_INFO, "Lexical Analysis");

    TokenList tokens = tokenize(file);

    if (compiler_context.opts.dump_tokens)
        tokenlist_print(&tokens);

    free(file);


    Arena parser_arena = { 0 };
    arena_init(&parser_arena);

    compiler_message(MSG_INFO, "Parsing");
    AstNode *node_root = parser_parse(&tokens, &parser_arena);

    if (compiler_context.opts.dump_ast)
        parser_print_ast(node_root, 2);


    compiler_message(MSG_INFO, "Constructing Symboltable");
    SymboltableList symboltable = symboltable_list_construct(node_root, 5);

    if (compiler_context.opts.dump_symboltable)
        symboltable_list_print(&symboltable);


    compiler_message(MSG_INFO, "Typechecking");
    check_types(node_root);

    // set_stack_layout(node_root);

    compiler_message(MSG_INFO, "Codegeneration");
    generate_code(node_root);

    if (!compiler_context.opts.compile_only) {
        compiler_message(MSG_INFO, "Assembling %s via nasm", compiler_context.filename.asm_);
        assemble();

        compiler_message(MSG_INFO, "Linking %s via cc", compiler_context.filename.obj);
        link_cc();

        compiler_message(MSG_INFO, "Binary `%s` has been built", compiler_context.filename.stripped);
    }

    compiler_message(MSG_INFO, "Compilation finished");

    symboltable_list_destroy(&symboltable);
    arena_free(&parser_arena);
    tokenlist_destroy(&tokens);

    return EXIT_SUCCESS;
}
