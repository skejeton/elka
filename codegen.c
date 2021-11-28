#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void sumka_codegen_instr_ic(SumkaCodegen *cg, SumkaInstruction instr, sumka_default_int_td icarg) {
    memcpy(cg->lut + cg->lut_trail, &icarg, sizeof(icarg));   
    cg->lut_indices[cg->lut_index_count] = cg->lut_trail;
    cg->lut_trail += sizeof(icarg);
    size_t index = cg->lut_index_count++;

    uint32_t *out = &cg->instrs[cg->instr_count++];
    *out = instr | (index << 6);
}

void sumka_codegen_instr_sc(SumkaCodegen *cg, SumkaInstruction instr, char *scarg) {
    // Write to LUT
    size_t len = strlen(scarg)+1;
    memcpy(cg->lut + cg->lut_trail, scarg, len);
    cg->lut_indices[cg->lut_index_count] = cg->lut_trail;
    cg->lut_trail += len;
    size_t index = cg->lut_index_count++;
    
    // Emit instruction
    uint32_t *out = &cg->instrs[cg->instr_count++];
    *out = instr | (index << 6);
}

size_t sumka_codegen_branch(SumkaCodegen *cg) {
    size_t ret = cg->instr_count;
    cg->instrs[cg->instr_count++] = SUMKA_INSTR_JIF_IUC;
    return ret;
}

void sumka_codegen_leave(SumkaCodegen *cg, size_t genesis) {
    cg->instrs[genesis] |= (cg->instr_count-genesis) << 6;
}

void sumka_codegen_instr_iuc(SumkaCodegen *cg, SumkaInstruction instr, size_t iuc) {
    // Emit instruction
    uint32_t *out = &cg->instrs[cg->instr_count++];
    *out = instr | (iuc << 6);
}

void sumka_codegen_instr(SumkaCodegen *cg, SumkaInstruction instr) {
    uint32_t *out = &cg->instrs[cg->instr_count++];
    *out = instr;
}

const char *INSTR_MNEM[] = {
    [SUMKA_INSTR_PUSH_SC]      = "push-sc",
    [SUMKA_INSTR_PUSH_IC]      = "push-ic",
    [SUMKA_INSTR_PUSH_IUC_I]   = "push-iuc-i",
    [SUMKA_INSTR_CALL_IUC]     = "call-iuc",
    [SUMKA_INSTR_LOAD_IUC]     = "load-iuc",
    [SUMKA_INSTR_CALL_SC]      = "call-sc",
    [SUMKA_INSTR_CALL_FFI_IUC] = "call-ffi",
    [SUMKA_INSTR_CLR]          = "clr",
    [SUMKA_INSTR_RETN]         = "retn",
    [SUMKA_INSTR_JIF_IUC]      = "jif-iuc",
    [SUMKA_INSTR_BORROW_IUC]   = "borrow-iuc",
    [SUMKA_INSTR_LESS]         = "less",
    [SUMKA_INSTR_ADD]          = "add",
    [SUMKA_INSTR_SUB]          = "sub",
};


void sumka_codegen_dump_instr(SumkaCodegen *cg, uint32_t instr_id) {
    uint32_t instr = cg->instrs[instr_id];

    printf("%-3u", instr_id);
    printf("    \x1b[31m%s\x1b[0m", INSTR_MNEM[SUMKA_INSTR_ID(instr)]);
    switch (instr & 0x3F) {
        case SUMKA_INSTR_CALL_SC:
        case SUMKA_INSTR_PUSH_SC:
            printf(" \x1b[32m\"%s\"\x1b[0m", &cg->lut[cg->lut_indices[instr >> 6]]);
            break;
        case SUMKA_INSTR_CALL_IUC:
        case SUMKA_INSTR_BORROW_IUC:
        case SUMKA_INSTR_LOAD_IUC:
        case SUMKA_INSTR_PUSH_IUC_I:
        case SUMKA_INSTR_JIF_IUC:
            printf(" \x1b[34m%u\x1b[0m", instr >> 6);
            break;
        case SUMKA_INSTR_PUSH_IC:
            printf(" \x1b[34m%li\x1b[0m", *(sumka_default_int_td*)&cg->lut[cg->lut_indices[instr >> 6]]);
            break;
        case SUMKA_INSTR_CALL_FFI_IUC:
            printf(" \x1b[34m%s\x1b[0m", cg->refl->refls[instr >> 6].name);
            break;
    }
    printf("\n");
}

void sumka_codegen_dbgdmp(SumkaCodegen *cg) {
    for (size_t i = 0; i < cg->instr_count; i += 1) {
        for (size_t j = 0; j < cg->refl->refl_count; j += 1) {
            if (cg->refl->refls[j].tag == SUMKA_TAG_FUN && cg->refl->refls[j].fn.addr == i && cg->refl->refls[j].present) {
                printf("\x1b[33m%s\x1b[0m:\n", cg->refl->refls[j].name);
            }
        }
        
        sumka_codegen_dump_instr(cg, i);
    }
}

void sumka_codegen_dispose(SumkaCodegen *cg) {
    // Don't do anything for now,
    // Although maybe I can free reflection here? Probably a bad idea.
    (void) cg;
}

