#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "constants.h"
#include "variables.h"

static var_s vars[MAX_VARS_IN_SCOPE] = { 0 };
static uint16_t vars_count = 0;

var_s* get_variable(char* name) {
    for (int i = vars_count - 1; i >= 0; i--) {
        if (!strcmp(name, vars[i].name)) { return &vars[i]; }
    }
    return NULL;
}

uint16_t store_variable(type_t type, char* name, uint8_t scope) {
    assert(vars_count < MAX_VARS_IN_SCOPE);
    assert(get_variable(name) == NULL);

    // store type
    vars[vars_count].type = type;

    // store name
    memset(vars[vars_count].name, NULL, MAX_TOKEN_LEN);
    strcpy(vars[vars_count].name, name);

    // store scope
    vars[vars_count].scope = scope;

    // calculate and store address
    uint16_t address = 0;
    if (vars_count > 0) {
        address = vars[vars_count - 1].address + types[vars[vars_count - 1].type].size;
    }
    vars[vars_count].address = address;

    // increment
    vars_count++;

    return address;
}
