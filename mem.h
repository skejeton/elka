// Memory management
#ifndef SUMKA_MEM_H_
#define SUMKA_MEM_H_

#include "codegen.h"
#include <stddef.h>
#include <stdint.h>

typedef struct SumkaMem {
    size_t *stack;
    size_t stack_trail;
} SumkaMem;


SumkaMem sumka_mem_init();

__attribute__((unused))
static void sumka_mem_push_default_int(SumkaMem *mem, sumka_default_int_td value) {
    mem->stack[mem->stack_trail++] = value;
}

__attribute__((unused))
static sumka_default_int_td sumka_mem_pop_default_int(SumkaMem *mem) {
    return mem->stack[--mem->stack_trail];
}

void sumka_mem_deinit(SumkaMem *mem);

#endif // SUMKA_MEM_H_
