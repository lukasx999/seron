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
        { "dump-ast",         no_argument, &compiler_context.opts.dump_ast,         1 },
        { "dump-tokens",      no_argument, &compiler_context.opts.dump_tokens,      1 },
        { "dump-symboltable", no_argument, &compiler_context.opts.dump_symboltable, 1 },
        { "debug-asm",        no_argument, &compiler_context.opts.debug_asm,        1 },
        { NULL, 0, NULL, 0 },
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
                compiler_context.opts.show_warnings = true;
                break;

            default:
                throw_error_simple("Unknown option");
                break;

        }

    }

    if (optind >= argc)
        print_usage(argv);

    const char *filename = argv[optind];
    check_fileextension(filename, "spx");
    set_filenames(filename, "spx");

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
// TODO: fix gen types
// TODO: char literal





int main(int argc, char *argv[]) {

    parse_args(argc, argv);

    char *file = read_file(compiler_context.filename.raw);

    printf("Tokenizing...\n");
    TokenList tokens = tokenize(file);
    if (compiler_context.opts.dump_tokens)
        tokenlist_print(&tokens);

    printf("Parsing...\n");
    AstNode *node_root = parser_parse(&tokens);
    if (compiler_context.opts.dump_ast)
        parser_print_ast(node_root);

    printf("Building Symboltable...\n");
    Symboltable symboltable = symboltable_construct(node_root, 5);
    if (compiler_context.opts.dump_symboltable)
        symboltable_print(&symboltable);

#if 1
    printf("Checking Types...\n");
    check_types(node_root);
#endif
    printf("Generating Code...\n");
    generate_code(node_root);

    printf("Building Binary...\n");
    build_binary();

    printf("%s%sBinary `%s` has been built!%s\n", COLOR_BOLD, COLOR_BLUE, compiler_context.filename.stripped, COLOR_END);






    symboltable_destroy(&symboltable);
    parser_free_ast(node_root);
    tokenlist_destroy(&tokens);
    free(file);

    return EXIT_SUCCESS;
}
