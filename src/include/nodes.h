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

#endif
