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


#ifdef DEBUG
struct {
    char name[MAX_TOKEN_LEN];
    uint8_t child_count;
    uint8_t output_token_count;
} typedef node_s;

static node_s node_constants[] = {
    // Parser Only
    [NT_NONE] = { "none", 0, 0 },
    [NT_ROOT] = { "root", 0, 0 },
    [NT_CONSUME] = { "consume", 0, 0 },
    [NT_STATEMENT_LIST] = { "statement_list", 0, 0 },

    // Shared
    [NT_STATEMENT] = { "statement", 2, 0 },
    [NT_DECLARATION] = { "declaration", 1, 2 },
    [NT_ASSIGNMENT] = { "assignment", 1, 1 },
    [NT_EXPRESSION] = { "expression", 3, 0 },
    [NT_EXPRESSION_OP] = { "expression_op", 0, 1 },
    [NT_TERM] = { "term", 3, 0 },
    [NT_TERM_OP] = { "term_op", 0, 1 },
    [NT_FACTOR] = { "factor", 2, 0 },
    [NT_UNARY_OP] = { "unary_op", 0, 1 },
    [NT_VARIABLE] = { "variable", 0, 1 },
    [NT_CONSTANT] = { "constant", 0, 1 },
    [NT_CAST] = { "cast", 1, 1 },

    // debug
    [NT_DEBUG_UNINDENT_NODE] = { "debug_unindent_node", 0, 0 },
};
#endif

#define MAX_AST_CHILDREN 4
struct {
    uint16_t offset;
    node_t node_type;
    uint8_t value_type;
    uint16_t parent_offset;
    uint16_t children[MAX_AST_CHILDREN];
    uint16_t scratch;
} typedef ast_s;

#define AST_ADDR_NODE_TYPE(x) (x)
#define AST_ADDR_VALUE_TYPE(x) (AST_ADDR_NODE_TYPE(x) + 1)
#define AST_ADDR_SCRATCH(x) (AST_ADDR_VALUE_TYPE(x) + 1)
#define AST_ADDR_PARENT(x) (AST_ADDR_SCRATCH(x) + 2)
#define AST_ADDR_CHILD(x, y) ((x == 0) \
                          ? 0 \
                          : (AST_ADDR_PARENT(x) + 2 + (uint16_t)y * 2))



void ast_read_node(FILE*, uint16_t, ast_s*);
uint16_t ast_new_node(FILE*, node_t, uint16_t, uint8_t);
uint16_t ast_insert_new_node(FILE*, ast_s*);
void ast_overwrite_scratch(FILE*, uint16_t, uint16_t);

#endif
