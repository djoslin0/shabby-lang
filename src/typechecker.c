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
static ast_s peeked_node = { 0 };

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

static void write_ast_type(type_t type, uint16_t offset) {
    fseek(ast_ptr, offset + 1, 0);
    fputc(type, ast_ptr);
}

static uint16_t insert_cast(type_t type, uint16_t parent_offset, uint8_t child_index, uint16_t child_offset) {
    // write the new cast node
    fseek(ast_ptr, 0, SEEK_END);
    uint16_t cast_offset = write_ast_node(ast_ptr, NT_CAST, parent_offset,
                                          child_index, 1, 1);
    // insert token into ast file
    fputs(types[type].name, ast_ptr);
    fputc(NULL, ast_ptr);

    // write cast's the value type
    write_ast_type(type, cast_offset);

    // overwrite pointers
    overwrite_child_pointer(ast_ptr, cast_offset, 0, child_offset);
    return cast_offset;
}

static void write_propagate(type_t type) {
    uint16_t last_offset = cur_node.offset;
    write_ast_type(type, cur_node.offset);
    uint8_t depth = 0;
    while (TRUE) {
        printf("      %s\n", node_constants[cur_node.node_type].name);

        // up-cast last node if it's parent is a larger byte size
        if (type < cur_node.value_type) {
            // insert cast between cur_node and last_offset
            assert(cur_node.child_count > 0);
            for (uint8_t i = 0; i < cur_node.child_count; i++) {
                if (cur_node.children[i] == last_offset) {
                    insert_cast(cur_node.value_type, cur_node.offset, i, last_offset);
                    return;
                }
            }
        }

        // check for exit conditions
        if (type == cur_node.value_type) { return; }
        switch (cur_node.node_type) {
            case NT_CAST: if (depth > 0) { return; } else { break; }
            case NT_STATEMENT:
            case NT_DECLARATION:
            case NT_ASSIGNMENT:
                // check for a type error
                if (type > cur_node.value_type) {
                    printf("\nType error: \n"
                           "'%s' provided, when '%s' was expected.\n\n",
                           types[type].name,
                           types[cur_node.value_type].name);
                    assert(FALSE);
                }
                return;
            default: break;
        }

        // write out type information
        write_ast_type(type, cur_node.offset);


        // check children to see if they need to be altered
        for (uint8_t i = 0; i < cur_node.child_count; i++) {
            if (cur_node.children[i] == NULL) { continue; }
            read_ast_node(ast_ptr, cur_node.children[i], &peeked_node);
            switch (peeked_node.node_type) {
                case NT_EXPRESSION_OP:
                case NT_TERM_OP:
                case NT_UNARY_OP:
                    // always set operators to the same type as their parent
                    write_ast_type(type, peeked_node.offset);
                    break;
                default:
                    // cast children who need it
                    if (peeked_node.value_type == TYPE_NONE) { break; }
                    if (type <= peeked_node.value_type) { break; }
                    insert_cast(type, cur_node.offset, i, peeked_node.offset);
            }
        }

        last_offset = cur_node.offset;
        read_ast_node(ast_ptr, cur_node.parent_offset, &cur_node);
        depth++;
    }
}

  //////////////////
 // typechecking //
//////////////////

static void tc_cast(void) {
    read_token();
    type_t type = get_type(token);
    assert(type != TYPE_NONE);

    write_propagate(type);
}

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
        write_propagate(TYPE_BYTE);
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
    write_ast_type(var->type, cur_node.offset);
}

static void tc_declaration(void) {
    read_token();
    type_t type = get_type(token);
    assert(type != TYPE_NONE);

    read_token();
    store_variable(type, token, 0);

    write_ast_type(type, cur_node.offset);
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
        printf("\n");
        printf("%04X: %s\n", offset, node_constants[cur_node.node_type].name);
        switch (cur_node.node_type) {
            case NT_DECLARATION: tc_declaration(); break;
            case NT_ASSIGNMENT: tc_assignment(); break;
            case NT_VARIABLE: tc_variable(); break;
            case NT_CONSTANT: tc_constant(); break;
            case NT_CAST: tc_cast(); break;
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
