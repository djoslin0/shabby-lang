#ifndef BYTECODE_H
#define BYTECODE_H

#include "constants.h"

typedef struct {
    uint8_t params;
    uint8_t param_size;
    #ifdef DEBUG
    char name[MAX_TOKEN_LEN+1];
    #endif
} bytecode_s;

typedef enum {
    // misc
    BC_NOOP = 0,
    BC_EXTEND,

    // jumps
    BC_JUMP,
    BC_IJUMP,
    BC_LABEL,

    // functions
    BC_CALL,
    BC_RET,

    // program counter
    BC_PUSH_PC,
    BC_POP_PC,

    // frame pointer
    BC_PUSH_FP,
    BC_POP_FP,

    // stack basics
    BC_PUSH_ZEROS,

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
} bytecode_t;

static bytecode_s bytecode[] = {
    // misc
    [BC_NOOP] = { 0, 0, DBG_STR("noop") },
    [BC_EXTEND] = { 0, 1, DBG_STR("extend") },

    // jumps
    [BC_JUMP] = { 0, 0, DBG_STR("jump") },
    [BC_IJUMP] = { 1, 2, DBG_STR("ijump") },
    [BC_LABEL] = { 1, 2, DBG_STR("label") },

    // functions
    [BC_CALL] = { 2, 2, DBG_STR("call") },
    [BC_RET] = { 0, 0, DBG_STR("ret") },

    // program counter
    [BC_PUSH_PC] = { 0, 0, DBG_STR("push_pc") },
    [BC_POP_PC] = { 0, 0, DBG_STR("pop_pc") },

    // frame pointer
    [BC_PUSH_FP] = { 0, 0, DBG_STR("push_fp") },
    [BC_POP_FP] = { 0, 0, DBG_STR("pop_fp") },

    // stack basics
    [BC_PUSH_ZEROS] = { 1, 2, DBG_STR("push_zeros") },

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
