#include "arm_cpu.h"
#include "debug.h"
#include "timing.h"
#include "arm_timing.h"

ARMCPU::ARMCPU(CPU& cpu) : parentCPU(cpu) {
    Debug::log::info("Initializing ARMCPU with parent CPU");
}

ARMCPU::~ARMCPU() {
    // Cleanup logic if necessary
}

void ARMCPU::execute(uint32_t cycles) {
    Debug::log::info("Executing ARM instructions for " + std::to_string(cycles) + " cycles");
    Debug::log::info("Parent CPU memory size: " + std::to_string(parentCPU.getMemory().getSize()) + " bytes");
    
    while (cycles > 0) {
        // Check if we're still in ARM mode - if not, break out early
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            Debug::log::info("Mode switched to Thumb during execution, breaking out of ARM execution");
            break;
        }
        
        uint32_t pc = parentCPU.R()[15]; // Get current PC
        uint32_t instruction = parentCPU.getMemory().read32(pc); // Fetch instruction
        
        Debug::log::info("Fetched ARM instruction: " + Debug::toHexString(instruction, 8) + 
                        " at PC: " + Debug::toHexString(pc, 8));
        
        bool pc_modified = decodeAndExecute(instruction);
        
        // Only increment PC if instruction didn't modify it (e.g., not a branch)
        if (!pc_modified) {
            parentCPU.R()[15] = pc + 4;
            Debug::log::info("Incremented PC to: " + Debug::toHexString(parentCPU.R()[15], 8));
        }
        
        cycles -= 1; // Placeholder for cycle deduction
    }
}

// New timing-aware execution method
void ARMCPU::executeWithTiming(uint32_t cycles, TimingState* timing) {
    Debug::log::info("Executing ARM instructions with timing for " + std::to_string(cycles) + " cycles");
    
    while (cycles > 0) {
        // Check if we're still in ARM mode - if not, break out early
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            Debug::log::info("Mode switched to Thumb during timing execution, breaking out of ARM execution");
            break;
        }
        
        // Calculate cycles until next timing event
        uint32_t cycles_until_event = timing_cycles_until_next_event(timing);
        
        // Fetch next instruction to determine its cycle cost
        uint32_t pc = parentCPU.R()[15];
        uint32_t instruction = parentCPU.getMemory().read32(pc);
        uint32_t instruction_cycles = calculateInstructionCycles(instruction);
        
        Debug::log::debug("Next ARM instruction: " + Debug::toHexString(instruction, 8) + 
                         " at PC: " + Debug::toHexString(pc, 8) + 
                         " will take " + std::to_string(instruction_cycles) + " cycles");
        Debug::log::debug("Cycles until next event: " + std::to_string(cycles_until_event));
        
        // Check if instruction will complete before next timing event
        if (instruction_cycles <= cycles_until_event) {
            // Execute instruction normally
            bool pc_modified = decodeAndExecute(instruction);
            
            if (!pc_modified) {
                parentCPU.R()[15] = pc + 4;
            }
            
            // Update timing
            timing_advance(timing, instruction_cycles);
            cycles -= instruction_cycles;
            
        } else {
            // Process timing event first, then continue
            Debug::log::debug("Processing timing event before ARM instruction");
            timing_advance(timing, cycles_until_event);
            timing_process_timer_events(timing);
            timing_process_video_events(timing);
            cycles -= cycles_until_event;
        }
    }
}

// Calculate cycles for next instruction without executing it
uint32_t ARMCPU::calculateInstructionCycles(uint32_t instruction) {
    // Convert CPU registers to array format for the C function
    uint32_t registers[16];
    for (int i = 0; i < 16; i++) {
        registers[i] = parentCPU.R()[i];
    }
    
    uint32_t pc = parentCPU.R()[15];
    uint32_t cpsr = parentCPU.CPSR();
    return arm_calculate_instruction_cycles(instruction, pc, registers, cpsr);
}

bool ARMCPU::decodeAndExecute(uint32_t instruction) {
    Debug::log::info("Decoding and executing ARM instruction: " + Debug::toHexString(instruction, 8));
    
    // Check condition first
    if (!checkCondition(instruction)) {
        Debug::log::debug("ARM instruction condition not met, skipping");
        return false; // PC not modified
    }
    
    // More detailed instruction decoding
    uint32_t format = ARM_GET_FORMAT(instruction);
    bool pc_modified = false;
    
    // Special case handling for multiply instructions (format 000 with specific bit pattern)
    if (format == 0 && (instruction & 0x0FC000F0) == 0x00000090) {
        arm_multiply(instruction);
        uint32_t rd = (instruction >> 16) & 0xF;
        pc_modified = (rd == 15);
    }
    // BX (Branch and Exchange) instruction (format 000 with specific bit pattern)
    else if (format == 0 && (instruction & 0x0FFFFFF0) == 0x012FFF10) {
        arm_bx(instruction);
        pc_modified = true; // BX always modifies PC
    }
    // PSR transfer instructions (format 001 with specific bit pattern)
    else if (format == 1 && (instruction & 0x0FB00000) == 0x01000000) {
        arm_psr_transfer(instruction);
        if ((instruction & 0x00200000) == 0) { // MRS instruction
            uint32_t rd = (instruction >> 12) & 0xF;
            pc_modified = (rd == 15);
        }
    }
    // Coprocessor instructions (format 110/111 with specific patterns)
    else if (format == 6) {
        if ((instruction & 0x0E000000) == 0x0C000000) {
            // Coprocessor data transfer
            arm_coprocessor_transfer(instruction);
        } else {
            // Coprocessor operation
            arm_coprocessor_operation(instruction);
        }
    }
    else if (format == 7) {
        if ((instruction & 0x0F000000) == 0x0F000000) {
            // Software interrupt
            arm_software_interrupt(instruction);
            pc_modified = true;
        } else {
            // Coprocessor register transfer
            arm_coprocessor_register(instruction);
            bool load = (instruction >> 20) & 1;
            if (load) {
                uint32_t rd = (instruction >> 12) & 0xF;
                pc_modified = (rd == 15);
            }
        }
    }
    // Execute instruction based on format using lookup table
    else if (format < 8 && arm_instruction_table[format]) {
        (this->*arm_instruction_table[format])(instruction);
        
        // Determine if PC was modified based on instruction type
        switch (format) {
            case 0: // Data processing
            case 1:
                {
                    uint32_t rd = ARM_GET_RD(instruction);
                    pc_modified = (rd == 15);
                }
                break;
            case 2: // Single data transfer
            case 3:
                {
                    bool load = (instruction >> 20) & 1;
                    uint32_t rd = (instruction >> 12) & 0xF;
                    pc_modified = load && (rd == 15);
                }
                break;
            case 4: // Block data transfer
                {
                    bool load = (instruction >> 20) & 1;
                    uint16_t register_list = instruction & 0xFFFF;
                    pc_modified = load && (register_list & (1 << 15));
                }
                break;
            case 5: // Branch
                pc_modified = true;
                break;
            case 6: // Coprocessor (already handled above)
            case 7: // Coprocessor/SWI (already handled above)
                break;
        }
    } else {
        Debug::log::error("Unknown ARM instruction format: " + std::to_string(format));
        arm_undefined(instruction);
        pc_modified = true; // Undefined instruction changes PC
    }
    
    return pc_modified;
}

// Check if instruction condition is satisfied
bool ARMCPU::checkCondition(uint32_t instruction) {
    ARMCondition condition = (ARMCondition)ARM_GET_CONDITION(instruction);
    return arm_check_condition(condition, parentCPU.CPSR());
}

// Data processing instruction handler
void ARMCPU::arm_data_processing(uint32_t instruction) {
    uint32_t opcode = ARM_GET_OPCODE(instruction);
    uint32_t rd = ARM_GET_RD(instruction);
    uint32_t rn = ARM_GET_RN(instruction);
    bool set_flags = ARM_GET_S_BIT(instruction);
    
    uint32_t carry_out = 0;
    uint32_t operand2 = calculateOperand2(instruction, &carry_out);
    
    Debug::log::debug("ARM Data Processing: opcode=" + std::to_string(opcode) + 
                     " rd=R" + std::to_string(rd) + " rn=R" + std::to_string(rn) + 
                     " operand2=0x" + Debug::toHexString(operand2, 8) + 
                     " set_flags=" + std::to_string(set_flags));
    
    switch (opcode) {
        case ARM_OP_AND: arm_and(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_EOR: arm_eor(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_SUB: arm_sub(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_RSB: arm_rsb(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_ADD: arm_add(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_ADC: arm_adc(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_SBC: arm_sbc(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_RSC: arm_rsc(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_TST: arm_tst(rn, operand2, carry_out); break;
        case ARM_OP_TEQ: arm_teq(rn, operand2, carry_out); break;
        case ARM_OP_CMP: arm_cmp(rn, operand2, carry_out); break;
        case ARM_OP_CMN: arm_cmn(rn, operand2, carry_out); break;
        case ARM_OP_ORR: arm_orr(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_MOV: arm_mov(rd, operand2, set_flags, carry_out); break;
        case ARM_OP_BIC: arm_bic(rd, rn, operand2, set_flags, carry_out); break;
        case ARM_OP_MVN: arm_mvn(rd, operand2, set_flags, carry_out); break;
        default:
            Debug::log::error("Unknown ARM data processing opcode: " + std::to_string(opcode));
            break;
    }
}

// Data processing operation implementations
void ARMCPU::arm_and(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] & operand2;
    parentCPU.R()[rd] = result;
    if (set_flags) updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_eor(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] ^ operand2;
    parentCPU.R()[rd] = result;
    if (set_flags) updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_sub(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint64_t result64 = (uint64_t)parentCPU.R()[rn] - (uint64_t)operand2;
    uint32_t result = (uint32_t)result64;
    parentCPU.R()[rd] = result;
    if (set_flags) {
        bool carry = (result64 <= 0xFFFFFFFF);
        bool overflow = ((parentCPU.R()[rn] ^ operand2) & (parentCPU.R()[rn] ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_rsb(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint64_t result64 = (uint64_t)operand2 - (uint64_t)parentCPU.R()[rn];
    uint32_t result = (uint32_t)result64;
    parentCPU.R()[rd] = result;
    if (set_flags) {
        bool carry = (result64 <= 0xFFFFFFFF);
        bool overflow = ((operand2 ^ parentCPU.R()[rn]) & (operand2 ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_add(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint64_t result64 = (uint64_t)parentCPU.R()[rn] + (uint64_t)operand2;
    uint32_t result = (uint32_t)result64;
    parentCPU.R()[rd] = result;
    if (set_flags) {
        bool carry = (result64 > 0xFFFFFFFF);
        bool overflow = ((parentCPU.R()[rn] ^ result) & (operand2 ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_adc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint32_t carry_in = (parentCPU.CPSR() >> 29) & 1;
    uint64_t result64 = (uint64_t)parentCPU.R()[rn] + (uint64_t)operand2 + (uint64_t)carry_in;
    uint32_t result = (uint32_t)result64;
    parentCPU.R()[rd] = result;
    if (set_flags) {
        bool carry = (result64 > 0xFFFFFFFF);
        bool overflow = ((parentCPU.R()[rn] ^ result) & (operand2 ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_sbc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint32_t carry_in = (parentCPU.CPSR() >> 29) & 1;
    uint64_t result64 = (uint64_t)parentCPU.R()[rn] - (uint64_t)operand2 - (uint64_t)(1 - carry_in);
    uint32_t result = (uint32_t)result64;
    parentCPU.R()[rd] = result;
    if (set_flags) {
        bool carry = (result64 <= 0xFFFFFFFF);
        bool overflow = ((parentCPU.R()[rn] ^ operand2) & (parentCPU.R()[rn] ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_rsc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint32_t carry_in = (parentCPU.CPSR() >> 29) & 1;
    uint64_t result64 = (uint64_t)operand2 - (uint64_t)parentCPU.R()[rn] - (uint64_t)(1 - carry_in);
    uint32_t result = (uint32_t)result64;
    parentCPU.R()[rd] = result;
    if (set_flags) {
        bool carry = (result64 <= 0xFFFFFFFF);
        bool overflow = ((operand2 ^ parentCPU.R()[rn]) & (operand2 ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_tst(uint32_t rn, uint32_t operand2, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] & operand2;
    updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_teq(uint32_t rn, uint32_t operand2, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] ^ operand2;
    updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_cmp(uint32_t rn, uint32_t operand2, uint32_t carry_out) {
    UNUSED(carry_out);
    uint64_t result64 = (uint64_t)parentCPU.R()[rn] - (uint64_t)operand2;
    uint32_t result = (uint32_t)result64;
    bool carry = (result64 <= 0xFFFFFFFF);
    bool overflow = ((parentCPU.R()[rn] ^ operand2) & (parentCPU.R()[rn] ^ result) & 0x80000000) != 0;
    updateFlags(result, carry, overflow);
}

void ARMCPU::arm_cmn(uint32_t rn, uint32_t operand2, uint32_t carry_out) {
    UNUSED(carry_out);
    uint64_t result64 = (uint64_t)parentCPU.R()[rn] + (uint64_t)operand2;
    uint32_t result = (uint32_t)result64;
    bool carry = (result64 > 0xFFFFFFFF);
    bool overflow = ((parentCPU.R()[rn] ^ result) & (operand2 ^ result) & 0x80000000) != 0;
    updateFlags(result, carry, overflow);
}

void ARMCPU::arm_orr(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] | operand2;
    parentCPU.R()[rd] = result;
    if (set_flags) updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_mov(uint32_t rd, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    parentCPU.R()[rd] = operand2;
    if (set_flags) updateFlagsLogical(operand2, carry_out);
}

void ARMCPU::arm_bic(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] & ~operand2;
    parentCPU.R()[rd] = result;
    if (set_flags) updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_mvn(uint32_t rd, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = ~operand2;
    parentCPU.R()[rd] = result;
    if (set_flags) updateFlagsLogical(result, carry_out);
}

// Helper functions
uint32_t ARMCPU::calculateOperand2(uint32_t instruction, uint32_t* carry_out) {
    if (ARM_GET_IMMEDIATE_FLAG(instruction)) {
        return arm_calculate_immediate_operand(instruction, carry_out);
    } else {
        return arm_calculate_shifted_register(instruction, parentCPU.R().data(), carry_out);
    }
}

// Multiply instruction handler
void ARMCPU::arm_multiply(uint32_t instruction) {
    uint32_t opcode = (instruction >> 21) & 0xF;
    bool accumulate = (instruction >> 21) & 1;
    bool set_flags = (instruction >> 20) & 1;
    uint32_t rd = (instruction >> 16) & 0xF;
    uint32_t rn = (instruction >> 12) & 0xF;
    uint32_t rs = (instruction >> 8) & 0xF;
    uint32_t rm = instruction & 0xF;
    
    Debug::log::debug("ARM Multiply: opcode=" + std::to_string(opcode) + 
                     " rd=R" + std::to_string(rd) + " rn=R" + std::to_string(rn) + 
                     " rs=R" + std::to_string(rs) + " rm=R" + std::to_string(rm));
    
    if (accumulate) {
        // MLA: Multiply and accumulate
        uint64_t result64 = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs] + (uint64_t)parentCPU.R()[rn];
        parentCPU.R()[rd] = (uint32_t)result64;
    } else {
        // MUL: Multiply
        uint64_t result64 = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs];
        parentCPU.R()[rd] = (uint32_t)result64;
    }
    
    if (set_flags) {
        uint32_t result = parentCPU.R()[rd];
        // Update N and Z flags
        if (result & 0x80000000) {
            parentCPU.CPSR() |= CPU::FLAG_N;
        } else {
            parentCPU.CPSR() &= ~CPU::FLAG_N;
        }
        
        if (result == 0) {
            parentCPU.CPSR() |= CPU::FLAG_Z;
        } else {
            parentCPU.CPSR() &= ~CPU::FLAG_Z;
        }
        
        // C and V flags are unpredictable
    }
}

// Single data transfer (LDR/STR) instruction handler
void ARMCPU::arm_single_data_transfer(uint32_t instruction) {
    bool immediate = !((instruction >> 25) & 1);
    bool pre_indexed = (instruction >> 24) & 1;
    bool up = (instruction >> 23) & 1;
    bool byte = (instruction >> 22) & 1;
    bool writeback = (instruction >> 21) & 1;
    bool load = (instruction >> 20) & 1;
    uint32_t rn = (instruction >> 16) & 0xF;
    uint32_t rd = (instruction >> 12) & 0xF;
    uint32_t offset = instruction & 0xFFF;
    
    Debug::log::debug("ARM Single Data Transfer: " + 
                     std::string(load ? "LDR" : "STR") + 
                     std::string(byte ? "B" : "") + 
                     " rd=R" + std::to_string(rd) + " rn=R" + std::to_string(rn));
    
    uint32_t address = parentCPU.R()[rn];
    uint32_t offset_value;
    
    if (immediate) {
        offset_value = offset;
    } else {
        // Register offset with optional shift
        uint32_t carry_out = 0;
        offset_value = arm_calculate_shifted_register(instruction, parentCPU.R().data(), &carry_out);
    }
    
    // Calculate effective address
    if (pre_indexed) {
        if (up) {
            address += offset_value;
        } else {
            address -= offset_value;
        }
    }
    
    // Perform load or store
    if (load) {
        if (byte) {
            parentCPU.R()[rd] = parentCPU.getMemory().read8(address);
        } else {
            parentCPU.R()[rd] = parentCPU.getMemory().read32(address);
        }
    } else {
        if (byte) {
            parentCPU.getMemory().write8(address, parentCPU.R()[rd] & 0xFF);
        } else {
            parentCPU.getMemory().write32(address, parentCPU.R()[rd]);
        }
    }
    
    // Handle post-indexed addressing and writeback
    if (!pre_indexed) {
        if (up) {
            parentCPU.R()[rn] += offset_value;
        } else {
            parentCPU.R()[rn] -= offset_value;
        }
    } else if (writeback) {
        parentCPU.R()[rn] = address;
    }
}

// Block data transfer (LDM/STM) instruction handler
void ARMCPU::arm_block_data_transfer(uint32_t instruction) {
    bool pre_indexed = (instruction >> 24) & 1;
    bool up = (instruction >> 23) & 1;
    bool s_bit = (instruction >> 22) & 1;
    bool writeback = (instruction >> 21) & 1;
    bool load = (instruction >> 20) & 1;
    uint32_t rn = (instruction >> 16) & 0xF;
    uint16_t register_list = instruction & 0xFFFF;
    
    Debug::log::debug("ARM Block Data Transfer: " + 
                     std::string(load ? "LDM" : "STM") + 
                     " rn=R" + std::to_string(rn) + 
                     " reglist=0x" + Debug::toHexString(register_list, 4));
    
    uint32_t address = parentCPU.R()[rn];
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
                parentCPU.R()[i] = parentCPU.getMemory().read32(address);
            } else {
                parentCPU.getMemory().write32(address, parentCPU.R()[i]);
            }
            address += 4;
        }
    }
    
    // Handle writeback
    if (writeback) {
        if (up) {
            parentCPU.R()[rn] = start_address + reg_count * 4;
        } else {
            parentCPU.R()[rn] = start_address;
        }
    }
    
    // Handle S bit (user mode register transfer)
    if (s_bit && load && (register_list & (1 << 15))) {
        // If PC is loaded and S bit is set, restore CPSR from SPSR
        // This is a mode-dependent operation that would need privilege checking
        Debug::log::debug("ARM Block Transfer with S bit and PC load - CPSR restore");
    }
}

// Branch instruction handler
void ARMCPU::arm_branch(uint32_t instruction) {
    bool link = (instruction >> 24) & 1;
    int32_t offset = instruction & 0xFFFFFF;
    
    // Sign extend 24-bit offset to 32-bit
    if (offset & 0x800000) {
        offset |= 0xFF000000;
    }
    
    // Offset is in words, convert to bytes
    offset <<= 2;
    
    Debug::log::debug("ARM Branch: " + std::string(link ? "BL" : "B") + 
                     " offset=0x" + Debug::toHexString(offset, 8));
    
    uint32_t pc = parentCPU.R()[15];
    
    if (link) {
        // Branch with Link - save return address
        parentCPU.R()[14] = pc + 4; // LR = PC + 4
    }
    
    // Update PC
    parentCPU.R()[15] = pc + offset + 8; // PC = PC + offset + 8
}

// Software Interrupt handler
void ARMCPU::arm_software_interrupt(uint32_t instruction) {
    uint32_t swi_number = instruction & 0xFFFFFF;
    
    Debug::log::debug("ARM Software Interrupt: SWI 0x" + Debug::toHexString(swi_number, 6));
    
    // Save current PC and CPSR
    uint32_t pc = parentCPU.R()[15];
    uint32_t cpsr = parentCPU.CPSR();
    
    // Switch to SVC mode and disable interrupts
    parentCPU.CPSR() = (cpsr & ~0x1F) | 0x13; // SVC mode
    parentCPU.CPSR() |= 0x80; // Disable IRQ
    
    // Save return address in LR_svc
    parentCPU.R()[14] = pc + 4;
    
    // Jump to SWI vector
    parentCPU.R()[15] = 0x08; // SWI vector address
    
    // Note: In a full implementation, we would also need to:
    // - Save CPSR to SPSR_svc
    // - Handle different processor modes properly
    // - Implement proper register banking
}

// PSR Transfer instruction handler
void ARMCPU::arm_psr_transfer(uint32_t instruction) {
    bool immediate = (instruction >> 25) & 1;
    bool psr = (instruction >> 22) & 1; // 0=CPSR, 1=SPSR
    bool msr = (instruction >> 21) & 1; // 0=MRS, 1=MSR
    
    if (msr) {
        // MSR - Move to status register
        uint32_t field_mask = (instruction >> 16) & 0xF;
        uint32_t value;
        
        if (immediate) {
            uint32_t carry_out = 0;
            value = arm_calculate_immediate_operand(instruction, &carry_out);
        } else {
            uint32_t rm = instruction & 0xF;
            value = parentCPU.R()[rm];
        }
        
        Debug::log::debug("ARM PSR Transfer: MSR " + std::string(psr ? "SPSR" : "CPSR") + 
                         " field_mask=0x" + Debug::toHexString(field_mask, 1) + 
                         " value=0x" + Debug::toHexString(value, 8));
        
        // Apply field mask to determine which bits to update
        uint32_t mask = 0;
        if (field_mask & 1) mask |= 0x000000FF; // Control field
        if (field_mask & 2) mask |= 0x0000FF00; // Extension field
        if (field_mask & 4) mask |= 0x00FF0000; // Status field
        if (field_mask & 8) mask |= 0xFF000000; // Flags field
        
        if (psr) {
            // Update SPSR (would need proper register banking)
            Debug::log::debug("MSR SPSR not fully implemented - needs register banking");
        } else {
            // Update CPSR
            uint32_t old_cpsr = parentCPU.CPSR();
            parentCPU.CPSR() = (old_cpsr & ~mask) | (value & mask);
        }
    } else {
        // MRS - Move from status register
        uint32_t rd = (instruction >> 12) & 0xF;
        
        Debug::log::debug("ARM PSR Transfer: MRS R" + std::to_string(rd) + 
                         " " + std::string(psr ? "SPSR" : "CPSR"));
        
        if (psr) {
            // Read SPSR (would need proper register banking)
            Debug::log::debug("MRS SPSR not fully implemented - needs register banking");
            parentCPU.R()[rd] = parentCPU.CPSR(); // Fallback to CPSR
        } else {
            // Read CPSR
            parentCPU.R()[rd] = parentCPU.CPSR();
        }
    }
}

// Coprocessor operation handler
void ARMCPU::arm_coprocessor_operation(uint32_t instruction) {
    uint32_t cp_num = (instruction >> 8) & 0xF;
    
    Debug::log::debug("ARM Coprocessor Operation: CP" + std::to_string(cp_num));
    
    // GBA doesn't have coprocessors, so this should generate an undefined instruction exception
    Debug::log::error("Coprocessor " + std::to_string(cp_num) + " not implemented on GBA");
    arm_undefined(instruction);
}

// Coprocessor data transfer handler
void ARMCPU::arm_coprocessor_transfer(uint32_t instruction) {
    uint32_t cp_num = (instruction >> 8) & 0xF;
    
    Debug::log::debug("ARM Coprocessor Data Transfer: CP" + std::to_string(cp_num));
    
    // GBA doesn't have coprocessors
    Debug::log::error("Coprocessor " + std::to_string(cp_num) + " not implemented on GBA");
    arm_undefined(instruction);
}

// Coprocessor register transfer handler
void ARMCPU::arm_coprocessor_register(uint32_t instruction) {
    uint32_t cp_num = (instruction >> 8) & 0xF;
    
    Debug::log::debug("ARM Coprocessor Register Transfer: CP" + std::to_string(cp_num));
    
    // GBA doesn't have coprocessors
    Debug::log::error("Coprocessor " + std::to_string(cp_num) + " not implemented on GBA");
    arm_undefined(instruction);
}

// Undefined instruction handler
void ARMCPU::arm_undefined(uint32_t instruction) {
    Debug::log::error("ARM Undefined Instruction: 0x" + Debug::toHexString(instruction, 8));
    
    // In a real ARM processor, this would:
    // 1. Save current PC and CPSR
    // 2. Switch to Undefined mode
    // 3. Jump to undefined instruction vector (0x04)
    
    uint32_t pc = parentCPU.R()[15];
    uint32_t cpsr = parentCPU.CPSR();
    
    // Switch to Undefined mode
    parentCPU.CPSR() = (cpsr & ~0x1F) | 0x1B; // Undefined mode
    parentCPU.CPSR() |= 0x80; // Disable IRQ
    
    // Save return address
    parentCPU.R()[14] = pc + 4;
    
    // Jump to undefined instruction vector
    parentCPU.R()[15] = 0x04;
}

void ARMCPU::updateFlags(uint32_t result, bool carry, bool overflow) {
    // Update N (Negative) flag
    if (result & 0x80000000) {
        parentCPU.CPSR() |= CPU::FLAG_N;
    } else {
        parentCPU.CPSR() &= ~CPU::FLAG_N;
    }
    
    // Update Z (Zero) flag
    if (result == 0) {
        parentCPU.CPSR() |= CPU::FLAG_Z;
    } else {
        parentCPU.CPSR() &= ~CPU::FLAG_Z;
    }
    
    // Update C (Carry) flag
    if (carry) {
        parentCPU.CPSR() |= CPU::FLAG_C;
    } else {
        parentCPU.CPSR() &= ~CPU::FLAG_C;
    }
    
    // Update V (Overflow) flag
    if (overflow) {
        parentCPU.CPSR() |= CPU::FLAG_V;
    } else {
        parentCPU.CPSR() &= ~CPU::FLAG_V;
    }
}

void ARMCPU::updateFlagsLogical(uint32_t result, uint32_t carry_out) {
    // Update N and Z flags
    updateFlags(result, false, false);
    
    // Update C flag from shifter carry out
    if (carry_out) {
        parentCPU.CPSR() |= CPU::FLAG_C;
    } else {
        parentCPU.CPSR() &= ~CPU::FLAG_C;
    }
    
    // V flag is unchanged for logical operations
}

// Exception handling utilities
void ARMCPU::handleException(uint32_t vector_address, uint32_t new_mode, bool disable_irq, bool disable_fiq) {
    uint32_t pc = parentCPU.R()[15];
    uint32_t cpsr = parentCPU.CPSR();
    
    Debug::log::debug("ARM Exception: vector=0x" + Debug::toHexString(vector_address, 8) + 
                     " mode=0x" + Debug::toHexString(new_mode, 2));
    
    // Save current CPSR to SPSR of new mode (simplified - needs proper banking)
    // In a full implementation, we would need proper register banking
    
    // Switch to new mode
    parentCPU.CPSR() = (cpsr & ~0x1F) | new_mode;
    
    // Disable interrupts if requested
    if (disable_irq) {
        parentCPU.CPSR() |= 0x80;
    }
    if (disable_fiq) {
        parentCPU.CPSR() |= 0x40;
    }
    
    // Save return address in LR of new mode
    parentCPU.R()[14] = pc + 4;
    
    // Jump to exception vector
    parentCPU.R()[15] = vector_address;
}

// Processor mode utilities
bool ARMCPU::isPrivilegedMode() {
    uint32_t mode = parentCPU.CPSR() & 0x1F;
    return mode != 0x10; // User mode is 0x10, all others are privileged
}

void ARMCPU::switchToMode(uint32_t new_mode) {
    uint32_t current_mode = parentCPU.CPSR() & 0x1F;
    
    if (current_mode == new_mode) {
        return; // No change needed
    }
    
    Debug::log::debug("ARM Mode Switch: 0x" + Debug::toHexString(current_mode, 2) + 
                     " -> 0x" + Debug::toHexString(new_mode, 2));
    
    // Update mode bits in CPSR
    parentCPU.CPSR() = (parentCPU.CPSR() & ~0x1F) | new_mode;
    
    // Note: In a full implementation, we would need to:
    // 1. Save current mode's banked registers
    // 2. Load new mode's banked registers
    // 3. Handle SPSR switching
}

// Memory management utilities
bool ARMCPU::checkMemoryAccess(uint32_t address, bool is_write, bool is_privileged) {
    UNUSED(is_privileged);
    // Basic memory protection check
    // In a full implementation, this would check:
    // 1. Memory map regions
    // 2. Access permissions
    // 3. Cache and MMU settings
    
    // For GBA, we have specific memory regions with different access rules
    if (address < 0x08000000) {
        // Internal memory regions - always accessible
        return true;
    } else if (address < 0x0E000000) {
        // ROM regions - read-only
        return !is_write;
    } else if (address < 0x10000000) {
        // SRAM region - read/write
        return true;
    } else {
        // Invalid memory region
        Debug::log::error("ARM Memory Access Violation: address=0x" + 
                         Debug::toHexString(address, 8) + " write=" + std::to_string(is_write));
        return false;
    }
}

// Advanced operand calculation with memory access timing
uint32_t ARMCPU::calculateOperand2Advanced(uint32_t instruction, uint32_t* carry_out, uint32_t* cycles) {
    *cycles = 0;
    
    if (ARM_GET_IMMEDIATE_FLAG(instruction)) {
        // Immediate operand - no extra cycles
        return arm_calculate_immediate_operand(instruction, carry_out);
    } else {
        // Register operand with shift
        uint32_t rm = instruction & 0xF;
        uint32_t shift_type = (instruction >> 5) & 3;
        bool shift_by_register = (instruction >> 4) & 1;
        
        if (shift_by_register) {
            // Shifted by register - adds 1 cycle
            *cycles += 1;
            
            uint32_t rs = (instruction >> 8) & 0xF;
            uint32_t shift_amount = parentCPU.R()[rs] & 0xFF;
            
            return arm_apply_shift(parentCPU.R()[rm], shift_type, shift_amount, carry_out);
        } else {
            // Shifted by immediate - no extra cycles
            uint32_t shift_amount = (instruction >> 7) & 0x1F;
            
            return arm_apply_shift(parentCPU.R()[rm], shift_type, shift_amount, carry_out);
        }
    }
}

// Shift operations
uint32_t ARMCPU::arm_apply_shift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, uint32_t* carry_out) {
    *carry_out = (parentCPU.CPSR() >> 29) & 1; // Default to current C flag
    
    if (shift_amount == 0) {
        return value;
    }
    
    switch (shift_type) {
        case 0: // LSL - Logical Shift Left
            if (shift_amount >= 32) {
                *carry_out = (shift_amount == 32) ? (value & 1) : 0;
                return 0;
            } else {
                *carry_out = (value >> (32 - shift_amount)) & 1;
                return value << shift_amount;
            }
            
        case 1: // LSR - Logical Shift Right
            if (shift_amount >= 32) {
                *carry_out = (shift_amount == 32) ? ((value >> 31) & 1) : 0;
                return 0;
            } else {
                *carry_out = (value >> (shift_amount - 1)) & 1;
                return value >> shift_amount;
            }
            
        case 2: // ASR - Arithmetic Shift Right
            if (shift_amount >= 32) {
                if (value & 0x80000000) {
                    *carry_out = 1;
                    return 0xFFFFFFFF;
                } else {
                    *carry_out = 0;
                    return 0;
                }
            } else {
                *carry_out = (value >> (shift_amount - 1)) & 1;
                return ((int32_t)value) >> shift_amount;
            }
            
        case 3: // ROR - Rotate Right / RRX
            if (shift_amount == 0) {
                // RRX - Rotate Right with Extend
                *carry_out = value & 1;
                return (value >> 1) | (((parentCPU.CPSR() >> 29) & 1) << 31);
            } else {
                shift_amount &= 0x1F; // Only lower 5 bits matter
                if (shift_amount == 0) {
                    return value;
                }
                *carry_out = (value >> (shift_amount - 1)) & 1;
                return (value >> shift_amount) | (value << (32 - shift_amount));
            }
            
        default:
            return value;
    }
}

// BX (Branch and Exchange) instruction handler
void ARMCPU::arm_bx(uint32_t instruction) {
    uint32_t rm = instruction & 0xF; // Target register is in bits 3-0
    uint32_t target = parentCPU.R()[rm];
    
    Debug::log::debug("ARM BX R" + std::to_string(rm) + " = 0x" + Debug::toHexString(target, 8));
    
    // Set PC to target address with bit 0 cleared
    parentCPU.R()[15] = target & ~1;
    
    // Update processor mode based on bit 0 of target
    if (target & 1) {
        parentCPU.CPSR() |= CPU::FLAG_T; // Set Thumb mode
        Debug::log::debug("ARM BX: Switching to Thumb mode");
    } else {
        parentCPU.CPSR() &= ~CPU::FLAG_T; // Clear Thumb mode (ARM)
        Debug::log::debug("ARM BX: Staying in ARM mode");
    }
}
