#ifndef NODES_H
#define NODES_H

enum {
    NT_NONE,
    NT_ROOT,
    NT_EXPRESSION,
    NT_EXPRESSION_OP,
    NT_EXPRESSION_PAREN,
    NT_TERM,
    NT_TERM_OP,
    NT_FACTOR,
    NT_UNARY_OP,
    NT_CONSTANT,
    #ifdef DEBUG
    NT_DEBUG_UNINDENT_NODE,
    #endif
} typedef node_type;

#endif