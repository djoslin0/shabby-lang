#ifndef VARIABLES_H
#define VARIABLES_H

#include "constants.h"
#include "types.h"

#define MAX_VARS_IN_SCOPE 64

struct {
    type_t type;
    char name[MAX_TOKEN_LEN];
    uint8_t scope;
    uint8_t address;
} typedef var_s;

var_s* get_variable(char*);
uint16_t store_variable(type_t, char*);
void scope_increment(void);
void scope_decrement(void);

#endif
