#include <stdio.h>
#include "symbols.h"

void fput16(uint16_t value, FILE* fp) {
    fputc(value % 256, fp);
    fputc((value >> 8) % 256, fp);
}

uint16_t fget16(FILE* fp) {
    return fgetc(fp) | (fgetc(fp) << 8);
}
