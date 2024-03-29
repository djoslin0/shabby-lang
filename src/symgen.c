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

  /////////////////////
 // token utilities //
/////////////////////

static char token[MAX_TOKEN_LEN+1];
// TODO: consolidate
static void read_token(void) {
    int last_position = ftell(ast_ptr);
    memset(token, NULL, MAX_TOKEN_LEN);
    fgets(token, MAX_TOKEN_LEN, ast_ptr);
    assert(strlen(token) < MAX_TOKEN_LEN);
    fseek(ast_ptr, last_position + strlen(token) + 1, 0);
}

  //////////
 // misc //
//////////

static ast_s cur_node = { 0 };
static ast_s peeked_node = { 0 };

  ////////////////////////////
 // scheduled future nodes //
////////////////////////////

typedef struct {
    uint16_t offset;
    uint8_t attempts;
} future_info_s;

static future_info_s future_stack[FUTURE_STACK_SIZE] = { 0 };
static uint16_t future_stack_count = 0;

static void future_prepend(uint16_t offset, uint8_t attempts) {
    for (int i = future_stack_count; i > 0; i--) {
        future_stack[i] = future_stack[i - 1];
    }
    future_stack[0].offset = offset;
    future_stack[0].attempts = attempts;
    future_stack_count++;
}

static void future_push(uint16_t offset, uint8_t attempts) {
    if (offset == NULL) { return; }
    future_stack[future_stack_count].offset = offset;
    future_stack[future_stack_count].attempts = attempts;
    future_stack_count++;
    assert(future_stack_count < FUTURE_STACK_SIZE);
}

static future_info_s future_pop(void) {
    assert(future_stack_count > 0);
    future_stack_count--;
    return future_stack[future_stack_count];
}

  /////////////////////////////
 // symbol generation phase //
/////////////////////////////

#define PENDING_FLAG_DIRTY (1 << 15)

static void sg_declaration(uint8_t attempts) {
    assert(attempts < 3);

    // read type
    read_token();

    bool is_type_ready = FALSE;
    uint16_t type_offset = NULL;
    uint16_t pending = 0;

    // find type size
    uint16_t size = 0;
    switch (get_type(token)) {
        case TYPE_BYTE: size = 1; is_type_ready = TRUE; break;
        case TYPE_SHORT: size = 2; is_type_ready = TRUE; break;
        default:
            type_offset = get_user_type(ast_ptr, token, cur_node.parent_offset);
            assert(type_offset != NULL);
            pending = ast_get_param(ast_ptr, NT_CLASS, type_offset, NTP_CLASS_PENDING);
            is_type_ready = (pending == PENDING_FLAG_DIRTY);
            if (is_type_ready) {
                size = ast_get_param(ast_ptr, NT_CLASS, type_offset, NTP_CLASS_BYTES);
            }
            break;
    }

    // figure out if we should add or subtract to parent class's pending amount
    int8_t pending_diff = 0;
    if (is_type_ready && attempts > 0) {
        pending_diff = -1;
    } else if (!is_type_ready && attempts == 0) {
        pending_diff = 1;
    }

    if (!is_type_ready) {
        // reschedule node
        printf("RESCHEDULE!\n");
        future_prepend(cur_node.offset, attempts + 1);
    } else {
        // set this declaration's size
        ast_set_param(ast_ptr, cur_node.node_type, cur_node.offset, NTP_DECLARATION_BYTES, size);
    }

    // search for class to add this declaration's size to
    uint16_t parent_offset = cur_node.parent_offset;
    while (parent_offset != NULL) {
        ast_read_node(ast_ptr, parent_offset, &peeked_node);
        switch (peeked_node.node_type) {
            case NT_CLASS:
                if (is_type_ready) {
                    size += ast_get_param(ast_ptr, NT_CLASS, parent_offset, NTP_CLASS_BYTES);
                    ast_set_param(ast_ptr, NT_CLASS, parent_offset, NTP_CLASS_BYTES, size);
                }
                // dirty the class so we know a member has been read
                pending = ast_get_param(ast_ptr, NT_CLASS, parent_offset, NTP_CLASS_PENDING);
                pending += pending_diff;
                ast_set_param(ast_ptr, NT_CLASS, parent_offset, NTP_CLASS_PENDING, PENDING_FLAG_DIRTY | pending);
                return;
            case NT_STATEMENT: break;
            default: assert(FALSE);
        }
        parent_offset = peeked_node.parent_offset;
    }

}

static void sg_statement(void) {

    printf("%02X %02X\n", cur_node.children[0], cur_node.children[1]);

    // push next statement
    future_push(cur_node.children[0], 0);

    // push child
    future_push(cur_node.children[1], 0);
}

static void sg_class(void) {
    // if there is no child, there is nothing to do.
    if (cur_node.children[0] == NULL) { return; }
    future_push(cur_node.children[0], 0);
}

static void sg_evaluate(void) {
    // move to root node
    fseek(ast_ptr, 0, 0);
    future_push(fget16(ast_ptr), 0);

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        future_info_s info = future_pop();
        // navigate to offset and parse node
        ast_read_node(ast_ptr, info.offset, &cur_node);
        printf("%s\n", node_constants[cur_node.node_type].name);

        switch (cur_node.node_type) {
            case NT_CLASS: sg_class(); break;
            case NT_STATEMENT: sg_statement(); break;
            case NT_DECLARATION: sg_declaration(info.attempts); break;
            default: break;
        }
    }
    assert(bail < 1000);
}

void symgen(FILE *ast_ptr_arg) {
    ast_ptr = ast_ptr_arg;

    sg_evaluate();
}

  //////////
 // main //
//////////

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char ast_buffer[256] = { 0 };
    sprintf(ast_buffer, "../bin/compilation/%s.ast", "out");
    ast_ptr = fopen(ast_buffer, "r+b");

    symgen(ast_ptr);

    fclose(ast_ptr);
    return 0;

    // make pedantic compilers happy
    node_constants[0] = node_constants[0];
    types[0] = types[0];
    argv[0] = argv[0];
}
