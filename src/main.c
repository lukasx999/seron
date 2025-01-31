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
#include "main.h"



struct CompilerContext compiler_context = { 0 };



static void check_fileextension(const char *filename, const char *extension) {

    if (strlen(filename) <= strlen(extension) + 1) // eg: `.spx`
        throw_error_simple("Invalid filename `%s`", filename);

    size_t dot_offset = strlen(filename) - 1 - strlen(extension);

    if (filename[dot_offset] != '.')
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

static void build_binary(void) {
    // TODO: ensure nasm and ld are installed

    char *asm       = compiler_context.filename.asm;
    char *obj       = compiler_context.filename.obj;
    const char *bin = compiler_context.filename.stripped;

    // Assemble
    run_cmd((char*[]) {
        "nasm",
        asm,
        "-felf64",
        "-o", obj,
        "-gdwarf",
        NULL
    });

    // Link
    run_cmd((char*[]) {
        "ld",
        obj,
        "-o", (char*) bin,
        NULL
    }
    );

}

static void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [options] file...\n", argv[0]);
    fprintf(stderr, "Options:\n");
    fprintf(stderr,
            "\t--dump-ast\n"
            "\t--dump-tokens\n"
            "\t--dump-symboltable\n"
            );
    exit(EXIT_FAILURE);
}

static void set_filenames(const char *raw, const char *extension) {
    compiler_context.filename.raw = raw;

    size_t dot_offset = strlen(raw) - 1 - strlen(extension);

    char *stripped = compiler_context.filename.stripped;
    strncpy(stripped, raw, dot_offset);

    char *asm = compiler_context.filename.asm;
    strncpy(asm, stripped, 256);
    strcat(asm, ".s");

    char *obj = compiler_context.filename.obj;
    strncpy(obj, stripped, 256);
    strcat(obj, ".o");
}

static void parse_args(int argc, char *argv[]) {

    int opt_index = 0;
    struct option opts[] = {
        { "dump-ast",         no_argument, (int*) &compiler_context.dump_ast,         true },
        { "dump-tokens",      no_argument, (int*) &compiler_context.dump_tokens,      true },
        { "dump-symboltable", no_argument, (int*) &compiler_context.dump_symboltable, true },
        { "debug-asm",        no_argument, (int*) &compiler_context.debug_asm,        true },
        // TODO: --pedantic
        // TODO: link with libc
    };

    while (1) {
        int c = getopt_long(argc, argv, "W", opts, &opt_index);

        if (c == -1)
            break;

        switch (c) {

            case 0:
                /* long option */
                break;

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

    const char *filename = argv[optind];
    const char *extension = "spx";
    check_fileextension(filename, extension);
    set_filenames(filename, extension);

}


// TODO: type checking + semantic analysis
// TODO: synchronizing parser
// TODO: ast optimization pass
// TODO: asm gets generated even if compiler fails -> semantic analysis
// TODO: expect-style testing
// TODO: unit tests for parser + symboltable
// TODO: assignment
// TODO: pointers
// TODO: call args
// TODO: --run argument

// TODO: type checking

// TODO: fix gen types





int main(int argc, char *argv[]) {

    // TODO: stacking long arguments
    parse_args(argc, argv);

    char       *file        = read_file(compiler_context.filename.raw);
    TokenList   tokens      = tokenize(file);
    AstNode    *node_root   = parser_parse(&tokens);
    Symboltable symboltable = symboltable_construct(node_root, 5);
#if 0
    check_types(root);
#endif
    generate_code(node_root);
    build_binary();


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
