#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "constants.h"
#include "variables.h"

static var_s vars[MAX_VARS_IN_SCOPE] = { 0 };
static uint16_t vars_count = 0;
static uint8_t current_scope = 0;

void scope_increment(void) {
    assert(current_scope < 100);
    current_scope++;
}

void scope_decrement(void) {
    assert(current_scope > 0);
    current_scope--;
    while (vars_count > 0 && vars[vars_count - 1].scope > current_scope) {
        vars_count--;
    }
}

var_s* get_variable(char* name) {
    for (int i = vars_count - 1; i >= 0; i--) {
        if (!strcmp(name, vars[i].name)) { return &vars[i]; }
    }
    return NULL;
}

uint16_t store_variable(type_t type, char* name, uint16_t size) {
    assert(vars_count < MAX_VARS_IN_SCOPE);
    var_s* same_name = get_variable(name);
    assert(same_name == NULL || same_name->scope < current_scope);

    // store type
    vars[vars_count].type = type;

    // store name
    memset(vars[vars_count].name, NULL, MAX_TOKEN_LEN);
    strcpy(vars[vars_count].name, name);

    // store scope
    vars[vars_count].scope = current_scope;

    // store size
    vars[vars_count].size = size;

    // calculate and store address
    uint16_t address = 0;
    if (vars_count > 0 && vars[vars_count - 1].scope == current_scope) {
        address = vars[vars_count - 1].address + vars[vars_count - 1].size;
    }
    vars[vars_count].address = address;

    // increment
    vars_count++;

    return address;

    // make pedantic compilers happy
    types[0] = types[0];
}
