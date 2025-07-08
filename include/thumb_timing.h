#ifndef THUMB_TIMING_H
#define THUMB_TIMING_H

#include <stdint.h>
#include <stdbool.h>

// Thumb instruction cycle timing based on GBA Technical Data
// Most instructions: 1S (1 sequential cycle)
// Memory access instructions: 1S + memory access cycles
// Branch instructions: 2S + 1N (2 sequential + 1 non-sequential)
// Multiply instructions: 1S + m cycles (where m depends on operand value)

// Base cycle costs for different instruction types
#define THUMB_CYCLES_ALU            1  // ALU operations (AND, EOR, LSL, etc.)
#define THUMB_CYCLES_MOV_IMM        1  // Move immediate
#define THUMB_CYCLES_CMP_IMM        1  // Compare immediate
#define THUMB_CYCLES_ADD_SUB_IMM    1  // Add/subtract immediate
#define THUMB_CYCLES_SHIFT_IMM      1  // Shift immediate
#define THUMB_CYCLES_HI_REG_OP      1  // High register operations
#define THUMB_CYCLES_PC_REL_LOAD    1  // PC-relative load (+ memory access)
#define THUMB_CYCLES_REG_OFFSET     1  // Register offset load/store (+ memory access)
#define THUMB_CYCLES_IMM_OFFSET     1  // Immediate offset load/store (+ memory access)
#define THUMB_CYCLES_SP_REL         1  // SP-relative load/store (+ memory access)
#define THUMB_CYCLES_LOAD_ADDR      1  // Load address
#define THUMB_CYCLES_SP_ADJUST      1  // SP adjust
#define THUMB_CYCLES_PUSH_POP_BASE  1  // Base cycles for push/pop (+ transfer cycles)
#define THUMB_CYCLES_MULTIPLE_BASE  1  // Base cycles for multiple load/store (+ transfer cycles)
#define THUMB_CYCLES_BRANCH_COND    1  // Conditional branch (not taken)
#define THUMB_CYCLES_BRANCH_TAKEN   3  // Conditional branch (taken) or unconditional branch
#define THUMB_CYCLES_BRANCH_LINK    3  // Branch with link
#define THUMB_CYCLES_SWI            3  // Software interrupt

// Additional cycles for memory operations
#define THUMB_CYCLES_TRANSFER_REG   1  // Per register in multiple transfer
#define THUMB_CYCLES_MULTIPLY_MIN   1  // Minimum multiply cycles
#define THUMB_CYCLES_MULTIPLY_MAX   4  // Maximum multiply cycles (for 32-bit operands)

// Instruction format identification masks and values
#define THUMB_FORMAT_MASK_SHIFT_IMM     0xE000  // Bits 15-13
#define THUMB_FORMAT_VAL_SHIFT_IMM      0x0000  // 000xxxxxxxxxxxxx

#define THUMB_FORMAT_MASK_ADD_SUB       0xF800  // Bits 15-11  
#define THUMB_FORMAT_VAL_ADD_SUB        0x1800  // 00011xxxxxxxxxxx

#define THUMB_FORMAT_MASK_MOV_CMP_ADD_SUB 0xE000 // Bits 15-13
#define THUMB_FORMAT_VAL_MOV_CMP_ADD_SUB  0x2000 // 001xxxxxxxxxxxxx

#define THUMB_FORMAT_MASK_ALU           0xFC00  // Bits 15-10
#define THUMB_FORMAT_VAL_ALU            0x4000  // 010000xxxxxxxxxx

#define THUMB_FORMAT_MASK_HI_REG        0xFC00  // Bits 15-10
#define THUMB_FORMAT_VAL_HI_REG         0x4400  // 010001xxxxxxxxxx

#define THUMB_FORMAT_MASK_PC_REL        0xF800  // Bits 15-11
#define THUMB_FORMAT_VAL_PC_REL         0x4800  // 01001xxxxxxxxxxx

#define THUMB_FORMAT_MASK_LOAD_STORE_REG 0xF000 // Bits 15-12
#define THUMB_FORMAT_VAL_LOAD_STORE_REG  0x5000 // 0101xxxxxxxxxxxx

#define THUMB_FORMAT_MASK_LOAD_STORE_IMM 0xE000 // Bits 15-13
#define THUMB_FORMAT_VAL_LOAD_STORE_IMM  0x6000 // 011xxxxxxxxxxxxx

#define THUMB_FORMAT_MASK_LOAD_STORE_HALF 0xF000 // Bits 15-12
#define THUMB_FORMAT_VAL_LOAD_STORE_HALF  0x8000 // 1000xxxxxxxxxxxx

#define THUMB_FORMAT_MASK_SP_REL        0xF000  // Bits 15-12
#define THUMB_FORMAT_VAL_SP_REL         0x9000  // 1001xxxxxxxxxxxx

#define THUMB_FORMAT_MASK_LOAD_ADDR     0xF000  // Bits 15-12
#define THUMB_FORMAT_VAL_LOAD_ADDR      0xA000  // 1010xxxxxxxxxxxx

#define THUMB_FORMAT_MASK_SP_ADJUST     0xFF00  // Bits 15-8
#define THUMB_FORMAT_VAL_SP_ADJUST      0xB000  // 10110000xxxxxxxx

#define THUMB_FORMAT_MASK_PUSH_POP      0xF600  // Bits 15,14,12,10,9
#define THUMB_FORMAT_VAL_PUSH           0xB400  // 1011010xxxxxxxxx
#define THUMB_FORMAT_VAL_POP            0xBC00  // 1011110xxxxxxxxx

#define THUMB_FORMAT_MASK_MULTIPLE      0xF000  // Bits 15-12
#define THUMB_FORMAT_VAL_MULTIPLE       0xC000  // 1100xxxxxxxxxxxx

#define THUMB_FORMAT_MASK_BRANCH_COND   0xF000  // Bits 15-12
#define THUMB_FORMAT_VAL_BRANCH_COND    0xD000  // 1101xxxxxxxxxxxx

#define THUMB_FORMAT_MASK_SWI           0xFF00  // Bits 15-8
#define THUMB_FORMAT_VAL_SWI            0xDF00  // 11011111xxxxxxxx

#define THUMB_FORMAT_MASK_BRANCH        0xF800  // Bits 15-11
#define THUMB_FORMAT_VAL_BRANCH         0xE000  // 11100xxxxxxxxxxx

#define THUMB_FORMAT_MASK_BRANCH_LINK   0xF000  // Bits 15-12
#define THUMB_FORMAT_VAL_BRANCH_LINK    0xF000  // 1111xxxxxxxxxxxx

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
uint32_t thumb_calculate_instruction_cycles(uint16_t instruction, uint32_t pc, uint32_t* registers);
uint32_t thumb_get_multiply_cycles(uint32_t operand);
uint32_t thumb_count_registers(uint16_t register_list);
bool thumb_is_branch_taken(uint16_t instruction, uint32_t cpsr);

#ifdef __cplusplus
}
#endif

#endif // THUMB_TIMING_H
