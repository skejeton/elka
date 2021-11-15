#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* my_strdup(char *name) {
    size_t l = strlen(name);
    char *b = malloc(l+1);
    memcpy(b, name, l);
    b[l] = 0;
    return b;
}

SumkaLabel* sumka_codegen_label(SumkaCodegen *cg, char *name) {
    cg->labels[cg->label_count].name = my_strdup(name);
    cg->labels[cg->label_count].at = cg->instr_count;
    return &cg->labels[cg->label_count++];
}

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
    [SUMKA_INSTR_CALL_IUC]     = "call",
    [SUMKA_INSTR_LOAD_IUC]     = "load",
    [SUMKA_INSTR_CALL_SC]      = "call-sc",
    [SUMKA_INSTR_CALL_FFI_IUC] = "call-ffi",
    [SUMKA_INSTR_RETN]         = "retn",
};

void sumka_codegen_dbgdmp(SumkaCodegen *cg) {
    size_t lbl_index = 0;
    
    // XXX: remove this?
    /*for (size_t i = 0; i < sizeof(cg->lut);) {
        if (cg->lut[i] == 0)
            break;
        printf("%s\n", (char*)&cg->lut[i]);
        i += strlen((char*)&cg->lut[i])+1;
    }*/

    for (size_t i = 0; i < cg->instr_count; i += 1) {
        while (lbl_index < cg->label_count && cg->labels[lbl_index].at <= i) {
            printf("\x1b[33m%s\x1b[0m:\n", cg->labels[lbl_index++].name);
        }
        
        uint32_t instr = cg->instrs[i];
        // 0x3F = 6 bit mask
        // TODO: this should probably be constant declared somewhere
        printf("    \x1b[31m%s\x1b[0m", INSTR_MNEM[instr & 0x3F]);
        switch (instr & 0x3F) {
            case SUMKA_INSTR_CALL_SC:
            case SUMKA_INSTR_PUSH_SC:
                printf(" \x1b[32m\"%s\"\x1b[0m", &cg->lut[cg->lut_indices[instr >> 6]]);
                break;
            case SUMKA_INSTR_CALL_IUC:
            case SUMKA_INSTR_LOAD_IUC:
            case SUMKA_INSTR_CALL_FFI_IUC:
                printf(" \x1b[34m%u\x1b[0m", instr >> 6);
                break;
            case SUMKA_INSTR_PUSH_IC:
                printf(" \x1b[34m%li\x1b[0m", *(sumka_default_int_td*)&cg->lut[cg->lut_indices[instr >> 6]]);
        }
        printf("\n");
    }
}

void sumka_codegen_dispose(SumkaCodegen *cg) {
    for (size_t i = 0; i < cg->label_count; i += 1)
        free(cg->labels[i].name);
}

