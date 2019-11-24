#include <stdio.h>
#include <string.h>
#include <assert.h> 
#include "symbols.h"
#include "file.h"

static FILE *src_ptr = NULL;
static FILE *tok_ptr = NULL;
static FILE *ast_ptr = NULL;

static uint16_t token_count = 0;
struct {
    char string[MAX_TOKEN_LEN+1];
    uint16_t next_index;
} typedef token;

static token cur_token = {
    .string = { 0 },
    .next_index = 0
};

enum {
    NONE,
    ROOT,
    EXPRESSION,
    TERM,
    FACTOR,
    BINARY_OP,
    UNARY_OP,
    CONSTANT
} typedef node_type;

#ifdef DEBUG
    int* stack_start;
    static int _indent = 0;
    #define INDENT(x) _indent += x
    #define DPRINT(x, y) _dprint(x, y)

    static void _dprint(char* string, bool print_token) {
        int stack_mem = 10;
        printf("%05lX  ", stack_start - (&stack_mem));
        for (int i = 0; i < _indent; i++) { printf("    "); }
        printf("%s", string);
        if (print_token) {
            printf(": %s", cur_token.string);
        }
        printf("\n");
    }
#else
    #define INDENT(x)
    #define DPRINT(x, y)
#endif

// forward declarations
static uint16_t parse_constant();
static uint16_t parse_unary_op();
static uint16_t parse_binary_op();
static uint16_t parse_factor();
static uint16_t parse_term();
static uint16_t parse_expression();

static void next_token() {
    // bounds checking
    if (cur_token.next_index >= token_count) {
        cur_token.string[0] = NULL;
        cur_token.next_index++;
        return;
    }

    // obtain token offsets
    fseek(tok_ptr, 2 + cur_token.next_index * 3, 0);
    uint16_t start_index = fget16(tok_ptr);
    uint8_t length = fgetc(tok_ptr);
    assert(length <= MAX_TOKEN_LEN);

    // obtain token string
    fseek(src_ptr, start_index, 0);
    fgets(cur_token.string, length+1, src_ptr);
    cur_token.string[length] = NULL;
    
    cur_token.next_index++;
}

static uint16_t parse_constant() {
    // <constant> ::= ( '0-9'+ )

    // validate constant
    for (uint8_t i = 0; i < MAX_TOKEN_LEN; i++) {
        if (cur_token.string[i] == NULL) { break; }
        assert(is_numeric(cur_token.string[i]));
    }

    DPRINT("constant", TRUE);

    // output
    uint16_t offset = ftell(ast_ptr);
    fputc(CONSTANT, ast_ptr);
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);

    next_token();

    return offset;
}

static uint16_t parse_unary_op() {
    // <unary_op> ::= ( '-' )

    // validate unary op
    assert(is_unary_op(cur_token.string[0]));
    assert(cur_token.string[1] == NULL);
    DPRINT("unary_op", TRUE);

    // output
    uint16_t offset = ftell(ast_ptr);
    fputc(UNARY_OP, ast_ptr);
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);

    next_token();

    return offset;
}

static uint16_t parse_binary_op() {
    // <binary_op> ::= ( '+' | '-' | '*' | '/' )

    // validate binary op
    assert(is_binary_op(cur_token.string[0]));
    assert(cur_token.string[1] == NULL);

    DPRINT("binary_op", TRUE);

    // output
    uint16_t offset = ftell(ast_ptr);
    fputc(BINARY_OP, ast_ptr);
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);

    next_token();

    return offset;
}

static uint16_t parse_factor() {
    // <factor> ::= [ <unary_op> ] ( <constant> | '(' <expression> ')' )
    DPRINT("<factor>", FALSE);
    INDENT(1);
 
    uint16_t unary_op = NULL;
    uint16_t factor = NULL;
 
    // check for unary ops
    if (is_unary_op(cur_token.string[0])) {
        unary_op = parse_unary_op();
    }
    
    if (is_numeric(cur_token.string[0])) {
        // constant
        factor = parse_constant();
    } else if (cur_token.string[0] == '(' && cur_token.string[1] == NULL) {
        // (expression)
        next_token();
        factor = parse_expression();
        assert(cur_token.string[0] == ')' && cur_token.string[1] == NULL);
        next_token();
    } else {
        // unmatched
        assert(FALSE);
    }
    INDENT(-1);

    // output
    uint16_t offset = ftell(ast_ptr);
    fputc(FACTOR, ast_ptr);
    fput16(unary_op, ast_ptr);
    fput16(factor, ast_ptr);
    return offset;
}

static uint16_t parse_term() {
    // <term> ::= <factor> { ( '*' | '/' ) <factor> }
    DPRINT("<term>", FALSE);
    INDENT(1);
    uint16_t left_factor = parse_factor();
    uint16_t binary_op = NULL;
    uint16_t right_factor = NULL;

    if (is_term_op(cur_token.string[0])) {
        binary_op = parse_binary_op();
        right_factor = parse_factor();
    }
    INDENT(-1);

    // output
    uint16_t offset = ftell(ast_ptr);
    fputc(TERM, ast_ptr);
    fput16(left_factor, ast_ptr);
    fput16(binary_op, ast_ptr);
    fput16(right_factor, ast_ptr);
    return offset;
}

static uint16_t parse_expression() {
    // <expression> ::= <term> { ( '+' | '-' ) <term> }
    
    DPRINT("<expression>", FALSE);
    INDENT(1);

    uint16_t left_term = parse_term();
    uint16_t binary_op = NULL;
    uint16_t right_term = NULL;

    if (is_expression_op(cur_token.string[0])) {
        binary_op = parse_binary_op();
        right_term = parse_term();
    }

    INDENT(-1);

    // output
    uint16_t offset = ftell(ast_ptr);
    fputc(EXPRESSION, ast_ptr);
    fput16(left_term, ast_ptr);
    fput16(binary_op, ast_ptr);
    fput16(right_term, ast_ptr);
    return offset;
}

void parse(FILE* src_ptr_arg, FILE* tok_ptr_arg, FILE* out_ptr_arg) {
    #ifdef DEBUG
        // track stack depth in bytes
        int stack_mem = 10;
        stack_start = &stack_mem;

        // print debug header
        printf("\n");
        printf("stack  node\n");
        printf("-----  -------------------\n");
    #endif

    src_ptr = src_ptr_arg;
    tok_ptr = tok_ptr_arg;
    ast_ptr = out_ptr_arg;
 
    // read token info
    token_count = fget16(tok_ptr);
    next_token();
 
    // allocate space for root pointer
    fput16(NULL, ast_ptr);

    uint16_t root = parse_expression();

    // write root pointer
    fseek(ast_ptr, 0, 0);
    fput16(root, ast_ptr);
}

int main(void) {
    src_ptr = fopen("examples/expression.bs", "r");
    tok_ptr = fopen("out/expression.tok", "r");
    ast_ptr = fopen("out/expression.ast", "w");
    parse(src_ptr, tok_ptr, ast_ptr);
    fclose(src_ptr);
    fclose(tok_ptr);
    fclose(ast_ptr);
    return 0;
}