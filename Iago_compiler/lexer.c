#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"


Token next_token(const char* src, int* index) {
    while (isspace(src[*index])) (*index)++;

    if (src[*index] == '\0') return (Token){ TOKEN_EOF, NULL };

    if (src[*index] == '@') {
        if (strncmp(&src[*index], "@use:", 5) == 0) {
            *index += 5;
            return (Token){ TOKEN_AT_USE, strdup("@use:") };
        } else if (strncmp(&src[*index], "@import:", 8) == 0) {
            *index += 8;
            return (Token){ TOKEN_AT_IMPORT, strdup("@import:") };
        } else if (strncmp(&src[*index], "@define:", 8) == 0) {
            *index += 8;
            return (Token){ TOKEN_AT_DEFINE, strdup("@define:") };
        } else if (strncmp(&src[*index], "@return", 7) == 0) {
            *index += 7;
            return (Token){ TOKEN_AT_RETURN, strdup("@return") };
        }
    }

    if (src[*index] == '<') {
        int start = ++(*index);
        while (src[*index] != '>' && src[*index] != '\0') (*index)++;
        if (src[*index] == '>') {
            int len = *index - start;
            char* content = strndup(&src[start], len);
            (*index)++;
            return (Token){ TOKEN_ANGLE_STRING, content };
        }
    }

    if (src[*index] == '=' && src[*index + 1] == '>') {
        *index += 2;
        return (Token){ TOKEN_FAT_ARROW, strdup("=>") };
    }

    if (src[*index] == ':' && src[*index + 1] == ':') {
        *index += 2;
        return (Token){ TOKEN_DOUBLE_COLON, strdup("::") };
    }

    if (src[*index] == '<' && src[*index + 1] == '-') {
        *index += 2;
        return (Token){ TOKEN_ARROW_ASSIGN, strdup("<-") };
    }

    if (src[*index] == '(') { (*index)++; return (Token){ TOKEN_LPAREN, strdup("(") }; }
    if (src[*index] == ')') { (*index)++; return (Token){ TOKEN_RPAREN, strdup(")") }; }
    if (src[*index] == '{') { (*index)++; return (Token){ TOKEN_LBRACE, strdup("{") }; }
    if (src[*index] == '}') { (*index)++; return (Token){ TOKEN_RBRACE, strdup("}") }; }
    if (src[*index] == ',') { (*index)++; return (Token){ TOKEN_COMMA, strdup(",") }; }
    if (src[*index] == ';') { (*index)++; return (Token){ TOKEN_SEMICOLON, strdup(";") }; }

    if (isdigit(src[*index])) {
        int start = *index;
        while (isdigit(src[*index]) || src[*index] == '.') (*index)++;
        char* num = strndup(&src[start], *index - start);
        return (Token){ TOKEN_NUMBER, num };
    }

    if (src[*index] == '"') {
        int start = ++(*index);
        while (src[*index] != '"' && src[*index] != '\0') (*index)++;
        char* str = strndup(&src[start], *index - start);
        if (src[*index] == '"') (*index)++;
        return (Token){ TOKEN_STRING, str };
    }

    if (isalpha(src[*index])) {
        int start = *index;
        while (isalnum(src[*index]) || src[*index] == '_') (*index)++;
        char* word = strndup(&src[start], *index - start);

        if (strcmp(word, "as") == 0) return (Token){ TOKEN_AS, word };
        if (strcmp(word, "int") == 0) return (Token){ TOKEN_TYPE_INT, word };
        if (strcmp(word, "str") == 0) return (Token){ TOKEN_TYPE_STR, word };
        if (strcmp(word, "float") == 0) return (Token){ TOKEN_TYPE_FLOAT, word };
        if (strcmp(word, "func") == 0) return (Token){ TOKEN_FUNC, word };
        if (strcmp(word, "match") == 0) return (Token){ TOKEN_MATCH, word };

        return (Token){ TOKEN_IDENTIFIER, word };
    }

    (*index)++;
    return (Token){ TOKEN_UNKNOWN, strndup(&src[*index - 1], 1) };
}
