/**
 * arm_execute_phase1.c
 * 
 * ARM instruction execution core implementation in C for improved performance.
 * This file contains the first phase of migrating ARM instruction execution from C++ to C.
 */

#include "arm_execute_phase1.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> // Include for debug output

// Use the existing macros from arm_timing.h through arm_execute_phase1.h

// Flag manipulation helpers
#define SET_FLAG(cpsr, flag) ((cpsr) |= (flag))
#define CLEAR_FLAG(cpsr, flag) ((cpsr) &= ~(flag))
#define GET_FLAG(cpsr, flag) (((cpsr) & (flag)) != 0)

// Helper functions for flag updates
void arm_update_flags(ArmCPUState* state, uint32_t result, uint32_t carry_out, bool set_flags) {
    if (!set_flags) return;
    
    // Update N (Negative) flag
    if (result & 0x80000000) {
        state->cpsr |= ARM_FLAG_N;
    } else {
        state->cpsr &= ~ARM_FLAG_N;
    }
    
    // Update Z (Zero) flag
    if (result == 0) {
        state->cpsr |= ARM_FLAG_Z;
    } else {
        state->cpsr &= ~ARM_FLAG_Z;
    }
    
    // Update C (Carry) flag based on carry_out
    if (carry_out) {
        state->cpsr |= ARM_FLAG_C;
    } else {
        state->cpsr &= ~ARM_FLAG_C;
    }
    
    // V flag is unchanged for logical operations
}

void arm_update_flags_add(ArmCPUState* state, uint32_t op1, uint32_t op2, uint32_t result, bool set_flags) {
    if (!set_flags) return;
    
    // Update N and Z flags
    arm_update_flags(state, result, 0, true);
    
    // Update C flag for addition (carry out)
    uint64_t result64 = (uint64_t)op1 + (uint64_t)op2;
    bool carry = (result64 > 0xFFFFFFFF);
    if (carry) {
        state->cpsr |= ARM_FLAG_C;
    } else {
        state->cpsr &= ~ARM_FLAG_C;
    }
    
    // Update V flag for addition (overflow)
    bool overflow = ((op1 ^ result) & (op2 ^ result) & 0x80000000) != 0;
    if (overflow) {
        state->cpsr |= ARM_FLAG_V;
    } else {
        state->cpsr &= ~ARM_FLAG_V;
    }
}

void arm_update_flags_sub(ArmCPUState* state, uint32_t op1, uint32_t op2, uint32_t result, bool set_flags) {
    if (!set_flags) return;
    
    // Update N and Z flags
    arm_update_flags(state, result, 0, true);
    
    // Update C flag for subtraction (NOT borrow)
    uint64_t result64 = (uint64_t)op1 - (uint64_t)op2;
    bool carry = (result64 <= 0xFFFFFFFF);
    if (carry) {
        state->cpsr |= ARM_FLAG_C;
    } else {
        state->cpsr &= ~ARM_FLAG_C;
    }
    
    // Update V flag for subtraction (overflow)
    bool overflow = ((op1 ^ op2) & (op1 ^ result) & 0x80000000) != 0;
    if (overflow) {
        state->cpsr |= ARM_FLAG_V;
    } else {
        state->cpsr &= ~ARM_FLAG_V;
    }
}

uint32_t arm_calculate_operand2(ArmCPUState* state, uint32_t instruction, uint32_t* carry_out) {
    *carry_out = 0; // Default carry out
    
    if (ARM_GET_IMMEDIATE_FLAG(instruction)) {
        // Immediate operand
        uint32_t immediate = ARM_GET_IMMEDIATE_VALUE(instruction);
        uint32_t rotate = ARM_GET_ROTATE_IMM(instruction) * 2;
        
        if (rotate == 0) {
            return immediate;
        }
        
        *carry_out = (immediate >> (rotate - 1)) & 1;
        return (immediate >> rotate) | (immediate << (32 - rotate));
    } else {
        // Register operand with optional shift
        uint32_t rm = ARM_GET_RM(instruction);
        uint32_t value = state->registers[rm];
        uint32_t shift_type = ARM_GET_SHIFT_TYPE(instruction);
        
        if (ARM_GET_REG_SHIFT_FLAG(instruction)) {
            // Shift amount in register
            uint32_t rs = (instruction >> 8) & 0xF;
            uint32_t shift_amount = state->registers[rs] & 0xFF;
            
            // Implement basic shifts (simplified)
            switch (shift_type) {
                case 0: // LSL
                    if (shift_amount == 0) return value;
                    if (shift_amount < 32) {
                        *carry_out = (value >> (32 - shift_amount)) & 1;
                        return value << shift_amount;
                    }
                    return 0;
                case 1: // LSR
                    if (shift_amount == 0) return value;
                    if (shift_amount < 32) {
                        *carry_out = (value >> (shift_amount - 1)) & 1;
                        return value >> shift_amount;
                    }
                    return 0;
                case 2: // ASR
                    if (shift_amount == 0) return value;
                    if (shift_amount < 32) {
                        *carry_out = (value >> (shift_amount - 1)) & 1;
                        return (int32_t)value >> shift_amount;
                    }
                    return (value & 0x80000000) ? 0xFFFFFFFF : 0;
                case 3: // ROR
                    if (shift_amount == 0) return value;
                    shift_amount &= 31;
                    if (shift_amount == 0) return value;
                    *carry_out = (value >> (shift_amount - 1)) & 1;
                    return (value >> shift_amount) | (value << (32 - shift_amount));
            }
        } else {
            // Immediate shift amount
            uint32_t shift_amount = ARM_GET_SHIFT_IMM(instruction);
            
            switch (shift_type) {
                case 0: // LSL
                    if (shift_amount == 0) return value;
                    *carry_out = (value >> (32 - shift_amount)) & 1;
                    return value << shift_amount;
                case 1: // LSR
                    if (shift_amount == 0) shift_amount = 32;
                    *carry_out = (value >> (shift_amount - 1)) & 1;
                    return value >> shift_amount;
                case 2: // ASR
                    if (shift_amount == 0) shift_amount = 32;
                    *carry_out = (value >> (shift_amount - 1)) & 1;
                    return (int32_t)value >> shift_amount;
                case 3: // ROR
                    if (shift_amount == 0) {
                        // RRX - rotate right through carry
                        *carry_out = value & 1;
                        return (value >> 1) | ((state->cpsr & ARM_FLAG_C) ? 0x80000000 : 0);
                    }
                    *carry_out = (value >> (shift_amount - 1)) & 1;
                    return (value >> shift_amount) | (value << (32 - shift_amount));
            }
        }
    }
    
    return 0;
}

// Data processing instruction implementations
bool arm_execute_data_processing(ArmCPUState* state, uint32_t instruction, void* memory_interface) {
    UNUSED(memory_interface);
    uint32_t opcode = ARM_GET_OPCODE(instruction);
    uint32_t rd = ARM_GET_RD(instruction);
    uint32_t rn = ARM_GET_RN(instruction);
    bool set_flags = ARM_GET_S_BIT(instruction);
    
    uint32_t carry_out = 0;
    uint32_t operand2 = arm_calculate_operand2(state, instruction, &carry_out);
    
    switch (opcode) {
        case ARM_OP_AND: {
            uint32_t result = state->registers[rn] & operand2;
            state->registers[rd] = result;
            arm_update_flags(state, result, carry_out, set_flags);
            break;
        }
        case ARM_OP_EOR: {
            uint32_t result = state->registers[rn] ^ operand2;
            state->registers[rd] = result;
            arm_update_flags(state, result, carry_out, set_flags);
            break;
        }
        case ARM_OP_SUB: {
            uint32_t result = state->registers[rn] - operand2;
            state->registers[rd] = result;
            arm_update_flags_sub(state, state->registers[rn], operand2, result, set_flags);
            break;
        }
        case ARM_OP_RSB: {
            uint32_t result = operand2 - state->registers[rn];
            state->registers[rd] = result;
            arm_update_flags_sub(state, operand2, state->registers[rn], result, set_flags);
            break;
        }
        case ARM_OP_ADD: {
            uint32_t result = state->registers[rn] + operand2;
            state->registers[rd] = result;
            arm_update_flags_add(state, state->registers[rn], operand2, result, set_flags);
            break;
        }
        case ARM_OP_ADC: {
            uint32_t carry_in = (state->cpsr & ARM_FLAG_C) ? 1 : 0;
            uint32_t result = state->registers[rn] + operand2 + carry_in;
            state->registers[rd] = result;
            arm_update_flags_add(state, state->registers[rn], operand2 + carry_in, result, set_flags);
            break;
        }
        case ARM_OP_SBC: {
            uint32_t carry_in = (state->cpsr & ARM_FLAG_C) ? 1 : 0;
            uint32_t result = state->registers[rn] - operand2 - (1 - carry_in);
            state->registers[rd] = result;
            arm_update_flags_sub(state, state->registers[rn], operand2 + (1 - carry_in), result, set_flags);
            break;
        }
        case ARM_OP_RSC: {
            uint32_t carry_in = (state->cpsr & ARM_FLAG_C) ? 1 : 0;
            uint32_t result = operand2 - state->registers[rn] - (1 - carry_in);
            state->registers[rd] = result;
            arm_update_flags_sub(state, operand2, state->registers[rn] + (1 - carry_in), result, set_flags);
            break;
        }
        case ARM_OP_TST: {
            uint32_t result = state->registers[rn] & operand2;
            arm_update_flags(state, result, carry_out, true); // TST always sets flags
            break;
        }
        case ARM_OP_TEQ: {
            uint32_t result = state->registers[rn] ^ operand2;
            arm_update_flags(state, result, carry_out, true); // TEQ always sets flags
            break;
        }
        case ARM_OP_CMP: {
            uint32_t result = state->registers[rn] - operand2;
            arm_update_flags_sub(state, state->registers[rn], operand2, result, true); // CMP always sets flags
            break;
        }
        case ARM_OP_CMN: {
            uint32_t result = state->registers[rn] + operand2;
            arm_update_flags_add(state, state->registers[rn], operand2, result, true); // CMN always sets flags
            break;
        }
        case ARM_OP_ORR: {
            uint32_t result = state->registers[rn] | operand2;
            state->registers[rd] = result;
            arm_update_flags(state, result, carry_out, set_flags);
            break;
        }
        case ARM_OP_MOV: {
            state->registers[rd] = operand2;
            arm_update_flags(state, operand2, carry_out, set_flags);
            break;
        }
        case ARM_OP_BIC: {
            uint32_t result = state->registers[rn] & ~operand2;
            state->registers[rd] = result;
            arm_update_flags(state, result, carry_out, set_flags);
            break;
        }
        case ARM_OP_MVN: {
            uint32_t result = ~operand2;
            state->registers[rd] = result;
            arm_update_flags(state, result, carry_out, set_flags);
            break;
        }
        default:
            return false; // Unknown opcode
    }
    
    return (rd == 15); // Return true if PC was modified
}

/**
 * Check if an ARM condition is satisfied based on the CPSR flags.
 * 
 * @param condition ARM condition code (0-15)
 * @param cpsr Current Program Status Register value
 * @return true if the condition is satisfied
 */
bool arm_execute_check_condition(ARMCondition condition, uint32_t cpsr) {
    bool satisfied = false;
    
    // Extract flags
    bool n_flag = (cpsr & ARM_FLAG_N) != 0;
    bool z_flag = (cpsr & ARM_FLAG_Z) != 0;
    bool c_flag = (cpsr & ARM_FLAG_C) != 0;
    bool v_flag = (cpsr & ARM_FLAG_V) != 0;
    
    switch (condition) {
        case ARM_COND_EQ: // EQ - Equal (Z set)
            satisfied = z_flag;
            break;
        case ARM_COND_NE: // NE - Not equal (Z clear)
            satisfied = !z_flag;
            break;
        case ARM_COND_CS: // CS/HS - Carry set/unsigned higher or same (C set)
            satisfied = c_flag;
            break;
        case ARM_COND_CC: // CC/LO - Carry clear/unsigned lower (C clear)
            satisfied = !c_flag;
            break;
        case ARM_COND_MI: // MI - Minus/negative (N set)
            satisfied = n_flag;
            break;
        case ARM_COND_PL: // PL - Plus/positive or zero (N clear)
            satisfied = !n_flag;
            break;
        case ARM_COND_VS: // VS - Overflow (V set)
            satisfied = v_flag;
            break;
        case ARM_COND_VC: // VC - No overflow (V clear)
            satisfied = !v_flag;
            break;
        case ARM_COND_HI: // HI - Unsigned higher (C set and Z clear)
            satisfied = c_flag && !z_flag;
            break;
        case ARM_COND_LS: // LS - Unsigned lower or same (C clear or Z set)
            satisfied = !c_flag || z_flag;
            break;
        case ARM_COND_GE: // GE - Signed greater than or equal (N == V)
            satisfied = n_flag == v_flag;
            break;
        case ARM_COND_LT: // LT - Signed less than (N != V)
            satisfied = n_flag != v_flag;
            break;
        case ARM_COND_GT: // GT - Signed greater than (Z clear AND (N == V))
            satisfied = !z_flag && (n_flag == v_flag);
            break;
        case ARM_COND_LE: // LE - Signed less than or equal (Z set OR (N != V))
            satisfied = z_flag || (n_flag != v_flag);
            break;
        case ARM_COND_AL: // AL - Always
            satisfied = true;
            break;
        case ARM_COND_NV: // NV - Never (deprecated, treated as always in ARMv4)
            satisfied = true;
            break;
    }
    
    return satisfied;
}

/**
 * Execute a single ARM instruction.
 * 
 * This is the main entry point for ARM instruction execution from the C++ code.
 * 
 * @param state Pointer to ARM CPU state
 * @param instruction 32-bit ARM instruction to execute
 * @param memory_interface Pointer to memory interface functions
 * @return true if the PC was modified during execution
 */
bool arm_execute_instruction(ArmCPUState* state, uint32_t instruction, void* memory_interface) {
    // Check condition first - most instructions are conditional
    ARMCondition condition = (ARMCondition)ARM_GET_CONDITION(instruction);
    if (!arm_execute_check_condition(condition, state->cpsr)) {
        return false; // Condition failed, PC not modified
    }
    
    // Get the basic instruction format
    uint8_t format = ARM_GET_FORMAT(instruction);
    
    // Track whether PC (R15) was modified
    bool pc_modified = false;
    
    // Dispatch based on instruction format
    switch (format) {
        case 0: // 000: Data processing or multiply
        case 1: // 001: Data processing with immediate operand or PSR transfer
            // Check for multiply instruction (special case in format 0)
            if (format == 0 && (instruction & 0x0F8000F0) == 0x00000090) {
                // Multiply instruction (mask excludes accumulate bit 21)
                pc_modified = arm_execute_multiply(state, instruction);
            } else {
                // Data processing instruction
                pc_modified = arm_execute_data_processing(state, instruction, memory_interface);
            }
            break;
            
        case 2: // 010: Single data transfer
        case 3: // 011: Single data transfer (or undefined)
            pc_modified = arm_execute_single_data_transfer(state, instruction, (ArmMemoryInterface*)memory_interface);
            break;
            
        case 4: // 100: Block data transfer
            pc_modified = arm_execute_block_data_transfer(state, instruction, (ArmMemoryInterface*)memory_interface);
            break;
            
        case 5: // 101: Branch and branch with link
            pc_modified = arm_execute_branch(state, instruction);
            break;
            
        case 6: // 110: Coprocessor data transfer
        case 7: // 111: Coprocessor operations and software interrupt
            // Check for software interrupt (SWI)
            if (format == 7 && (instruction & 0x0F000000) == 0x0F000000) {
                pc_modified = arm_execute_software_interrupt(state, instruction);
            } else {
                // Coprocessor instructions - not implemented yet
                pc_modified = false;
            }
            break;
            
        default:
            // Unknown instruction format
            pc_modified = false;
            break;
    }
    
    return pc_modified;
}

// Single data transfer instruction implementation
bool arm_execute_single_data_transfer(ArmCPUState* state, uint32_t instruction, ArmMemoryInterface* memory_interface) {
    // Decode instruction fields
    bool immediate = !((instruction >> 25) & 1);
    bool pre_indexed = (instruction >> 24) & 1;
    bool up = (instruction >> 23) & 1;
    bool byte = (instruction >> 22) & 1;
    bool writeback = (instruction >> 21) & 1;
    bool load = (instruction >> 20) & 1;
    uint32_t rn = (instruction >> 16) & 0xF;
    uint32_t rd = (instruction >> 12) & 0xF;
    uint32_t offset = instruction & 0xFFF;
    
    uint32_t address = state->registers[rn];
    uint32_t offset_value;
    
    if (immediate) {
        offset_value = offset;
    } else {
        // Register offset with optional shift
        uint32_t carry_out = 0;
        offset_value = arm_calculate_operand2(state, instruction, &carry_out);
    }
    
    // Calculate effective address
    if (pre_indexed) {
        if (up) {
            address += offset_value;
        } else {
            address -= offset_value;
        }
    }
    
    // Perform load or store operation
    if (load) {
        if (byte) {
            // Load byte and zero-extend
            state->registers[rd] = memory_interface->read8(memory_interface->context, address);
        } else {
            // Load word
            state->registers[rd] = memory_interface->read32(memory_interface->context, address);
        }
    } else {
        if (byte) {
            // Store byte (lower 8 bits)
            memory_interface->write8(memory_interface->context, address, state->registers[rd] & 0xFF);
        } else {
            // Store word
            memory_interface->write32(memory_interface->context, address, state->registers[rd]);
        }
    }
    
    // Handle post-indexed addressing and writeback
    if (!pre_indexed) {
        // Post-indexed: update base register after memory access
        if (up) {
            state->registers[rn] += offset_value;
        } else {
            state->registers[rn] -= offset_value;
        }
    } else if (writeback) {
        // Pre-indexed with writeback: update base register with calculated address
        state->registers[rn] = address;
    }
    
    // Return true if PC (R15) was modified
    return (rd == 15);
}

/**
 * Execute ARM multiply instruction (MUL/MLA).
 */
bool arm_execute_multiply(ArmCPUState* state, uint32_t instruction) {
    bool accumulate = ARM_GET_MULTIPLY_ACCUMULATE(instruction);
    bool set_flags = ARM_GET_S_BIT(instruction);
    uint32_t rd = (instruction >> 16) & 0xF;  // Bits 19-16 for multiply
    uint32_t rn = (instruction >> 12) & 0xF;  // Bits 15-12 for multiply
    uint32_t rs = ARM_GET_MULTIPLY_RS(instruction);
    uint32_t rm = ARM_GET_RM(instruction);
    
    uint64_t result64;
    if (accumulate) {
        // MLA: Multiply and accumulate
        result64 = (uint64_t)state->registers[rm] * (uint64_t)state->registers[rs] + (uint64_t)state->registers[rn];
    } else {
        // MUL: Multiply
        result64 = (uint64_t)state->registers[rm] * (uint64_t)state->registers[rs];
    }
    
    uint32_t result = (uint32_t)result64;
    state->registers[rd] = result;
    
    if (set_flags) {
        // Update N and Z flags
        if (result & 0x80000000) {
            state->cpsr |= ARM_FLAG_N;
        } else {
            state->cpsr &= ~ARM_FLAG_N;
        }
        
        if (result == 0) {
            state->cpsr |= ARM_FLAG_Z;
        } else {
            state->cpsr &= ~ARM_FLAG_Z;
        }
        
        // C and V flags are unpredictable for multiply instructions
    }
    
    // Return true if PC (R15) was modified
    return (rd == 15);
}

/**
 * Execute ARM block data transfer instruction (LDM/STM).
 */
bool arm_execute_block_data_transfer(ArmCPUState* state, uint32_t instruction, ArmMemoryInterface* memory_interface) {
    bool pre_indexed = ARM_GET_PRE_INDEX(instruction);
    bool up = ARM_GET_UP_DOWN(instruction);
    bool psr_force_user = ARM_GET_PSR_FORCE_USER(instruction);
    bool writeback = ARM_GET_WRITEBACK(instruction);
    bool load = ARM_GET_LOAD_STORE(instruction);
    uint32_t rn = ARM_GET_RN(instruction);
    uint16_t register_list = ARM_GET_REGISTER_LIST(instruction);
    
    uint32_t address = state->registers[rn];
    uint32_t start_address = address;
    
    // Count number of registers in list
    int reg_count = 0;
    for (int i = 0; i < 16; i++) {
        if (register_list & (1 << i)) {
            reg_count++;
        }
    }
    
    // Adjust start address for different addressing modes
    if (!up) {
        address -= reg_count * 4;
        start_address = address;
    }
    
    if (pre_indexed) {
        if (up) {
            address += 4;
        } else {
            address += 4;
        }
    }
    
    // Process register list
    for (int i = 0; i < 16; i++) {
        if (register_list & (1 << i)) {
            if (load) {
                state->registers[i] = memory_interface->read32(memory_interface->context, address);
            } else {
                memory_interface->write32(memory_interface->context, address, state->registers[i]);
            }
            address += 4;
        }
    }
    
    // Handle writeback
    if (writeback) {
        if (up) {
            state->registers[rn] = start_address + reg_count * 4;
        } else {
            state->registers[rn] = start_address;
        }
    }
    
    // Handle PSR force user mode (S bit)
    if (psr_force_user && load && (register_list & (1 << 15))) {
        // If PC is loaded and S bit is set, restore CPSR from SPSR
        // This is a mode-dependent operation that would need privilege checking
        // For now, we'll just note that this is a special case
    }
    
    // Return true if PC (R15) was modified
    return load && (register_list & (1 << 15));
}

/**
 * Execute ARM branch instruction (B/BL).
 */
bool arm_execute_branch(ArmCPUState* state, uint32_t instruction) {
    bool link = ARM_GET_LINK_BIT(instruction);
    int32_t offset = ARM_GET_BRANCH_OFFSET(instruction);
    
    // Sign extend 24-bit offset to 32-bit
    if (offset & 0x800000) {
        offset |= 0xFF000000;
    }
    
    // Offset is in words, convert to bytes
    offset <<= 2;
    
    uint32_t pc = state->registers[15];
    
    if (link) {
        // Branch with Link - save return address
        state->registers[14] = pc + 4; // LR = PC + 4
    }
    
    // Update PC (branch target = PC + offset + 8)
    state->registers[15] = pc + offset + 8;
    
    // Branch instructions always modify PC
    return true;
}

/**
 * Execute ARM software interrupt instruction (SWI).
 */
bool arm_execute_software_interrupt(ArmCPUState* state, uint32_t instruction) {
    uint32_t swi_number = ARM_GET_SWI_NUMBER(instruction);
    UNUSED(swi_number);
    
    // Save current PC and CPSR
    uint32_t pc = state->registers[15];
    uint32_t cpsr = state->cpsr;
    
    // Switch to SVC mode and disable interrupts
    state->cpsr = (cpsr & ~0x1F) | 0x13; // SVC mode
    state->cpsr |= 0x80; // Disable IRQ
    
    // Save return address in LR_svc
    state->registers[14] = pc + 4;
    
    // Jump to SWI vector
    state->registers[15] = 0x08; // SWI vector address
    
    // Note: In a full implementation, we would also need to:
    // - Save CPSR to SPSR_svc
    // - Handle different processor modes properly
    // - Implement proper register banking
    
    // SWI instructions always modify PC
    return true;
}