#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"

  ///////////////////
 // file pointers //
///////////////////

static FILE *src_ptr = NULL; // source code
static FILE *tok_ptr = NULL; // token offsets
static FILE *ast_ptr = NULL; // output

  ///////////////////////
 // token information //
///////////////////////

struct {
    char string[MAX_TOKEN_LEN+1];
    uint16_t next_index;
} typedef token;

static token cur_token = { 0 }; // the current token being evaluated
static token peeked_token = { 0 }; // the token we last peeked at
static uint16_t token_count = 0; // the total number of tokens

  ////////////////////////////
 // scheduled future nodes //
////////////////////////////

struct {
    node_type node;
    uint16_t write_offset;
} typedef future_node;

static future_node future_stack[FUTURE_STACK_SIZE] = { 0 };
static uint16_t future_stack_count = 0;

static void future_push(node_type node, uint16_t write_offset) {
    future_stack[future_stack_count].node = node;
    future_stack[future_stack_count].write_offset = write_offset;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static future_node future_pop(void) {
    assert(future_stack_count > 0);
    future_stack_count--;
    return future_stack[future_stack_count];
}

  /////////////////////////////
 // debugging functionality //
/////////////////////////////

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

  ///////////////
 // utilities //
///////////////

static void write_current_offset_to(uint8_t write_offset) {
    // write parent pointer
    uint16_t cur_offset = ftell(ast_ptr);
    fseek(ast_ptr, write_offset, 0);
    fput16(cur_offset, ast_ptr);
    fseek(ast_ptr, cur_offset, 0);
}

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

static void peek_token(uint16_t index) {
    // bounds checking
    if (index >= token_count) {
        peeked_token.string[0] = NULL;
        peeked_token.next_index = index + 1;
        return;
    }

    // obtain token offsets
    fseek(tok_ptr, 2 + index * 3, 0);
    uint16_t start_index = fget16(tok_ptr);
    uint8_t length = fgetc(tok_ptr);
    assert(length <= MAX_TOKEN_LEN);

    // obtain token string
    fseek(src_ptr, start_index, 0);
    fgets(peeked_token.string, length+1, src_ptr);
    peeked_token.string[length] = NULL;

    peeked_token.next_index = index + 1;
}

  ///////////////////////////
 // parsing functionality //
///////////////////////////

static void parse_constant(uint16_t pointer_offset) {
    // <constant> ::= ( '0-9'+ )
    // output: <type> <token>

    // validate constant
    for (uint8_t i = 0; i < MAX_TOKEN_LEN; i++) {
        if (cur_token.string[i] == NULL) { break; }
        assert(is_numeric(cur_token.string[i]));
    }

    DPRINT("constant", TRUE);

    // write to parent pointer
    write_current_offset_to(pointer_offset);

    // output type
    fputc(NT_CONSTANT, ast_ptr);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);

    next_token();
}

static void parse_unary_op(uint16_t pointer_offset) {
    // <unary_op> ::= ( '-' )
    // output: <type> <token>

    // validate unary op
    if (!is_unary_op(cur_token.string[0])) { return; }
    assert(cur_token.string[1] == NULL);

    DPRINT("unary_op", TRUE);

    // write to parent pointer
    write_current_offset_to(pointer_offset);

    // output type
    fputc(NT_UNARY_OP, ast_ptr);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);

    next_token();
}

static void parse_factor(uint16_t pointer_offset) {
    // <factor> ::= [ <unary_op> ] ( <constant> | '(' <expression> ')' )
    // output: <type> <*constant | *expression> <*unary_op>

    DPRINT("<factor>", FALSE);
    INDENT(1);

    // write to parent pointer
    write_current_offset_to(pointer_offset);

    // unindent
    #ifdef DEBUG
    future_push(NT_DEBUG_UNINDENT_NODE, NULL);
    #endif

    // output type
    fputc(NT_FACTOR, ast_ptr);

    // peek ahead if current token is a unary operator
    char* value_str = cur_token.string;
    if (is_unary_op(cur_token.string[0])) {
        peek_token(cur_token.next_index);
        value_str = peeked_token.string;
    }

    if (is_numeric(value_str[0])) {
        // schedule/allocate constant
        future_push(NT_CONSTANT, ftell(ast_ptr));
        fput16(NULL, ast_ptr);
    } else if (value_str[0] == '(' && value_str[1] == NULL) {
        // consume opening paren
        next_token();

        // schedule/allocate expression
        future_push(NT_EXPRESSION_PAREN, NULL);
        future_push(NT_EXPRESSION, ftell(ast_ptr));
        fput16(NULL, ast_ptr);
    } else {
        // unmatched
        assert(FALSE);
    }

    // schedule/allocate unary_op
    future_push(NT_UNARY_OP, ftell(ast_ptr));
    fput16(NULL, ast_ptr); // unary_op

}

static void parse_term_op(uint16_t pointer_offset) {
    // <term_op> ::= ( '+' | '-' )
    // output: <type> <token>

    // validate term op
    if (!is_term_op(cur_token.string[0])) { return; }
    assert(cur_token.string[1] == NULL);

    DPRINT("term_op", TRUE);

    // write to parent pointer
    write_current_offset_to(pointer_offset);

    // output type
    fputc(NT_TERM_OP, ast_ptr);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);

    next_token();

    // schedule right factor
    future_push(NT_FACTOR, pointer_offset - 2);
}

static void parse_term(uint16_t pointer_offset) {
    // <term> ::= <factor> { ( '*' | '/' ) <factor> }
    // output: <type> <*right_factor> <*term_op> <*left_factor>

    DPRINT("<term>", FALSE);
    INDENT(1);

    // unindent
    #ifdef DEBUG
    future_push(NT_DEBUG_UNINDENT_NODE, NULL);
    #endif

    // write to parent pointer
    write_current_offset_to(pointer_offset);

    // output type
    fputc(NT_TERM, ast_ptr);

    // allocate right factor
    fput16(NULL, ast_ptr); // right factor

    // schedule/allocate term op
    future_push(NT_TERM_OP, ftell(ast_ptr));
    fput16(NULL, ast_ptr); // factor op

    // schedule/allocate left factor
    future_push(NT_FACTOR, ftell(ast_ptr));
    fput16(NULL, ast_ptr); // left factor
}

static void parse_expression_paren(void) {
    // eats closing paren
    // output:

    assert(cur_token.string[0] == ')');
    assert(cur_token.string[1] == NULL);

    next_token();
}

static void parse_expression_op(uint16_t pointer_offset) {
    // <expression_op> ::= ( '+' | '-' )
    // output: <type> <token>

    // validate expression op
    if (!is_expression_op(cur_token.string[0])) { return; }
    assert(cur_token.string[1] == NULL);

    DPRINT("expression_op", TRUE);

    // write to parent pointer
    write_current_offset_to(pointer_offset);

    // output type
    fputc(NT_EXPRESSION_OP, ast_ptr);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);

    next_token();

    // schedule right term
    future_push(NT_TERM, pointer_offset - 2);
}

static void parse_expression(uint16_t pointer_offset) {
    // <expression> ::= <term> { ( '+' | '-' ) <term> }
    // output: <type> <*right_term> <*expression_op> <*left_term>

    DPRINT("<expression>", FALSE);
    INDENT(1);

    // unindent
    #ifdef DEBUG
    future_push(NT_DEBUG_UNINDENT_NODE, NULL);
    #endif

    // write to parent pointer
    write_current_offset_to(pointer_offset);

    // output type
    fputc(NT_EXPRESSION, ast_ptr);

    // allocate right term
    fput16(NULL, ast_ptr); // right term

    // schedule/allocate expression op
    future_push(NT_EXPRESSION_OP, ftell(ast_ptr));
    fput16(NULL, ast_ptr); // term op

    // schedule/allocate left term
    future_push(NT_TERM, ftell(ast_ptr));
    fput16(NULL, ast_ptr); // left term
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

    // schedule root node
    future_push(NT_EXPRESSION, 0);

    while(future_stack_count > 0) {
        future_node n = future_pop();
        switch(n.node) {
            case NT_NONE:
            case NT_ROOT: assert(FALSE); break;
            case NT_EXPRESSION: parse_expression(n.write_offset); break;
            case NT_EXPRESSION_OP: parse_expression_op(n.write_offset); break;
            case NT_EXPRESSION_PAREN: parse_expression_paren(); break;
            case NT_TERM: parse_term(n.write_offset); break;
            case NT_TERM_OP: parse_term_op(n.write_offset); break;
            case NT_FACTOR: parse_factor(n.write_offset); break;
            case NT_UNARY_OP: parse_unary_op(n.write_offset); break;
            case NT_CONSTANT: parse_constant(n.write_offset); break;
            #ifdef DEBUG
            case NT_DEBUG_UNINDENT_NODE: INDENT(-1); break;
            #endif
       }
    }
}

  //////////
 // main //
//////////

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char src_buffer[128] = { 0 };
    sprintf(src_buffer, "../examples/%s.src", argv[1]);
    src_ptr = fopen(src_buffer, "r");

    char tok_buffer[128] = { 0 };
    sprintf(tok_buffer, "../bin/%s.tok", argv[1]);
    tok_ptr = fopen(tok_buffer, "r");

    char ast_buffer[128] = { 0 };
    sprintf(ast_buffer, "../bin/%s.ast", argv[1]);
    ast_ptr = fopen(ast_buffer, "w+");

    parse(src_ptr, tok_ptr, ast_ptr);

    fclose(src_ptr);
    fclose(tok_ptr);
    fclose(ast_ptr);
    return 0;
}
