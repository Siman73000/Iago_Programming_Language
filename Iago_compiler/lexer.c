#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

typedef struct TrieNode {
    struct TrieNode* children[128];
    int is_terminal;
    TokenType token_type;
} TrieNode;

static TrieNode* fixed_root = NULL;
static TrieNode* keyword_root = NULL;
static int trie_initialized = 0;

static char* dup_range(const char* start, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static char* dup_cstr(const char* s) {
    return dup_range(s, strlen(s));
}

static TrieNode* trie_create_node(void) {
    TrieNode* node = (TrieNode*)calloc(1, sizeof(TrieNode));
    if (!node) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
    return node;
}

static void trie_insert(TrieNode* root, const char* text, TokenType type) {
    TrieNode* cur = root;

    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c >= 128) {
            fprintf(stderr, "Non-ASCII trie token not supported: %s\n", text);
            exit(EXIT_FAILURE);
        }

        if (!cur->children[c]) {
            cur->children[c] = trie_create_node();
        }
        cur = cur->children[c];
    }

    cur->is_terminal = 1;
    cur->token_type = type;
}

static TokenType trie_match_exact(TrieNode* root, const char* text) {
    TrieNode* cur = root;

    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c >= 128 || !cur->children[c]) {
            return TOKEN_IDENTIFIER;
        }
        cur = cur->children[c];
    }

    if (cur->is_terminal) {
        return cur->token_type;
    }

    return TOKEN_IDENTIFIER;
}

static int trie_match_longest(TrieNode* root, const char* src, int start, Token* out) {
    TrieNode* cur = root;
    int i = start;
    int last_match_end = -1;
    TokenType last_match_type = TOKEN_UNKNOWN;

    while (src[i] != '\0') {
        unsigned char c = (unsigned char)src[i];
        if (c >= 128 || !cur->children[c]) {
            break;
        }

        cur = cur->children[c];
        i++;

        if (cur->is_terminal) {
            last_match_end = i;
            last_match_type = cur->token_type;
        }
    }

    if (last_match_end != -1) {
        out->type = last_match_type;
        out->text = dup_range(&src[start], (size_t)(last_match_end - start));
        return last_match_end - start;
    }

    return 0;
}

static void init_tries(void) {
    if (trie_initialized) {
        return;
    }

    fixed_root = trie_create_node();
    keyword_root = trie_create_node();

    /* Fixed tokens */
    trie_insert(fixed_root, "@use:", TOKEN_AT_USE);
    trie_insert(fixed_root, "@import:", TOKEN_AT_IMPORT);
    trie_insert(fixed_root, "@define:", TOKEN_AT_DEFINE);
    trie_insert(fixed_root, "@return", TOKEN_AT_RETURN);

    trie_insert(fixed_root, "=>", TOKEN_FAT_ARROW);
    trie_insert(fixed_root, "::", TOKEN_DOUBLE_COLON);
    trie_insert(fixed_root, "<-", TOKEN_ARROW_ASSIGN);
    trie_insert(fixed_root, "->", TOKEN_ARROW);
    trie_insert(fixed_root, "++", TOKEN_PLUSPLUS);

    trie_insert(fixed_root, "(", TOKEN_LPAREN);
    trie_insert(fixed_root, ")", TOKEN_RPAREN);
    trie_insert(fixed_root, "{", TOKEN_LBRACE);
    trie_insert(fixed_root, "}", TOKEN_RBRACE);
    trie_insert(fixed_root, "[", TOKEN_LBRACKET);
    trie_insert(fixed_root, "]", TOKEN_RBRACKET);
    trie_insert(fixed_root, ",", TOKEN_COMMA);
    trie_insert(fixed_root, ";", TOKEN_SEMICOLON);
    trie_insert(fixed_root, ":", TOKEN_COLON);
    trie_insert(fixed_root, "-", TOKEN_MINUS);
    trie_insert(fixed_root, "=", TOKEN_EQUAL);
    trie_insert(fixed_root, ".", TOKEN_DOT);
    trie_insert(fixed_root, "#", TOKEN_HASH);
    trie_insert(fixed_root, "$", TOKEN_DOLLAR);

    /* Keywords */
    trie_insert(keyword_root, "as", TOKEN_AS);
    trie_insert(keyword_root, "int", TOKEN_TYPE_INT);
    trie_insert(keyword_root, "str", TOKEN_TYPE_STR);
    trie_insert(keyword_root, "float", TOKEN_TYPE_FLOAT);
    trie_insert(keyword_root, "func", TOKEN_FUNC);
    trie_insert(keyword_root, "match", TOKEN_MATCH);
    trie_insert(keyword_root, "for", TOKEN_FOR);
    trie_insert(keyword_root, "_", TOKEN_UNDERSCORE);

    trie_initialized = 1;
}

static Token lex_number(const char* src, int* index) {
    int start = *index;
    int seen_dot = 0;

    while (isdigit((unsigned char)src[*index]) || (!seen_dot && src[*index] == '.')) {
        if (src[*index] == '.') {
            seen_dot = 1;
        }
        (*index)++;
    }

    return (Token){
        TOKEN_NUMBER,
        dup_range(&src[start], (size_t)(*index - start))
    };
}

static Token lex_string(const char* src, int* index) {
    int start;

    (*index)++; /* skip opening quote */
    start = *index;

    while (src[*index] != '"' && src[*index] != '\0') {
        if (src[*index] == '\\' && src[*index + 1] != '\0') {
            *index += 2;
        } else {
            (*index)++;
        }
    }

    {
        char* text = dup_range(&src[start], (size_t)(*index - start));
        if (src[*index] == '"') {
            (*index)++;
        }
        return (Token){ TOKEN_STRING, text };
    }
}

static Token lex_angle_string(const char* src, int* index) {
    int start;

    (*index)++; /* skip '<' */
    start = *index;

    while (src[*index] != '>' && src[*index] != '\0') {
        (*index)++;
    }

    if (src[*index] == '>') {
        char* text = dup_range(&src[start], (size_t)(*index - start));
        (*index)++; /* skip '>' */
        return (Token){ TOKEN_ANGLE_STRING, text };
    }

    return (Token){ TOKEN_UNKNOWN, dup_cstr("<") };
}

static Token lex_identifier_or_keyword(const char* src, int* index) {
    int start = *index;

    while (isalnum((unsigned char)src[*index]) || src[*index] == '_') {
        (*index)++;
    }

    {
        char* word = dup_range(&src[start], (size_t)(*index - start));
        TokenType type = trie_match_exact(keyword_root, word);
        return (Token){ type, word };
    }
}

Token next_token(const char* src, int* index) {
    Token tok;
    int consumed;

    init_tries();

    while (1) {
        while (isspace((unsigned char)src[*index])) {
            (*index)++;
        }

        if (src[*index] == '\\' && src[*index + 1] == '\\') {
            *index += 2;
            while (src[*index] != '\n' && src[*index] != '\0') {
                (*index)++;
            }
            continue;
        }

        break;
    }

    if (src[*index] == '\0') {
        return (Token){ TOKEN_EOF, NULL };
    }

    if (isalpha((unsigned char)src[*index]) || src[*index] == '_') {
        return lex_identifier_or_keyword(src, index);
    }

    if (isdigit((unsigned char)src[*index])) {
        return lex_number(src, index);
    }

    if (src[*index] == '"') {
        return lex_string(src, index);
    }

    consumed = trie_match_longest(fixed_root, src, *index, &tok);
    if (consumed > 0) {
        *index += consumed;
        return tok;
    }

    if (src[*index] == '<') {
        return lex_angle_string(src, index);
    }

    {
        char bad[2] = { src[*index], '\0' };
        (*index)++;
        return (Token){ TOKEN_UNKNOWN, dup_cstr(bad) };
    }
}