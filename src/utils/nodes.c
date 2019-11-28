#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nodes.h"
#include "file.h"

void ast_read_node(FILE* ast_ptr, uint16_t offset, ast_s* result) {
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
}

static void ast_rewrite_node(FILE* ast_ptr, ast_s* node) {
    // output: <node_type> <value_type> <scratch> <*parent> <children*...>
    fseek(ast_ptr, node->offset, 0);

    // write node_type
    fputc(node->node_type, ast_ptr);
    // write value_type
    fputc(node->value_type, ast_ptr);
    // write scratch
    fput16(node->scratch, ast_ptr);
    // write parent*
    fput16(node->parent_offset, ast_ptr);
    // write <children*...>
    uint8_t child_count = node_constants[node->node_type].child_count;
    for (int i = 0; i < child_count; i++) {
        fput16(node->children[i], ast_ptr);
    }
}

uint16_t ast_new_node(FILE* ast_ptr, node_t node_type, uint16_t parent_offset, uint8_t child_index) {
    // insert new node on the end
    fseek(ast_ptr, 0, SEEK_END);
    uint16_t my_offset = ftell(ast_ptr);

    // set up new node
    ast_s node = { 0 };
    node.offset = my_offset;
    node.node_type = node_type;
    node.parent_offset = parent_offset;

    // write new node
    ast_rewrite_node(ast_ptr, &node);

    // write to parent
    fseek(ast_ptr, AST_ADDR_CHILD(parent_offset, child_index), 0);
    fput16(node.offset, ast_ptr);
    fseek(ast_ptr, 0, SEEK_END);

    return my_offset;
}

uint16_t ast_insert_new_node(FILE* ast_ptr, ast_s* node) {

    // get parent type
    fseek(ast_ptr, AST_ADDR_NODE_TYPE(node->parent_offset), 0);
    node_t parent_type = fgetc(ast_ptr);

    // read parent's children
    uint8_t parent_child_count = node_constants[parent_type].child_count;
    assert(parent_child_count > 0);
    uint16_t parents_children[MAX_AST_CHILDREN] = { 0 };
    fseek(ast_ptr, AST_ADDR_CHILD(node->parent_offset, 0), 0);
    for (int i = 0; i < parent_child_count; i++) {
        parents_children[i] = fget16(ast_ptr);
    }

    // compare parent's children to mine to find where to insert
    uint8_t parents_child_index = (uint8_t) -1;
    uint8_t child_count = node_constants[node->node_type].child_count;
    for (int i = 0; i < child_count; i++) {
        uint16_t child = node->children[i];
        if (child == NULL) { continue; }

        for (int j = 0; j < parent_child_count; j++) {
            if (child == parents_children[j]) {
                parents_child_index = j;
                goto found;
            }
        }
    }
    assert(FALSE);
found:

    // insert new node on the end
    fseek(ast_ptr, 0, SEEK_END);
    uint16_t my_offset = ftell(ast_ptr);

    // write new node
    node->offset = my_offset;
    ast_rewrite_node(ast_ptr, node);

    // write to parent
    fseek(ast_ptr, AST_ADDR_CHILD(node->parent_offset, parents_child_index), 0);
    fput16(node->offset, ast_ptr);

    // write to children
    for (int i = 0; i < child_count; i++) {
        if (node->children[i] == NULL) { continue; }
        fseek(ast_ptr, AST_ADDR_PARENT(node->children[i]), 0);
        fput16(my_offset, ast_ptr);
    }

    fseek(ast_ptr, 0, SEEK_END);
    return my_offset;
}

void ast_overwrite_scratch(FILE* ast_ptr, uint16_t offset, uint16_t value) {
    // save the fp
    uint16_t return_offset = ftell(ast_ptr);

    // write scratch
    fseek(ast_ptr, AST_ADDR_SCRATCH(offset), 0);
    fput16(value, ast_ptr);

    // reset the fp
    fseek(ast_ptr, return_offset, 0);
}
