#include "gc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

SumkaRef get_vacant_ref(SumkaAlloc *alloc) {
    // Find vacant entry
    for (int i = 0; i < SUMKA_DEFAULT_ENTRY_COUNT; i += 1) {
        if (alloc->entries[i].refcount == 0)
            return (SumkaRef){i};
    }
    fprintf(stderr, "Vacant GC entry was not found!\n");
    exit(-1);
}

SumkaAllocEntry* get_entry(SumkaAlloc *alloc, SumkaRef ref) {
    return &alloc->entries[ref.entry_id];
}

void* sumka_alloc_get(SumkaAlloc *alloc, SumkaRef ref) {
    if (alloc->entries[ref.entry_id].refcount == 0 && alloc->entries[ref.entry_id].ptr == NULL)
        fprintf(stderr, "(sumka) attempt to access freed area\n");
    return alloc->entries[ref.entry_id].ptr;
}

SumkaRef sumka_gc_alloc_str(SumkaAlloc *alloc, char *data) {
    SumkaRef ref = get_vacant_ref(alloc);
    get_entry(alloc, ref)->ptr = strdup(data);
    get_entry(alloc, ref)->is_str = true;
    get_entry(alloc, ref)->refcount = 0;
    return ref;
}

SumkaRef sumka_gc_alloc_int(SumkaAlloc *alloc, sumka_default_int_td data) {
    SumkaRef ref = get_vacant_ref(alloc);
    
    // FIXME: Inefficent
    sumka_default_int_td *dest = malloc(sizeof(data));
    *dest = data;
    get_entry(alloc, ref)->ptr = dest;
    get_entry(alloc, ref)->is_str = false;
    get_entry(alloc, ref)->refcount = 0;
    return ref;
}

SumkaRef sumka_gc_clone(SumkaAlloc *alloc, SumkaRef ref) {
    if (get_entry(alloc, ref)->is_str)
        return sumka_gc_alloc_str(alloc, sumka_alloc_get(alloc, ref));
    else 
        // FIXME: This is a really bad assumption, I should ideally copy (and have things sized)
        return sumka_gc_alloc_int(alloc, *(sumka_default_int_td*)get_entry(alloc, ref)->ptr);
}

void sumka_stackref_push(SumkaAlloc *alloc, SumkaRef ref) {
    get_entry(alloc, ref)->refcount += 1;
    alloc->stackrefs[alloc->stackref_trail++] = ref;
}

SumkaRef sumka_stackref_pop(SumkaAlloc *alloc) {
    SumkaRef ref = alloc->stackrefs[--alloc->stackref_trail];
    get_entry(alloc, ref)->refcount -= 1;
    return ref;
}

SumkaRef sumka_stackref_get(SumkaAlloc *alloc, size_t offs) {
    return alloc->stackrefs[alloc->stackref_trail-1-offs];
}

void sumka_gc_frame(SumkaAlloc *alloc) {
    alloc->frames[alloc->frame_trail++] = alloc->stackref_trail;
}

void sumka_gc_retract(SumkaAlloc *alloc) {
    size_t trail;
    if (alloc->frame_trail == 0)
        trail = 0;
    else
        trail = alloc->frames[--alloc->frame_trail];
    if (trail > alloc->stackref_trail) {
        fprintf(stderr, "(sumka) Warning: retract trail is bigger passed than already on the GC");
        return;
    }

    for (size_t i = trail; i < alloc->stackref_trail; i += 1) {
        SumkaAllocEntry *ent = get_entry(alloc, alloc->stackrefs[i]);

        if (ent->refcount == 0) {
            fprintf(stderr, "(sumka) Warning: trying to unmark gc entry with refcount of 0");
            continue;
        }

        ent->refcount -= 1;
    }

    alloc->stackref_trail = trail;
}

void sumka_gc_sweep(SumkaAlloc *alloc) {
    for (int i = 0; i < SUMKA_DEFAULT_ENTRY_COUNT; i += 1) {
        if (alloc->entries[i].refcount == 0 && alloc->entries[i].ptr != NULL)
        {
            free(alloc->entries[i].ptr);
            // Prevent double free
            alloc->entries[i].ptr = NULL;
        }
    }
}
