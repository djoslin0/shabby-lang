#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "constants.h"
#include "types.h"

type_t get_type(char* s) {
    uint16_t types_count = sizeof(types) / sizeof(type_s);
    for (int i = 0; i < types_count; i++) {
        if (i == TYPE_NONE) { continue; }
        if (!strcmp(s, types[i].name)) { return (type_t)i; }
    }
    return TYPE_NONE;
}
