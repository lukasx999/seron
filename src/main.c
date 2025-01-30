#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <alloca.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "backend.h"
#include "symboltable.h"
#include "analysis.h"



static void check_fileextension(const char *filename, const char *extension) {

    size_t dot_offset = strcspn(filename, ".");
    if (dot_offset == strlen(filename))
        throw_error_simple("File extension missing");

    if (strncmp(filename + dot_offset + 1, extension, strlen(extension)))
        throw_error_simple("File extension must be `.%s`", extension);
}

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
        throw_error_simple("Source file `%s` does not exist", filename);

    struct stat statbuf = { 0 };
    stat(filename, &statbuf);
    size_t size = statbuf.st_size;

    char *buf = calloc(size + 1, sizeof(char));
    fread(buf, sizeof(char), size, file);

    fclose(file);
    return buf;
}

static void run_cmd(char *const argv[]) {
    if (fork() == 0)
        execvp(argv[0], argv);
    wait(NULL);
}

static void build_binary(bool link_with_libc) {
    // TODO: ensure nasm and ld are installed

    // Assemble
    run_cmd((char*[]) {
        "nasm",
        "out.s",
        "-felf64",
        "-o", "out.o",
        "-gdwarf",
        NULL
    });

    // Link
    run_cmd(
        link_with_libc
        ? (char*[]) {
            "ld",
            "out.o",
            "-lc",
            "-dynamic-linker", "/lib64/ld-linux-x86-64.so.2",
            "-o", "out",
            NULL
        }
        : (char*[]) {
            "ld",
            "out.o",
            "-o", "out",
            NULL
        }
    );

}




// TODO: cmdline args
// TODO: type checking + semantic analysis
// TODO: synchronizing parser
// TODO: ast optimization pass
// TODO: asm gets generated even if compiler fails -> semantic analysis
// TODO: expect-style testing
// TODO: unit tests for parser + symboltable
// TODO: assignment
// TODO: pointers
// TODO: call args

// TODO: global compiler context
// TODO: type checking
// TODO: getopt

// TODO:
/*
cmdline args:
-W, -S, -c, --dump-ast, --dump-tokens, -lc, --run
*/



struct CompilerContext {
    const char *filename;
    bool show_warnings;
    bool debug_asm;
    bool dump_ast, dump_tokens, dump_symboltable;
} compiler_context;

void init_context(void) {
    compiler_context = (struct CompilerContext) {
        .filename         = NULL,
        .show_warnings    = false,
        .debug_asm        = false,
        .dump_ast         = false,
        .dump_tokens      = false,
        .dump_symboltable = false,
    };
}

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "\t--dump-ast\n"
            "\t--dump-tokens\n"
            "\t--dump-symboltable\n"
            );
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char *argv[]) {

    int opt_index = 0;
    struct option opts[] = {
        { "dump-ast",         no_argument, (int*) &compiler_context.dump_ast,         true },
        { "dump-tokens",      no_argument, (int*) &compiler_context.dump_tokens,      true },
        { "dump-symboltable", no_argument, (int*) &compiler_context.dump_symboltable, true },
        { "debug-asm",        no_argument, (int*) &compiler_context.debug_asm,        true },
        // TODO: --pedantic
    };

    while (1) {
        int c = getopt_long(argc, argv, "W", opts, &opt_index);

        if (c == -1)
            break;

        switch (c) {

            case 'W':
                compiler_context.show_warnings = true;
                break;

            default:
                throw_error_simple("Unknown option");
                break;

        }

    }

    if (optind >= argc)
        print_usage(argv);

    compiler_context.filename = argv[optind];

}




int main(int argc, char *argv[]) {

    init_context();
    parse_args(argc, argv);



    const char *filename = compiler_context.filename;
    assert(filename != NULL);

    check_fileextension(filename, "spx");

    char       *file        = read_file(filename);
    TokenList   tokens      = tokenize(file);
    AstNode    *node_root   = parser_parse(&tokens, filename);
    Symboltable symboltable = symboltable_construct(node_root, 5);
#if 0
    check_types(root);
#endif
    generate_code(node_root, filename, false);
    build_binary(false);


    if (compiler_context.dump_tokens)
        tokenlist_print(&tokens);

    if (compiler_context.dump_ast)
        parser_print_ast(node_root);

    if (compiler_context.dump_symboltable)
        symboltable_print(&symboltable);




    symboltable_destroy(&symboltable);
    parser_free_ast(node_root);
    tokenlist_destroy(&tokens);
    free(file);

    return EXIT_SUCCESS;
}
