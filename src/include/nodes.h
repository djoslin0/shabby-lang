#ifndef NODES_H
#define NODES_H

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

    // Debug
    #ifdef DEBUG
        NT_DEBUG_UNINDENT_NODE,
    #endif
} typedef node_t;


#ifdef DEBUG
struct {
    char name[MAX_TOKEN_LEN];
    uint8_t output_token_count;
} typedef node_s;

static node_s node_constants[] = {
    // Parser Only
    [NT_NONE] = { .name = "none", .output_token_count = 0 },
    [NT_ROOT] = { .name = "root", .output_token_count = 0 },
    [NT_CONSUME] = { .name = "consume", .output_token_count = 0 },
    [NT_STATEMENT_LIST] = { .name = "statement_list", .output_token_count = 0 },

    // Shared
    [NT_STATEMENT] = { .name = "statement", .output_token_count = 0 },
    [NT_DECLARATION] = { .name = "declaration", .output_token_count = 2 },
    [NT_ASSIGNMENT] = { .name = "assignment", .output_token_count = 1 },
    [NT_EXPRESSION] = { .name = "expression", .output_token_count = 0 },
    [NT_EXPRESSION_OP] = { .name = "expression_op", .output_token_count = 1 },
    [NT_TERM] = { .name = "term", .output_token_count = 0 },
    [NT_TERM_OP] = { .name = "term_op", .output_token_count = 1 },
    [NT_FACTOR] = { .name = "factor", .output_token_count = 0 },
    [NT_UNARY_OP] = { .name = "unary_op", .output_token_count = 1 },
    [NT_VARIABLE] = { .name = "variable", .output_token_count = 1 },
    [NT_CONSTANT] = { .name = "constant", .output_token_count = 1 },

    // debug
    [NT_DEBUG_UNINDENT_NODE] = { .name = "debug_unindent_node", .output_token_count = 0 },
};
#endif

#endif
