#ifndef PARSER_H
#define PARSER_H


typedef enum {
    AST_VAR_DECL,
    AST_PRINT,
    AST_BINARY_EXPR,
    AST_NUMBER,
    AST_IDENTIFIER
} ASTNodeType;


typedef struct ASTNode {
    ASTNodeType type;
    struct ASTNode* left;
    struct ASTNode* right;
    char* name;
    int value;
} ASTNode;





#endif