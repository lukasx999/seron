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

#include "util.h"
#include "lexer.h"
#include "parser.h"
#include "types.h"
#include "backend.h"
#include "symboltable.h"
#include "seronc.h"



struct CompilerContext compiler_context = { 0 };




static void check_fileextension(const char *filename, const char *extension) {

    if (strlen(filename) <= strlen(extension) + 1) { // eg: `.___`
        compiler_message(MSG_ERROR, "Invalid filename `%s`", filename);
        exit(1);
    }

    size_t dot_offset = strlen(filename) - 1 - strlen(extension);

    if (filename[dot_offset] != '.') {
        compiler_message(MSG_ERROR, "File extension missing");
        exit(1);
    }

    if (strncmp(filename + dot_offset + 1, extension, strlen(extension))) {
        compiler_message(MSG_ERROR, "File extension must be `.%s`", extension);
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

// Returns non-zero if process failed to run
static int run_cmd_sync(char *const argv[]) {
    int magic = 55;
    int status;

    // TODO: get exit status
    // dont try link if assembly fails
    if (!fork()) {
        execvp(argv[0], argv);
        exit(magic); // Failed to exec
    }

    wait(&status);
    return WEXITSTATUS(status) == magic;
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
        compiler_message(MSG_ERROR, "`nasm` not found in $PATH");
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
        compiler_message(MSG_ERROR, "`cc` not found in $PATH");
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

static void set_filenames(const char *raw, const char *extension) {
    compiler_context.filename.raw = raw;

    size_t dot_offset = strlen(raw) - 1 - strlen(extension);

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
    check_fileextension(filename, "srn");
    set_filenames(filename, "srn");

}


// TODO: analysis: dont allow statements in global scope
// TODO: synchronizing parser
// TODO: synchronizing typechecker
// TODO: change Token in ast to Token*
// TODO: pointers (addressof)
// TODO: ABI: spill arguments onto stack
// TODO: char literal
// TODO: id into tokenlist instead of pointer for ast
// TODO: arena allocator for astnodes
// TODO: var declaration address
// TODO: precompute stack frame layout

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


    compiler_message(MSG_INFO, "Parsing");
    AstNode *node_root = parser_parse(&tokens);
    if (compiler_context.opts.dump_ast)
        parser_print_ast(node_root, 2);


    compiler_message(MSG_INFO, "Constructing Symboltable");
    Symboltable symboltable = symboltable_construct(node_root, 5);
    if (compiler_context.opts.dump_symboltable)
        symboltable_print(&symboltable);


    compiler_message(MSG_INFO, "Typechecking");
    check_types(node_root);

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

    symboltable_destroy(&symboltable);
    parser_free_ast(node_root);
    tokenlist_destroy(&tokens);

    return EXIT_SUCCESS;
}
