#ifndef VARIABLES_H
#define VARIABLES_H

#include "constants.h"
#include "types.h"

#define MAX_VARS_IN_SCOPE 64

typedef struct {
    type_t type;
    char name[MAX_TOKEN_LEN+1];
    uint8_t scope;
    uint8_t address;
    uint16_t size;
    uint16_t offset;
    uint16_t user_type_offset;
} var_s;

var_s* get_variable(char*);
uint16_t store_variable(type_t, char*, uint16_t, uint16_t, uint16_t);
void scope_increment(void);
void scope_decrement(void);

#endif
