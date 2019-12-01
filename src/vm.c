#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"
#include "bytecode.h"

  ///////////////////
 // file pointers //
///////////////////

static FILE *bin_ptr = NULL; // binary bytecode

  //////////////////////
 // evaluation stack //
//////////////////////

#define EVAL_STACK_SIZE 256
static uint8_t eval_stack[EVAL_STACK_SIZE];
static uint8_t eval_stack_count = 0;

// 8 bit
static void eval_push8(uint8_t value) {
    eval_stack[eval_stack_count] = value;
    eval_stack_count++;
    assert(eval_stack_count < EVAL_STACK_SIZE);
}

static uint8_t eval_pop8(void) {
    assert(eval_stack_count > 0);
    eval_stack_count--;
    return eval_stack[eval_stack_count];
}

static uint8_t eval_get8(uint16_t index) {
    assert(index >= 0 && index < eval_stack_count);
    return eval_stack[index];
}

static void eval_set8(uint16_t index, uint8_t value) {
    assert(index >= 0 && index < eval_stack_count);
    eval_stack[index] = value;
}

// 16 bit
static void eval_push16(uint16_t value) {
    *(uint16_t*)(&eval_stack[eval_stack_count]) = value;
    eval_stack_count += 2;
    assert(eval_stack_count < EVAL_STACK_SIZE);
}

static uint16_t eval_pop16(void) {
    assert(eval_stack_count > 0);
    eval_stack_count -= 2;
    return *(uint16_t*)(&eval_stack[eval_stack_count]);
}

static uint16_t eval_get16(uint16_t index) {
    assert(index >= 0 && index < eval_stack_count - 1);
    return *(uint16_t*)(&eval_stack[index]);
}

static void eval_set16(uint16_t index, uint16_t value) {
    assert(index >= 0 && index < eval_stack_count - 1);
    *(uint16_t*)(&eval_stack[index]) = value;
}

#ifdef DEBUG
    static void output_eval_stack(void) {
        printf("\t  >>  ");
        for (int i = 0; i < eval_stack_count; i++) {
            printf("%d ", (int8_t)eval_stack[i]);
        }
        printf("\n");
    }
#endif

  //////////////////
 // vm functions //
//////////////////

// misc
static void vm_extend(void) { ((int8_t)eval_get8(eval_stack_count - 1)) >= 0 ? eval_push8(0) : eval_push8((uint8_t)-1); }

// jumps
static void vm_jump(void) { fseek(bin_ptr, eval_pop16(), 0); }
static void vm_ijump(void) { fseek(bin_ptr, fget16(bin_ptr), 0); }

// stack basics
static void vm_push_zeros(void) { uint16_t zeros = fget16(bin_ptr); while (zeros-- > 0) { eval_push8(0); } }

static void vm_push8(void) { eval_push8(fgetc(bin_ptr)); }
static void vm_pop8(void) { eval_pop8(); }

static void vm_push16(void) { eval_push16(fget16(bin_ptr)); }
static void vm_pop16(void) { eval_pop16(); }

// pointers
static void vm_iget8(void) { eval_push8(eval_get8(fget16(bin_ptr))); }
static void vm_get8(void) { eval_push8(eval_get8(eval_pop16())); }
static void vm_set8(void) { eval_set8(eval_pop16(), eval_pop8()); }

static void vm_iget16(void) { eval_push16(eval_get16(fget16(bin_ptr))); }
static void vm_get16(void) { eval_push16(eval_get16(eval_pop16())); }
static void vm_set16(void) { eval_set16(eval_pop16(), eval_pop16()); }

// math
static void vm_neg8(void) { eval_push8(-eval_pop8()); }
static void vm_add8(void) { eval_push8(eval_pop8() + eval_pop8()); }
static void vm_sub8(void) { eval_push8(eval_pop8() - eval_pop8()); }
static void vm_mul8(void) { eval_push8(eval_pop8() * eval_pop8()); }
static void vm_div8(void) { eval_push8(eval_pop8() / eval_pop8()); }

static void vm_neg16(void) { eval_push16(-eval_pop16()); }
static void vm_add16(void) { eval_push16(eval_pop16() + eval_pop16()); }
static void vm_sub16(void) { eval_push16(eval_pop16() - eval_pop16()); }
static void vm_mul16(void) { eval_push16(eval_pop16() * eval_pop16()); }
static void vm_div16(void) { eval_push16(eval_pop16() / eval_pop16()); }

void vm(FILE* bin_ptr_arg) {
    bin_ptr = bin_ptr_arg;

    // print vm header
    #ifdef DEBUG
        printf("\n");
        printf("executed\n");
        printf("--------\n");
    #endif

    while (TRUE) {
        bytecode_t type = fgetc(bin_ptr);
        if (type == (bytecode_t)EOF) { break; }

        switch (type) {
            // misc
            case BC_NOOP: break;
            case BC_EXTEND: vm_extend(); break;

            // jumps
            case BC_JUMP: vm_jump(); break;
            case BC_IJUMP: vm_ijump(); break;

            // stack basics
            case BC_PUSH_ZEROS: vm_push_zeros(); break;

            case BC_PUSH8: vm_push8(); break;
            case BC_POP8: vm_pop8(); break;

            case BC_PUSH16: vm_push16(); break;
            case BC_POP16: vm_pop16(); break;

            // pointers
            case BC_GET8: vm_get8(); break;
            case BC_IGET8: vm_iget8(); break;
            case BC_SET8: vm_set8(); break;

            case BC_GET16: vm_get16(); break;
            case BC_IGET16: vm_iget16(); break;
            case BC_SET16: vm_set16(); break;

            // math
            case BC_NEG8: vm_neg8(); break;
            case BC_ADD8: vm_add8(); break;
            case BC_SUB8: vm_sub8(); break;
            case BC_MUL8: vm_mul8(); break;
            case BC_DIV8: vm_div8(); break;

            case BC_NEG16: vm_neg16(); break;
            case BC_ADD16: vm_add16(); break;
            case BC_SUB16: vm_sub16(); break;
            case BC_MUL16: vm_mul16(); break;
            case BC_DIV16: vm_div16(); break;

            // misc
            case BC_EOF: return;

            // does not run in VM
            case BC_LABEL: assert(FALSE);
        }
        #ifdef DEBUG
            printf("%s", bytecode[type].name);
            output_eval_stack();
        #else
            // make pedantic compilers happy
            bytecode[type] = bytecode[type];
        #endif
    }
}

  //////////
 // main //
//////////

int main(int argc, char *argv[]) {
    assert(argc == 2);

    char bin_buffer[128] = { 0 };
    sprintf(bin_buffer, "../bin/%s.bin", argv[1]);
    bin_ptr = fopen(bin_buffer, "rb");

    vm(bin_ptr);

    fclose(bin_ptr);
    return 0;
}
