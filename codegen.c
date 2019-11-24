#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"

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

  //////////////////////
 // evaluation stack //
//////////////////////

// TODO: Remove all evaluation stack stuff, it needs to be done in the next stage
static uint16_t eval_stack[32];
static uint16_t eval_stack_count = 0;

static void eval_push(uint16_t offset) {
    if (offset == NULL) { return; }
    eval_stack[eval_stack_count] = offset;
    eval_stack_count++;
    assert(eval_stack_count < 32);
}

static uint16_t eval_pop(void) {
    assert(eval_stack_count > 0);
    eval_stack_count--;
    return eval_stack[eval_stack_count];
}

static void output_eval_stack(void) {
    printf("\t  >>  ");
    for (int i = 0; i < eval_stack_count; i++) {
        printf("%d ", (int16_t)eval_stack[i]);
    }
    printf("\n");
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

  /////////////////////
 // code generation //
/////////////////////

static void gen_constant(void) {
    read_token();
    printf("push %s", token);
    eval_push(atoi(token));
    output_eval_stack();
}

static void gen_unary_op(void) {
    read_token();
    switch (token[0]) {
        case '-': printf("neg"); eval_push(-eval_pop()); output_eval_stack(); break;
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
        case '*': printf("mul"); eval_push(eval_pop() * eval_pop()); output_eval_stack(); break;
        case '/': printf("div"); eval_push(eval_pop() / eval_pop()); output_eval_stack(); break;
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
        case '+': printf("add"); eval_push(eval_pop() + eval_pop()); output_eval_stack(); break;
        case '-': printf("sub"); eval_push(eval_pop() - eval_pop()); output_eval_stack(); break;
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

    printf("\n");
    printf("final output\n");
    printf("------------\n");
    output_eval_stack();
}

  //////////
 // main //
//////////

int main(void) {
    src_ptr = fopen("examples/expression.bs", "r");
    ast_ptr = fopen("out/expression.ast", "r");
    gen_ptr = fopen("out/expression.bin", "w");
    gen(src_ptr, ast_ptr, gen_ptr);
    fclose(src_ptr);
    fclose(ast_ptr);
    fclose(gen_ptr);
    return 0;
}
