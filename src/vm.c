#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symbols.h"
#include "file.h"
#include "nodes.h"
#include "bytecode.h"

  ///////////////////
 // file pointers //
///////////////////

static FILE *gen_ptr = NULL; // gen bytecode

  //////////////////////
 // evaluation stack //
//////////////////////

#define EVAL_STACK_SIZE 128
static uint16_t eval_stack[EVAL_STACK_SIZE];
static uint16_t eval_stack_count = 0;

static void eval_push(uint16_t value) {
    eval_stack[eval_stack_count] = value;
    eval_stack_count++;
    assert(eval_stack_count < EVAL_STACK_SIZE);
}

static uint16_t eval_pop(void) {
    assert(eval_stack_count > 0);
    eval_stack_count--;
    return eval_stack[eval_stack_count];
}

#ifdef DEBUG
    static void output_eval_stack(void) {
        printf("\t  >>  ");
        for (int i = 0; i < eval_stack_count; i++) {
            printf("%d ", (int16_t)eval_stack[i]);
        }
        printf("\n");
    }
#endif

  //////////////////
 // vm functions //
//////////////////

// stack
static void vm_push(void) { eval_push(fget16(gen_ptr)); }
static void vm_pop(void) { fget16(gen_ptr); }

// pointers
static void vm_iget(void) { eval_push(eval_stack[fget16(gen_ptr)]); }
static void vm_get(void) { eval_push(eval_stack[eval_pop()]); }
static void vm_set(void) {
    uint16_t value = eval_pop();
    uint16_t address = eval_pop();
    // TODO: remove this divide by two hack. We need to move toward the stack being byte-based instead of short-based
    eval_stack[address / 2] = value;
}

// math
static void vm_neg(void) { eval_push(-eval_pop()); }
static void vm_add(void) { eval_push(eval_pop() + eval_pop()); }
static void vm_sub(void) { eval_push(eval_pop() - eval_pop()); }
static void vm_mul(void) { eval_push(eval_pop() * eval_pop()); }
static void vm_div(void) { eval_push(eval_pop() / eval_pop()); }

void vm(FILE* gen_ptr_arg) {
    gen_ptr = gen_ptr_arg;

    // print vm header
    #ifdef DEBUG
        printf("\n");
        printf("executed\n");
        printf("--------\n");
    #endif

    while (TRUE) {
        bytecode_t type = fgetc(gen_ptr);
        if (type == (bytecode_t)EOF) { break; }

        switch (type) {
            // misc
            case BC_NOOP: break;

            // stack
            case BC_PUSH: vm_push(); break;
            case BC_POP: vm_pop(); break;

            // pointers
            case BC_GET: vm_get(); break;
            case BC_IGET: vm_iget(); break;
            case BC_SET: vm_set(); break;

            // math
            case BC_NEG: vm_neg(); break;
            case BC_ADD: vm_add(); break;
            case BC_SUB: vm_sub(); break;
            case BC_MUL: vm_mul(); break;
            case BC_DIV: vm_div(); break;
            case BC_EOF: return;
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

    char gen_buffer[128] = { 0 };
    sprintf(gen_buffer, "../bin/%s.gen", argv[1]);
    gen_ptr = fopen(gen_buffer, "r");

    vm(gen_ptr);

    fclose(gen_ptr);
    return 0;
}
