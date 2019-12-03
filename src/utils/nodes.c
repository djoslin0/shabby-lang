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
    uint8_t param_count = node_constants[result->node_type].param_count;
    if (param_count > 0) {
        fseek(ast_ptr, param_count * sizeof(uint16_t), SEEK_CUR);
    }
}

static void ast_rewrite_node(FILE* ast_ptr, ast_s* node) {
    // output: <node_type> <value_type><scratch> <*parent> <children*...>
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
    for (int i = 0; i < node_constants[node_type].param_count; i++) { fput16(0, ast_ptr); }

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
    for (int i = 0; i < node_constants[node->node_type].param_count; i++) { fput16(0, ast_ptr); }

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

uint16_t ast_get_param(FILE* ast_ptr, node_t node_type, uint16_t offset, uint8_t param_index) {
    // save the fp
    uint16_t return_offset = ftell(ast_ptr);

    // read byte_size
    fseek(ast_ptr, AST_ADDR_PARAM(node_type, offset, param_index), 0);
    uint16_t value = fget16(ast_ptr);

    // reset the fp
    fseek(ast_ptr, return_offset, 0);

    return value;
}

void ast_set_param(FILE* ast_ptr, node_t node_type, uint16_t offset, uint8_t param_index, uint16_t value) {
    // save the fp
    uint16_t return_offset = ftell(ast_ptr);

    // read byte_size
    fseek(ast_ptr, AST_ADDR_PARAM(node_type, offset, param_index), 0);
    fput16(value, ast_ptr);

    // reset the fp
    fseek(ast_ptr, return_offset, 0);
}

void ast_peek_token(FILE* ast_ptr, char* buffer) {
    int last_position = ftell(ast_ptr);
    memset(buffer, NULL, MAX_TOKEN_LEN);
    fgets(buffer, MAX_TOKEN_LEN, ast_ptr);
    assert(strlen(buffer) < MAX_TOKEN_LEN);
    fseek(ast_ptr, last_position + strlen(buffer) + 1, 0);
}

uint16_t ast_get_member(FILE* ast_ptr, uint16_t user_type_offset, char* member_token) {
    ast_s peeked_node = { 0 };
    // read user type
    ast_read_node(ast_ptr, user_type_offset, &peeked_node);
    assert(peeked_node.node_type == NT_CLASS);

    uint16_t next_offset = peeked_node.children[0];
    while (next_offset != NULL) {
        // read statement, prepare next statement
        ast_read_node(ast_ptr, next_offset, &peeked_node);
        next_offset = peeked_node.children[0];
        assert(peeked_node.node_type == NT_STATEMENT);

        // read declaration
        if (peeked_node.children[1] == NULL) { continue; }
        ast_read_node(ast_ptr, peeked_node.children[1], &peeked_node);
        if (peeked_node.node_type != NT_DECLARATION) { continue; }

        // check for matching token
        char peeked_token[MAX_TOKEN_LEN+1];
        ast_peek_token(ast_ptr, peeked_token); // skip over type token
        ast_peek_token(ast_ptr, peeked_token); // retrieve member token
        if (strcmp(peeked_token, member_token)) { continue; }

        // found
        return peeked_node.offset;
    }

    assert(FALSE);
}

uint16_t ast_get_member_address(FILE* ast_ptr, uint16_t type_member_offset) {
    ast_s peeked_node = { 0 };
    // read user type
    ast_read_node(ast_ptr, type_member_offset, &peeked_node);
    assert(peeked_node.node_type == NT_DECLARATION);

    // read parent
    ast_read_node(ast_ptr, peeked_node.parent_offset, &peeked_node);
    assert(peeked_node.node_type == NT_STATEMENT);

    uint16_t address = 0;
    uint16_t next_offset = peeked_node.parent_offset;
    while (next_offset != NULL) {
        // read statement, prepare next statement
        ast_read_node(ast_ptr, next_offset, &peeked_node);
        next_offset = peeked_node.parent_offset;
        if (peeked_node.node_type != NT_STATEMENT) { break; }

        // read declaration
        if (peeked_node.children[1] == NULL) { continue; }
        ast_read_node(ast_ptr, peeked_node.children[1], &peeked_node);
        if (peeked_node.node_type != NT_DECLARATION) { continue; }

        // add to address
        address += ast_get_param(ast_ptr, NT_DECLARATION, peeked_node.offset, NTP_DECLARATION_BYTES);
    }

    return address;
}
