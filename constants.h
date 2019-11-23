#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <inttypes.h>

#define DEBUG 1
#define FALSE 0
#define TRUE (!FALSE)
#define bool uint8_t

// required because pedantic mode is on
extern bool make_iso_compilers_happy;

#endif