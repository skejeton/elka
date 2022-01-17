#include "mem.h"
#include "codegen.h"
#include <stdlib.h>

#define ELKA_STACK_SIZE (1 << 14)

ElkaMem elka_mem_init() {
    ElkaMem memory = {
        .stack = malloc(ELKA_STACK_SIZE*sizeof(size_t)),
        .stack_trail = 0
    };
    return memory;
}

void elka_mem_deinit(ElkaMem *mem) {
    free(mem->stack);
}
