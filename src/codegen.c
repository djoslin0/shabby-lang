#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"
#include "bytecode.h"

#define SCHEDULE_RAW_BYTECODE ((uint16_t)-1)

  ///////////////////
 // file pointers //
///////////////////

static FILE *src_ptr = NULL; // source code
static FILE *ast_ptr = NULL; // abstract syntax tree
static FILE *gen_ptr = NULL; // output

  /////////////////////
 // token utilities //
/////////////////////

static char token[MAX_TOKEN_LEN];
static void read_token(void) {
    int last_position = ftell(ast_ptr);
    memset(token, NULL, MAX_TOKEN_LEN);
    fgets(token, MAX_TOKEN_LEN, ast_ptr);
    assert(strlen(token) < MAX_TOKEN_LEN);
    fseek(ast_ptr, last_position + strlen(token) + 1, 0);
}

  //////////////
 // ast node //
//////////////

#define MAX_AST_CHILDREN 4
struct {
    node_t node_type;
    uint8_t value_type;
    uint16_t parent_offset;
    uint8_t child_count;
    uint16_t children[MAX_AST_CHILDREN];
} typedef ast_s;

static ast_s cur_node = { 0 };

static void read_ast_node(uint16_t offset) {
    fseek(ast_ptr, offset, 0);
    cur_node.node_type = fgetc(ast_ptr);
    cur_node.value_type = fgetc(ast_ptr);
    cur_node.parent_offset = fget16(ast_ptr);
    cur_node.child_count = fgetc(ast_ptr);
    assert(cur_node.child_count < MAX_AST_CHILDREN);
    memset(cur_node.children, NULL, MAX_AST_CHILDREN * sizeof(uint16_t));
    for (int i = 0; i < cur_node.child_count; i++) {
        cur_node.children[i] = fget16(ast_ptr);
    }
}

  ///////////
 // types //
///////////

struct {
    uint16_t size;
    char name[MAX_TOKEN_LEN];
} typedef type_s;

type_s types[] = {
    //{ .size = 0, .name = "void" },
    //{ .size = 1, .name = "byte" },
    { .size = 2, .name = "short" },
};

static type_s* get_type(char* s) {
    uint16_t types_count = sizeof(types) / sizeof(type_s);
    for (int i = 0; i < types_count; i++) {
        if (!strcmp(s, types[i].name)) { return &types[i]; }
    }
    return NULL;
}

  ///////////////
 // variables //
///////////////

#define MAX_VARS_IN_SCOPE 64
struct {
    type_s* type;
    char name[MAX_TOKEN_LEN];
    uint8_t scope;
    uint8_t address;
} typedef var_s;

var_s vars[MAX_VARS_IN_SCOPE] = { 0 };
uint16_t vars_count = 0;

static var_s* get_variable(char* name) {
    for (int i = vars_count - 1; i >= 0; i--) {
        if (!strcmp(name, vars[i].name)) { return &vars[i]; }
    }
    return NULL;
}

static uint16_t store_variable(type_s* type, char* name, uint8_t scope) {
    assert(vars_count < MAX_VARS_IN_SCOPE);

    // store type
    vars[vars_count].type = type;

    // store name
    memset(vars[vars_count].name, NULL, MAX_TOKEN_LEN);
    strcpy(vars[vars_count].name, name);

    // store scope
    vars[vars_count].scope = scope;

    // calculate and store address
    uint16_t address = 0;
    if (vars_count > 0) {
        address = vars[vars_count - 1].address + vars[vars_count - 1].type->size;
    }
    vars[vars_count].address = address;

    // increment
    vars_count++;

    return address;
}

  ////////////////////////////
 // scheduled future nodes //
////////////////////////////

static uint16_t future_stack[FUTURE_STACK_SIZE] = { 0 };
static uint16_t future_stack_count = 0;

/*static void future_stack_print(void) {
    printf("    ");
    for (int i = 0; i < future_stack_count; i++) {
        printf("%d ", future_stack[i]);
    }
    printf("\n");
}*/

static void future_push(uint16_t offset) {
    if (offset == NULL) { return; }
    future_stack[future_stack_count] = offset;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static void future_push_bytecode(bytecode_t bc) {
    future_push(bc);
    future_push(SCHEDULE_RAW_BYTECODE);
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
    output(BC_PUSH16, (uint16_t)atoi(token));
}

static void gen_variable(void) {
    read_token();

    var_s* var = get_variable(token);
    assert(var != NULL);

    output(BC_IGET16, var->address);
}

static void gen_unary_op(void) {
    read_token();
    switch (token[0]) {
        case '-': output(BC_NEG16); break;
        default: assert(FALSE); break;
    }
}

static void gen_factor(void) {
    uint16_t unary_op = cur_node.children[0];
    uint16_t value = cur_node.children[1];
    future_push(unary_op);
    future_push(value);
}

static void gen_term_op(void) {
    read_token();
    switch (token[0]) {
        case '*': output(BC_MUL16); break;
        case '/': output(BC_DIV16); break;
        default: assert(FALSE); break;
    }
}

static void gen_term(void) {
    uint16_t right_factor = cur_node.children[0];
    uint16_t term_op = cur_node.children[1];
    uint16_t left_factor = cur_node.children[2];
    future_push(term_op);
    future_push(left_factor);
    future_push(right_factor);
}

static void gen_expression_op(void) {
    read_token();
    switch (token[0]) {
        case '+': output(BC_ADD16); break;
        case '-': output(BC_SUB16); break;
        default: assert(FALSE); break;
    }
}

static void gen_expression(void) {
    uint16_t right_term = cur_node.children[0];
    uint16_t expression_op = cur_node.children[1];
    uint16_t left_term = cur_node.children[2];
    future_push(expression_op);
    future_push(left_term);
    future_push(right_term);
}

static void gen_assignment(void) {
    // variable identifier
    read_token();
    var_s* var = get_variable(token);
    assert(var != NULL);

    // push pointer
    output(BC_PUSH16, var->address);

    // schedule set
    future_push_bytecode(BC_SET16);

    // schedule expression
    uint16_t expression = cur_node.children[0];
    future_push(expression);
}

static void gen_declaration(void) {
    // variable type
    read_token();
    type_s* type = get_type(token);
    assert(type != NULL);

    // variable identifier
    read_token();
    var_s* var = get_variable(token);
    assert(var == NULL);

    uint16_t addr = store_variable(type, token, 0);

    // allocate space
    output(BC_PUSH16, 0);

    // push pointer
    output(BC_PUSH16, addr);

    // schedule set
    future_push_bytecode(BC_SET16);

    // schedule expression
    uint16_t expression = cur_node.children[0];
    future_push(expression);
}

static void gen_statement(void) {
    uint16_t next_statement = cur_node.children[0];
    uint16_t this_statement = cur_node.children[1];
    future_push(next_statement);
    future_push(this_statement);
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

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        uint16_t offset = future_pop();
        // check for raw bytecode output
        if (offset == SCHEDULE_RAW_BYTECODE) {
            output(future_pop());
            continue;
        }

        // navigate to offset and parse node
        read_ast_node(offset);
        switch(cur_node.node_type) {
            case NT_STATEMENT: gen_statement(); break;
            case NT_DECLARATION: gen_declaration(); break;
            case NT_ASSIGNMENT: gen_assignment(); break;
            case NT_EXPRESSION: gen_expression(); break;
            case NT_EXPRESSION_OP: gen_expression_op(); break;
            case NT_TERM: gen_term(); break;
            case NT_TERM_OP: gen_term_op(); break;
            case NT_FACTOR: gen_factor(); break;
            case NT_UNARY_OP: gen_unary_op(); break;
            case NT_VARIABLE: gen_variable(); break;
            case NT_CONSTANT: gen_constant(); break;
            default: assert(FALSE); break;
       }
    }
    assert(bail < 1000);
}

  //////////
 // main //
//////////

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char src_buffer[128] = { 0 };
    sprintf(src_buffer, "../examples/%s.src", argv[1]);
    src_ptr = fopen(src_buffer, "rb");

    char ast_buffer[128] = { 0 };
    sprintf(ast_buffer, "../bin/%s.ast", argv[1]);
    ast_ptr = fopen(ast_buffer, "rb");

    char gen_buffer[128] = { 0 };
    sprintf(gen_buffer, "../bin/%s.gen", argv[1]);
    gen_ptr = fopen(gen_buffer, "wb+");

    gen(src_ptr, ast_ptr, gen_ptr);

    fclose(src_ptr);
    fclose(ast_ptr);
    fclose(gen_ptr);
    return 0;
}
