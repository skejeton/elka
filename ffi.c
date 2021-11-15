#include "ffi.h"

void sumka_ffi_register(SumkaFFI *ffi, const char *name, void (*exec)(struct SumkaRuntime *runtime)) {
    ffi->mappings[ffi->mapping_count++] = (SumkaFFIMapping) {
        .name = name,
        .exec = exec
    };
}

