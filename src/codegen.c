#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"
#include "bytecode.h"

  ///////////////////
 // file pointers //
///////////////////

static FILE *src_ptr = NULL; // source code
static FILE *ast_ptr = NULL; // abstract syntax tree
static FILE *gen_ptr = NULL; // output

  //////////
 // misc //
//////////
static char token[MAX_TOKEN_LEN];

static void read_token(void) {
    memset(token, NULL, MAX_TOKEN_LEN);
    fgets(token, MAX_TOKEN_LEN, ast_ptr);
}

  ////////////////////////////
 // scheduled future nodes //
////////////////////////////

static uint16_t future_stack[FUTURE_STACK_SIZE] = { 0 };
static uint16_t future_stack_count = 0;

static void future_push(uint16_t offset) {
    if (offset == NULL) { return; }
    future_stack[future_stack_count] = offset;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static uint16_t future_pop(void) {
    assert(future_stack_count > 0);
    future_stack_count--;
    return future_stack[future_stack_count];
}

  ////////////
 // output //
////////////

static void output(bytecode_t type, ...) {
    // output type
    fputc(type, gen_ptr);
    #ifdef DEBUG
        printf("%s", bytecode[type].name);
    #endif

    // output params
    if (bytecode[type].params > 0) {
        va_list args;
        va_start(args, type);
        for (uint8_t i = 0; i < bytecode[type].params; i++) {
            uint16_t param = (uint16_t)va_arg(args, int);
            fput16(param, gen_ptr);
            #ifdef DEBUG
                printf(" %d", param);
            #endif
        }
        va_end(args);
    }
    #ifdef DEBUG
        printf("\n");
    #endif
}

  /////////////////////
 // code generation //
/////////////////////

static void gen_constant(void) {
    read_token();
    output(BC_PUSH, (uint16_t)atoi(token));
}

static void gen_unary_op(void) {
    read_token();
    switch (token[0]) {
        case '-': output(BC_NEG); break;
        default: assert(FALSE); break;
    }
}

static void gen_factor(void) {
    uint16_t value = fget16(ast_ptr);
    uint16_t unary_op = fget16(ast_ptr);
    future_push(unary_op);
    future_push(value);
}

static void gen_term_op(void) {
    read_token();
    switch (token[0]) {
        case '*': output(BC_MUL); break;
        case '/': output(BC_DIV); break;
        default: assert(FALSE); break;
    }
}

static void gen_term(void) {
    uint16_t right_factor = fget16(ast_ptr);
    uint16_t term_op = fget16(ast_ptr);
    uint16_t left_factor = fget16(ast_ptr);
    future_push(term_op);
    future_push(left_factor);
    future_push(right_factor);
}

static void gen_expression_op(void) {
    read_token();
    switch (token[0]) {
        case '+': output(BC_ADD); break;
        case '-': output(BC_SUB); break;
        default: assert(FALSE); break;
    }
}

static void gen_expression(void) {
    uint16_t right_term = fget16(ast_ptr);
    uint16_t expression_op = fget16(ast_ptr);
    uint16_t left_term = fget16(ast_ptr);
    future_push(expression_op);
    future_push(left_term);
    future_push(right_term);
}

void gen(FILE* src_ptr_arg, FILE* ast_ptr_arg, FILE* gen_ptr_arg) {
    #ifdef DEBUG
        // print debug header
        printf("\n");
        printf("assembly\n");
        printf("-------------------\n");
    #endif

    src_ptr = src_ptr_arg;
    ast_ptr = ast_ptr_arg;
    gen_ptr = gen_ptr_arg;

    // move to root node
    future_push(fget16(ast_ptr));

    while(future_stack_count > 0) {
        fseek(ast_ptr, future_pop(), 0);
        switch(fgetc(ast_ptr)) {
            case NT_EXPRESSION: gen_expression(); break;
            case NT_EXPRESSION_OP: gen_expression_op(); break;
            case NT_TERM: gen_term(); break;
            case NT_TERM_OP: gen_term_op(); break;
            case NT_FACTOR: gen_factor(); break;
            case NT_UNARY_OP: gen_unary_op(); break;
            case NT_CONSTANT: gen_constant(); break;
            default: assert(FALSE); break;
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

    char ast_buffer[128] = { 0 };
    sprintf(ast_buffer, "../bin/%s.ast", argv[1]);
    ast_ptr = fopen(ast_buffer, "r");

    char gen_buffer[128] = { 0 };
    sprintf(gen_buffer, "../bin/%s.gen", argv[1]);
    gen_ptr = fopen(gen_buffer, "w+");

    gen(src_ptr, ast_ptr, gen_ptr);

    fclose(src_ptr);
    fclose(ast_ptr);
    fclose(gen_ptr);
    return 0;
}
