#ifndef BYTECODE_H
#define BYTECODE_H

#include "constants.h"

struct {
    uint8_t params;
    uint8_t param_size;
    #ifdef DEBUG
    char name[MAX_TOKEN_LEN];
    #endif
} typedef bytecode_s;

enum {
    // misc
    BC_NOOP = 0,
    BC_EXTEND,

    // stack
    BC_PUSH8,
    BC_PUSH16,

    BC_POP8,
    BC_POP16,

    // pointers
    BC_SET8,
    BC_SET16,

    BC_GET8,
    BC_GET16,

    BC_IGET8,
    BC_IGET16,

    // math
    BC_NEG8,
    BC_NEG16,

    BC_ADD8,
    BC_ADD16,

    BC_SUB8,
    BC_SUB16,

    BC_MUL8,
    BC_MUL16,

    BC_DIV8,
    BC_DIV16,

    // misc
    BC_EOF = EOF,
} typedef bytecode_t;

static bytecode_s bytecode[] = {
    // misc
    [BC_NOOP] = { 0, 0, DBG_STR("noop") },
    [BC_EXTEND] = { 0, 1, DBG_STR("extend") },

    // stack
    [BC_PUSH8] = { 1, 1, DBG_STR("push8") },
    [BC_PUSH16] = { 1, 2, DBG_STR("push16") },

    [BC_POP8] = { 0, 0, DBG_STR("pop8") },
    [BC_POP16] = { 0, 0, DBG_STR("pop16") },

    // pointers
    [BC_SET8] = { 0, 0, DBG_STR("set8") },
    [BC_SET16] = { 0, 0, DBG_STR("set16") },

    [BC_GET8] = { 0, 0, DBG_STR("get8") },
    [BC_GET16] = { 0, 0, DBG_STR("get16") },

    [BC_IGET8] = { 1, 2, DBG_STR("iget8") },
    [BC_IGET16] = { 1, 2, DBG_STR("iget16") },

    // math
    [BC_NEG8] = { 0, 0, DBG_STR("neg8") },
    [BC_NEG16] = { 0, 0, DBG_STR("neg16") },

    [BC_ADD8] = { 0, 0, DBG_STR("add8") },
    [BC_ADD16] = { 0, 0, DBG_STR("add16") },

    [BC_SUB8] = { 0, 0, DBG_STR("sub8") },
    [BC_SUB16] = { 0, 0, DBG_STR("sub16") },

    [BC_MUL8] = { 0, 0, DBG_STR("mul8") },
    [BC_MUL16] = { 0, 0, DBG_STR("mul16") },

    [BC_DIV8] = { 0, 0, DBG_STR("div8") },
    [BC_DIV16] = { 0, 0, DBG_STR("div16") },
};

#endif
