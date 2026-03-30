#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNC_DECL,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_BINDING,
    AST_RETURN,
    AST_ASSIGN,
    AST_EXPR_STMT,
    AST_CALL,
    AST_ACCESS,
    AST_INDEX,
    AST_MATCH,
    AST_MATCH_CASE,
    AST_FOR,
    AST_NUMBER,
    AST_STRING,
    AST_IDENTIFIER
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    TokenType data_type;
    char* text;
    struct ASTNode** children;
    size_t child_count;
    size_t child_capacity;
} ASTNode;

ASTNode* parse_source(const char* src);
void print_ast(const ASTNode* node, int indent);
void free_ast(ASTNode* node);

#endif