#ifndef SUMKA_RUNTIME_H__
#define SUMKA_RUNTIME_H__
#include "codegen.h"
#include "mem.h"

typedef struct SumkaFrame {
    size_t return_addr;
    size_t stack_at;
} SumkaFrame; 

typedef struct SumkaRuntime {
    SumkaCodegen *cg;
    SumkaMem mem;
    SumkaFrame *callstack;
    
    // Registers
    size_t rsp;
    size_t rip;
} SumkaRuntime; 

void sumka_runtime_exec(SumkaRuntime *rt);
char* sumka_runtime_pop_str(SumkaRuntime *rt);
sumka_default_int_td sumka_runtime_pop_int(SumkaRuntime *rt);

#endif
