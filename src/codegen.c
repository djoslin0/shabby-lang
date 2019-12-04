#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"
#include "bytecode.h"
#include "types.h"
#include "variables.h"

  ///////////////////
 // file pointers //
///////////////////

static FILE *src_ptr = NULL; // source code
static FILE *ast_ptr = NULL; // abstract syntax tree
static FILE *gen_ptr = NULL; // output

  //////////
 // misc //
//////////

static ast_s cur_node = { 0 };
static ast_s peeked_node = { 0 };

#define BIT_CLASS_END (1 << 15)

#define SIZE_BC(x) (x + cur_node.value_type - 1)

  /////////////////////
 // token utilities //
/////////////////////

static char token[MAX_TOKEN_LEN+1];
// TODO: consolidate
static void read_token(void) {
    int last_position = ftell(ast_ptr);
    memset(token, NULL, MAX_TOKEN_LEN);
    fgets(token, MAX_TOKEN_LEN, ast_ptr);
    assert(strlen(token) < MAX_TOKEN_LEN);
    fseek(ast_ptr, last_position + strlen(token) + 1, 0);
}

  /////////////////////
 // class utilities //
/////////////////////

// TODO: remove me or consolidate with symgen
static bool is_class_member(uint16_t offset) {
    // search up
    while (offset != NULL) {
        // read current
        ast_read_node(ast_ptr, offset, &peeked_node);
        if (peeked_node.node_type == NT_CLASS) { return TRUE; }
        assert(peeked_node.node_type == NT_STATEMENT);
        offset = peeked_node.parent_offset;
    }

    return FALSE;
}

  ////////////////////////////
 // scheduled future nodes //
////////////////////////////

typedef enum {
    FUTURE_OFFSET,
    FUTURE_RAW_BYTECODE,
    FUTURE_CLASS_END,
} future_info_t;

typedef struct {
    future_info_t type;
    uint16_t data;
} future_info_s;

static future_info_s future_stack[FUTURE_STACK_SIZE] = { 0 };
static uint16_t future_stack_count = 0;

static void future_push_offset(uint16_t offset) {
    if (offset == NULL) { return; }
    future_stack[future_stack_count].type = FUTURE_OFFSET;
    future_stack[future_stack_count].data = offset;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static void future_push_bytecode(bytecode_t bc) {
    future_stack[future_stack_count].type = FUTURE_RAW_BYTECODE;
    future_stack[future_stack_count].data = bc;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static void future_push_class_end(uint16_t offset) {
    if (offset == NULL) { return; }
    future_stack[future_stack_count].type = FUTURE_CLASS_END;
    future_stack[future_stack_count].data = offset;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static future_info_s* future_pop(void) {
    assert(future_stack_count > 0);
    future_stack_count--;
    return &future_stack[future_stack_count];
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
        assert(bytecode[type].params != BC_VARIABLE_PARAMS);
        va_list args;
        va_start(args, type);
        for (uint8_t i = 0; i < bytecode[type].params; i++) {
            int param = va_arg(args, int);
            switch (bytecode[type].param_size) {
                case 0: break;
                case 1: fputc((uint8_t)param, gen_ptr); break;
                case 2: fput16((uint16_t)param, gen_ptr); break;
                default: assert(FALSE);
            }
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

static void gen_test(void) {
    uint16_t token_start = ftell(ast_ptr);

    // output start of test BC
    fputc(BC_TEST, gen_ptr);

    // count number of constants and output
    uint8_t c = fgetc(ast_ptr);
    uint16_t count = 0;
    while (c != NULL) {
        if (c == ' ') { count++; }
        c = fgetc(ast_ptr);
    }
    fput16(count, gen_ptr);

    // output constants
    int8_t constant = 0;
    int8_t sign = 1;
    fseek(ast_ptr, token_start, 0);
    c = fgetc(ast_ptr);
    while (c != NULL) {
        if (c == ' ') {
            fputc(constant * sign, gen_ptr);
            constant = 0;
            sign = 1;
         } else if (c == '-') {
             sign = -1;
         } else {
             constant *= 10;
             constant += (c - '0');
         }
        c = fgetc(ast_ptr);
    }
}


static void gen_cast(void) {
    type_t to_type = cur_node.value_type;

    ast_read_node(ast_ptr, cur_node.children[0], &peeked_node);
    type_t from_type = peeked_node.value_type;

    // remember to extend or pop after evaluating child node
    if (types[to_type].size == types[from_type].size) {
        // no op
    } else if (types[to_type].size > types[from_type].size) {
        for (int i = types[from_type].size; i < types[to_type].size; i++) {
            future_push_bytecode(BC_EXTEND);
        }
    } else {
        for (int i = types[to_type].size; i < types[from_type].size; i++) {
            future_push_bytecode(BC_POP8);
        }
    }

    // push the child node
    future_push_offset(peeked_node.offset);
}

static void gen_constant(void) {
    read_token();
    output(SIZE_BC(BC_PUSH8), (uint16_t)atoi(token));
}

static void gen_variable(void) {
    read_token();

    var_s* var = get_variable(token);
    assert(var != NULL);

    // get address offset
    uint16_t address = var->address + ast_get_param(ast_ptr, NT_VARIABLE, cur_node.offset, NTP_VARIABLE_ADDRESS);

    if (cur_node.value_type == TYPE_USER_DEFINED) {
        output(BC_PUSH16, address);
    } else {
        output(SIZE_BC(BC_IGET8), address);
    }
}

static void gen_unary_op(void) {
    read_token();
    switch (token[0]) {
        case '-': output(SIZE_BC(BC_NEG8)); break;
        default: assert(FALSE); break;
    }
}

static void gen_factor(void) {
    uint16_t unary_op = cur_node.children[0];
    uint16_t value = cur_node.children[1];
    future_push_offset(unary_op);
    future_push_offset(value);
}

static void gen_term_op(void) {
    read_token();
    switch (token[0]) {
        case '*': output(SIZE_BC(BC_MUL8)); break;
        case '/': output(SIZE_BC(BC_DIV8)); break;
        default: assert(FALSE); break;
    }
}

static void gen_term(void) {
    uint16_t right_factor = cur_node.children[0];
    uint16_t term_op = cur_node.children[1];
    uint16_t left_factor = cur_node.children[2];
    future_push_offset(term_op);
    future_push_offset(left_factor);
    future_push_offset(right_factor);
}

static void gen_expression_op(void) {
    read_token();
    switch (token[0]) {
        case '+': output(SIZE_BC(BC_ADD8)); break;
        case '-': output(SIZE_BC(BC_SUB8)); break;
        default: assert(FALSE); break;
    }
}

static void gen_expression(void) {
    uint16_t right_term = cur_node.children[0];
    uint16_t expression_op = cur_node.children[1];
    uint16_t left_term = cur_node.children[2];
    future_push_offset(expression_op);
    future_push_offset(left_term);
    future_push_offset(right_term);
}

static void gen_assignment(void) {
    // variable identifier
    read_token();
    var_s* var = get_variable(token);
    assert(var != NULL);

    // get address offset
    uint16_t address = var->address + ast_get_param(ast_ptr, NT_ASSIGNMENT, cur_node.offset, NTP_ASSIGNMENT_ADDRESS);

    // user types are handled differently: copy instead of set
    if (cur_node.value_type == TYPE_USER_DEFINED) {
        uint16_t user_type_size = ast_get_param(ast_ptr, NT_CLASS, var->user_type_offset, NTP_CLASS_BYTES);
        output(BC_PUSH16, user_type_size);
        output(BC_PUSH16, address);
        future_push_bytecode(BC_COPY);
    } else {
        output(BC_PUSH16, address);
        // schedule set
        future_push_bytecode(SIZE_BC(BC_SET8));
    }

    // schedule expression
    uint16_t expression = cur_node.children[0];
    future_push_offset(expression);
}

static void gen_declaration(void) {
    // variable type
    read_token();
    type_t type = get_type(token);
    // resolve user type
    uint16_t user_type_offset = NULL;
    if (type == TYPE_NONE) {
        uint16_t return_to = ftell(ast_ptr);
        user_type_offset = get_user_type(ast_ptr, token, cur_node.offset);
        fseek(ast_ptr, return_to, 0);
        type = TYPE_USER_DEFINED;
    }
    assert(type != TYPE_NONE);

    // discover byte size
    uint16_t bytes = ast_get_param(ast_ptr, cur_node.node_type, cur_node.offset, NTP_DECLARATION_BYTES);

    // remember variable
    read_token();
    uint16_t addr = store_variable(type, token, bytes, cur_node.offset, user_type_offset);

    // allocate bytes
    if (!is_class_member(cur_node.parent_offset)) {
        switch (bytes) {
            case 0: assert(FALSE);
            case 1: output(BC_PUSH8, 0); break;
            case 2: output(BC_PUSH16, 0); break;
            default: output(BC_PUSH_ZEROS, bytes); break;
        }
    }

    // call into class
    if (type == TYPE_USER_DEFINED) {
        output(BC_CALL, addr, user_type_offset);
        assert(cur_node.children[0] == NULL);
        return;
    }

    // don't set anything if there is nothing to set
    if (cur_node.children[0] == NULL) {
        return;
    }

    // push pointer
    output(BC_PUSH16, addr);

    // schedule set
    assert(bytes > 0 && bytes <= 2);
    future_push_bytecode(SIZE_BC(BC_SET8));

    // schedule expression
    uint16_t expression = cur_node.children[0];
    future_push_offset(expression);
}

static void gen_statement(void) {
    uint16_t next_statement = cur_node.children[0];
    uint16_t this_statement = cur_node.children[1];
    future_push_offset(next_statement);
    future_push_offset(this_statement);
}

static void gen_class(void) {
    scope_increment();
    output(BC_IJUMP, (BIT_CLASS_END | cur_node.offset));
    output(BC_LABEL, cur_node.offset);
    future_push_class_end(cur_node.offset);
    future_push_offset(cur_node.children[0]);
}

static void gen_class_end(uint16_t offset) {
    scope_decrement();
    output(BC_RET);
    output(BC_LABEL, (BIT_CLASS_END | offset));
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
    future_push_offset(fget16(ast_ptr));

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        future_info_s* cur_info = future_pop();
        switch (cur_info->type) {
            case FUTURE_CLASS_END: gen_class_end(cur_info->data); continue;
            case FUTURE_RAW_BYTECODE: output(cur_info->data); continue;
            case FUTURE_OFFSET: break;
        }

        // navigate to offset and parse node
        ast_read_node(ast_ptr, cur_info->data, &cur_node);
        printf("                %s:\n", node_constants[cur_node.node_type].name);
        switch(cur_node.node_type) {
            case NT_CLASS: gen_class(); break;
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
            case NT_CAST: gen_cast(); break;
            case NT_TEST: gen_test(); break;
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

    char src_buffer[256] = { 0 };
    sprintf(src_buffer, "%s", argv[1]);
    src_ptr = fopen(src_buffer, "rb");

    char ast_buffer[256] = { 0 };
    sprintf(ast_buffer, "../bin/compilation/%s.ast", "out");
    ast_ptr = fopen(ast_buffer, "rb");

    char gen_buffer[256] = { 0 };
    sprintf(gen_buffer, "../bin/compilation/%s.gen", "out");
    gen_ptr = fopen(gen_buffer, "wb+");

    gen(src_ptr, ast_ptr, gen_ptr);

    fclose(src_ptr);
    fclose(ast_ptr);
    fclose(gen_ptr);

    return 0;

    #ifdef DEBUG
    // make pedantic compilers happy
    node_constants[0] = node_constants[0];
    types[0] = types[0];
    #endif
}
