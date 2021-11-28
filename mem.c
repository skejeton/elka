#include "mem.h"
#include "codegen.h"
#include <stdlib.h>

#define SUMKA_STACK_SIZE (1 << 14)

SumkaMem sumka_mem_init() {
    SumkaMem memory = {
        .stack = malloc(SUMKA_STACK_SIZE*sizeof(size_t)),
        .stack_trail = 0
    };
    return memory;
}

void sumka_mem_deinit(SumkaMem *mem) {
    free(mem->stack);
}
