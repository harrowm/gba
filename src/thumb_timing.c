#include "thumb_timing.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Forward declarations for static helper functions
static uint32_t thumb_get_multiply_cycles(uint32_t operand);
static uint32_t thumb_count_registers(uint16_t register_list);

// Calculate cycles for a Thumb instruction before execution
uint32_t thumb_calculate_instruction_cycles(uint16_t instruction, uint32_t pc, uint32_t* registers, uint32_t cpsr) {
    // ARM7TDMI Pipeline Model:
    // The 3-stage pipeline (Fetch -> Decode -> Execute) runs in parallel.
    // While instruction N executes, instruction N+1 is decoded and N+2 is fetched.
    // Therefore, we only charge the INTERNAL execution cycles, not the fetch.
    // Exception: Pipeline breaks (branches) require 2 cycles to refill.
    uint32_t cycles = 0;
    
    // Suppress unused parameter warnings
    (void)registers;
    (void)pc;
    
    // Identify instruction format and calculate base cycles
    if ((instruction & THUMB_FORMAT_MASK_SHIFT_IMM) == THUMB_FORMAT_VAL_SHIFT_IMM) {
        // Format 1: Shift immediate (LSL, LSR, ASR)
        cycles = THUMB_CYCLES_SHIFT_IMM;
        
    } else if ((instruction & THUMB_FORMAT_MASK_ADD_SUB) == THUMB_FORMAT_VAL_ADD_SUB) {
        // Format 2: Add/subtract register/immediate
        cycles = THUMB_CYCLES_ADD_SUB_IMM;
        
    } else if ((instruction & THUMB_FORMAT_MASK_MOV_CMP_ADD_SUB) == THUMB_FORMAT_VAL_MOV_CMP_ADD_SUB) {
        // Format 3: MOV/CMP/ADD/SUB immediate
        uint8_t op = (instruction >> 11) & 0x3;
        switch (op) {
            case 0: cycles = THUMB_CYCLES_MOV_IMM; break;      // MOV
            case 1: cycles = THUMB_CYCLES_CMP_IMM; break;      // CMP
            case 2: 
            case 3: cycles = THUMB_CYCLES_ADD_SUB_IMM; break;  // ADD/SUB
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_ALU) == THUMB_FORMAT_VAL_ALU) {
        // Format 4: ALU operations
        uint8_t op = (instruction >> 6) & 0xF;
        if (op == 0xD) { // MUL
            // Multiply cycles depend on operand value
            uint8_t rs = (instruction >> 3) & 0x7;
            cycles = THUMB_CYCLES_ALU + thumb_get_multiply_cycles(registers[rs]);
        } else {
            cycles = THUMB_CYCLES_ALU;
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_HI_REG) == THUMB_FORMAT_VAL_HI_REG) {
        // Format 5: Hi register operations/branch exchange
        uint8_t op = (instruction >> 8) & 0x3;
        if (op == 3) { // BX
            cycles = THUMB_CYCLES_BRANCH_TAKEN;
        } else {
            cycles = THUMB_CYCLES_HI_REG_OP;
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_PC_REL) == THUMB_FORMAT_VAL_PC_REL) {
        // Format 6: PC-relative load (always a load)
        // LDR: 1S+1N+1I → base 1 + 1 internal; data access from addWaitCycles
        cycles = THUMB_CYCLES_PC_REL_LOAD + 1;
        
    } else if ((instruction & THUMB_FORMAT_MASK_LOAD_STORE_REG) == THUMB_FORMAT_VAL_LOAD_STORE_REG) {
        // Format 7/8: Load/store with register offset
        // ARM7TDMI timing:
        //   Load (LDR/LDRB/LDRH/LDRSB/LDRSH): 1S+1N+1I → base + 1 internal
        //   Store (STR/STRB/STRH): 2N → base only
        // Memory access from addWaitCycles. Bit 11 = L for format 7.
        // For format 8 (bit 9=1): ops are STRH(00)/LDSB(01)/LDRH(10)/LDSH(11)
        //   where bits 11:10 determine the operation. STRH has bit11=0.
        {
            bool is_load;
            if (instruction & (1 << 9)) {
                // Format 8: bit 11 = H flag (0=STRH, 1=load variant)
                is_load = (instruction >> 11) & 1;
            } else {
                // Format 7: bit 11 = L (1=load, 0=store)
                is_load = (instruction >> 11) & 1;
            }
            cycles = THUMB_CYCLES_REG_OFFSET + (is_load ? 1 : 0);
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_LOAD_STORE_IMM) == THUMB_FORMAT_VAL_LOAD_STORE_IMM) {
        // Format 9: Load/store with immediate offset
        // Load: +1 internal cycle; Store: no extra
        {
            bool is_load = (instruction >> 11) & 1;  // L bit
            cycles = THUMB_CYCLES_IMM_OFFSET + (is_load ? 1 : 0);
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_LOAD_STORE_HALF) == THUMB_FORMAT_VAL_LOAD_STORE_HALF) {
        // Format 10: Load/store halfword
        // Load: +1 internal cycle; Store: no extra
        {
            bool is_load = (instruction >> 11) & 1;  // L bit
            cycles = THUMB_CYCLES_IMM_OFFSET + (is_load ? 1 : 0);
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_SP_REL) == THUMB_FORMAT_VAL_SP_REL) {
        // Format 11: SP-relative load/store
        // Load: +1 internal cycle; Store: no extra
        {
            bool is_load = (instruction >> 11) & 1;  // L bit
            cycles = THUMB_CYCLES_SP_REL + (is_load ? 1 : 0);
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_LOAD_ADDR) == THUMB_FORMAT_VAL_LOAD_ADDR) {
        // Format 12: Load address
        cycles = THUMB_CYCLES_LOAD_ADDR;
        
    } else if ((instruction & THUMB_FORMAT_MASK_SP_ADJUST) == THUMB_FORMAT_VAL_SP_ADJUST) {
        // Format 13: Add offset to stack pointer
        cycles = THUMB_CYCLES_SP_ADJUST;
        
    } else if ((instruction & THUMB_FORMAT_MASK_PUSH_POP) == THUMB_FORMAT_VAL_PUSH ||
               (instruction & THUMB_FORMAT_MASK_PUSH_POP) == THUMB_FORMAT_VAL_POP) {
        // Format 14: Push/pop registers
        uint8_t register_list = instruction & 0xFF;
        uint8_t lr_pc_bit = (instruction >> 8) & 0x1;
        uint32_t num_registers = thumb_count_registers(register_list) + (lr_pc_bit ? 1 : 0);
        cycles = THUMB_CYCLES_PUSH_POP_BASE + (num_registers * THUMB_CYCLES_TRANSFER_REG);
        
    } else if ((instruction & THUMB_FORMAT_MASK_MULTIPLE) == THUMB_FORMAT_VAL_MULTIPLE) {
        // Format 15: Multiple load/store
        uint8_t register_list = instruction & 0xFF;
        uint32_t num_registers = thumb_count_registers(register_list);
        cycles = THUMB_CYCLES_MULTIPLE_BASE + (num_registers * THUMB_CYCLES_TRANSFER_REG);
        
    } else if ((instruction & THUMB_FORMAT_MASK_BRANCH_COND) == THUMB_FORMAT_VAL_BRANCH_COND) {
        // Format 16: Conditional branch
        uint8_t condition = (instruction >> 8) & 0xF;
        if (condition == 0xF) {
            // SWI
            cycles = THUMB_CYCLES_SWI;
        } else {
            // Check if condition is satisfied using CPSR flags
            // Extract flags from CPSR
            bool N = (cpsr >> 31) & 1;
            bool Z = (cpsr >> 30) & 1;
            bool C = (cpsr >> 29) & 1;
            bool V = (cpsr >> 28) & 1;
            
            bool condition_met = false;
            switch (condition) {
                case 0x0: condition_met = Z; break;              // EQ (equal)
                case 0x1: condition_met = !Z; break;             // NE (not equal)
                case 0x2: condition_met = C; break;              // CS/HS (carry set)
                case 0x3: condition_met = !C; break;             // CC/LO (carry clear)
                case 0x4: condition_met = N; break;              // MI (negative)
                case 0x5: condition_met = !N; break;             // PL (positive or zero)
                case 0x6: condition_met = V; break;              // VS (overflow)
                case 0x7: condition_met = !V; break;             // VC (no overflow)
                case 0x8: condition_met = C && !Z; break;        // HI (unsigned higher)
                case 0x9: condition_met = !C || Z; break;        // LS (unsigned lower or same)
                case 0xA: condition_met = (N == V); break;       // GE (signed greater or equal)
                case 0xB: condition_met = (N != V); break;       // LT (signed less than)
                case 0xC: condition_met = !Z && (N == V); break; // GT (signed greater)
                case 0xD: condition_met = Z || (N != V); break;  // LE (signed less or equal)
                case 0xE: condition_met = true; break;           // AL (always)
                default: condition_met = false; break;
            }
            
            // If branch is taken, it costs 3 cycles (1 internal + 2 pipeline refill)
            // If not taken, it costs 1 cycle (just execute and continue)
            cycles = condition_met ? THUMB_CYCLES_BRANCH_TAKEN : THUMB_CYCLES_BRANCH_COND;
        }
        
    } else if ((instruction & THUMB_FORMAT_MASK_BRANCH) == THUMB_FORMAT_VAL_BRANCH) {
        // Format 18: Unconditional branch
        cycles = THUMB_CYCLES_BRANCH_TAKEN;
        
    } else if ((instruction & THUMB_FORMAT_MASK_BRANCH_LINK) == THUMB_FORMAT_VAL_BRANCH_LINK) {
        // Format 19: Long branch with link
        cycles = THUMB_CYCLES_BRANCH_LINK;
        
    } else {
        // Unknown instruction, assume 1 cycle
        cycles = 1;
    }
    
    return cycles; // Return only internal execution cycles (fetch happens in parallel)
}

// Calculate multiply cycles based on operand value
// GBA multiply timing: 1S + m cycles, where m is 1-4 depending on operand
static uint32_t thumb_get_multiply_cycles(uint32_t operand) {
    if (operand == 0) return 1;
    if ((operand & 0xFFFFFF00) == 0 || (operand & 0xFFFFFF00) == 0xFFFFFF00) return 1;
    if ((operand & 0xFFFF0000) == 0 || (operand & 0xFFFF0000) == 0xFFFF0000) return 2;
    if ((operand & 0xFF000000) == 0 || (operand & 0xFF000000) == 0xFF000000) return 3;
    return 4;
}

// Count number of set bits in register list
static uint32_t thumb_count_registers(uint16_t register_list) {
    uint32_t count = 0;
    for (int i = 0; i < 8; i++) {
        if (register_list & (1 << i)) {
            count++;
        }
    }
    return count;
}

// Check if conditional branch is taken (requires CPU state)
bool thumb_is_branch_taken(uint16_t instruction, uint32_t cpsr) {
    uint8_t condition = (instruction >> 8) & 0xF;
    
    // Extract condition flags
    bool N = (cpsr >> 31) & 1;
    bool Z = (cpsr >> 30) & 1;
    bool C = (cpsr >> 29) & 1;
    bool V = (cpsr >> 28) & 1;
    
    switch (condition) {
        case 0x0: return Z;                    // EQ
        case 0x1: return !Z;                   // NE
        case 0x2: return C;                    // CS/HS
        case 0x3: return !C;                   // CC/LO
        case 0x4: return N;                    // MI
        case 0x5: return !N;                   // PL
        case 0x6: return V;                    // VS
        case 0x7: return !V;                   // VC
        case 0x8: return C && !Z;              // HI
        case 0x9: return !C || Z;              // LS
        case 0xA: return N == V;               // GE
        case 0xB: return N != V;               // LT
        case 0xC: return !Z && (N == V);       // GT
        case 0xD: return Z || (N != V);        // LE
        case 0xE: return true;                 // AL (always)
        case 0xF: return false;                // Reserved (SWI)
        default: return false;
    }
}
