#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"

typedef struct {
    Token* tokens;
    int count;
    int capacity;
    int pos;
} Parser;

static char* dup_cstr(const char* s) {
    size_t len;
    char* out;

    if (!s) {
        return NULL;
    }

    len = strlen(s);
    out = (char*)malloc(len + 1);
    if (!out) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    memcpy(out, s, len + 1);
    return out;
}

static const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_AT_USE: return "AT_USE";
        case TOKEN_AT_IMPORT: return "AT_IMPORT";
        case TOKEN_AT_DEFINE: return "AT_DEFINE";
        case TOKEN_AT_RETURN: return "AT_RETURN";
        case TOKEN_AS: return "AS";
        case TOKEN_TYPE_INT: return "TYPE_INT";
        case TOKEN_TYPE_STR: return "TYPE_STR";
        case TOKEN_TYPE_FLOAT: return "TYPE_FLOAT";
        case TOKEN_FUNC: return "FUNC";
        case TOKEN_ARROW_ASSIGN: return "ARROW_ASSIGN";
        case TOKEN_DOUBLE_COLON: return "DOUBLE_COLON";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COLON: return "COLON";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_EQUAL: return "EQUAL";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_DOT: return "DOT";
        case TOKEN_PLUSPLUS: return "PLUSPLUS";
        case TOKEN_STRING: return "STRING";
        case TOKEN_ANGLE_STRING: return "ANGLE_STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_MATCH: return "MATCH";
        case TOKEN_FOR: return "FOR";
        case TOKEN_UNDERSCORE: return "UNDERSCORE";
        case TOKEN_HASH: return "HASH";
        case TOKEN_DOLLAR: return "DOLLAR";
        case TOKEN_FAT_ARROW: return "FAT_ARROW";
        case TOKEN_UNKNOWN: return "UNKNOWN";
        default: return "INVALID_TOKEN";
    }
}

static const char* ast_type_name(ASTNodeType type) {
    switch (type) {
        case AST_PROGRAM: return "PROGRAM";
        case AST_FUNC_DECL: return "FUNC_DECL";
        case AST_BLOCK: return "BLOCK";
        case AST_VAR_DECL: return "VAR_DECL";
        case AST_BINDING: return "BINDING";
        case AST_RETURN: return "RETURN";
        case AST_ASSIGN: return "ASSIGN";
        case AST_EXPR_STMT: return "EXPR_STMT";
        case AST_CALL: return "CALL";
        case AST_ACCESS: return "ACCESS";
        case AST_INDEX: return "INDEX";
        case AST_MATCH: return "MATCH";
        case AST_MATCH_CASE: return "MATCH_CASE";
        case AST_FOR: return "FOR";
        case AST_NUMBER: return "NUMBER";
        case AST_STRING: return "STRING";
        case AST_IDENTIFIER: return "IDENTIFIER";
        default: return "UNKNOWN_AST";
    }
}

static ASTNode* ast_new(ASTNodeType type) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    node->type = type;
    node->data_type = TOKEN_UNKNOWN;
    node->text = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    return node;
}

static void ast_add_child(ASTNode* parent, ASTNode* child) {
    ASTNode** new_children;
    size_t new_capacity;

    if (!parent || !child) {
        return;
    }

    if (parent->child_count == parent->child_capacity) {
        new_capacity = (parent->child_capacity == 0) ? 4 : parent->child_capacity * 2;
        new_children = (ASTNode**)realloc(parent->children, new_capacity * sizeof(ASTNode*));
        if (!new_children) {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
        }
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }

    parent->children[parent->child_count++] = child;
}

static void parser_add_token(Parser* p, Token token) {
    Token* new_tokens;
    int new_capacity;

    if (p->count == p->capacity) {
        new_capacity = (p->capacity == 0) ? 64 : p->capacity * 2;
        new_tokens = (Token*)realloc(p->tokens, (size_t)new_capacity * sizeof(Token));
        if (!new_tokens) {
            fprintf(stderr, "Out of memory\n");
            exit(EXIT_FAILURE);
        }
        p->tokens = new_tokens;
        p->capacity = new_capacity;
    }

    p->tokens[p->count++] = token;
}

static void tokenize_source(Parser* p, const char* src) {
    int index = 0;

    while (1) {
        Token tok = next_token(src, &index);
        parser_add_token(p, tok);
        if (tok.type == TOKEN_EOF) {
            break;
        }
    }
}

static void parser_destroy(Parser* p) {
    int i;

    for (i = 0; i < p->count; i++) {
        free(p->tokens[i].text);
    }

    free(p->tokens);
    p->tokens = NULL;
    p->count = 0;
    p->capacity = 0;
    p->pos = 0;
}

static Token* current(Parser* p) {
    if (p->pos >= p->count) {
        return &p->tokens[p->count - 1];
    }
    return &p->tokens[p->pos];
}

static Token* peek(Parser* p, int offset) {
    int idx = p->pos + offset;
    if (idx >= p->count) {
        return &p->tokens[p->count - 1];
    }
    return &p->tokens[idx];
}

static int is_at_end(Parser* p) {
    return current(p)->type == TOKEN_EOF;
}

static Token* advance_token(Parser* p) {
    if (!is_at_end(p)) {
        p->pos++;
    }
    return &p->tokens[p->pos - 1];
}

static int check(Parser* p, TokenType type) {
    return current(p)->type == type;
}

static int match(Parser* p, TokenType type) {
    if (check(p, type)) {
        advance_token(p);
        return 1;
    }
    return 0;
}

static int is_type_token(TokenType type) {
    return type == TOKEN_TYPE_INT ||
           type == TOKEN_TYPE_STR ||
           type == TOKEN_TYPE_FLOAT;
}

static void parse_error(Parser* p, const char* message) {
    Token* tok = current(p);
    fprintf(stderr, "Parse error near token %s", token_type_name(tok->type));
    if (tok->text) {
        fprintf(stderr, " ('%s')", tok->text);
    }
    fprintf(stderr, ": %s\n", message);
}

static Token* expect(Parser* p, TokenType type, const char* message) {
    Token* tok = current(p);
    if (tok->type != type) {
        parse_error(p, message);
        return NULL;
    }
    advance_token(p);
    return tok;
}

static void skip_directive(Parser* p) {
    if (match(p, TOKEN_AT_USE)) {
        if (check(p, TOKEN_ANGLE_STRING)) advance_token(p);
        if (match(p, TOKEN_AS)) {
            if (check(p, TOKEN_IDENTIFIER)) advance_token(p);
        }
        return;
    }

    if (match(p, TOKEN_AT_IMPORT)) {
        if (check(p, TOKEN_ANGLE_STRING)) advance_token(p);
        return;
    }

    if (match(p, TOKEN_AT_DEFINE)) {
        while (!is_at_end(p) && !match(p, TOKEN_SEMICOLON)) {
            advance_token(p);
        }
        return;
    }
}

static void skip_unknown_statement(Parser* p) {
    int brace_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;
    int saw_block = 0;

    fprintf(stderr, "Warning: skipping unsupported statement starting at %s",
            token_type_name(current(p)->type));
    if (current(p)->text) {
        fprintf(stderr, " ('%s')", current(p)->text);
    }
    fprintf(stderr, "\n");

    while (!is_at_end(p)) {
        TokenType t = current(p)->type;

        if (t == TOKEN_LBRACE) {
            saw_block = 1;
            brace_depth++;
            advance_token(p);
            continue;
        }

        if (t == TOKEN_RBRACE) {
            if (brace_depth > 0) {
                brace_depth--;
                advance_token(p);
                if (saw_block && brace_depth == 0 && paren_depth == 0 && bracket_depth == 0) {
                    break;
                }
                continue;
            }
            break;
        }

        if (t == TOKEN_LPAREN) {
            paren_depth++;
            advance_token(p);
            continue;
        }

        if (t == TOKEN_RPAREN) {
            if (paren_depth > 0) {
                paren_depth--;
            }
            advance_token(p);
            continue;
        }

        if (t == TOKEN_LBRACKET) {
            bracket_depth++;
            advance_token(p);
            continue;
        }

        if (t == TOKEN_RBRACKET) {
            if (bracket_depth > 0) {
                bracket_depth--;
            }
            advance_token(p);
            continue;
        }

        if (t == TOKEN_SEMICOLON && brace_depth == 0 && paren_depth == 0 && bracket_depth == 0) {
            advance_token(p);
            break;
        }

        advance_token(p);
    }
}

static int can_start_expr(TokenType type) {
    return type == TOKEN_NUMBER ||
           type == TOKEN_STRING ||
           type == TOKEN_IDENTIFIER ||
           type == TOKEN_HASH ||
           type == TOKEN_DOLLAR ||
           type == TOKEN_UNDERSCORE ||
           type == TOKEN_LPAREN;
}

static ASTNode* parse_expr(Parser* p);

static ASTNode* parse_binding(Parser* p) {
    Token* name_tok;
    ASTNode* binding;
    ASTNode* expr;

    name_tok = expect(p, TOKEN_IDENTIFIER, "Expected identifier in binding");
    if (!name_tok) {
        return NULL;
    }

    if (!expect(p, TOKEN_DOUBLE_COLON, "Expected '::' in binding")) {
        return NULL;
    }

    expr = parse_expr(p);
    if (!expr) {
        return NULL;
    }

    binding = ast_new(AST_BINDING);
    binding->text = dup_cstr(name_tok->text);
    ast_add_child(binding, expr);
    return binding;
}

static ASTNode* parse_var_decl(Parser* p) {
    Token* type_tok;
    ASTNode* node;
    ASTNode* binding;

    type_tok = current(p);
    if (!is_type_token(type_tok->type)) {
        parse_error(p, "Expected a type token");
        return NULL;
    }

    advance_token(p);

    if (!expect(p, TOKEN_EQUAL, "Expected '=' after type")) {
        return NULL;
    }

    node = ast_new(AST_VAR_DECL);
    node->data_type = type_tok->type;
    node->text = dup_cstr(type_tok->text);

    binding = parse_binding(p);
    if (!binding) {
        free_ast(node);
        return NULL;
    }
    ast_add_child(node, binding);

    while (match(p, TOKEN_COMMA)) {
        binding = parse_binding(p);
        if (!binding) {
            free_ast(node);
            return NULL;
        }
        ast_add_child(node, binding);
    }

    if (!expect(p, TOKEN_SEMICOLON, "Expected ';' after variable declaration")) {
        free_ast(node);
        return NULL;
    }

    return node;
}

static ASTNode* parse_return_stmt(Parser* p) {
    ASTNode* node;
    ASTNode* expr;

    if (!expect(p, TOKEN_AT_RETURN, "Expected '@return'")) {
        return NULL;
    }

    if (!expect(p, TOKEN_ARROW_ASSIGN, "Expected '<-' after '@return'")) {
        return NULL;
    }

    expr = parse_expr(p);
    if (!expr) {
        return NULL;
    }

    if (!expect(p, TOKEN_SEMICOLON, "Expected ';' after return statement")) {
        free_ast(expr);
        return NULL;
    }

    node = ast_new(AST_RETURN);
    ast_add_child(node, expr);
    return node;
}

static ASTNode* parse_primary(Parser* p) {
    Token* tok;
    ASTNode* node;
    ASTNode* inner;

    tok = current(p);

    if (tok->type == TOKEN_NUMBER) {
        advance_token(p);
        node = ast_new(AST_NUMBER);
        node->text = dup_cstr(tok->text);
        return node;
    }

    if (tok->type == TOKEN_STRING) {
        advance_token(p);
        node = ast_new(AST_STRING);
        node->text = dup_cstr(tok->text);
        return node;
    }

    if (tok->type == TOKEN_IDENTIFIER || tok->type == TOKEN_UNDERSCORE) {
        advance_token(p);
        node = ast_new(AST_IDENTIFIER);
        node->text = dup_cstr(tok->text);
        return node;
    }

    if (tok->type == TOKEN_HASH) {
        advance_token(p);
        node = ast_new(AST_IDENTIFIER);
        node->text = dup_cstr("#");
        return node;
    }

    if (tok->type == TOKEN_DOLLAR) {
        advance_token(p);
        node = ast_new(AST_IDENTIFIER);
        node->text = dup_cstr("$");
        return node;
    }

    if (match(p, TOKEN_LPAREN)) {
        inner = parse_expr(p);
        if (!inner) {
            return NULL;
        }
        if (!expect(p, TOKEN_RPAREN, "Expected ')' after parenthesized expression")) {
            free_ast(inner);
            return NULL;
        }
        return inner;
    }

    parse_error(p, "Expected primary expression");
    return NULL;
}

static ASTNode* parse_postfix(Parser* p) {
    ASTNode* expr;
    ASTNode* node;
    ASTNode* arg;
    Token* name_tok;

    expr = parse_primary(p);
    if (!expr) {
        return NULL;
    }

    while (1) {
        if (match(p, TOKEN_LPAREN)) {
            node = ast_new(AST_CALL);
            ast_add_child(node, expr);

            if (!check(p, TOKEN_RPAREN)) {
                arg = parse_expr(p);
                if (!arg) {
                    free_ast(node);
                    return NULL;
                }
                ast_add_child(node, arg);

                while (match(p, TOKEN_COMMA)) {
                    arg = parse_expr(p);
                    if (!arg) {
                        free_ast(node);
                        return NULL;
                    }
                    ast_add_child(node, arg);
                }
            }

            if (!expect(p, TOKEN_RPAREN, "Expected ')' after call arguments")) {
                free_ast(node);
                return NULL;
            }

            expr = node;
            continue;
        }

        if (match(p, TOKEN_ARROW)) {
            name_tok = expect(p, TOKEN_IDENTIFIER, "Expected identifier after '->'");
            if (!name_tok) {
                free_ast(expr);
                return NULL;
            }

            node = ast_new(AST_ACCESS);
            node->data_type = TOKEN_ARROW;
            node->text = dup_cstr(name_tok->text);
            ast_add_child(node, expr);
            expr = node;
            continue;
        }

        if (match(p, TOKEN_DOUBLE_COLON)) {
            if (check(p, TOKEN_IDENTIFIER) || check(p, TOKEN_DOLLAR) || check(p, TOKEN_UNDERSCORE)) {
                name_tok = current(p);
                advance_token(p);

                node = ast_new(AST_ACCESS);
                node->data_type = TOKEN_DOUBLE_COLON;
                node->text = dup_cstr(name_tok->text);
                ast_add_child(node, expr);
                expr = node;
                continue;
            }

            parse_error(p, "Expected identifier, '$', or '_' after '::'");
            free_ast(expr);
            return NULL;
        }

        if (match(p, TOKEN_LBRACKET)) {
            ASTNode* index_expr;

            index_expr = parse_expr(p);
            if (!index_expr) {
                free_ast(expr);
                return NULL;
            }

            if (!expect(p, TOKEN_RBRACKET, "Expected ']' after index expression")) {
                free_ast(expr);
                free_ast(index_expr);
                return NULL;
            }

            node = ast_new(AST_INDEX);
            ast_add_child(node, expr);
            ast_add_child(node, index_expr);
            expr = node;
            continue;
        }

        break;
    }

    return expr;
}

static ASTNode* parse_expr(Parser* p) {
    return parse_postfix(p);
}

static ASTNode* parse_assignment_stmt(Parser* p) {
    Token* name_tok;
    ASTNode* node;
    ASTNode* expr;

    name_tok = expect(p, TOKEN_IDENTIFIER, "Expected identifier on left side of assignment");
    if (!name_tok) {
        return NULL;
    }

    if (!expect(p, TOKEN_EQUAL, "Expected '=' in assignment")) {
        return NULL;
    }

    expr = parse_expr(p);
    if (!expr) {
        return NULL;
    }

    if (!expect(p, TOKEN_SEMICOLON, "Expected ';' after assignment")) {
        free_ast(expr);
        return NULL;
    }

    node = ast_new(AST_ASSIGN);
    node->text = dup_cstr(name_tok->text);
    ast_add_child(node, expr);
    return node;
}

static ASTNode* parse_expr_stmt(Parser* p) {
    ASTNode* node;
    ASTNode* expr;

    expr = parse_expr(p);
    if (!expr) {
        return NULL;
    }

    if (!expect(p, TOKEN_SEMICOLON, "Expected ';' after expression statement")) {
        free_ast(expr);
        return NULL;
    }

    node = ast_new(AST_EXPR_STMT);
    ast_add_child(node, expr);
    return node;
}

static ASTNode* parse_match_pattern(Parser* p) {
    Token* tok;
    ASTNode* node;

    tok = current(p);

    if (tok->type == TOKEN_NUMBER) {
        advance_token(p);
        node = ast_new(AST_NUMBER);
        node->text = dup_cstr(tok->text);
        return node;
    }

    if (tok->type == TOKEN_STRING) {
        advance_token(p);
        node = ast_new(AST_STRING);
        node->text = dup_cstr(tok->text);
        return node;
    }

    if (tok->type == TOKEN_IDENTIFIER || tok->type == TOKEN_UNDERSCORE) {
        advance_token(p);
        node = ast_new(AST_IDENTIFIER);
        node->text = dup_cstr(tok->text);
        return node;
    }

    parse_error(p, "Expected match pattern");
    return NULL;
}

static ASTNode* parse_match_stmt(Parser* p) {
    ASTNode* node;
    ASTNode* scrutinee;
    ASTNode* case_node;
    ASTNode* pattern;
    ASTNode* value;

    if (!expect(p, TOKEN_MATCH, "Expected 'match'")) {
        return NULL;
    }

    scrutinee = parse_expr(p);
    if (!scrutinee) {
        return NULL;
    }

    if (!expect(p, TOKEN_LBRACE, "Expected '{' after match expression")) {
        free_ast(scrutinee);
        return NULL;
    }

    node = ast_new(AST_MATCH);
    ast_add_child(node, scrutinee);

    while (!check(p, TOKEN_RBRACE) && !is_at_end(p)) {
        pattern = parse_match_pattern(p);
        if (!pattern) {
            free_ast(node);
            return NULL;
        }

        if (!expect(p, TOKEN_FAT_ARROW, "Expected '=>' in match case")) {
            free_ast(pattern);
            free_ast(node);
            return NULL;
        }

        value = parse_expr(p);
        if (!value) {
            free_ast(pattern);
            free_ast(node);
            return NULL;
        }

        case_node = ast_new(AST_MATCH_CASE);
        ast_add_child(case_node, pattern);
        ast_add_child(case_node, value);
        ast_add_child(node, case_node);

        if (match(p, TOKEN_COMMA)) {
            if (check(p, TOKEN_RBRACE)) {
                break;
            }
            continue;
        } else {
            break;
        }
    }

    if (!expect(p, TOKEN_RBRACE, "Expected '}' to close match")) {
        free_ast(node);
        return NULL;
    }

    return node;
}

static ASTNode* parse_for_stmt(Parser* p) {
    Token* type_tok;
    Token* iter_tok;
    Token* collection_tok;
    Token* update_tok;
    ASTNode* node;
    ASTNode* init_binding;
    ASTNode* start_expr;
    ASTNode* collection_node;
    ASTNode* update_node;
    ASTNode* body;

    if (!expect(p, TOKEN_FOR, "Expected 'for'")) {
        return NULL;
    }

    if (!expect(p, TOKEN_LPAREN, "Expected '(' after 'for'")) {
        return NULL;
    }

    type_tok = current(p);
    if (!is_type_token(type_tok->type)) {
        parse_error(p, "Expected loop variable type in for header");
        return NULL;
    }
    advance_token(p);

    iter_tok = expect(p, TOKEN_IDENTIFIER, "Expected loop variable name");
    if (!iter_tok) {
        return NULL;
    }

    if (!expect(p, TOKEN_DOUBLE_COLON, "Expected '::' in for initializer")) {
        return NULL;
    }

    start_expr = parse_expr(p);
    if (!start_expr) {
        return NULL;
    }

    if (!expect(p, TOKEN_COMMA, "Expected ',' after for initializer")) {
        free_ast(start_expr);
        return NULL;
    }

    collection_tok = expect(p, TOKEN_IDENTIFIER, "Expected collection identifier in for header");
    if (!collection_tok) {
        free_ast(start_expr);
        return NULL;
    }

    if (!expect(p, TOKEN_COMMA, "Expected ',' after collection identifier")) {
        free_ast(start_expr);
        return NULL;
    }

    update_tok = expect(p, TOKEN_IDENTIFIER, "Expected update variable in for header");
    if (!update_tok) {
        free_ast(start_expr);
        return NULL;
    }

    if (!expect(p, TOKEN_PLUSPLUS, "Expected '++' in for header")) {
        free_ast(start_expr);
        return NULL;
    }

    if (!expect(p, TOKEN_RPAREN, "Expected ')' after for header")) {
        free_ast(start_expr);
        return NULL;
    }

    body = parse_block(p);
    if (!body) {
        free_ast(start_expr);
        return NULL;
    }

    node = ast_new(AST_FOR);
    node->data_type = type_tok->type;
    node->text = dup_cstr(iter_tok->text);

    init_binding = ast_new(AST_BINDING);
    init_binding->text = dup_cstr(iter_tok->text);
    ast_add_child(init_binding, start_expr);

    collection_node = ast_new(AST_IDENTIFIER);
    collection_node->text = dup_cstr(collection_tok->text);

    update_node = ast_new(AST_IDENTIFIER);
    update_node->text = dup_cstr(update_tok->text);

    ast_add_child(node, init_binding);
    ast_add_child(node, collection_node);
    ast_add_child(node, update_node);
    ast_add_child(node, body);

    return node;
}

static ASTNode* parse_statement(Parser* p) {
    if (is_type_token(current(p)->type) && peek(p, 1)->type == TOKEN_EQUAL) {
        return parse_var_decl(p);
    }

    if (check(p, TOKEN_AT_RETURN)) {
        return parse_return_stmt(p);
    }

    if (check(p, TOKEN_MATCH)) {
        return parse_match_stmt(p);
    }

    if (check(p, TOKEN_FOR)) {
        return parse_for_stmt(p);
    }

    if (check(p, TOKEN_IDENTIFIER) && peek(p, 1)->type == TOKEN_EQUAL) {
        return parse_assignment_stmt(p);
    }

    if (can_start_expr(current(p)->type)) {
        return parse_expr_stmt(p);
    }

    skip_unknown_statement(p);
    return NULL;
}

static ASTNode* parse_block(Parser* p) {
    ASTNode* block;
    ASTNode* stmt;

    if (!expect(p, TOKEN_LBRACE, "Expected '{' to start block")) {
        return NULL;
    }

    block = ast_new(AST_BLOCK);

    while (!check(p, TOKEN_RBRACE) && !is_at_end(p)) {
        stmt = parse_statement(p);
        if (stmt) {
            ast_add_child(block, stmt);
        }
    }

    if (!expect(p, TOKEN_RBRACE, "Expected '}' to end block")) {
        free_ast(block);
        return NULL;
    }

    return block;
}

static ASTNode* parse_func_decl(Parser* p) {
    Token* return_type_tok;
    Token* name_tok;
    ASTNode* func;
    ASTNode* body;

    return_type_tok = current(p);
    if (!is_type_token(return_type_tok->type)) {
        parse_error(p, "Expected function return type");
        return NULL;
    }
    advance_token(p);

    if (!expect(p, TOKEN_FUNC, "Expected 'func' after function return type")) {
        return NULL;
    }

    name_tok = expect(p, TOKEN_IDENTIFIER, "Expected function name");
    if (!name_tok) {
        return NULL;
    }

    if (!expect(p, TOKEN_ARROW_ASSIGN, "Expected '<-' after function name")) {
        return NULL;
    }

    if (!expect(p, TOKEN_LPAREN, "Expected '(' after '<-'")) {
        return NULL;
    }

    if (!expect(p, TOKEN_RPAREN, "Expected ')' in empty parameter list")) {
        return NULL;
    }

    body = parse_block(p);
    if (!body) {
        return NULL;
    }

    func = ast_new(AST_FUNC_DECL);
    func->data_type = return_type_tok->type;
    func->text = dup_cstr(name_tok->text);
    ast_add_child(func, body);
    return func;
}

static ASTNode* parse_program(Parser* p) {
    ASTNode* program = ast_new(AST_PROGRAM);
    ASTNode* node;

    while (!is_at_end(p)) {
        if (check(p, TOKEN_AT_USE) || check(p, TOKEN_AT_IMPORT) || check(p, TOKEN_AT_DEFINE)) {
            skip_directive(p);
            continue;
        }

        if (is_type_token(current(p)->type) && peek(p, 1)->type == TOKEN_FUNC) {
            node = parse_func_decl(p);
            if (!node) {
                free_ast(program);
                return NULL;
            }
            ast_add_child(program, node);
            continue;
        }

        if (is_type_token(current(p)->type) && peek(p, 1)->type == TOKEN_EQUAL) {
            node = parse_var_decl(p);
            if (!node) {
                free_ast(program);
                return NULL;
            }
            ast_add_child(program, node);
            continue;
        }

        fprintf(stderr, "Warning: skipping unexpected top-level token %s",
                token_type_name(current(p)->type));
        if (current(p)->text) {
            fprintf(stderr, " ('%s')", current(p)->text);
        }
        fprintf(stderr, "\n");
        advance_token(p);
    }

    return program;
}

ASTNode* parse_source(const char* src) {
    Parser p;
    ASTNode* root;

    p.tokens = NULL;
    p.count = 0;
    p.capacity = 0;
    p.pos = 0;

    tokenize_source(&p, src);
    root = parse_program(&p);
    parser_destroy(&p);
    return root;
}

void print_ast(const ASTNode* node, int indent) {
    size_t i;

    if (!node) {
        return;
    }

    for (i = 0; i < (size_t)indent; i++) {
        printf("  ");
    }

    printf("%s", ast_type_name(node->type));

    if (node->text) {
        printf(" text=\"%s\"", node->text);
    }

    if (node->data_type != TOKEN_UNKNOWN) {
        printf(" data_type=%s", token_type_name(node->data_type));
    }

    printf("\n");

    for (i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], indent + 1);
    }
}

void free_ast(ASTNode* node) {
    size_t i;

    if (!node) {
        return;
    }

    for (i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }

    free(node->children);
    free(node->text);
    free(node);
}