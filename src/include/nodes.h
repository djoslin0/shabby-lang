#ifndef NODES_H
#define NODES_H
#include "constants.h"

enum {
    // Parser Only
    NT_NONE,
    NT_ROOT,
    NT_CONSUME,
    NT_STATEMENT_LIST,

    // Shared
    NT_CLASS,
    NT_STATEMENT,
    NT_DECLARATION,
    NT_ASSIGNMENT,
    NT_EXPRESSION,
    NT_EXPRESSION_OP,
    NT_TERM,
    NT_TERM_OP,
    NT_FACTOR,
    NT_UNARY_OP,
    NT_VARIABLE,
    NT_CONSTANT,
    NT_CAST,

    // Debug
    #ifdef DEBUG
        NT_DEBUG_UNINDENT_NODE,
    #endif
} typedef node_t;


#define NTP_DECLARATION_BYTES 0
#define NTP_CLASS_BYTES 0
#define NTP_CLASS_PENDING 1

#ifdef DEBUG
struct {
    char name[MAX_TOKEN_LEN];
    uint8_t child_count;
    uint8_t param_count;
    uint8_t output_token_count;
} typedef node_s;

static node_s node_constants[] = {
    // Parser Only
    [NT_NONE] = { "none", 0, 0, 0 },
    [NT_ROOT] = { "root", 0, 0, 0 },
    [NT_CONSUME] = { "consume", 0, 0, 0 },
    [NT_STATEMENT_LIST] = { "statement_list", 0, 0, 0 },

    // Shared
    [NT_CLASS] = { "class", 1, 2, 1 },
    [NT_STATEMENT] = { "statement", 2, 0, 0 },
    [NT_DECLARATION] = { "declaration", 1, 1, 2 },
    [NT_ASSIGNMENT] = { "assignment", 1, 0, 1 },
    [NT_EXPRESSION] = { "expression", 3, 0, 0 },
    [NT_EXPRESSION_OP] = { "expression_op", 0, 0, 1 },
    [NT_TERM] = { "term", 3, 0, 0 },
    [NT_TERM_OP] = { "term_op", 0, 0, 1 },
    [NT_FACTOR] = { "factor", 2, 0, 0 },
    [NT_UNARY_OP] = { "unary_op", 0, 0, 1 },
    [NT_VARIABLE] = { "variable", 0, 0, 1 },
    [NT_CONSTANT] = { "constant", 0, 0, 1 },
    [NT_CAST] = { "cast", 1, 0, 1 },

    // debug
    [NT_DEBUG_UNINDENT_NODE] = { "debug_unindent_node", 0, 0, 0 },
};
#endif

#define MAX_AST_CHILDREN 4
struct {
    uint16_t offset;
    node_t node_type;
    uint8_t value_type;
    uint16_t scratch;
    uint16_t parent_offset;
    uint16_t children[MAX_AST_CHILDREN];
} typedef ast_s;

#define AST_ADDR_NODE_TYPE(offset) (offset)
#define AST_ADDR_VALUE_TYPE(offset) (AST_ADDR_NODE_TYPE(offset) + 1)
#define AST_ADDR_SCRATCH(offset) (AST_ADDR_VALUE_TYPE(offset) + 1)
#define AST_ADDR_PARENT(offset) (AST_ADDR_SCRATCH(offset) + 2)
#define AST_ADDR_CHILD(offset, child_index) ((offset == 0) \
                             ? 0 \
                             : (AST_ADDR_PARENT(offset) + 2 + (uint16_t)child_index * 2))
#define AST_ADDR_PARAM(node_type, offset, param_index) ((offset == 0) \
                             ? 0 \
                             : (AST_ADDR_CHILD(offset, node_constants[node_type].child_count ) + (uint16_t)param_index * 2))

void ast_read_node(FILE*, uint16_t, ast_s*);
uint16_t ast_new_node(FILE*, node_t, uint16_t, uint8_t);
uint16_t ast_insert_new_node(FILE*, ast_s*);
void ast_overwrite_scratch(FILE*, uint16_t, uint16_t);
uint16_t ast_get_param(FILE*, node_t, uint16_t, uint8_t);
void ast_set_param(FILE*, node_t, uint16_t, uint8_t, uint16_t);

#endif
