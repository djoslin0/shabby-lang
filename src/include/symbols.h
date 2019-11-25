#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "constants.h"

bool is_whitespace(uint8_t);
bool is_alpha(uint8_t);
bool is_numeric(uint8_t);
bool is_alphanumeric(uint8_t);
bool is_math_op(uint8_t);
bool is_term_op(uint8_t);
bool is_expression_op(uint8_t);
bool is_binary_op(uint8_t);
bool is_unary_op(uint8_t);
bool is_identifier(char*);

#endif
