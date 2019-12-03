#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nodes.h"
#include "constants.h"
#include "types.h"

type_t get_type(char* s) {
    uint16_t types_count = sizeof(types) / sizeof(type_s);
    for (int i = 0; i < types_count; i++) {
        if (i == TYPE_NONE) { continue; }
        if (!strcmp(s, types[i].name)) { return (type_t)i; }
    }
    return TYPE_NONE;
}

uint16_t get_user_type(FILE* ast_ptr, char* token, uint16_t offset) {
    ast_s peeked_node = { 0 };
    // read current
    ast_read_node(ast_ptr, offset, &peeked_node);
    uint16_t highest_offset = peeked_node.parent_offset;

    // search up
    while (offset != NULL) {
        // read current
        ast_read_node(ast_ptr, offset, &peeked_node);
        uint16_t next_offset = highest_offset;
        if (peeked_node.offset == highest_offset) {
            highest_offset = peeked_node.parent_offset;
        }

        // look only at statement
        if (peeked_node.node_type != NT_STATEMENT) { goto next_up; }

        // explore down before going up
        if (peeked_node.children[0] != NULL) { next_offset = peeked_node.children[0]; }

        // look only at statements child
        ast_read_node(ast_ptr, peeked_node.children[1], &peeked_node);
        if (peeked_node.node_type != NT_CLASS) { goto next_up; }

        // look only at matching tokens
        char peeked_token[MAX_TOKEN_LEN+1];
        ast_peek_token(ast_ptr, peeked_token);
        if (strcmp(token, peeked_token)) { goto next_up; }

        return peeked_node.offset;

next_up:
        // go to next offset
        offset = next_offset;
    }

    assert(FALSE);
    // make pedantic compilers happy
    node_constants[0] = node_constants[0];
}
