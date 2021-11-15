
#ifndef SUMKA_GC_H__
#define SUMKA_GC_H__
#include <stdbool.h>
#include "codegen.h"
#include <stddef.h>

#define SUMKA_DEFAULT_ENTRY_COUNT 1024
#define SUMKA_DEFAULT_STACKREF_SIZE 1024

typedef struct SumkaAllocEntry {
    // if refcount = 0, entry is vacant
    size_t refcount; 
    bool is_str;
    void *ptr;
} SumkaAllocEntry;

typedef struct SumkaRef {
    size_t entry_id;
} SumkaRef;

typedef struct SumkaAlloc {
    SumkaAllocEntry entries[1024];
    SumkaRef stackrefs[1024];
    size_t stackref_trail;
    size_t frames[1024];
    size_t frame_trail;
} SumkaAlloc;


void* sumka_alloc_get(SumkaAlloc *alloc, SumkaRef ref);
SumkaRef sumka_gc_alloc_str(SumkaAlloc *alloc, char *data);
SumkaRef sumka_gc_alloc_int(SumkaAlloc *alloc, sumka_default_int_td data);
SumkaRef sumka_gc_clone(SumkaAlloc *alloc, SumkaRef ref);
void sumka_gc_retract(SumkaAlloc *alloc);
void sumka_gc_frame(SumkaAlloc *alloc);
void sumka_stackref_push(SumkaAlloc *alloc, SumkaRef ref);
SumkaRef sumka_stackref_pop(SumkaAlloc *alloc);
SumkaRef sumka_stackref_get(SumkaAlloc *alloc, size_t offs);
void sumka_gc_sweep(SumkaAlloc *alloc);

#endif // SUMKA_GC_H__
