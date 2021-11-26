#include "runtime.h"
#include "gc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* FIXME: Use hash table
 */
static
size_t find_label(SumkaRuntime *rt, char *name) {
    SumkaReflItem *item = sumka_refl_lup(rt->cg->refl, name);
    
    if (!item && !item->present)
        goto error;
    if (item->tag != SUMKA_TAG_FUN)
        goto error2;
    
    return item->fn.addr;
error:
    fprintf(stdout, "%s() not found!\n", name);
    exit(12);
error2:
    fprintf(stdout, "%s() is not a function!\n", name);
    exit(12);
}

// Looks up string REAL address
static
char* lup_sc(SumkaRuntime *rt, uint32_t instr) {
    return (char*)&rt->cg->lut[rt->cg->lut_indices[instr>>6]];
}

static
sumka_default_int_td lup_ic(SumkaRuntime *rt, uint32_t instr) {
    return *(sumka_default_int_td*)&rt->cg->lut[rt->cg->lut_indices[instr>>6]];
}

static inline
void push_call(SumkaRuntime *rt, size_t val) {
    if (rt->rsp >= (1 << 14)) {
        printf("Stack overflow!\n");
        exit(-1);
    }
    rt->callstack[rt->rsp++] = val;    
}

char* sumka_runtime_pop_str(SumkaRuntime *rt) {
    return (char*)sumka_alloc_get(&rt->alloc, sumka_stackref_pop(&rt->alloc));
}

sumka_default_int_td sumka_runtime_pop_int(SumkaRuntime *rt) {
    return *(sumka_default_int_td*)sumka_alloc_get(&rt->alloc, sumka_stackref_pop(&rt->alloc));
}

void sumka_runtime_exec(SumkaRuntime *rt) {
    rt->callstack = malloc(sizeof(size_t)*(1 << 14));
    rt->rip = find_label(rt, "main");



    printf("(sumka) found main at %zu\n", rt->rip);
    while (rt->rip < rt->cg->instr_count) {
        uint32_t instr = rt->cg->instrs[rt->rip];
        
        /* FIXME: For now i sweep every instruction but i need to do that less
         */
        sumka_gc_sweep(&rt->alloc);

        SumkaInstruction kind = SUMKA_INSTR_ID(instr);
        switch (kind) {
            case SUMKA_INSTR_CALL_IUC: {
                sumka_gc_frame(&rt->alloc);
                push_call(rt, rt->rip);
                rt->rip = instr >> 6;
            } break;
            case SUMKA_INSTR_CALL_FFI_IUC: {
                rt->cg->refl->refls[instr >> 6].ffi_fn(rt);
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_LOAD_IUC: {
                sumka_stackref_push(&rt->alloc, sumka_gc_clone(&rt->alloc, sumka_stackref_get(&rt->alloc, instr>>6)));
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_CALL_SC: {
                size_t label = find_label(rt, lup_sc(rt, instr));
                rt->cg->instrs[rt->rip] = (SUMKA_INSTR_CALL_IUC) | (label << 6);
            } break;
            case SUMKA_INSTR_PUSH_SC: {
                // We shouldn't data push from LUT here
                char *s = lup_sc(rt, instr);
                sumka_stackref_push(&rt->alloc, sumka_gc_alloc_str(&rt->alloc, s));
                rt->rip += 1;  
            } break;
            case SUMKA_INSTR_PUSH_IC: {
                // We shouldn't data push from LUT here
                sumka_default_int_td val = lup_ic(rt, instr);
                sumka_stackref_push(&rt->alloc, sumka_gc_alloc_int(&rt->alloc, val));
                rt->rip += 1;  
            } break;
            case SUMKA_INSTR_CLR: {
                sumka_gc_retract(&rt->alloc);
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_RETN: {
                if (rt->rsp == 0)
                    goto cleanup;

                rt->rip = rt->callstack[--rt->rsp]+1;
            } break;
        }
    }
cleanup:
    sumka_gc_sweep(&rt->alloc);
    free(rt->callstack);
}
