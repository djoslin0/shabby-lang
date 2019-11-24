#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include "constants.h"

void fput16(uint16_t, FILE*);
uint16_t fget16(FILE*);

#endif
