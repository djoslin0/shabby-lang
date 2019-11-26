#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "constants.h"
#include "nodes.h"
#include "file.h"

void read_ast_node(FILE* ast_ptr, uint16_t offset, ast_s* result) {
    fseek(ast_ptr, offset, 0);
    result->node_type = fgetc(ast_ptr);
    result->value_type = fgetc(ast_ptr);
    result->parent_offset = fget16(ast_ptr);
    result->child_count = fgetc(ast_ptr);
    assert(result->child_count < MAX_AST_CHILDREN);
    memset(result->children, NULL, MAX_AST_CHILDREN * sizeof(uint16_t));
    for (int i = 0; i < result->child_count; i++) {
        result->children[i] = fget16(ast_ptr);
    }

    // make pedantic compilers happy
    node_constants[0] = node_constants[0];
}
