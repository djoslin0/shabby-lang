#include <stdio.h>
#include <string.h>
#include "constants.h"

FILE *in_ptr = NULL;
FILE *out_ptr = NULL;
char c = 0;
short out_index = -1;

bool is_whitespace(char c) {
    return (c == ' ' || c == '\t')
        || (c == '\r' || c == '\n');
}

bool is_alphanumeric(char c) {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9');
}

bool is_math_op(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/')
        || (c == '|' || c == '&' || c == '^' || c == '%');
}

void read(void) {
    c = fgetc(in_ptr);
    out_index++;
}

void consume_whitespace(void) {
    while (is_whitespace(c)) { read(); }
}

void next_token(void) {
    // identifiers
    if (is_alphanumeric(c)) {
        while (is_alphanumeric(c) || (c == '_')) { read(); }
        return;
    }

    // symbols
    char last = c;
    read();
    if (c == '=' && (is_math_op(last) || last == '=')) { read(); }
}

void output(short start_index, unsigned char length) {
    fputc(start_index % 256, out_ptr);
    fputc((start_index >> 8), out_ptr);
    fputc(length, out_ptr);

    #ifdef DEBUG
        printf("%02X %02X %02X   ",
               start_index % 256,
               (start_index >> 8) % 256,
               length);
        fseek(in_ptr, start_index, 0);
        char buffer[length + 1];
        fgets(buffer, length + 1, in_ptr);
        buffer[length] = 0;
        printf("'%s' \n", buffer);
        fseek(in_ptr, length + 1, start_index);
    #endif
}

void tokenize(FILE* ptr) {
    in_ptr = ptr;
    read();
    while (c != EOF) {
        consume_whitespace();
        short token_start = out_index;
        next_token();
        output(token_start, (out_index - token_start));
    }
}

int main(void) {
    #ifdef DEBUG
        printf("offsets    token\n");
    #endif
    in_ptr = fopen("examples/expression.bs", "r");
    out_ptr = fopen("out/a.tok", "w");
    tokenize(in_ptr);
    fclose(out_ptr);
    fclose(in_ptr);
    return 0;
}