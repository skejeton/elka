#ifndef ELKA_RUNTIME_H__
#define ELKA_RUNTIME_H__
#include "codegen.h"
#include "mem.h"

typedef struct ElkaFrame {
    size_t return_addr;
    size_t stack_at;
} ElkaFrame; 

typedef struct ElkaRuntime {
    ElkaCodegen *cg;
    ElkaMem mem;
    ElkaFrame *callstack;
    
    // Registers
    size_t rsp;
    size_t rip;
} ElkaRuntime; 

void elka_runtime_exec(ElkaRuntime *rt);
char* elka_runtime_pop_str(ElkaRuntime *rt);
elka_default_int_td sumka_runtime_pop_int(ElkaRuntime *rt);

#endif
