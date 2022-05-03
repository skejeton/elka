#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void elka_codegen_instr_ic(ElkaCodegen *cg, ElkaInstruction instr, elka_default_int_td icarg) {
    memcpy(cg->lut + cg->lut_trail, &icarg, sizeof(icarg));   
    cg->lut_indices[cg->lut_index_count] = cg->lut_trail;
    cg->lut_trail += sizeof(icarg);
    size_t index = cg->lut_index_count++;

    uint32_t *out = &cg->instrs[cg->instr_count++];
    *out = instr | (index << 6);
}

void elka_codegen_instr_sc(ElkaCodegen *cg, ElkaInstruction instr, char *scarg) {
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

size_t elka_codegen_branch(ElkaCodegen *cg) {
    size_t ret = cg->instr_count;
    cg->instrs[cg->instr_count++] = ELKA_INSTR_JIF_IUC;
    return ret;
}

void elka_codegen_leave(ElkaCodegen *cg, size_t genesis) {
    cg->instrs[genesis] |= (cg->instr_count-genesis) << 6;
}

void elka_codegen_instr_iuc(ElkaCodegen *cg, ElkaInstruction instr, size_t iuc) {
    // Emit instruction
    uint32_t *out = &cg->instrs[cg->instr_count++];
    *out = instr | (iuc << 6);
}

void elka_codegen_instr(ElkaCodegen *cg, ElkaInstruction instr) {
    uint32_t *out = &cg->instrs[cg->instr_count++];
    *out = instr;
}

const char *INSTR_MNEM[] = {
    [ELKA_INSTR_PUSH_SC]      = "push-sc",
    [ELKA_INSTR_PUSH_IC]      = "push-ic",
    [ELKA_INSTR_PUSH_IUC_I]   = "push-iuc-i",
    [ELKA_INSTR_CALL_IUC]     = "call-iuc",
    [ELKA_INSTR_LOAD_IUC]     = "load-iuc",
    [ELKA_INSTR_GIF_IUC]      = "gif-iuc",
    [ELKA_INSTR_CALL_SC]      = "call-sc",
    [ELKA_INSTR_CALL_FFI_IUC] = "call-ffi",
    [ELKA_INSTR_CLR]          = "clr",
    [ELKA_INSTR_RETN]         = "retn",
    [ELKA_INSTR_JIF_IUC]      = "jif-iuc",
    [ELKA_INSTR_BORROW_IUC]   = "borrow-iuc",
    [ELKA_INSTR_LESS]         = "less",
    [ELKA_INSTR_ADD]          = "add",
    [ELKA_INSTR_SUB]          = "sub",
    [ELKA_INSTR_SET]          = "set",
    [ELKA_INSTR_BASED_IUC]    = "based-iuc"
};


void elka_codegen_dump_instr(ElkaCodegen *cg, uint32_t instr_id) {
    uint32_t instr = cg->instrs[instr_id];

    printf("%-3u", instr_id);
    printf("    \x1b[31m%s\x1b[0m", INSTR_MNEM[ELKA_INSTR_ID(instr)]);
    switch (instr & 0x3F) {
        case ELKA_INSTR_CALL_SC:
        case ELKA_INSTR_PUSH_SC:
            printf(" \x1b[32m\"%s\"\x1b[0m", &cg->lut[cg->lut_indices[instr >> 6]]);
            break;
        case ELKA_INSTR_CALL_IUC:
        case ELKA_INSTR_BORROW_IUC:
        case ELKA_INSTR_LOAD_IUC:
        case ELKA_INSTR_SET:
        case ELKA_INSTR_GIF_IUC:
        case ELKA_INSTR_PUSH_IUC_I:
        case ELKA_INSTR_JIF_IUC:
        case ELKA_INSTR_BASED_IUC:
            printf(" \x1b[34m%u\x1b[0m", instr >> 6);
            break;
        case ELKA_INSTR_PUSH_IC:
            printf(" \x1b[34m%li\x1b[0m", *(elka_default_int_td*)&cg->lut[cg->lut_indices[instr >> 6]]);
            break;
        case ELKA_INSTR_CALL_FFI_IUC:
            printf(" \x1b[34m%s\x1b[0m", cg->refl->refls[instr >> 6].name);
            break;
    }
    printf("\n");
}

void elka_codegen_dbgdmp(ElkaCodegen *cg) {
    for (size_t i = 0; i < cg->instr_count; i += 1) {
        for (size_t j = 0; j < cg->refl->refl_count; j += 1) {
            if (cg->refl->refls[j].tag == ELKA_TAG_FUN && cg->refl->refls[j].fn.addr == i && cg->refl->refls[j].present) {
                printf("\x1b[33m%s\x1b[0m:\n", cg->refl->refls[j].name);
            }
        }
        
        elka_codegen_dump_instr(cg, i);
    }
}

void elka_codegen_dispose(ElkaCodegen *cg) {
    // Don't do anything for now,
    // Although maybe I can free reflection here? Probably a bad idea.
    (void) cg;
}

