#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

static char* read_file(const char* path) {
    FILE* f;
    long size;
    size_t bytes_read;
    char* buffer;

    f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return NULL;
    }

    size = ftell(f);
    if (size < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }

    rewind(f);

    buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "Out of memory while reading file\n");
        fclose(f);
        return NULL;
    }

    bytes_read = fread(buffer, 1, (size_t)size, f);
    fclose(f);

    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Failed to read entire file\n");
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

int main(int argc, char** argv) {
    char* source;
    ASTNode* ast;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.iago>\n", argv[0]);
        return 1;
    }

    source = read_file(argv[1]);
    if (!source) {
        return 1;
    }

    ast = parse_source(source);
    if (!ast) {
        fprintf(stderr, "Parse failed.\n");
        free(source);
        return 1;
    }

    printf("=== AST ===\n");
    print_ast(ast, 0);

    free_ast(ast);
    free(source);
    return 0;
}