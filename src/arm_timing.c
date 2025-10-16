#include "arm_timing.h"
#include "timing.h"
#include <stdint.h>
#include <stdbool.h>

// Lookup table for common instruction patterns (indexed by bits 27-20)
// This covers the most frequent instruction patterns to avoid conditional logic
static const uint8_t arm_instruction_cycles_lut[256] = {
    // 0x00-0x1F: Data processing with immediate operand 2
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    // 0x20-0x3F: Data processing with register operand 2  
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    // 0x40-0x7F: Single data transfer (LDR/STR) - 1 cycle prefetch, memory cycles added separately
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    // 0x80-0x9F: Block data transfer
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    // 0xA0-0xBF: Branch
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    // 0xC0-0xEF: Coprocessor
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    // 0xF0-0xFF: Software interrupt
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3
};

// Get sequential access cycles for PC location (for prefetch)
static uint32_t get_seq_cycles_for_pc(uint32_t pc) {
    uint8_t region = (pc >> 24) & 0xFF;
    switch (region) {
        case 0x00: return 1; // BIOS - 1 cycle sequential
        case 0x02: return 3; // EWRAM - 3 cycles sequential for 32-bit
        case 0x03: return 1; // IWRAM - 1 cycle sequential
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D: return 3; // ROM - 3 cycles sequential (default wait state)
        default: return 1; // Other regions default to 1
    }
}

// Calculate cycles for an ARM instruction before execution
uint32_t arm_calculate_instruction_cycles(uint32_t instruction, uint32_t pc, uint32_t* registers, uint32_t cpsr) {
    // ARM7TDMI Pipeline Model:
    // The 3-stage pipeline (Fetch -> Decode -> Execute) runs in parallel.
    // While instruction N executes, instruction N+1 is decoded and N+2 is fetched.
    // Therefore, we only charge the INTERNAL execution cycles, not the fetch.
    // Exception: Pipeline breaks (branches, PC writes) require 2 cycles to refill.
    
    // Fast-path optimization for ARM_COND_AL (always execute) - most common case
    ARMCondition condition = (ARMCondition)ARM_GET_CONDITION(instruction);
    if (condition != ARM_COND_AL) {
        // Check condition - if not met, instruction takes 1 cycle (just the execute slot)
        if (!arm_check_condition(condition, cpsr)) {
            return 1; // Failed condition: 1 cycle to evaluate, fetch happened in parallel
        }
    }
    
    // Try lookup table first for common patterns
    uint32_t lut_index = (instruction >> 20) & 0xFF; // Bits 27-20
    uint32_t base_cycles = arm_instruction_cycles_lut[lut_index];
    
    // For simple cases, return the base cycles
    uint32_t format = ARM_GET_FORMAT(instruction);
    if (format == 0x5) { // Branch - simple case
        // Branch: 1 cycle internal + 2 cycles for pipeline refill = 3 total
        return 3;
    }
    if (format == 0x6 || format == 0x7) { // Coprocessor/SWI - check for SWI
        if ((instruction & 0x0F000000) == 0x0F000000) {
            return 3; // SWI: 1 cycle internal + 2 cycles pipeline refill
        }
        return 1; // Coprocessor: just 1 cycle (not implemented, treat as NOP)
    }
    
    // Complex cases that need detailed analysis
    uint32_t extra_cycles = 0;
    
    // ARM instruction decoding based on bits 27-25 and secondary decoding
    if (format == 0x0 || format == 0x1) {
        // 00x: Data processing and misc instructions
        if ((instruction & 0x0FB00FF0) == 0x01000090) {
            // Single Data Swap (SWP)
            extra_cycles = 1; // +1 cycle for swap operation
        } else if ((instruction & 0x0FC000F0) == 0x00000090) {
            // Multiply instructions - this requires register value lookup
            extra_cycles = ARM_CYCLES_MULTIPLY_BASE;
            uint32_t rm = ARM_GET_RM(instruction);
            extra_cycles += arm_get_multiply_cycles(registers[rm]);
        } else if ((instruction & 0x0E000000) == 0x00000000) {
            // Data processing - 1 cycle base
            extra_cycles = 0;
            
            // Check for MSR/MRS (PSR transfer) instructions
            // MSR: bits 27-26=00, bits 24-23=10, bit 21=1, bits 19-16=0xF (or other patterns)
            // MRS: bits 27-26=00, bits 24-23=00, bit 21=0, bits 19-16=0xF
            bool is_psr_transfer = ((instruction & 0x0FB00000) == 0x01000000) ||  // MRS
                                   ((instruction & 0x0FB00000) == 0x01200000);     // MSR
            
            if (!is_psr_transfer) {
                // Add extra cycle if shift amount is specified by register
                if (!ARM_GET_IMMEDIATE_FLAG(instruction) && ARM_GET_REG_SHIFT_FLAG(instruction)) {
                    extra_cycles += ARM_CYCLES_SHIFT_BY_REG;
                }
                // Check for PC as destination - adds pipeline refill cycles
                uint32_t rd = ARM_GET_RD(instruction);
                if (rd == 15) {
                    extra_cycles += 2; // Pipeline refill for PC modification
                }
            }
            // PSR transfer instructions (MSR/MRS) take 1 cycle base only
        } else {
            extra_cycles = 0; // Unknown, treat as 1 cycle base
        }
    } else if (format == 0x2 || format == 0x3) {
        // 01x: Single data transfer (LDR/STR)
        // 1 cycle internal + 1 cycle for address calculation = 2 cycles base
        // Memory wait cycles will be added by memory.cpp
        extra_cycles = 1;
    } else if (format == 0x4) {
        // 100: Block data transfer (LDM/STM)
        // Count registers and charge per register
        // Memory wait cycles will be added by memory.cpp for each access
        uint16_t register_list = instruction & 0xFFFF;
        uint32_t num_registers = arm_count_registers(register_list);
        extra_cycles = num_registers * ARM_CYCLES_TRANSFER_REG;
    } else {
        // Fallback: 1 cycle base
        extra_cycles = 0;
    }
    
    return 1 + extra_cycles; // 1 cycle base (internal execution) + extra cycles
}

// Check if ARM condition is satisfied
bool arm_check_condition(ARMCondition condition, uint32_t cpsr) {
    // Fast path for most common conditions
    if (condition == ARM_COND_AL) return true;  // Always - most common
    
    // Extract condition flags only when needed
    bool N = (cpsr >> 31) & 1;  // Negative
    bool Z = (cpsr >> 30) & 1;  // Zero
    bool C = (cpsr >> 29) & 1;  // Carry
    bool V = (cpsr >> 28) & 1;  // Overflow
    
    // Order by frequency: EQ, NE, CS, CC are most common after AL
    switch (condition) {
        case ARM_COND_EQ: return Z;                    // Equal
        case ARM_COND_NE: return !Z;                   // Not equal
        case ARM_COND_CS: return C;                    // Carry set
        case ARM_COND_CC: return !C;                   // Carry clear
        case ARM_COND_MI: return N;                    // Minus
        case ARM_COND_PL: return !N;                   // Plus
        case ARM_COND_GE: return N == V;               // Greater or equal
        case ARM_COND_LT: return N != V;               // Less than
        case ARM_COND_GT: return !Z && (N == V);       // Greater than
        case ARM_COND_LE: return Z || (N != V);        // Less than or equal
        case ARM_COND_HI: return C && !Z;              // Higher
        case ARM_COND_LS: return !C || Z;              // Lower or same
        case ARM_COND_VS: return V;                    // Overflow
        case ARM_COND_VC: return !V;                   // No overflow
        case ARM_COND_NV: return true;                 // Never (deprecated, treat as always)
        default: return false;
    }
}

// Calculate multiply cycles based on operand value (same logic as Thumb)
uint32_t arm_get_multiply_cycles(uint32_t operand) {
    if (operand == 0) return ARM_CYCLES_MULTIPLY_MIN;
    if ((operand & 0xFFFFFF00) == 0 || (operand & 0xFFFFFF00) == 0xFFFFFF00) return ARM_CYCLES_MULTIPLY_MIN;
    if ((operand & 0xFFFF0000) == 0 || (operand & 0xFFFF0000) == 0xFFFF0000) return ARM_CYCLES_MULTIPLY_MIN + 1;
    if ((operand & 0xFF000000) == 0 || (operand & 0xFF000000) == 0xFF000000) return ARM_CYCLES_MULTIPLY_MIN + 2;
    return ARM_CYCLES_MULTIPLY_MAX;
}

// Count number of set bits in register list for LDM/STM
// Uses Brian Kernighan's algorithm for efficient bit counting
uint32_t arm_count_registers(uint16_t register_list) {
    uint32_t count = 0;
    uint32_t value = register_list;
    
    // Brian Kernighan's algorithm: n & (n-1) clears the lowest set bit
    while (value) {
        count++;
        value &= value - 1;
    }
    
    return count;
}

// Calculate shifted register operand (simplified - full implementation would handle all cases)
uint32_t arm_calculate_shifted_register(uint32_t instruction, uint32_t* registers, uint32_t* carry_out) {
    uint32_t rm = ARM_GET_RM(instruction);
    uint32_t value = registers[rm];
    ARMShiftType shift_type = (ARMShiftType)ARM_GET_SHIFT_TYPE(instruction);
    uint32_t shift_amount;
    
    if (ARM_GET_REG_SHIFT_FLAG(instruction)) {
        // Shift amount specified by register
        uint32_t rs = ARM_GET_RS(instruction);
        shift_amount = registers[rs] & 0xFF;
    } else {
        // Shift amount is immediate
        shift_amount = ARM_GET_SHIFT_AMOUNT(instruction);
    }
    
    // Handle special case of no shift
    if (shift_amount == 0) {
        *carry_out = 0; // Carry unchanged
        return value;
    }
    
    switch (shift_type) {
        case ARM_SHIFT_LSL: // Logical shift left
            if (shift_amount >= 32) {
                *carry_out = (shift_amount == 32) ? (value & 1) : 0;
                return 0;
            }
            *carry_out = (value >> (32 - shift_amount)) & 1;
            return value << shift_amount;
            
        case ARM_SHIFT_LSR: // Logical shift right
            if (shift_amount >= 32) {
                *carry_out = (shift_amount == 32) ? ((value >> 31) & 1) : 0;
                return 0;
            }
            *carry_out = (value >> (shift_amount - 1)) & 1;
            return value >> shift_amount;
            
        case ARM_SHIFT_ASR: // Arithmetic shift right
            if (shift_amount >= 32) {
                *carry_out = (value >> 31) & 1;
                return (value & 0x80000000) ? 0xFFFFFFFF : 0;
            }
            *carry_out = (value >> (shift_amount - 1)) & 1;
            return (int32_t)value >> shift_amount;
            
        case ARM_SHIFT_ROR: // Rotate right
            shift_amount &= 31; // Rotate is modulo 32
            if (shift_amount == 0) {
                *carry_out = 0;
                return value;
            }
            *carry_out = (value >> (shift_amount - 1)) & 1;
            return (value >> shift_amount) | (value << (32 - shift_amount));
            
        default:
            *carry_out = 0;
            return value;
    }
}

// Calculate immediate operand with rotation
uint32_t arm_calculate_immediate_operand(uint32_t instruction, uint32_t* carry_out) {
    uint32_t immediate = instruction & 0xFF;
    uint32_t rotate = ((instruction >> 8) & 0xF) * 2;
    
    if (rotate == 0) {
        *carry_out = 0; // Carry unchanged
        return immediate;
    }
    
    // Rotate right by rotate amount
    *carry_out = (immediate >> (rotate - 1)) & 1;
    return (immediate >> rotate) | (immediate << (32 - rotate));
}
