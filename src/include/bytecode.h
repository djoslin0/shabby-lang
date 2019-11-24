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
    BC_NOOP = 0,
    BC_PUSH,
    BC_NEG,
    BC_ADD,
    BC_SUB,
    BC_MUL,
    BC_DIV,
    BC_EOF = EOF,
} typedef bytecode_t;

static bytecode_s bytecode[] = {
    [BC_NOOP] = { 0, DBG_STR("noop") },
    [BC_PUSH] = { 1, DBG_STR("push") },
    [BC_NEG] = { 0, DBG_STR("neg") },
    [BC_ADD] = { 0, DBG_STR("add") },
    [BC_SUB] = { 0, DBG_STR("sub") },
    [BC_MUL] = { 0, DBG_STR("mul") },
    [BC_DIV] = { 0, DBG_STR("div") },
};

#endif
