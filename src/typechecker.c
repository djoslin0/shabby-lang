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

struct {
    uint8_t constant_count;
    int32_t constant_value;
} typedef const_expr_t;

#define MAX_SCRATCH 64
static const_expr_t scratch[MAX_SCRATCH] = { 0 };
static uint8_t on_scratch_index = 0;

  ////////////////////////////
 // scheduled future nodes //
////////////////////////////

#define FUTURE_SCOPE_DECREMENT ((uint16_t)-1)

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

static void ast_write_type(type_t type, uint16_t offset) {
    fseek(ast_ptr, offset + 1, 0);
    fputc(type, ast_ptr);
}

static uint16_t insert_cast(type_t type, uint16_t parent_offset, uint16_t child_offset) {
    ast_s node = { 0 };
    node.node_type = NT_CAST;
    node.parent_offset = parent_offset;
    node.children[0] = child_offset;
    node.value_type = type;
    printf("<inserted cast>\n");
    ast_insert_new_node(ast_ptr, &node);
    fprintf(ast_ptr, "%s", node_constants[NT_CAST].name);
    return node.offset;
}

  ///////////////////////////////
 // constant expression phase //
///////////////////////////////

static void ce_propagate(uint32_t value) {
    ast_s node = cur_node;
    while (TRUE) {
        // search for parent that is type <term> or <expression> and has a <term_op> or a <expression_op>
        bool searching = TRUE;
        while (searching) {
            ast_read_node(ast_ptr, node.parent_offset, &node);
            switch (node.node_type) {
                case NT_STATEMENT: return;
                case NT_TERM:
                case NT_EXPRESSION:
                    if (node.children[1] != NULL) { searching = FALSE; }
                    break;
                case NT_CAST:
                    read_token();
                    // cast constant expression value
                    switch (get_type(token)) {
                        case TYPE_BYTE: value = (int8_t)value; break;
                        case TYPE_SHORT: value = (int16_t)value; break;
                        default: assert(FALSE);
                    }
                    break;
                case NT_FACTOR:
                    // apply unary op on constant expression value
                    if (node.children[0] == NULL) { break; }
                    ast_read_node(ast_ptr, node.children[0], &peeked_node);
                    read_token();
                    switch (token[0]) {
                        case '-': value = -value; break;
                        default: assert(FALSE);
                    }
                    break;
                default: break;
            }
        }

        // create scratch structure
        if (cur_node.scratch == 0) {
            on_scratch_index++;
            assert(on_scratch_index < MAX_SCRATCH);
            node.scratch = on_scratch_index;
            ast_overwrite_scratch(ast_ptr, node.offset, on_scratch_index);
        }

        const_expr_t* ce = &scratch[node.scratch];

        // increment
        ce->constant_count++;
        assert(ce->constant_count <= 2);

        // this term/expression hasn't been seen by another constant yet, store the value
        if (ce->constant_count == 1) {
            ce->constant_value = value;
            return;
        }

        // retrieve term/expression operator
        ast_read_node(ast_ptr, node.children[1], &peeked_node);
        read_token();

        // evaluate term/expression
        switch(token[0]) {
            case '+': ce->constant_value += value; break;
            case '-': ce->constant_value -= value; break;
            case '*': ce->constant_value *= value; break;
            case '/': ce->constant_value /= value; break;
            default: assert(FALSE);
        }

        // decide on type
        type_t type = TYPE_NONE;
        if (ce->constant_value >= -128 && ce->constant_value <= 127) {
            type = TYPE_BYTE;
        } else if (ce->constant_value >= -32768 && ce->constant_value <= 32767) {
            type = TYPE_SHORT;
        } else {
            printf("Type error!\n"
                   "Constant expression exceeds the largest datatype bounds.\n"
                   "Evaluated to: %d\n", ce->constant_value);
            assert(FALSE);
        }

        // write type
        ast_write_type(type, node.offset);

        // pass this value up to the next term/expression
        value = ce->constant_value;
    }
}

static void ec_evaluate(void) {
    // move to root node
    fseek(ast_ptr, 0, 0);
    future_push(fget16(ast_ptr));

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        uint16_t offset = future_pop();
        // navigate to offset and parse node
        ast_read_node(ast_ptr, offset, &cur_node);

        // search children
        uint8_t child_count = node_constants[cur_node.node_type].child_count;
        for (int i = 0; i < child_count; i++) {
            future_push(cur_node.children[i]);
        }

        // extract constant value
        if (cur_node.node_type == NT_CONSTANT) {
            int32_t value = 0;
            assert(fscanf(ast_ptr, "%d", &value) == 1);
            ce_propagate(value);
        }

    }
    assert(bail < 1000);
}

  ////////////////////////
 // typechecking phase //
////////////////////////

static void tc_propagate(type_t type) {
    ast_s node = cur_node;
    ast_write_type(type, node.offset);
    uint8_t depth = 0;
    while (TRUE) {
        printf("      %s\n", node_constants[node.node_type].name);

        // check for exit conditions
        if (type == node.value_type && depth > 0) { return; }
        if (type < node.value_type && depth > 0) { return; }
        switch (node.node_type) {
            case NT_CAST: if (depth > 0) { return; } else { break; }
            case NT_STATEMENT:
            case NT_DECLARATION:
            case NT_ASSIGNMENT:
                // check for a type error
                if (type > node.value_type) {
                    printf("\nType error: \n"
                        "'%s' provided, when '%s' was expected.\n\n",
                        types[type].name,
                        types[node.value_type].name);
                    assert(FALSE);
                }
                return;
            default: break;
        }

        // write out type information
        ast_write_type(type, node.offset);

        // check children to see if they need to be altered
        uint8_t child_count = node_constants[node.node_type].child_count;
        for (uint8_t i = 0; i < child_count; i++) {
            if (node.children[i] == NULL) { continue; }
            ast_read_node(ast_ptr, node.children[i], &peeked_node);
            switch (peeked_node.node_type) {
                case NT_EXPRESSION_OP:
                case NT_TERM_OP:
                case NT_UNARY_OP:
                    if (type == TYPE_USER_DEFINED) { assert(FALSE); }
                    // always set operators to the same type as their parent
                    ast_write_type(type, peeked_node.offset);
                    break;
                default: break;
            }
        }

        // iterate
        ast_read_node(ast_ptr, node.parent_offset, &node);
        depth++;
    }
}

static void tc_cast(void) {
    read_token();
    type_t type = get_type(token);
    assert(type != TYPE_NONE);

    tc_propagate(type);
}

static void tc_variable(void) {
    read_token();
    var_s* var = get_variable(token);
    assert(var->type != TYPE_NONE);
    tc_propagate(var->type);
}

static void tc_constant(void) {
    int32_t value = 0;
    assert(fscanf(ast_ptr, "%d", &value) == 1);

    // apply unary op of factor
    ast_read_node(ast_ptr, cur_node.parent_offset, &peeked_node);
    assert(peeked_node.node_type == NT_FACTOR);
    if (peeked_node.children[0] != NULL) {
        ast_read_node(ast_ptr, peeked_node.children[0], &peeked_node);
        read_token();
        switch (token[0]) {
            case '-': value = -value; break;
            default: assert(FALSE);
        }
    }

    // find the first <expression> or <term> with a type
    ast_read_node(ast_ptr, cur_node.parent_offset, &peeked_node);
    while (TRUE) {
        switch (peeked_node.node_type) {
            case NT_DECLARATION:
            case NT_ASSIGNMENT:
            case NT_STATEMENT:
                goto failed_search;
            default: break;
        }
        printf("%d\n", peeked_node.scratch);
        if (scratch[peeked_node.scratch].constant_count == 2 && peeked_node.value_type != NULL) {
            tc_propagate(peeked_node.value_type);
            return;
        }
        ast_read_node(ast_ptr, peeked_node.parent_offset, &peeked_node);
    }

failed_search:
    // figure out which type to assign to the constant
    if (value >= -128 && value <= 127) {
        tc_propagate(TYPE_BYTE);
    } else if (value >= -32768 && value <= 32767) {
        tc_propagate(TYPE_SHORT);
    } else {
        assert(FALSE);
    }
}

static void tc_term(void) {
    if (cur_node.value_type == TYPE_NONE) { return; }
    tc_propagate(cur_node.value_type);
}

static void tc_expression(void) {
    if (cur_node.value_type == TYPE_NONE) { return; }
    tc_propagate(cur_node.value_type);
}

static void tc_assignment(void) {
    read_token();
    var_s* var = get_variable(token);
    assert(var->type != TYPE_NONE);
    ast_write_type(var->type, cur_node.offset);
}

static void tc_declaration(void) {
    read_token();
    type_t type = get_type(token);
    if (type == TYPE_NONE) {
        type = TYPE_USER_DEFINED;
    }

    read_token();
    store_variable(type, token, types[type].size);

    ast_write_type(type, cur_node.offset);
}

static void tc_class(void) {
    scope_increment();
    future_push(FUTURE_SCOPE_DECREMENT);
}

static void tc_evaluate(void) {
    // move to root node
    fseek(ast_ptr, 0, 0);
    future_push(fget16(ast_ptr));

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        uint16_t offset = future_pop();
        if (offset == FUTURE_SCOPE_DECREMENT) {
            scope_decrement();
            continue;
        }
        // navigate to offset and parse node
        ast_read_node(ast_ptr, offset, &cur_node);
        printf("\n");
        printf("%04X: %s\n", offset, node_constants[cur_node.node_type].name);
        switch (cur_node.node_type) {
            case NT_CLASS: tc_class(); break;
            case NT_DECLARATION: tc_declaration(); break;
            case NT_ASSIGNMENT: tc_assignment(); break;
            case NT_EXPRESSION: tc_expression(); break;
            case NT_TERM: tc_term(); break;
            case NT_VARIABLE: tc_variable(); break;
            case NT_CONSTANT: tc_constant(); break;
            case NT_CAST: tc_cast(); break;
            default: break;
        }
        uint8_t child_count = node_constants[cur_node.node_type].child_count;
        for (int i = 0; i < child_count; i++) {
            future_push(cur_node.children[i]);
        }
    }
    assert(bail < 1000);
}

  ///////////////////
 // casting phase //
///////////////////

static void cast_evaluate(void) {
    // move to root node
    fseek(ast_ptr, 0, 0);
    future_push(fget16(ast_ptr));

    int bail = 0;
    while(future_stack_count > 0 && ++bail < 1000) {
        uint16_t offset = future_pop();
        // navigate to offset and parse node
        ast_read_node(ast_ptr, offset, &cur_node);

        // search children
        uint8_t child_count = node_constants[cur_node.node_type].child_count;
        for (int i = 0; i < child_count; i++) {
            // schedule child that exists
            if (cur_node.children[i] == NULL) { continue; }
            future_push(cur_node.children[i]);

            // figure out if we need to cast
            if (cur_node.node_type == NT_CAST) { continue; }
            if (cur_node.value_type == TYPE_NONE) { continue; }
            ast_read_node(ast_ptr, cur_node.children[i], &peeked_node);

            // only up-cast
            if (peeked_node.value_type == TYPE_NONE) { continue; }
            if (peeked_node.value_type == cur_node.value_type) { continue; }
            assert(peeked_node.value_type < cur_node.value_type);
            insert_cast(cur_node.value_type, cur_node.offset, peeked_node.offset);
        }

    }
    assert(bail < 1000);
}

void typecheck(FILE *ast_ptr_arg) {
    ast_ptr = ast_ptr_arg;

    // evaluate constant expressions
    ec_evaluate();

    // evaluate and propagate types
    tc_evaluate();

    // insert casts where required
    cast_evaluate();
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
