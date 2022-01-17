// Memory management
#ifndef ELKA_MEM_H_
#define ELKA_MEM_H_

#include "codegen.h"
#include <stddef.h>
#include <stdint.h>

typedef struct ElkaMem {
    size_t *stack;
    size_t stack_trail;
} ElkaMem;


ElkaMem elka_mem_init();

__attribute__((unused))
static void elka_mem_push_default_int(ElkaMem *mem, sumka_default_int_td value) {
    mem->stack[mem->stack_trail++] = value;
}

__attribute__((unused))
static elka_default_int_td sumka_mem_pop_default_int(ElkaMem *mem) {
    return mem->stack[--mem->stack_trail];
}

void elka_mem_deinit(ElkaMem *mem);

#endif // ELKA_MEM_H_
