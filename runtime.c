#include "runtime.h"
#include "codegen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* FIXME: Use hash table
 */
static
size_t find_label(ElkaRuntime *rt, char *name) {
    ElkaReflItem *item = elka_refl_lup(rt->cg->refl, name);
    
    if (!item && !item->present)
        goto error;
    if (item->tag != ELKA_TAG_FUN)
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
char* lup_sc(ElkaRuntime *rt, uint32_t instr) {
    return (char*)&rt->cg->lut[rt->cg->lut_indices[instr>>6]];
}

static
elka_default_int_td lup_ic(ElkaRuntime *rt, uint32_t instr) {
    return *(elka_default_int_td*)&rt->cg->lut[rt->cg->lut_indices[instr>>6]];
}

static
void push_call(ElkaRuntime *rt) {
    /*
    if (rt->rsp >= (1 << 14)) {
        printf("Stack overflow!\n");
        exit(-1);
    }
    */
    rt->callstack[rt->rsp++] = (ElkaFrame) { rt->rip, rt->mem.stack_trail };    
}

char* elka_runtime_pop_str(ElkaRuntime *rt) {
    return (char*)elka_mem_pop_default_int(&rt->mem);
}

elka_default_int_td elka_runtime_pop_int(ElkaRuntime *rt) {
    return elka_mem_pop_default_int(&rt->mem);
}

void elka_runtime_exec(ElkaRuntime *rt) {
    rt->callstack = malloc(sizeof(ElkaFrame)*(1 << 14));
    rt->rip = find_label(rt, "main");
    rt->mem = elka_mem_init();
    push_call(rt);

    //printf("(elka) found main at %zu\n", rt->rip);
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
        elka_codegen_dump_instr(rt->cg, rt->rip);
        */
        ElkaInstruction kind = ELKA_INSTR_ID(instr);
        
        switch (kind) {
            // FIXME: RENAME THIS INSTRUCTION TO SET_IUC
            case ELKA_INSTR_SET: {
                size_t trail = rt->callstack[rt->rsp-1].stack_at;
                size_t offs = trail + (instr >> 6);
                rt->mem.stack[offs] = elka_mem_pop_default_int(&rt->mem);
                rt->rip += 1;
            } break;
            case ELKA_INSTR_LESS: {
                elka_default_int_td b = rt->mem.stack[--rt->mem.stack_trail];
                elka_default_int_td a = rt->mem.stack[--rt->mem.stack_trail];
                rt->mem.stack[rt->mem.stack_trail++] = a < b;
                rt->rip += 1;
            } break;
            case ELKA_INSTR_ADD: {
                elka_default_int_td b = rt->mem.stack[--rt->mem.stack_trail];
                elka_default_int_td a = rt->mem.stack[--rt->mem.stack_trail];
                rt->mem.stack[rt->mem.stack_trail++] = a + b;
                rt->rip += 1;
            } break;
            case ELKA_INSTR_SUB: {
                elka_default_int_td b = rt->mem.stack[--rt->mem.stack_trail];
                elka_default_int_td a = rt->mem.stack[--rt->mem.stack_trail];
                rt->mem.stack[rt->mem.stack_trail++] = a - b;
                rt->rip += 1;
            } break;
            case ELKA_INSTR_BORROW_IUC: {
                rt->callstack[rt->rsp-1].stack_at -= instr >> 6;
                rt->rip += 1;
            } break;
            case ELKA_INSTR_CALL_IUC: {
                push_call(rt);
                rt->rip = instr >> 6;
            } break;
            case ELKA_INSTR_CALL_FFI_IUC: {
                rt->cg->refl->refls[instr >> 6].ffi_fn(rt);
                rt->rip += 1;
            } break;
            case ELKA_INSTR_PUSH_IUC_I: {
                elka_mem_push_default_int(&rt->mem, instr >> 6);
                rt->rip += 1;
            } break;
            case ELKA_INSTR_LOAD_IUC: {
                size_t trail = rt->callstack[rt->rsp-1].stack_at;
                size_t offs = trail + (instr >> 6);
                elka_mem_push_default_int(&rt->mem, rt->mem.stack[offs]);
                rt->rip += 1;
            } break;
            case ELKA_INSTR_BASED_IUC: {
                elka_default_int_td offs = elka_mem_pop_default_int(&rt->mem);
                size_t trail = rt->callstack[rt->rsp-1].stack_at;
                size_t base = trail + (instr >> 6);
                elka_mem_push_default_int(&rt->mem, rt->mem.stack[base+offs]);
                rt->rip += 1;
            } break;
            case ELKA_INSTR_CALL_SC: {
                size_t label = find_label(rt, lup_sc(rt, instr));
                rt->cg->instrs[rt->rip] = (ELKA_INSTR_CALL_IUC) | (label << 6);
            } break;
            case ELKA_INSTR_PUSH_SC: {
                char *s = lup_sc(rt, instr);
                elka_mem_push_default_int(&rt->mem, (elka_default_int_td)s);
                rt->rip += 1;  
            } break;
            case ELKA_INSTR_PUSH_IC: {
                elka_default_int_td val = lup_ic(rt, instr);
                elka_mem_push_default_int(&rt->mem, (elka_default_int_td)val);
                rt->rip += 1;  
            } break;
            case ELKA_INSTR_CLR: {
                elka_default_int_td val = elka_mem_pop_default_int(&rt->mem);
                rt->mem.stack_trail = rt->callstack[rt->rsp-1].stack_at;
                elka_mem_push_default_int(&rt->mem, val);

                rt->rip += 1;
            } break;
            case ELKA_INSTR_JIF_IUC: {
                elka_default_int_td val = elka_mem_pop_default_int(&rt->mem);
                rt->rip += (!val)*((instr >> 6) - 1)+1;
                /*
                if (val) 
                    rt->rip += 1;
                else
                    rt->rip += instr >> 6;
                    */
            } break;
            case ELKA_INSTR_GIF_IUC: {
                elka_default_int_td val = elka_mem_pop_default_int(&rt->mem);
                rt->rip -= (!!val)*((instr >> 6) + 1)-1;
                /*
                if (val) 
                    rt->rip += 1;
                else
                    rt->rip += instr >> 6;
                    */
            } break;
            case ELKA_INSTR_RETN: {
                if (rt->rsp == 1)
                    goto cleanup;
                rt->rip = rt->callstack[--rt->rsp].return_addr+1;
            } break;
        }
    }
cleanup:
    elka_mem_deinit(&rt->mem);
    free(rt->callstack);
}
