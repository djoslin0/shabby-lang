#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nodes.h"
#include "file.h"

void read_ast_node(FILE* ast_ptr, uint16_t offset, ast_s* result) {
    fseek(ast_ptr, offset, 0);
    result->offset = offset;
    result->node_type = fgetc(ast_ptr);
    result->value_type = fgetc(ast_ptr);
    result->scratch = fget16(ast_ptr);
    result->parent_offset = fget16(ast_ptr);
    memset(result->children, NULL, MAX_AST_CHILDREN * sizeof(uint16_t));
    uint8_t child_count = node_constants[result->node_type].child_count;
    for (int i = 0; i < child_count; i++) {
        result->children[i] = fget16(ast_ptr);
    }
    // make pedantic compilers happy
    node_constants[0] = node_constants[0];
}

static void write_current_offset_to(FILE* ast_ptr, uint16_t write_offset) {
    // write to pointer
    uint16_t cur_offset = ftell(ast_ptr);
    fseek(ast_ptr, write_offset, 0);
    fput16(cur_offset, ast_ptr);
    fseek(ast_ptr, cur_offset, 0);
}

uint16_t write_ast_node(FILE* ast_ptr, node_t node_type,
                        uint16_t parent_offset, uint8_t child_index) {
    // output: <node_type> <value_type> <scratch> <*parent> <children*...>
    uint16_t my_offset = ftell(ast_ptr);

    // write to parent
    if (parent_offset == 0 && child_index == 0) {
        write_current_offset_to(ast_ptr, 0);
    } else {
        fseek(ast_ptr, my_offset, 0);
        write_current_offset_to(ast_ptr,
                                parent_offset + 6 + (uint16_t)child_index * 2);
    }

    // write node_type
    fputc(node_type, ast_ptr);
    // write value_type
    fputc(NULL, ast_ptr);
    // write scratch
    fput16(0, ast_ptr);
    // write parent*
    fput16(parent_offset, ast_ptr);
    // write <children*...>
    uint8_t child_count = node_constants[node_type].child_count;
    for (int i = 0; i < child_count; i++) {
        fput16(NULL, ast_ptr);
    }
    return my_offset;
}

void overwrite_child_pointer(FILE* ast_ptr, uint16_t parent_offset,
                             uint8_t child_index, uint16_t child_offset) {
    uint16_t cur_offset = ftell(ast_ptr);

    // write to parent pointer
    uint16_t offset = parent_offset + 6 + child_index * 2;
    fseek(ast_ptr, offset, 0);
    fput16(child_offset, ast_ptr);

    // write to child pointer
    offset = child_offset + 4;
    fseek(ast_ptr, offset, 0);
    fput16(parent_offset, ast_ptr);

    fseek(ast_ptr, cur_offset, 0);
}

void overwrite_scratch(FILE* ast_ptr, uint16_t offset, uint16_t scratch) {
    uint16_t cur_offset = ftell(ast_ptr);

    // write scratch
    fseek(ast_ptr, offset + 2, 0);
    fput16(scratch, ast_ptr);

    fseek(ast_ptr, cur_offset, 0);
}
