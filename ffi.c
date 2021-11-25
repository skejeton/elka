#include "ffi.h"
#include <string.h>

int sumka_ffi_find(SumkaFFI *ffi, const char *name) {
    /* FIXME: Use hash table, not scan everything every time, dangit!
     */
    for (size_t i = 0; i < ffi->mapping_count; i += 1) {
        const char *got = ffi->mappings[i].name;
        if (strcmp(name, got) == 0) {
            return i;
        }
    }
    return -1;
}

void sumka_ffi_register(SumkaFFI *ffi, const char *name, void (*exec)(struct SumkaRuntime *runtime)) {
    ffi->mappings[ffi->mapping_count++] = (SumkaFFIMapping) {
        .name = name,
        .exec = exec
    };
}

