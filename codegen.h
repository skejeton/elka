#ifndef SUMKA_CODEGEN_H__
#define SUMKA_CODEGEN_H__
#include <stddef.h>
#include "typesystem.h"
#include <stdint.h>

/*
 * The instruction numbers are packed in 6 bits
 * Instruction parameters depend on addressing mode.
 * 
 * No postfix means there's no arguments accepted
 *
 * There are post-fixes for each instructions
 * according to their arguments:
 *
 * SC  = String Constant/(or Symbol)
 * IUC = Inline uint Constant
 * IC  = Integer constant (LUT)
 */

typedef int64_t sumka_default_int_td;

typedef enum SumkaInstruction {
    SUMKA_INSTR_PUSH_SC      = 0,
    SUMKA_INSTR_PUSH_IC      = 1,
    SUMKA_INSTR_CALL_SC      = 2,
    
    /* This will reserve data specified by
     * string length on stack 
     */
    SUMKA_INSTR_LOAD_IUC     = 5,
    SUMKA_INSTR_CALL_IUC     = 6,
    SUMKA_INSTR_RETN         = 7,
    SUMKA_INSTR_CALL_FFI_IUC = 8
} SumkaInstruction;

typedef struct SumkaCodegen {
    // Constant lookup table
    uint8_t lut[1 << 14];
    size_t lut_trail;
    
    // Maps to lut
    size_t lut_indices[1024];
    size_t lut_index_count;
    
    // The instructions are fixed 32 bit
    uint32_t instrs[1024];
    struct SumkaLabel {
        char *name;
        size_t at;
        // FIXME: This is just ridiculous, make reflection
        bool has_arg;
        SumkaType type;
    } labels[1024];
    size_t instr_count;
    size_t label_count;
} SumkaCodegen;

typedef struct SumkaLabel SumkaLabel;

// Puts label at current code state
SumkaLabel* sumka_codegen_label(SumkaCodegen *cg, char *name);

// Puts SC-Addressed instruction into the instruction list
void sumka_codegen_instr_sc(SumkaCodegen *cg, SumkaInstruction instr, char *scarg);

// Puts IC-Addressed instruction into the instruction list
void sumka_codegen_instr_ic(SumkaCodegen *cg, SumkaInstruction instr, sumka_default_int_td icarg);
// Puts IUC-Addressed instruction into the instruction list
void sumka_codegen_instr_iuc(SumkaCodegen *cg, SumkaInstruction instr, size_t iuc);

// Puts simple instruction into the instruction list
void sumka_codegen_instr(SumkaCodegen *cg, SumkaInstruction instr);

// Debug dump for codegen output
void sumka_codegen_dbgdmp(SumkaCodegen *cg);

void sumka_codegen_dispose(SumkaCodegen *cg);
#endif

