#include "runtime.h"
#include "gc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* FIXME: Use hash table
 */
static
size_t find_label(SumkaRuntime *rt, char *name) {
    for (size_t i = 0; i < rt->cg->label_count; i += 1) {
        if (strcmp(rt->cg->labels[i].name, name) == 0)
            return rt->cg->labels[i].at;
    }
    
    fprintf(stdout, "\"%s\"() not found!\n", name);
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

// Looks up string lut offset
/*
static
size_t lup_sc_addr(SumkaRuntime *rt, uint32_t instr) {
    return rt->cg->lut_indices[instr>>6];
}*/

/* FIXME: 1 << 14, is very assuming here
 */
static inline
void push_sizet(SumkaRuntime *rt, size_t val) {
    if (rt->rsp >= ((1 << 14) - sizeof(size_t))) {
        printf("Stack overflow!\n");
        exit(-1);
    }
    *((size_t*)(rt->stack+rt->rsp)) = val;    
    rt->rsp += sizeof(val);
}

/*
static inline
size_t pop_sizet(SumkaRuntime *rt) {
    return *((size_t*)(rt->stack+rt->rsp-sizeof(size_t)));
    rt->rsp -= sizeof(size_t);
}
*/

char* sumka_runtime_pop_str(SumkaRuntime *rt) {
    return (char*)sumka_alloc_get(&rt->alloc, sumka_stackref_pop(&rt->alloc));
}

sumka_default_int_td sumka_runtime_pop_int(SumkaRuntime *rt) {
    return *(sumka_default_int_td*)sumka_alloc_get(&rt->alloc, sumka_stackref_pop(&rt->alloc));
}

void sumka_runtime_exec(SumkaRuntime *rt) {
    /* FIXME: 1 << 14 bytes... isn't ought to be enough for anybody
     */
    rt->stack = malloc(1 << 14);
    // SETUP
    rt->rip = find_label(rt, "main");


    printf("(sumka) found main at %zu\n", rt->rip);
    while (rt->rip < rt->cg->instr_count) {
        uint32_t instr = rt->cg->instrs[rt->rip];
        
        /* FIXME: For now i sweep every instruction but i need to do that less
         */
        sumka_gc_sweep(&rt->alloc);

        /* TODO: 0x3F = 6 bit mask, this should probably
        *        be a constant declared somewhere
        */     
        SumkaInstruction kind = instr & 0x3F;
        switch (kind) {
            case SUMKA_INSTR_CALL_IUC: {
                /* XXX: Do we really need this?
                 */
                sumka_gc_frame(&rt->alloc);
                push_sizet(rt, rt->rsp);
                push_sizet(rt, rt->rbp);
                push_sizet(rt, rt->rip);
                rt->rbp = rt->rsp;
                rt->rip = instr >> 6;
            } break;
            case SUMKA_INSTR_CALL_FFI_IUC: {
                rt->ffi->mappings[instr >> 6].exec(rt);
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
            case SUMKA_INSTR_RETN: {
                sumka_gc_retract(&rt->alloc);
                if (rt->rbp == 0)
                    goto cleanup;
                size_t rbp = rt->rbp;
                rt->rip = *(size_t*)(rt->stack+rbp-sizeof(size_t))+1;
                rt->rbp = *(size_t*)(rt->stack+rbp-sizeof(size_t)*2);
                rt->rsp = *(size_t*)(rt->stack+rbp-sizeof(size_t)*3);
            } break;
        }
    }
cleanup:
    sumka_gc_sweep(&rt->alloc);
    free(rt->stack);
}
