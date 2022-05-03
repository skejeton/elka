#ifndef ELKA_CODEGEN_H__
#define ELKA_CODEGEN_H__
#include <stddef.h>
#include "refl.h"
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
 
#define ELKA_INSTR_ID(x) ((ElkaInstruction)(x & 0x3F))
#define ELKA_IUC_ARG1(x) ((size_t)( x >> 6 ))

typedef int64_t elka_default_int_td;

typedef enum ElkaInstruction {
    ELKA_INSTR_PUSH_SC      = 0,
    ELKA_INSTR_PUSH_IC      = 1,
    ELKA_INSTR_CALL_SC      = 2,
    
    /* This will reserve data specified by
     * string length on stack 
     */
    ELKA_INSTR_LOAD_IUC     = 5,
    ELKA_INSTR_CALL_IUC     = 6,
    ELKA_INSTR_RETN         = 7,

    // Clears the stack frame of current function
    ELKA_INSTR_CLR          = 8,
    ELKA_INSTR_CALL_FFI_IUC = 9,
    
    ELKA_INSTR_JIF_IUC      = 10,
    ELKA_INSTR_BORROW_IUC   = 11,
    ELKA_INSTR_LESS         = 12,
    ELKA_INSTR_ADD          = 13,
    ELKA_INSTR_SUB          = 14,
    ELKA_INSTR_PUSH_IUC_I   = 15,
    ELKA_INSTR_SET          = 16,
    ELKA_INSTR_GIF_IUC      = 17,
    
    // Load based on the IUC parameter, with relative index on stack
    ELKA_INSTR_BASED_IUC    = 18
} ElkaInstruction;

typedef struct ElkaCodegen {
    // I might remove the reflection here
    ElkaRefl *refl;
    
    // Constant lookup table
    uint8_t lut[1 << 14];
    size_t lut_trail;
    
    // Maps to lut
    size_t lut_indices[1024];
    size_t lut_index_count;
    
    // The instructions are fixed 32 bit
    uint32_t instrs[1024];
    size_t instr_count;
    
    size_t branch_stack[1024];
    size_t branch_stack_size;
} ElkaCodegen;

typedef struct ElkaLabel ElkaLabel;

size_t elka_codegen_branch(ElkaCodegen *cg);

void elka_codegen_leave(ElkaCodegen *cg, size_t genesis);

// Puts SC-Addressed instruction into the instruction list
void elka_codegen_instr_sc(ElkaCodegen *cg, ElkaInstruction instr, char *scarg);

// Puts IC-Addressed instruction into the instruction list
void elka_codegen_instr_ic(ElkaCodegen *cg, ElkaInstruction instr, elka_default_int_td icarg);
// Puts IUC-Addressed instruction into the instruction list
void elka_codegen_instr_iuc(ElkaCodegen *cg, ElkaInstruction instr, size_t iuc);

// Puts simple instruction into the instruction list
void elka_codegen_instr(ElkaCodegen *cg, ElkaInstruction instr);

// Debug dump for a singular instruction
void elka_codegen_dump_instr(ElkaCodegen *cg, uint32_t instr);

// Debug dump for codegen output
void elka_codegen_dbgdmp(ElkaCodegen *cg);

void elka_codegen_dispose(ElkaCodegen *cg);
#endif

