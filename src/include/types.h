#ifndef TYPES_H
#define TYPES_H

enum {
    TYPE_NONE,
    TYPE_BYTE,
    TYPE_SHORT,
    //TYPE_INT,
    //TYPE_FLOAT,
    TYPE_USER_DEFINED,
} typedef type_t;

struct {
    uint16_t size;
    char name[MAX_TOKEN_LEN];
} typedef type_s;

static type_s types[] = {
    [TYPE_USER_DEFINED] = { .size = 0, .name = "@USER" },
    [TYPE_BYTE] = { .size = 1, .name = "byte" },
    [TYPE_SHORT] = { .size = 2, .name = "short" },
    //[TYPE_INT] = { .size = 4, .name = "int" },
    //[TYPE_FLOAT] = { .size = 4, .name = "float" },
};

type_t get_type(char* s);

#endif
