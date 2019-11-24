#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <inttypes.h>

#define FALSE 0
#define TRUE (!FALSE)
#define bool uint8_t

#undef NULL
#define NULL 0

#define DEBUG 1
#ifdef DEBUG
    #define DBG_STR(x) x
#else
    #define DBG_STR(x)
#endif

#define MAX_TOKEN_LEN 32

// required because pedantic mode is on
extern bool make_iso_compilers_happy;

#endif