#include <stdio.h>
#include <stdarg.h>
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
} typedef token_s;

static token_s cur_token = { 0 }; // the current token being evaluated
static token_s peeked_token = { 0 }; // the token we last peeked at
static uint16_t token_count = 0; // the total number of tokens

  ////////////////////////////
 // scheduled future nodes //
////////////////////////////

#define FUTURE_FLAG_STATEMENTS (1 << 0)

struct {
    node_t node;
    uint16_t parent_offset;
    uint8_t child_index;
    uint8_t flags;
} typedef future_node_s;

static future_node_s future_stack[FUTURE_STACK_SIZE] = { 0 };
static uint16_t future_stack_count = 0;

static void future_push(node_t node, uint16_t parent_offset,
                        uint8_t child_index, uint8_t flags) {
    future_stack[future_stack_count].node = node;
    future_stack[future_stack_count].parent_offset = parent_offset;
    future_stack[future_stack_count].child_index = child_index;
    future_stack[future_stack_count].flags = flags;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static future_node_s* future_pop(void) {
    assert(future_stack_count > 0);
    future_stack_count--;
    return &future_stack[future_stack_count];
}

  /////////////////////////////
 // debugging functionality //
/////////////////////////////

#ifdef DEBUG
    static void peek_token(uint16_t index);
    int* stack_start;
    static int _indent = 0;
    #define INDENT(x) _indent += x
    #define DPRINT(x, y) _dprint(x, y)

    static void _dprint(char* string, uint8_t tokens) {
        // print stack depth
        int stack_mem = 10;
        printf("%05lX  ", stack_start - (&stack_mem));

        // print indent
        for (int i = 0; i < _indent; i++) { printf("    "); }

        // print header
        printf("%s: ", string);

        // print tokens
        for (int i = 0; i < tokens; i++) {
            peek_token(cur_token.next_index + i - 1);
            printf("%s ", peeked_token.string);
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

static uint16_t output(node_t node_type, uint16_t parent_offset,
                   uint8_t child_index, uint8_t child_count,
                   uint8_t symbols) {
    DPRINT(node_constants[node_type].name, symbols);
    return write_ast_node(ast_ptr, node_type, parent_offset,
                          child_index, child_count, symbols);
}

  ///////////////////////////
 // parsing functionality //
///////////////////////////

static void parse_cast(uint16_t parent_offset, uint8_t child_index) {
    // <cast> ::= '<' <type> '>' <factor>
    // output: [base node] <token>

    // validate identifier
    assert(is_alpha(cur_token.string[0]) || cur_token.string[0] == '_');
    for (uint8_t i = 0; i < MAX_TOKEN_LEN; i++) {
        if (cur_token.string[i] == NULL) { break; }
        assert(is_alphanumeric(cur_token.string[i]) || cur_token.string[i] == '_');
    }

    // write base node
    uint8_t my_offset = output(NT_CAST, parent_offset, child_index, 1, 1);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);
    next_token();

    // schedule factor
    future_push(NT_FACTOR, my_offset, 0, NULL);
    future_push(NT_CONSUME, NULL, NULL, '>');
}

static void parse_constant(uint16_t parent_offset, uint8_t child_index) {
    // <constant> ::= ( '0-9'+ )
    // output: [base node] <token>

    // validate constant
    for (uint8_t i = 0; i < MAX_TOKEN_LEN; i++) {
        if (cur_token.string[i] == NULL) { break; }
        assert(is_numeric(cur_token.string[i]));
    }

    // write base node
    output(NT_CONSTANT, parent_offset, child_index, 1, 1);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);
    next_token();
}

static void parse_variable(uint16_t parent_offset, uint8_t child_index) {
    // <variable> ::= ( '[a-zA-Z_][a-zA-Z0-9_]*' )
    // output: [base node] <token>

    // validate identifier
    assert(is_alpha(cur_token.string[0]) || cur_token.string[0] == '_');
    for (uint8_t i = 0; i < MAX_TOKEN_LEN; i++) {
        if (cur_token.string[i] == NULL) { break; }
        assert(is_alphanumeric(cur_token.string[i]) || cur_token.string[i] == '_');
    }

    // write base node
    output(NT_VARIABLE, parent_offset, child_index, 1, 1);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);
    next_token();
}

static void parse_unary_op(uint16_t parent_offset, uint8_t child_index) {
    // <unary_op> ::= ( '-' )
    // output: [base node] <token>

    // validate unary op
    if (!is_unary_op(cur_token.string[0])) { return; }
    assert(cur_token.string[1] == NULL);

    // write base node
    output(NT_UNARY_OP, parent_offset, child_index, 0, 1);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);
    next_token();
}

static void parse_factor(uint16_t parent_offset, uint8_t child_index) {
    // <factor> ::= [ <unary_op> ] ( <constant> | '(' <expression> ')' | <variable> | <cast> )
    // output: [base node] <*unary_op> <*constant | *expression | *variable>

    // write base node
    uint16_t my_offset = output(NT_FACTOR, parent_offset, child_index, 2, 0);

    // indentation
    #ifdef DEBUG
    INDENT(1);
    future_push(NT_DEBUG_UNINDENT_NODE, NULL, NULL, NULL);
    #endif

    // peek ahead if current token is a unary operator
    char* value_str = cur_token.string;
    if (is_unary_op(cur_token.string[0])) {
        peek_token(cur_token.next_index);
        value_str = peeked_token.string;
    }

    printf("'%s'\n", value_str);
    if (is_numeric(value_str[0])) {
        // schedule constant
        future_push(NT_CONSTANT, my_offset, 1, NULL);
    } else if (value_str[0] == '(' && value_str[1] == NULL) {
        // consume opening paren
        next_token();
        // schedule expression
        future_push(NT_CONSUME, NULL, NULL, ')');
        future_push(NT_EXPRESSION, my_offset, 1, NULL);
    } else if (value_str[0] == '<' && value_str[1] == NULL) {
        // consume opening '<'
        next_token();
        // schedule cast
        future_push(NT_CAST, my_offset, 1, NULL);
    } else {
        // schedule variable
        future_push(NT_VARIABLE, my_offset, 1, NULL);
    }

    // schedule unary_op
    future_push(NT_UNARY_OP, my_offset, 0, NULL);
}

static void parse_term_op(uint16_t parent_offset, uint8_t child_index) {
    // <term_op> ::= ( '+' | '-' )
    // output: [base node] <token>

    // validate term op
    if (!is_term_op(cur_token.string[0])) { return; }
    assert(cur_token.string[1] == NULL);

    // write base node
    output(NT_TERM_OP, parent_offset, child_index, 0, 1);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);
    next_token();

    // schedule right factor
    future_push(NT_FACTOR, parent_offset, 0, NULL);
}

static void parse_term(uint16_t parent_offset, uint8_t child_index) {
    // <term> ::= <factor> { ( '*' | '/' ) <factor> }
    // output: [base node] <*right_factor> <*term_op> <*left_factor>

    // write base node
    uint16_t my_offset = output(NT_TERM, parent_offset, child_index, 3, 0);

    // indentation
    #ifdef DEBUG
    INDENT(1);
    future_push(NT_DEBUG_UNINDENT_NODE, NULL, NULL, NULL);
    #endif

    // schedule term op
    future_push(NT_TERM_OP, my_offset, 1, NULL);

    // schedule left factor
    future_push(NT_FACTOR, my_offset, 2, NULL);
}

static void parse_expression_op(uint16_t parent_offset, uint8_t child_index) {
    // <expression_op> ::= ( '+' | '-' )
    // output: [base node] <token>

    // validate expression op
    if (!is_expression_op(cur_token.string[0])) { return; }
    assert(cur_token.string[1] == NULL);

    // write base node
    output(NT_EXPRESSION_OP, parent_offset, child_index, 0, 1);

    // output token
    fputs(cur_token.string, ast_ptr);
    fputc(NULL, ast_ptr);
    next_token();

    // schedule right term
    future_push(NT_TERM, parent_offset, 0, NULL);
}

static void parse_expression(uint16_t parent_offset, uint8_t child_index) {
    // <expression> ::= <term> { ( '+' | '-' ) <term> }
    // output: [base node] <*right_term> <*expression_op> <*left_term>

    // write base node
    uint16_t my_offset = output(NT_EXPRESSION, parent_offset, child_index, 3, 0);

    // indentation
    #ifdef DEBUG
    INDENT(1);
    future_push(NT_DEBUG_UNINDENT_NODE, NULL, NULL, NULL);
    #endif

    // schedule expression op
    future_push(NT_EXPRESSION_OP, my_offset, 1, NULL);

    // schedule left term
    future_push(NT_TERM, my_offset, 2, NULL);
}

static void parse_assignment(uint16_t parent_offset, uint8_t child_index) {
    // <assignment> ::= <identifier> '=' <expression>
    // output: [base node] <*expression> <var_identifier>

    // write base node
    uint16_t my_offset = output(NT_ASSIGNMENT, parent_offset, child_index, 1, 1);

    // indentation
    #ifdef DEBUG
    INDENT(1);
    future_push(NT_DEBUG_UNINDENT_NODE, NULL, NULL, NULL);
    #endif

    // output var identifier
    assert(is_identifier(cur_token.string));
    fputs(cur_token.string, ast_ptr); // var identifier
    fputc(NULL, ast_ptr);
    next_token();

    // ensure we set the variable
    assert(cur_token.string[0] == '=' && cur_token.string[1] == NULL);
    next_token();

    // schedule expression
    future_push(NT_EXPRESSION, my_offset, 0, NULL);
}

static void parse_declaration(uint16_t parent_offset, uint8_t child_index) {
    // <declaration> ::= <var_type> <identifier> '=' <expression>
    // output: [base node] <*expression> <var_type_token> <var_identifier>

    // write base node
    uint16_t my_offset = output(NT_DECLARATION, parent_offset, child_index, 1, 2);

    // indentation
    #ifdef DEBUG
    future_push(NT_DEBUG_UNINDENT_NODE, NULL, NULL, NULL);
    #endif

    // output var type token
    assert(is_identifier(cur_token.string));
    fputs(cur_token.string, ast_ptr); // var token
    fputc(NULL, ast_ptr);
    next_token();

    // output var identifier
    assert(is_identifier(cur_token.string));
    fputs(cur_token.string, ast_ptr); // var identifier
    fputc(NULL, ast_ptr);
    next_token();

    // ensure we set the variable
    assert(cur_token.string[0] == '=' && cur_token.string[1] == NULL);
    next_token();

    // schedule expression
    future_push(NT_EXPRESSION, my_offset, 0, NULL);
}

static void parse_statement(uint16_t parent_offset, uint8_t child_index, uint8_t flags) {
    // <statement> ::= <declaration> ';'
    // output: [base node] <*next_statement> <*assignment | *declaration>

    if (!is_identifier(cur_token.string)) { return; }

    // write base node
    uint16_t my_offset = output(NT_STATEMENT, parent_offset, child_index, 2, 0);

    // schedule next statement
    if (flags & FUTURE_FLAG_STATEMENTS) {
        future_push(NT_STATEMENT, my_offset, 0, flags);
    }

    // indentation
    #ifdef DEBUG
    INDENT(1);
    future_push(NT_DEBUG_UNINDENT_NODE, NULL, NULL, NULL);
    #endif

    // expect ';' at the end
    future_push(NT_CONSUME, NULL, NULL, ';');

    peek_token(cur_token.next_index);
    if (peeked_token.string[0] == '=' && peeked_token.string[1] == NULL) {
        // schedule assignment
        future_push(NT_ASSIGNMENT, my_offset, 1, NULL);
    } else {
        // schedule declaration
        future_push(NT_DECLARATION, my_offset, 1, NULL);
    }
}

static void parse_statement_list(uint16_t parent_offset, uint8_t child_index) {
    // <statement_list> ::= <statement> [<statement> ...]
    // output:

    DPRINT("statement_list", FALSE);
    INDENT(1);

    // unindent
    #ifdef DEBUG
    future_push(NT_DEBUG_UNINDENT_NODE, NULL, NULL, NULL);
    #endif

    // schedule statement
    future_push(NT_STATEMENT, parent_offset, child_index, FUTURE_FLAG_STATEMENTS);
}

static void parse_consume(char c) {
    // eats given character from token stream
    // output:

    if (cur_token.string[0] != c) {
        printf("Syntax error!\n Expected '%c' but got '%c'.\n", c, cur_token.string[0]);
    }

    assert(cur_token.string[0] == c);
    assert(cur_token.string[1] == NULL);

    next_token();
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
    future_push(NT_STATEMENT_LIST, NULL, NULL, NULL);

    while(future_stack_count > 0) {
        future_node_s* n = future_pop();
        printf("%s\n", node_constants[n->node].name);
        switch(n->node) {
            case NT_CONSUME: parse_consume((char)n->flags); break;
            case NT_STATEMENT_LIST: parse_statement_list(n->parent_offset, n->child_index); break;
            case NT_STATEMENT: parse_statement(n->parent_offset, n->child_index, n->flags); break;
            case NT_DECLARATION: parse_declaration(n->parent_offset, n->child_index); break;
            case NT_ASSIGNMENT: parse_assignment(n->parent_offset, n->child_index); break;
            case NT_EXPRESSION: parse_expression(n->parent_offset, n->child_index); break;
            case NT_EXPRESSION_OP: parse_expression_op(n->parent_offset, n->child_index); break;
            case NT_TERM: parse_term(n->parent_offset, n->child_index); break;
            case NT_TERM_OP: parse_term_op(n->parent_offset, n->child_index); break;
            case NT_FACTOR: parse_factor(n->parent_offset, n->child_index); break;
            case NT_UNARY_OP: parse_unary_op(n->parent_offset, n->child_index); break;
            case NT_VARIABLE: parse_variable(n->parent_offset, n->child_index); break;
            case NT_CONSTANT: parse_constant(n->parent_offset, n->child_index); break;
            case NT_CAST: parse_cast(n->parent_offset, n->child_index); break;
            #ifdef DEBUG
            case NT_DEBUG_UNINDENT_NODE: INDENT(-1); break;
            #endif
            default: assert(FALSE); break;
       }
    }
    assert(future_stack_count == 0);
}

  //////////
 // main //
//////////

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char src_buffer[128] = { 0 };
    sprintf(src_buffer, "../examples/%s.src", argv[1]);
    src_ptr = fopen(src_buffer, "rb");

    char tok_buffer[128] = { 0 };
    sprintf(tok_buffer, "../bin/%s.tok", argv[1]);
    tok_ptr = fopen(tok_buffer, "rb");

    char ast_buffer[128] = { 0 };
    sprintf(ast_buffer, "../bin/%s.ast", argv[1]);
    ast_ptr = fopen(ast_buffer, "wb+");

    parse(src_ptr, tok_ptr, ast_ptr);

    fclose(src_ptr);
    fclose(tok_ptr);
    fclose(ast_ptr);

    return 0;

    // make pedantic compilers happy
    node_constants[0] = node_constants[0];
}
