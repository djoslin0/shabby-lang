#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"
#include "types.h"

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

static char token[MAX_TOKEN_LEN+1];
static void read_token(void) {
    int last_position = ftell(ast_ptr);
    memset(token, NULL, MAX_TOKEN_LEN);
    fgets(token, MAX_TOKEN_LEN, ast_ptr);
    assert(strlen(token) < MAX_TOKEN_LEN);
    fseek(ast_ptr, last_position + strlen(token) + 1, 0);
}

  //////////////
 // graphing //
//////////////

void graph(FILE *ast_ptr_arg, FILE *dot_ptr_arg) {
    ast_ptr = ast_ptr_arg;
    dot_ptr = dot_ptr_arg;

    // write header
    fputs("digraph L {\n", dot_ptr);
    fputs("  node [shape=Mrecord,style=filled]\n", dot_ptr);

    // move past root pointer
    future_push(fget16(ast_ptr));

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        ast_s node = { 0 };
        // navigate to offset and parse node
        uint16_t offset = future_pop();
        ast_read_node(ast_ptr, offset, &node);

        // write node header
        node_s constants = node_constants[node.node_type];
        fprintf(dot_ptr, "  %d [label=<%s", offset, constants.name);

        // write info header
        if (constants.param_count > 0 || constants.output_token_count > 0) {
            fputs("<br/><font point-size='10'>", dot_ptr);
        }

        // write params
        if (constants.param_count > 0) {
            for (int i = 0; i < constants.param_count; i++) {
                uint16_t param = ast_get_param(ast_ptr, node.node_type, node.offset, i);
                fprintf(dot_ptr, "%d", param);
                if (i != constants.param_count - 1) {
                    fputs(", ", dot_ptr);
                }
            }
        }

        // write space between params and tokens
        if (constants.param_count > 0 && constants.output_token_count > 0) {
            fputs(", ", dot_ptr);
        }

        // write tokens
        if (constants.output_token_count > 0) {
            for (int i = 0; i < constants.output_token_count; i++) {
                read_token();
                fprintf(dot_ptr, "%s", token);
                if (i != constants.output_token_count - 1) {
                    fputs(" ", dot_ptr);
                }
            }
        }

        // write info footer
        if (constants.param_count > 0 || constants.output_token_count > 0) {
            fputs("</font>", dot_ptr);
        }

        // write end of label
        fprintf(dot_ptr, ">");

        // fill color with type
        switch (node.value_type) {
            case TYPE_NONE: fprintf(dot_ptr, ", fillcolor=\"white\""); break;
            case TYPE_USER_DEFINED: fprintf(dot_ptr, ", fillcolor=\"#FFEEEE\""); break;
            case TYPE_BYTE: fprintf(dot_ptr, ", fillcolor=\"#EEFFEE\""); break;
            case TYPE_SHORT: fprintf(dot_ptr, ", fillcolor=\"#EEEEFF\""); break;
            default: printf("%d\n", node.value_type); assert(FALSE);
        }

        // write node footer
        fprintf(dot_ptr, "]\n");

        // write children
        for (int i = node_constants[node.node_type].child_count - 1; i >= 0; i--) {
            if (node.children[i] == NULL) { continue; }
            fprintf(dot_ptr, "  %d -> %d\n", offset, node.children[i]);
            future_push(node.children[i]);
        }

        // write parent
        if (node.parent_offset != 0) {
            fprintf(dot_ptr, "  %d -> %d[penwidth=0.15, arrowhead=curve];\n", offset, node.parent_offset);
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

    char ast_buffer[256] = { 0 };
    sprintf(ast_buffer, "../bin/compilation/%s.ast", "out");
    ast_ptr = fopen(ast_buffer, "rb");

    char dot_buffer[256] = { 0 };
    sprintf(dot_buffer, "../bin/compilation/%s.dot", "out");
    dot_ptr = fopen(dot_buffer, "w+");

    graph(ast_ptr, dot_ptr);
    printf("Created graph: ../bin/compilation/%s.png\n", "out");
    fclose(dot_ptr);
    fclose(ast_ptr);

    return 0;

    // make pedantic compilers happy
    types[0] = types[0];
    argv[0] = argv[0];
}
#else
int main(int argc, char *argv[]) {
    return 0;
}
#endif
