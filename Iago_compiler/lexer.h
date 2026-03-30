#ifndef LEXER_H
#define LEXER_H


typedef enum {
    TOKEN_EOF,
    TOKEN_AT_USE,
    TOKEN_AT_IMPORT,
    TOKEN_AT_DEFINE,
    TOKEN_AT_RETURN,
    TOKEN_AS,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_STR,
    TOKEN_TYPE_FLOAT,
    TOKEN_FUNC,
    TOKEN_ARROW_ASSIGN,    // <-
    TOKEN_DOUBLE_COLON,    // ::
    TOKEN_LBRACKET,       // [
    TOKEN_RBRACKET,       // ]
    TOKEN_COLON,          // :
    TOKEN_MINUS,          // -
    TOKEN_EQUAL,          // =
    TOKEN_ARROW,          // ->
    TOKEN_DOT,            // .
    TOKEN_PLUSPLUS,       // ++
    TOKEN_FOR,            // for
    TOKEN_UNDERSCORE,     // _
    TOKEN_HASH,           // #
    TOKEN_DOLLAR,          // $
    TOKEN_LPAREN,          // (
    TOKEN_RPAREN,          // )
    TOKEN_LBRACE,          // {
    TOKEN_RBRACE,          // }
    TOKEN_COMMA,           // ,
    TOKEN_SEMICOLON,       // ;
    TOKEN_STRING,
    TOKEN_ANGLE_STRING,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_MATCH,
    TOKEN_FAT_ARROW,       // =>
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char* text;
} Token;

Token next_token(const char* src, int* index);


#endif