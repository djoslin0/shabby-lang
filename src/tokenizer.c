#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"

// file pointers
static FILE *src_ptr = NULL; // source code
static FILE *tok_ptr = NULL; // output

// input state
static uint8_t c = 0; // current character
static uint16_t c_index = -1; // index of the current character

// read the next character, output why we've advanced in debug mode
#ifdef DEBUG
    static void read(char* reason) {
        printf("%s  %c\n", reason, c);
        c = fgetc(src_ptr);
        c_index++;
    }
#else
    static void read(void) {
        c = fgetc(src_ptr);
        c_index++;
    }
#endif

static void consume_whitespace(void) {
    while (is_whitespace(c)) { read(DBG_STR("ws")); }
}

static void next_token(void) {
    // EOF
    if (c == (uint8_t)EOF) { return; }

    // identifiers [a-zA-Z_][0-9a-zA-Z_]+
    if (is_alphanumeric(c) || (c == '_')) {
        while (is_alphanumeric(c) || (c == '_')) {
            read(DBG_STR("an"));
        }
        return;
    }

    // symbols
    uint8_t last = c;
    read(DBG_STR("sy"));
    if (c == '=' && (is_math_op(last) || last == '=')) {
        read(DBG_STR("sy"));
    }
}

static void output(uint16_t start_index, uint8_t length) {
    fput16(start_index, tok_ptr);
    fputc(length, tok_ptr);
    assert(length <= MAX_TOKEN_LEN);
}

void tokenize(FILE* ptr) {
    src_ptr = ptr;

    // print header for why we consumed certain chars
    #ifdef DEBUG
        printf("\n");
        printf("rsn c\n");
        printf("--- -\n");
    #endif

    // consume first character
    read(DBG_STR("st"));

    // allocate space for token count
    fput16(0, tok_ptr);

    uint16_t token_count = 0;
    while (c != (uint8_t)EOF) {
        // ignore all whitespace
        consume_whitespace();

        // read token and remember start/length
        uint16_t token_start = c_index;
        next_token();
        uint16_t length = (c_index - token_start);

        // output if token has length
        if (length > 0) {
            token_count++;
            output(token_start, length);
        }
    }

    // write token count
    fseek(tok_ptr, 0, 0);
    fput16(token_count, tok_ptr);

    // print out entire file
    #ifdef DEBUG
        printf("\n");
        printf("raw       token\n");
        printf("--------  ----------------\n");
        for (uint16_t i = 0; i < token_count; i++) {
            // print raw token offsets
            uint16_t start_index = fget16(tok_ptr);
            uint8_t length = fgetc(tok_ptr);
            printf("%02X %02X %02X", (start_index >> 8), (start_index % 256), length);

            // print src token string
            char buffer[length + 1];
            fseek(src_ptr, start_index, 0);
            fgets(buffer, length + 1, src_ptr);
            buffer[length] = NULL;
            printf("  %s\n", buffer);
        }
    #endif
}

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char src_buffer[128] = { 0 };
    sprintf(src_buffer, "../examples/%s.src", argv[1]);
    src_ptr = fopen(src_buffer, "r");

    char tok_buffer[128] = { 0 };
    sprintf(tok_buffer, "../bin/%s.tok", argv[1]);
    tok_ptr = fopen(tok_buffer, "w+");

    tokenize(src_ptr);

    fclose(tok_ptr);
    fclose(src_ptr);
    return 0;
}
