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

  //////////
 // misc //
//////////

static ast_s cur_node = { 0 };

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
 // graphing //
//////////////

void graph(FILE *ast_ptr_arg, FILE *dot_ptr_arg) {
    ast_ptr = ast_ptr_arg;
    dot_ptr = dot_ptr_arg;

    // write header
    fputs("digraph L {\n", dot_ptr);
    fputs("node [shape=Mrecord,style=filled]\n", dot_ptr);

    // move past root pointer
    uint16_t offset = 2;

    int bail = 0;
    while(++bail < 1000) {
        // navigate to offset and parse node
        read_ast_node(ast_ptr, offset, &cur_node);

        node_s constants = node_constants[cur_node.node_type];

        fprintf(dot_ptr, "%d [", offset);

        switch (cur_node.value_type) {
            case TYPE_NONE: fprintf(dot_ptr, "fillcolor=\"white\","); break;
            case TYPE_BYTE: fprintf(dot_ptr, "fillcolor=\"#EEFFEE\","); break;
            case TYPE_SHORT: fprintf(dot_ptr, "fillcolor=\"#EEEEFF\","); break;
            default: printf("%d\n", cur_node.value_type); assert(FALSE);
        }

        fprintf(dot_ptr, "label=<%s", constants.name);
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

        for (int i = 0; i < cur_node.child_count; i++) {
            if (cur_node.children[i] == NULL) { continue; }
            fprintf(dot_ptr, "%d -> %d\n", offset, cur_node.children[i]);
        }

        if (cur_node.parent_offset != 0) {
            fprintf(dot_ptr, "%d -> %d[penwidth=0.15, arrowhead=curve];\n", offset, cur_node.parent_offset);
        }

        offset = ftell(ast_ptr);
        if (fgetc(ast_ptr) == EOF) { break; }
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

    // make pedantic compilers happy
    types[0] = types[0];

    return 0;
}
#else
int main(int argc, char *argv[]) {
    return 0;
}
#endif
