#ifndef BYTECODE_H
#define BYTECODE_H

#include "constants.h"

struct {
    uint8_t params;
    #ifdef DEBUG
    char name[MAX_TOKEN_LEN];
    #endif
} typedef bytecode_s;

enum {
    // misc
    BC_NOOP = 0,

    // stack
    BC_PUSH,
    BC_POP,

    // pointers
    BC_SET,
    BC_GET,
    BC_IGET,

    // math
    BC_NEG,
    BC_ADD,
    BC_SUB,
    BC_MUL,
    BC_DIV,

    // misc
    BC_EOF = EOF,

} typedef bytecode_t;

static bytecode_s bytecode[] = {
    // misc
    [BC_NOOP] = { 0, DBG_STR("noop") },

    // stack
    [BC_PUSH] = { 1, DBG_STR("push") },
    [BC_POP] = { 0, DBG_STR("pop") },

    // pointers
    [BC_SET] = { 0, DBG_STR("set") },
    [BC_GET] = { 0, DBG_STR("get") },
    [BC_IGET] = { 1, DBG_STR("iget") },

    // math
    [BC_NEG] = { 0, DBG_STR("neg") },
    [BC_ADD] = { 0, DBG_STR("add") },
    [BC_SUB] = { 0, DBG_STR("sub") },
    [BC_MUL] = { 0, DBG_STR("mul") },
    [BC_DIV] = { 0, DBG_STR("div") },
};

#endif
