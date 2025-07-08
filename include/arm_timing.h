#ifndef ARM_TIMING_H
#define ARM_TIMING_H

#include <stdint.h>
#include <stdbool.h>
#include "utility_macros.h" // Include for UNUSED macro

// ARM instruction cycle timing based on GBA Technical Data
// ARM instructions generally take more cycles than Thumb equivalents

// Base cycle costs for different ARM instruction types
#define ARM_CYCLES_DATA_PROCESSING     1  // Basic ALU operations
#define ARM_CYCLES_MULTIPLY_BASE       2  // Base multiply cycles (+ m cycles)
#define ARM_CYCLES_MULTIPLY_LONG       3  // Long multiply base (+ m cycles)
#define ARM_CYCLES_SINGLE_TRANSFER     1  // Base load/store (+ memory access)
#define ARM_CYCLES_BLOCK_TRANSFER_BASE 1  // Base LDM/STM (+ n registers + memory)
#define ARM_CYCLES_BRANCH             3  // Branch/Branch with link
#define ARM_CYCLES_SWI                3  // Software interrupt
#define ARM_CYCLES_PSR_TRANSFER       1  // PSR operations
#define ARM_CYCLES_COPROCESSOR        1  // Coprocessor operations

// Additional cycles for specific operations
#define ARM_CYCLES_SHIFT_BY_REG       1  // Extra cycle when shift amount is in register
#define ARM_CYCLES_TRANSFER_REG       1  // Per register in block transfer
#define ARM_CYCLES_MULTIPLY_MIN       1  // Minimum additional multiply cycles
#define ARM_CYCLES_MULTIPLY_MAX       4  // Maximum additional multiply cycles

// ARM instruction format identification
// Based on bits 27-24 (instruction[27:24])
#define ARM_FORMAT_DATA_PROCESSING_0   0x0  // 0000: Data processing
#define ARM_FORMAT_DATA_PROCESSING_1   0x1  // 0001: Data processing
#define ARM_FORMAT_SINGLE_TRANSFER_0   0x2  // 0010: Single data transfer
#define ARM_FORMAT_SINGLE_TRANSFER_1   0x3  // 0011: Single data transfer  
#define ARM_FORMAT_BLOCK_TRANSFER      0x4  // 0100: Block data transfer
#define ARM_FORMAT_BRANCH              0x5  // 0101: Branch/Branch with link
#define ARM_FORMAT_COPROCESSOR_0       0x6  // 0110: Coprocessor data transfer
#define ARM_FORMAT_COPROCESSOR_1       0x7  // 0111: Coprocessor data transfer
#define ARM_FORMAT_COPROCESSOR_2       0x8  // 1000: Coprocessor operation
#define ARM_FORMAT_MULTIPLY            0x9  // 1001: Multiply
#define ARM_FORMAT_SOFTWARE_INTERRUPT  0xA  // 1010: Software interrupt
#define ARM_FORMAT_PSR_TRANSFER        0xB  // 1011: PSR transfer
#define ARM_FORMAT_COPROCESSOR_3       0xC  // 1100: Coprocessor register transfer
#define ARM_FORMAT_UNDEFINED_0         0xD  // 1101: Undefined
#define ARM_FORMAT_UNDEFINED_1         0xE  // 1110: Undefined
#define ARM_FORMAT_SOFTWARE_INTERRUPT_2 0xF // 1111: Software interrupt

// ARM condition codes (bits 31-28)
typedef enum {
    ARM_COND_EQ = 0x0,  // Equal (Z set)
    ARM_COND_NE = 0x1,  // Not equal (Z clear)
    ARM_COND_CS = 0x2,  // Carry set (C set)
    ARM_COND_CC = 0x3,  // Carry clear (C clear)
    ARM_COND_MI = 0x4,  // Minus (N set)
    ARM_COND_PL = 0x5,  // Plus (N clear)
    ARM_COND_VS = 0x6,  // Overflow (V set)
    ARM_COND_VC = 0x7,  // No overflow (V clear)
    ARM_COND_HI = 0x8,  // Higher (C set and Z clear)
    ARM_COND_LS = 0x9,  // Lower or same (C clear or Z set)
    ARM_COND_GE = 0xA,  // Greater or equal (N == V)
    ARM_COND_LT = 0xB,  // Less than (N != V)
    ARM_COND_GT = 0xC,  // Greater than (Z clear and N == V)
    ARM_COND_LE = 0xD,  // Less than or equal (Z set or N != V)
    ARM_COND_AL = 0xE,  // Always
    ARM_COND_NV = 0xF   // Never (deprecated, treat as AL)
} ARMCondition;

// ARM data processing opcodes (bits 24-21)
typedef enum {
    ARM_OP_AND = 0x0,  // AND
    ARM_OP_EOR = 0x1,  // EOR (exclusive or)
    ARM_OP_SUB = 0x2,  // SUB
    ARM_OP_RSB = 0x3,  // RSB (reverse subtract)
    ARM_OP_ADD = 0x4,  // ADD
    ARM_OP_ADC = 0x5,  // ADC (add with carry)
    ARM_OP_SBC = 0x6,  // SBC (subtract with carry)
    ARM_OP_RSC = 0x7,  // RSC (reverse subtract with carry)
    ARM_OP_TST = 0x8,  // TST (test)
    ARM_OP_TEQ = 0x9,  // TEQ (test equivalence)
    ARM_OP_CMP = 0xA,  // CMP (compare)
    ARM_OP_CMN = 0xB,  // CMN (compare negated)
    ARM_OP_ORR = 0xC,  // ORR (or)
    ARM_OP_MOV = 0xD,  // MOV (move)
    ARM_OP_BIC = 0xE,  // BIC (bit clear)
    ARM_OP_MVN = 0xF   // MVN (move not)
} ARMDataProcessingOp;

// ARM shift types (bits 6-5 for register shifts, bits 1-0 for immediate shifts)
typedef enum {
    ARM_SHIFT_LSL = 0x0,  // Logical shift left
    ARM_SHIFT_LSR = 0x1,  // Logical shift right
    ARM_SHIFT_ASR = 0x2,  // Arithmetic shift right
    ARM_SHIFT_ROR = 0x3   // Rotate right
} ARMShiftType;

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
uint32_t arm_calculate_instruction_cycles(uint32_t instruction, uint32_t pc, uint32_t* registers, uint32_t cpsr);
bool arm_check_condition(ARMCondition condition, uint32_t cpsr);
uint32_t arm_get_multiply_cycles(uint32_t operand);
uint32_t arm_count_registers(uint16_t register_list);
uint32_t arm_calculate_shifted_register(uint32_t instruction, uint32_t* registers, uint32_t* carry_out);
uint32_t arm_calculate_immediate_operand(uint32_t instruction, uint32_t* carry_out);

#ifdef __cplusplus
}
#endif

// Helper macros for instruction decoding
#define ARM_GET_CONDITION(instr)     (((instr) >> 28) & 0xF)
#define ARM_GET_FORMAT(instr)        (((instr) >> 25) & 0x7)  // Bits 27-25 for primary decode
#define ARM_GET_SECONDARY(instr)     (((instr) >> 20) & 0xFF) // Bits 27-20 for secondary decode
#define ARM_GET_OPCODE(instr)        (((instr) >> 21) & 0xF)
#define ARM_GET_S_BIT(instr)         (((instr) >> 20) & 0x1)
#define ARM_GET_RN(instr)            (((instr) >> 16) & 0xF)
#define ARM_GET_RD(instr)            (((instr) >> 12) & 0xF)
#define ARM_GET_RS(instr)            (((instr) >> 8) & 0xF)
#define ARM_GET_RM(instr)            ((instr) & 0xF)
#define ARM_GET_IMMEDIATE_FLAG(instr) (((instr) >> 25) & 0x1)
#define ARM_GET_SHIFT_TYPE(instr)    (((instr) >> 5) & 0x3)
#define ARM_GET_SHIFT_AMOUNT(instr)  (((instr) >> 7) & 0x1F)
#define ARM_GET_REG_SHIFT_FLAG(instr) (((instr) >> 4) & 0x1)

#endif // ARM_TIMING_H
