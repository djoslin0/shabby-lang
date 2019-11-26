#include <stdio.h>
#include "constants.h"

bool is_whitespace(uint8_t c) {
    return (c == ' ' || c == '\t')
        || (c == '\r' || c == '\n');
}

bool is_alpha(uint8_t c) {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z');
}

bool is_numeric(uint8_t c) {
    return (c >= '0' && c <= '9');
}

bool is_alphanumeric(uint8_t c) {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9');
}

bool is_math_op(uint8_t c) {
    return (c == '+' || c == '-' || c == '*' || c == '/')
        || (c == '|' || c == '&' || c == '^' || c == '%');
}

bool is_term_op(uint8_t c) {
    return (c == '*' || c == '/');
}

bool is_expression_op(uint8_t c) {
    return (c == '+' || c == '-');
}

bool is_binary_op(uint8_t c) {
    return (is_term_op(c) || is_expression_op(c));
}

bool is_unary_op(uint8_t c) {
    return (c == '-');
}

bool is_identifier(char* str) {
    if (!is_alpha(*str) && *str != '_') { return FALSE; }
    while (is_alpha(*str) || *str == '_') { str++; }
    return (*str == NULL);
}
