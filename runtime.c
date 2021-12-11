#include "runtime.h"
#include "codegen.h"
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

static
void push_call(SumkaRuntime *rt) {
    /*
    if (rt->rsp >= (1 << 14)) {
        printf("Stack overflow!\n");
        exit(-1);
    }
    */
    rt->callstack[rt->rsp++] = (SumkaFrame) { rt->rip, rt->mem.stack_trail };    
}

char* sumka_runtime_pop_str(SumkaRuntime *rt) {
    return (char*)sumka_mem_pop_default_int(&rt->mem);
}

sumka_default_int_td sumka_runtime_pop_int(SumkaRuntime *rt) {
    return sumka_mem_pop_default_int(&rt->mem);
}

void sumka_runtime_exec(SumkaRuntime *rt) {
    rt->callstack = malloc(sizeof(SumkaFrame)*(1 << 14));
    rt->rip = find_label(rt, "main");
    rt->mem = sumka_mem_init();
    push_call(rt);

    //printf("(sumka) found main at %zu\n", rt->rip);
    while (true) {
        uint32_t instr = rt->cg->instrs[rt->rip];
        /*
        printf("[ ");
        for (size_t i = 0; i < rt->mem.stack_trail; i += 1) {
            if ( i == rt->callstack[rt->rsp-1].stack_at ) {
                printf("\x1b[34m%zi\x1b[0m ", rt->mem.stack[i]);
            }
            else {
                printf("%zi ", rt->mem.stack[i]);
            }
        }
        printf("]\n");
        sumka_codegen_dump_instr(rt->cg, rt->rip);
        */
        SumkaInstruction kind = SUMKA_INSTR_ID(instr);
        
        switch (kind) {
            // FIXME: RENAME THIS INSTRUCTION TO SET_IUC
            case SUMKA_INSTR_SET: {
                size_t trail = rt->callstack[rt->rsp-1].stack_at;
                size_t offs = trail + (instr >> 6);
                rt->mem.stack[offs] = sumka_mem_pop_default_int(&rt->mem);
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_LESS: {
                sumka_default_int_td b = rt->mem.stack[--rt->mem.stack_trail];
                sumka_default_int_td a = rt->mem.stack[--rt->mem.stack_trail];
                rt->mem.stack[rt->mem.stack_trail++] = a < b;
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_ADD: {
                sumka_default_int_td b = rt->mem.stack[--rt->mem.stack_trail];
                sumka_default_int_td a = rt->mem.stack[--rt->mem.stack_trail];
                rt->mem.stack[rt->mem.stack_trail++] = a + b;
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_SUB: {
                sumka_default_int_td b = rt->mem.stack[--rt->mem.stack_trail];
                sumka_default_int_td a = rt->mem.stack[--rt->mem.stack_trail];
                rt->mem.stack[rt->mem.stack_trail++] = a - b;
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_BORROW_IUC: {
                rt->callstack[rt->rsp-1].stack_at -= instr >> 6;
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_CALL_IUC: {
                push_call(rt);
                rt->rip = instr >> 6;
            } break;
            case SUMKA_INSTR_CALL_FFI_IUC: {
                rt->cg->refl->refls[instr >> 6].ffi_fn(rt);
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_PUSH_IUC_I: {
                sumka_mem_push_default_int(&rt->mem, instr >> 6);
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_LOAD_IUC: {
                size_t trail = rt->callstack[rt->rsp-1].stack_at;
                size_t offs = trail + (instr >> 6);
                sumka_mem_push_default_int(&rt->mem, rt->mem.stack[offs]);
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_BASED_IUC: {
                sumka_default_int_td offs = sumka_mem_pop_default_int(&rt->mem);
                size_t trail = rt->callstack[rt->rsp-1].stack_at;
                size_t base = trail + (instr >> 6);
                sumka_mem_push_default_int(&rt->mem, rt->mem.stack[base+offs]);
                rt->rip += 1;
            } break;
            case SUMKA_INSTR_CALL_SC: {
                size_t label = find_label(rt, lup_sc(rt, instr));
                rt->cg->instrs[rt->rip] = (SUMKA_INSTR_CALL_IUC) | (label << 6);
            } break;
            case SUMKA_INSTR_PUSH_SC: {
                char *s = lup_sc(rt, instr);
                sumka_mem_push_default_int(&rt->mem, (sumka_default_int_td)s);
                rt->rip += 1;  
            } break;
            case SUMKA_INSTR_PUSH_IC: {
                sumka_default_int_td val = lup_ic(rt, instr);
                sumka_mem_push_default_int(&rt->mem, (sumka_default_int_td)val);
                rt->rip += 1;  
            } break;
            case SUMKA_INSTR_CLR: {
                sumka_default_int_td val = sumka_mem_pop_default_int(&rt->mem);
                rt->mem.stack_trail = rt->callstack[rt->rsp-1].stack_at;
                sumka_mem_push_default_int(&rt->mem, val);

                rt->rip += 1;
            } break;
            case SUMKA_INSTR_JIF_IUC: {
                sumka_default_int_td val = sumka_mem_pop_default_int(&rt->mem);
                rt->rip += (!val)*((instr >> 6) - 1)+1;
                /*
                if (val) 
                    rt->rip += 1;
                else
                    rt->rip += instr >> 6;
                    */
            } break;
            case SUMKA_INSTR_GIF_IUC: {
                sumka_default_int_td val = sumka_mem_pop_default_int(&rt->mem);
                rt->rip -= (!!val)*((instr >> 6) + 1)-1;
                /*
                if (val) 
                    rt->rip += 1;
                else
                    rt->rip += instr >> 6;
                    */
            } break;
            case SUMKA_INSTR_RETN: {
                if (rt->rsp == 1)
                    goto cleanup;
                rt->rip = rt->callstack[--rt->rsp].return_addr+1;
            } break;
        }
    }
cleanup:
    sumka_mem_deinit(&rt->mem);
    free(rt->callstack);
}
