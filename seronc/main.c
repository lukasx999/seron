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
#include <arena.h>

#include "diagnostics.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "symboltable.h"
#include "main.h"


#define FILE_EXTENSION "sn"


struct CompilerContext compiler_ctx = { 0 };


static void check_fileextension(const char *filename) {

    if (strlen(filename) <= strlen(FILE_EXTENSION) + 1) { // eg: `.___`
        diagnostic(DIAG_ERROR, "Invalid filename `%s`", filename);
        exit(1);
    }

    size_t dot_offset = strlen(filename) - 1 - strlen(FILE_EXTENSION);

    if (filename[dot_offset] != '.') {
        diagnostic(DIAG_ERROR, "File extension missing");
        exit(1);
    }

    if (strncmp(filename + dot_offset + 1, FILE_EXTENSION, strlen(FILE_EXTENSION))) {
        diagnostic(DIAG_ERROR, "File extension must be `.%s`", FILE_EXTENSION);
        exit(1);
    }

}

static char *read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        diagnostic(DIAG_ERROR, "Source file `%s` does not exist", filename);
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
    char *asm_ = compiler_ctx.filename.asm_;
    char *obj  = compiler_ctx.filename.obj;

    int ret = run_cmd_sync((char*[]) {
        "nasm",
        asm_,
        "-felf64",
        "-o", obj,
        "-gdwarf",
        NULL
    });

    if (ret) {
        diagnostic(DIAG_ERROR, "Failed to assemble via `nasm` (is nasm installed?)");
        exit(EXIT_FAILURE);
    }

}

static void link_cc(void) {
    char *obj       = compiler_ctx.filename.obj;
    const char *bin = compiler_ctx.filename.stripped;

    int ret = run_cmd_sync((char*[]) {
        "cc",
        "-no-pie",
        "-lc",
        "-lraylib",
        obj,
        "-o", (char*) bin,
        NULL
    });

    if (ret) {
        diagnostic(DIAG_ERROR, "Failed to link via `cc`");
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
    compiler_ctx.filename.raw = raw;

    size_t dot_offset = strlen(raw) - 1 - strlen(FILE_EXTENSION);

    char *stripped = compiler_ctx.filename.stripped;
    strncpy(stripped, raw, dot_offset);

    char *asm = compiler_ctx.filename.asm_;
    strncpy(asm, stripped, NAME_MAX);
    strcat(asm, ".s");

    char *obj = compiler_ctx.filename.obj;
    strncpy(obj, stripped, NAME_MAX);
    strcat(obj, ".o");
}

static void parse_args(int argc, char **argv) {

    int opt_index = 0;
    struct option opts[] = {
        { "dump-ast",         no_argument, &compiler_ctx.opts.dump_ast,         1 },
        { "dump-tokens",      no_argument, &compiler_ctx.opts.dump_tokens,      1 },
        { "dump-symboltable", no_argument, &compiler_ctx.opts.dump_symboltable, 1 },
        { "verbose",          no_argument, &compiler_ctx.opts.verbose,          1 },
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

            case 'v': compiler_ctx.opts.verbose              = 1; break;
            case 'S': compiler_ctx.opts.compile_only         = 1; break;
            case 'c': compiler_ctx.opts.compile_and_assemble = 1; break;

            default:
                diagnostic(DIAG_ERROR, "Unknown option");
                exit(EXIT_FAILURE);
        }

    }

    if (optind >= argc)
        print_usage(argv);

    const char *filename = argv[optind];
    check_fileextension(filename);
    set_filenames(filename);
}



// TODO: get tokens from FILE* stream
// TODO: refactor parser to library, so it can be reused by lsp-server

int main(int argc, char **argv) {

    parse_args(argc, argv);

    // TODO: let the lexer use the FILE* stream directly
    char *file = read_file(compiler_ctx.filename.raw);
    compiler_ctx.src = file;

    if (compiler_ctx.opts.dump_tokens)
        lexer_print_tokens(file);

    Arena arena = { 0 };
    arena_init(&arena);

    AstNode *root = parse(file, &arena);

    if (compiler_ctx.opts.dump_ast)
        parser_print_ast(root, 2);

    symboltable_build(root, &arena);

    // check_types(node_root);
    codegen(root);

    if (!compiler_ctx.opts.compile_only) {
        assemble();

        if (!compiler_ctx.opts.compile_and_assemble)
            link_cc();

    }

    arena_free(&arena);
    free(file);

    return EXIT_SUCCESS;
}
