#ifndef SUMKA_RUNTIME_H__
#define SUMKA_RUNTIME_H__
#include "codegen.h"
#include "gc.h"
#include "ffi.h"

typedef struct SumkaRuntime {
    // TODO: I should probably pass it by value
    SumkaCodegen *cg;
    SumkaFFI *ffi;
    SumkaAlloc alloc;
    uint8_t *stack;
    
    // Registers
    size_t rsp;
    size_t rbp;
    size_t rip;
} SumkaRuntime; 

void sumka_runtime_exec(SumkaRuntime *rt);
char* sumka_runtime_pop_str(SumkaRuntime *rt);
sumka_default_int_td sumka_runtime_pop_int(SumkaRuntime *rt);

#endif
