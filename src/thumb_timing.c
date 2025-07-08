#include "thumb_timing.h"
#include "timing.h"
#include <stdint.h>
#include <stdbool.h>

// Calculate cycles for a Thumb instruction before execution
uint32_t thumb_calculate_instruction_cycles(uint16_t instruction, uint32_t pc, uint32_t* registers) {
    uint32_t cycles = 0;
    
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
        // Format 6: PC-relative load
        cycles = THUMB_CYCLES_PC_REL_LOAD;
        // Add memory access cycles for loading from calculated address
        uint32_t address = (pc & 0xFFFFFFFC) + ((instruction & 0xFF) << 2);
        cycles += timing_calculate_memory_cycles(address, 4);
        
    } else if ((instruction & THUMB_FORMAT_MASK_LOAD_STORE_REG) == THUMB_FORMAT_VAL_LOAD_STORE_REG) {
        // Format 7/8: Load/store with register offset
        cycles = THUMB_CYCLES_REG_OFFSET;
        // Add memory access cycles
        uint8_t access_size = ((instruction >> 10) & 0x1) ? 1 : 4; // Byte or word
        if ((instruction >> 11) & 0x1) { // Sign-extended loads
            access_size = ((instruction >> 10) & 0x1) ? 2 : 1; // Halfword or signed byte
        }
        // We'd need the calculated address to get exact timing, use average
        cycles += (access_size == 1) ? 1 : (access_size == 2) ? 1 : 2;
        
    } else if ((instruction & THUMB_FORMAT_MASK_LOAD_STORE_IMM) == THUMB_FORMAT_VAL_LOAD_STORE_IMM) {
        // Format 9: Load/store with immediate offset
        cycles = THUMB_CYCLES_IMM_OFFSET;
        uint8_t access_size = ((instruction >> 12) & 0x1) ? 1 : 4; // Byte or word
        cycles += (access_size == 1) ? 1 : 2;
        
    } else if ((instruction & THUMB_FORMAT_MASK_LOAD_STORE_HALF) == THUMB_FORMAT_VAL_LOAD_STORE_HALF) {
        // Format 10: Load/store halfword
        cycles = THUMB_CYCLES_IMM_OFFSET + 1; // Halfword access
        
    } else if ((instruction & THUMB_FORMAT_MASK_SP_REL) == THUMB_FORMAT_VAL_SP_REL) {
        // Format 11: SP-relative load/store
        cycles = THUMB_CYCLES_SP_REL + 2; // SP-relative typically accesses stack (WRAM)
        
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
            // We can't determine if branch is taken without CPU state, assume not taken
            // The actual emulator should check condition and adjust
            cycles = THUMB_CYCLES_BRANCH_COND;
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
    
    return cycles;
}

// Calculate multiply cycles based on operand value
// GBA multiply timing: 1S + m cycles, where m is 1-4 depending on operand
uint32_t thumb_get_multiply_cycles(uint32_t operand) {
    if (operand == 0) return 1;
    if ((operand & 0xFFFFFF00) == 0 || (operand & 0xFFFFFF00) == 0xFFFFFF00) return 1;
    if ((operand & 0xFFFF0000) == 0 || (operand & 0xFFFF0000) == 0xFFFF0000) return 2;
    if ((operand & 0xFF000000) == 0 || (operand & 0xFF000000) == 0xFF000000) return 3;
    return 4;
}

// Count number of set bits in register list
uint32_t thumb_count_registers(uint16_t register_list) {
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
