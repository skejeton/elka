#ifndef SUMKA_FFI_H__
#define SUMKA_FFI_H__
#include <stddef.h>
#include <stdint.h>

struct SumkaRuntime;

typedef void (*SumkaFFIExec)(struct SumkaRuntime *runtime);

typedef struct SumkaFFIMapping {
    const char *name;
    SumkaFFIExec exec;
} SumkaFFIMapping;

typedef struct SumkaFFI {
    // TODO: Get rid of static buffer
    SumkaFFIMapping mappings[1024];
    size_t mapping_count; 
} SumkaFFI;

// Please, only pass the string literals to name
void sumka_ffi_register(SumkaFFI *ffi, const char *name, SumkaFFIExec exec);

#endif
