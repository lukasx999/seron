#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <libgen.h>
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
#define TEMP_DIR "/tmp/seron"


struct CompilerContext compiler_ctx = { 0 };




typedef enum {
    TARGET_BINARY,
    TARGET_ASSEMBLY,
    TARGET_OBJECT,
    TARGET_RUN,
} CompilationTarget;

typedef struct {
    CompilationTarget target;
    // options are ints, because `struct option` only accept int pointers
    struct {
        int dump_ast;
        int dump_tokens;
        int dump_symboltable;
    } opts;
} CompilerOptions;

static CompilerOptions compiler_opts_default(void) {
    return (CompilerOptions) {
        .target = TARGET_RUN,
    };
}





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

    char *buf = NON_NULL(calloc(size + 1, sizeof(char)));
    fread(buf, sizeof(char), size, file);

    fclose(file);
    return buf;
}

static int run_cmd_sync(const char *const argv[]) {
    int status;

    if (!fork()) {
        execvp(argv[0], (char *const*) argv);
        exit(1); // Failed to exec
    }

    wait(&status);

    return WIFEXITED(status)
    ? WEXITSTATUS(status)
    : 1;
}

static void assemble(const char *asm_, const char *obj) {

    int ret = run_cmd_sync((const char*[]) {
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

static void link_cc(const char *obj, const char *bin) {

    int ret = run_cmd_sync((const char*[]) {
        "cc",
        "-no-pie",
        "-lc",
        obj,
        "-o",
        bin,
        NULL,
    });

    if (ret) {
        diagnostic(DIAG_ERROR, "Failed to link via `cc`");
        exit(EXIT_FAILURE);
    }

}

static void run(const char *bin) {

    int ret = run_cmd_sync((const char*[]) {
        bin,
        NULL,
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
            "\t-t, --target                    select compilation target\n"
            "\t\tbin, asm, obj, run\n"
            "\t--dump-ast\n"
            "\t--dump-tokens\n"
            "\t--dump-symboltable\n"
            );
    exit(EXIT_FAILURE);
}


static CompilerOptions parse_args(int argc, char **argv) {

    CompilerOptions opts = compiler_opts_default();

    int opt_index = 0;
    struct option options[] = {
        { "dump-ast",         no_argument,       &opts.opts.dump_ast,         1 },
        { "dump-tokens",      no_argument,       &opts.opts.dump_tokens,      1 },
        { "dump-symboltable", no_argument,       &opts.opts.dump_symboltable, 1 },
        // TODO:
        // { "target",           required_argument, &compiler_ctx.opts.dump_symboltable, 1 },
        { NULL, 0, NULL, 0 },
    };

    while (1) {
        int c = getopt_long(argc, argv, "t:", options, &opt_index);

        if (c == -1)
            break;

        switch (c) {
            case 0:
                /* long option */
                break;

            case 't':

                if (!strcmp(optarg, "bin")) {
                    opts.target = TARGET_BINARY;

                } else if (!strcmp(optarg, "obj")) {
                    opts.target = TARGET_OBJECT;

                } else if (!strcmp(optarg, "asm")) {
                    opts.target = TARGET_ASSEMBLY;

                } else if (!strcmp(optarg, "run")) {
                    opts.target = TARGET_RUN;

                } else {
                    diagnostic(DIAG_ERROR, "Unknown target");
                    exit(EXIT_FAILURE);
                }

                break;

            default:
                diagnostic(DIAG_ERROR, "Unknown option");
                exit(EXIT_FAILURE);
        }

    }

    if (optind >= argc)
        print_usage(argv);

    const char *filename = argv[optind];
    check_fileextension(filename);
    compiler_ctx.filename = filename;

    return opts;
}

static void dispatch(AstNode *root, CompilerOptions opts) {

    // damn.

    const char *filename = compiler_ctx.filename;

    char buf[PATH_MAX] = { 0 };
    strncpy(buf, filename, ARRAY_LEN(buf)-1);
    char *base = basename(buf);

    char buf2[PATH_MAX] = { 0 };
    strncpy(buf2, filename, ARRAY_LEN(buf2)-1);
    char *dir = dirname(buf2);

    size_t dot_offset = strlen(base) - 1 - strlen(FILE_EXTENSION);

    char base_bin[NAME_MAX] = { 0 };
    strncpy(base_bin, base, dot_offset);

    char base_asm[NAME_MAX] = { 0 };
    strncpy(base_asm, base_bin, ARRAY_LEN(base_asm)-1);
    strcat(base_asm, ".s");

    char base_obj[NAME_MAX] = { 0 };
    strncpy(base_obj, base_bin, ARRAY_LEN(base_obj)-1);
    strcat(base_obj, ".o");

    mkdir(TEMP_DIR, 0777);

    char tmp_bin[PATH_MAX] = TEMP_DIR;
    strncat(tmp_bin, base_bin, ARRAY_LEN(tmp_bin)-1);

    char tmp_asm[PATH_MAX] = TEMP_DIR;
    strncat(tmp_asm, base_asm, ARRAY_LEN(tmp_asm)-1);

    char tmp_obj[PATH_MAX] = TEMP_DIR;
    strncat(tmp_obj, base_obj, ARRAY_LEN(tmp_obj)-1);

    char rel_bin[PATH_MAX] = { 0 };
    snprintf(rel_bin, ARRAY_LEN(rel_bin), "%s/%s", dir, base_bin);

    char rel_asm[PATH_MAX] = { 0 };
    snprintf(rel_asm, ARRAY_LEN(rel_asm), "%s/%s", dir, base_asm);

    char rel_obj[PATH_MAX] = { 0 };
    snprintf(rel_obj, ARRAY_LEN(rel_obj), "%s/%s", dir, base_obj);

    switch (opts.target) {
        case TARGET_BINARY:
            codegen(root, tmp_asm);
            assemble(tmp_asm, tmp_obj);
            link_cc(tmp_obj, rel_bin);
            break;

        case TARGET_OBJECT:
            codegen(root, tmp_asm);
            assemble(tmp_asm, rel_obj);
            break;

        case TARGET_ASSEMBLY:
            codegen(root, rel_asm);
            break;

        case TARGET_RUN:
            codegen(root, tmp_asm);
            assemble(tmp_asm, tmp_obj);
            link_cc(tmp_obj, tmp_bin);
            run(tmp_bin);
            break;
    }

}

int main(int argc, char **argv) {
    compiler_opts_default();

    CompilerOptions opts = parse_args(argc, argv);

    char *file = read_file(compiler_ctx.filename);
    compiler_ctx.src = file;

    if (opts.opts.dump_tokens)
        lexer_print_tokens(file);

    Arena arena = { 0 };
    arena_init(&arena);

    AstNode *root = parse(file, &arena);

    if (opts.opts.dump_ast)
        parser_print_ast(root, 2);

    symboltable_build(root, &arena);

    dispatch(root, opts);

    arena_free(&arena);
    free(file);

    return EXIT_SUCCESS;
}
