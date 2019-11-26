#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"
#include "types.h"
#include "variables.h"

  ///////////////////
 // file pointers //
///////////////////

static FILE *ast_ptr = NULL; // ast input/output

  //////////
 // misc //
//////////

static ast_s cur_node = { 0 };

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

  /////////////////////
 // write utilities //
/////////////////////

static void write_ast_type(type_t type) {
    printf("%02X -> %02X\n", type, cur_node.offset + 1);
    fseek(ast_ptr, cur_node.offset + 1, 0);
    fputc(type, ast_ptr);
}

static void write_propagate(type_t type) {
    write_ast_type(type);
    while (cur_node.value_type == TYPE_NONE
           && cur_node.node_type != NT_STATEMENT
           && cur_node.node_type != NT_DECLARATION
           && cur_node.node_type != NT_ASSIGNMENT) {

        uint16_t parent_offset = cur_node.parent_offset;
        write_ast_type(type);

        switch (cur_node.node_type) {
            case NT_EXPRESSION:
            case NT_TERM:
                if (cur_node.children[1] != NULL) {
                    assert(cur_node.child_count > 1);
                    read_ast_node(ast_ptr, cur_node.children[1], &cur_node);
                    write_ast_type(type);
                }
                break;
            case NT_FACTOR:
                if (cur_node.children[0] != NULL) {
                    assert(cur_node.child_count > 0);
                    read_ast_node(ast_ptr, cur_node.children[0], &cur_node);
                    write_ast_type(type);
                }
            default: break;
        }

        read_ast_node(ast_ptr, parent_offset, &cur_node);
    }
}

  //////////////////
 // typechecking //
//////////////////

static void tc_variable(void) {
    read_token();
    var_s* var = get_variable(token);
    assert(var->type != TYPE_NONE);
    write_propagate(var->type);
}

static void tc_constant(void) {
    long value = 0;
    assert(fscanf(ast_ptr, "%ld", &value) == 1);

    // figure out which type to assign to the constant
    if (value >= -128 && value <= 127) {
        // TODO: change to byte
        write_propagate(TYPE_SHORT);
    } else if (value >= -32768 && value <= 32767) {
        write_propagate(TYPE_SHORT);
    } else {
        assert(FALSE);
    }
}

static void tc_assignment(void) {
    read_token();
    var_s* var = get_variable(token);
    assert(var->type != TYPE_NONE);
    write_ast_type(var->type);
}

static void tc_declaration(void) {
    read_token();
    type_t type = get_type(token);
    assert(type != TYPE_NONE);

    read_token();
    store_variable(type, token, 0);

    write_ast_type(type);
}

void typecheck(FILE *ast_ptr_arg) {
    ast_ptr = ast_ptr_arg;

    // move to root node
    future_push(fget16(ast_ptr));

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        uint16_t offset = future_pop();
        // navigate to offset and parse node
        read_ast_node(ast_ptr, offset, &cur_node);
        switch (cur_node.node_type) {
            case NT_DECLARATION: tc_declaration(); break;
            case NT_ASSIGNMENT: tc_assignment(); break;
            case NT_VARIABLE: tc_variable(); break;
            case NT_CONSTANT: tc_constant(); break;
            default: break;
        }
        for (int i = 0; i < cur_node.child_count; i++) {
            future_push(cur_node.children[i]);
        }
    }
    assert(bail < 1000);
}

  //////////
 // main //
//////////

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char ast_buffer[128] = { 0 };
    sprintf(ast_buffer, "../bin/%s.ast", argv[1]);
    ast_ptr = fopen(ast_buffer, "r+b");

    typecheck(ast_ptr);

    // make pedantic compilers happy
    node_constants[0] = node_constants[0];
    types[0] = types[0];

    fclose(ast_ptr);
    return 0;
}
