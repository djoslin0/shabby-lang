#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"

#ifdef DEBUG
  ///////////////////
 // file pointers //
///////////////////

static FILE *ast_ptr = NULL; // ast input
static FILE *dot_ptr = NULL; // output for graphviz

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

  //////////////
 // graphing //
//////////////

void graph(FILE *ast_ptr_arg, FILE *dot_ptr_arg) {
    ast_ptr = ast_ptr_arg;
    dot_ptr = dot_ptr_arg;

    // write header
    fputs("digraph L {\n", dot_ptr);
    fputs("node [shape=Mrecord]\n", dot_ptr);

    // move to root node
    future_push(fget16(ast_ptr));

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        uint16_t offset = future_pop();
        // navigate to offset and parse node
        read_ast_node(offset);

        node_s constants = node_constants[cur_node.node_type];

        fprintf(dot_ptr, "%d [label=<%s", offset, constants.name);
        if (constants.output_token_count > 0) {
            fputs("<br/><font point-size='10'>", dot_ptr);
            for (int i = 0; i < constants.output_token_count; i++) {
                read_token();
                fprintf(dot_ptr, "%s", token);
                if (i != constants.output_token_count - 1) {
                    fputs(" ", dot_ptr);
                }
            }
            fputs("</font>", dot_ptr);
        }
        fprintf(dot_ptr, ">]\n");

        if (cur_node.parent_offset != 0) {
            fprintf(dot_ptr, "%d -> %d\n", cur_node.parent_offset, offset);
        }
        for (int i = 0; i < cur_node.child_count; i++) {
            future_push(cur_node.children[i]);
        }
    }
    assert(bail < 1000);

    // write footer
    fputs("}\n", dot_ptr);
}

  //////////
 // main //
//////////

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char ast_buffer[128] = { 0 };
    sprintf(ast_buffer, "../bin/%s.ast", argv[1]);
    ast_ptr = fopen(ast_buffer, "rb");

    char dot_buffer[128] = { 0 };
    sprintf(dot_buffer, "../bin/%s.dot", argv[1]);
    dot_ptr = fopen(dot_buffer, "w+");

    graph(ast_ptr, dot_ptr);
    printf("Created graph: ../bin/%s.png\n", argv[1]);
    fclose(dot_ptr);
    fclose(ast_ptr);
    return 0;
}
#else
int main(int argc, char *argv[]) {
    return 0;
}
#endif
