#include <cstdint>
#include <cassert>
#include "arm_cpu.h"
#include "debug.h"
#include "timing.h"
#include "arm_timing.h"
#include "arm_instruction_cache.h"

// Helper for 32-bit rotate right
static inline uint32_t ror32(uint32_t value, unsigned int amount) {
    amount &= 31;
    return (value >> amount) | (value << (32 - amount));
}


// --- Compact 16-entry secondary decode table for ambiguous region ---
// constexpr ARMCPU::arm_secondary_decode_func_t ARMCPU::arm_secondary_decode_table[16] = {
//     // 0x0: AND reg
//     &ARMCPU::decode_arm_and_reg,
//     // 0x1: AND imm
//     &ARMCPU::decode_arm_and_imm,
//     // 0x2: EOR reg
//     &ARMCPU::decode_arm_eor_reg,
//     // 0x3: EOR imm
//     &ARMCPU::decode_arm_eor_imm,
//     // 0x4-0x8: undefined
//     &ARMCPU::decode_arm_undefined, &ARMCPU::decode_arm_undefined,
//     &ARMCPU::decode_arm_undefined, &ARMCPU::decode_arm_undefined,
//     &ARMCPU::decode_arm_undefined,
//     // 0x9: undefined (no longer used)
//     &ARMCPU::decode_arm_undefined,
//     // 0xA: undefined
//     &ARMCPU::decode_arm_undefined,
//     // 0xB: undefined (not MLA)
//     &ARMCPU::decode_arm_undefined,
//     // 0xC-0xF: undefined
//     &ARMCPU::decode_arm_undefined, &ARMCPU::decode_arm_undefined,
//     &ARMCPU::decode_arm_undefined, &ARMCPU::decode_arm_undefined
// };

// Secondary decode function for ambiguous region (data processing/MUL/MLA overlap)
// Phase 1: New entry point for ambiguous region
ARMCPU::ARMCPU(CPU& cpu) : parentCPU(cpu) {
    DEBUG_INFO("Initializing ARMCPU with parent CPU");
}

ARMCPU::~ARMCPU() {
    // Cleanup logic if necessary
}

void ARMCPU::execute(uint32_t cycles) {
    DEBUG_INFO("Executing ARM instructions for " + std::to_string(cycles) + " cycles. Memory size: " + std::to_string(parentCPU.getMemory().getSize()) + " bytes");
        
    exception_taken = false;
    while (cycles > 0) {
        // Check if we're still in ARM mode - if not, break out early
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            DEBUG_INFO("Mode switched to Thumb during execution, breaking out of ARM execution");
            break;
        }

        uint32_t pc = parentCPU.R()[15]; // Get current PC
        uint32_t instruction = parentCPU.getMemory().read32(pc); // Fetch instruction

        DEBUG_INFO("Fetched ARM instruction: " + debug_to_hex_string(instruction, 8) + " at PC: " + debug_to_hex_string(pc, 8));

        // Use cached execution path
        bool pc_modified = executeWithCache(pc, instruction);
        if (exception_taken) {
            break;
        }
        // Only increment PC if instruction didn't modify it (e.g., not a branch)
        if (!pc_modified) {
            parentCPU.R()[15] = pc + 4;
            DEBUG_INFO("Incremented PC to: 0x" + debug_to_hex_string(parentCPU.R()[15], 8));
        }
        // Debug: Print R[0] and R[1] after instruction execution
        DEBUG_INFO("After instruction: R[0]=0x" + debug_to_hex_string(parentCPU.R()[0], 8) + ", R[1]=0x" + debug_to_hex_string(parentCPU.R()[1], 8));
        cycles -= 1; // Placeholder for cycle deduction
    }
}

// New timing-aware execution method
void ARMCPU::executeWithTiming(uint32_t cycles, TimingState* timing) {
    // Use macro-based debug system
    DEBUG_INFO("Executing ARM instructions with timing for " + std::to_string(cycles) + " cycles");
    
    while (cycles > 0) {
        exception_taken = false;
        // Check if we're still in ARM mode - if not, break out early
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            DEBUG_INFO("Mode switched to Thumb during timing execution, breaking out of ARM execution");
            break;
        }
        
        // Calculate cycles until next timing event
        uint32_t cycles_until_event = timing_cycles_until_next_event(timing);
        
        // Fetch next instruction to determine its cycle cost
        uint32_t pc = parentCPU.R()[15];
        uint32_t instruction = parentCPU.getMemory().read32(pc);
        uint32_t instruction_cycles = calculateInstructionCycles(instruction);
        
        // Use debug macros for detailed instruction logging
        DEBUG_INFO("Next ARM instruction: 0x" + debug_to_hex_string(instruction, 8) 
                  + " at PC: 0x" + debug_to_hex_string(pc, 8)
                  + " will take " + std::to_string(instruction_cycles) + " cycles");
        DEBUG_INFO("Cycles until next event: " + std::to_string(cycles_until_event)); 
        
        // Check if instruction will complete before next timing event
        if (instruction_cycles <= cycles_until_event) {
            // Execute instruction normally with cache
            bool pc_modified = executeWithCache(pc, instruction);
            
            if (!pc_modified) {
                parentCPU.R()[15] = pc + 4;
            }
            
            // Update timing
            timing_advance(timing, instruction_cycles);
            cycles -= instruction_cycles;
            
        } else {
            // Process timing event first, then continue
            DEBUG_INFO("Processing timing event before executing instruction");
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

bool ARMCPU::executeWithCache(uint32_t pc, uint32_t instruction) {
    ARMCachedInstruction* cached = instruction_cache.lookup(pc, instruction);
    
    if (cached) {
        if (!checkConditionCached(cached->condition)) return false;
        (this->*(cached->execute_func))(*cached);
        if (exception_taken) return true;
        return cached->pc_modified;
    } else {
        DEBUG_INFO("CACHE MISS: PC=0x" + debug_to_hex_string(pc, 8) + 
                   " Instruction=0x" + debug_to_hex_string(instruction, 8));
        ARMCachedInstruction decoded = decodeInstruction(pc, instruction);
        instruction_cache.insert(pc, decoded);
        DEBUG_INFO("Executing decoded instruction: PC=0x" + debug_to_hex_string(pc, 8) + 
                   " Instruction=0x" + debug_to_hex_string(decoded.instruction, 8));
        if (!checkConditionCached(decoded.condition)) return false;
        (this->*(decoded.execute_func))(decoded);
        DEBUG_LOG("Executed instruction: PC=0x" + debug_to_hex_string(pc, 8) + 
                   " Instruction=0x" + debug_to_hex_string(decoded.instruction, 8));
        if (exception_taken) return true;
        return decoded.pc_modified;
    }
}

// Optimized flag update function for logical operations (AND, EOR, TST, TEQ, etc.)
FORCE_INLINE void ARMCPU::updateFlagsLogical(uint32_t result, uint32_t carry_out) {
    // Get current CPSR value once to reduce memory access
    uint32_t cpsr = parentCPU.CPSR();
    
    // Clear N, Z flags and set them based on result (bits 31 and 30)
    cpsr &= ~(0x80000000 | 0x40000000); // Clear N (bit 31) and Z (bit 30)
    
    // Set N if result is negative (bit 31 set)
    cpsr |= (result & 0x80000000);
    
    // Set Z if result is zero (bit 30)
    if (result == 0) {
        cpsr |= 0x40000000;
    }
    
    // Clear and set carry flag based on carry_out (bit 29)
    cpsr &= ~(1U << 29);
    cpsr |= (carry_out << 29);
    
    // Update CPSR by setting directly
    parentCPU.CPSR() = cpsr;
}

// Optimized general flag update function for arithmetic operations (ADD, SUB, etc.)
FORCE_INLINE void ARMCPU::updateFlags(uint32_t result, bool carry, bool overflow) {
    // Get current CPSR value once to reduce memory access
    uint32_t cpsr = parentCPU.CPSR();
    
    // Clear N, Z, C, V flags in one operation (bits 31, 30, 29, 28)
    cpsr &= ~(0x80000000 | 0x40000000 | 0x20000000 | 0x10000000);
    
    // Set N if result is negative (bit 31 set)
    cpsr |= (result & 0x80000000);
    
    // Set Z if result is zero (bit 30)
    if (result == 0) {
        cpsr |= 0x40000000;
    }
    
    // Set C flag based on carry parameter (bit 29)
    if (carry) {
        cpsr |= 0x20000000;
    }
    
    // Set V flag based on overflow parameter (bit 28)
    if (overflow) {
        cpsr |= 0x10000000;
    }
    
    // Update CPSR
    parentCPU.CPSR() = cpsr;  // Direct assignment instead of setCPSR method
}

// Add simplified exception handling implementation
void ARMCPU::handleException(uint32_t vector_address, uint32_t new_mode, bool disable_irq, bool disable_fiq) {
    DEBUG_INFO("handleException: vector=0x" + debug_to_hex_string(vector_address, 8) + ", new_mode=0x" + debug_to_hex_string(new_mode, 2) + ", PC=0x" + debug_to_hex_string(parentCPU.R()[15], 8));
    assert((new_mode & 0x1F) >= 0x10 && (new_mode & 0x1F) <= 0x1F && "Invalid new_mode in handleException");
    // Save the current CPSR (SPSR not supported in this implementation)
    uint32_t old_cpsr = parentCPU.CPSR();

    // Calculate the return address (PC+4)
    uint32_t return_address = parentCPU.R()[15] + 4;

    // Set mode and swap in correct banked registers first
    CPU::Mode mode_enum = static_cast<CPU::Mode>(new_mode & 0x1F);
    parentCPU.setMode(mode_enum);

    // Set the correct banked LR for the new mode (after swap)
    switch (mode_enum) {
        case CPU::SVC:
            parentCPU.bankedLR(CPU::SVC) = return_address;
            break;
        case CPU::IRQ:
            parentCPU.bankedLR(CPU::IRQ) = return_address;
            break;
        case CPU::FIQ:
            parentCPU.bankedLR(CPU::FIQ) = return_address;
            break;
        case CPU::ABT:
            parentCPU.bankedLR(CPU::ABT) = return_address;
            break;
        case CPU::UND:
            parentCPU.bankedLR(CPU::UND) = return_address;
            break;
        default:
            parentCPU.R()[14] = return_address;
            break;
    }

    // Create new CPSR value
    uint32_t new_cpsr = (old_cpsr & ~0x1F) | (new_mode & 0x1F);
    if (disable_irq) {
        new_cpsr |= 0x80; // Set I bit
    }
    if (disable_fiq) {
        new_cpsr |= 0x40; // Set F bit
    }
    parentCPU.CPSR() = new_cpsr;

    // In a real ARM CPU, we would save old_cpsr to SPSR, but it's not supported here
    DEBUG_INFO("SPSR write not supported, old CPSR value 0x" + 
               debug_to_hex_string(old_cpsr, 8) + " not saved");

    // Set the PC to the exception vector
    parentCPU.R()[15] = vector_address;

    DEBUG_INFO("Handling exception: vector=0x" + 
               debug_to_hex_string(vector_address, 8) + 
               " new mode=0x" + debug_to_hex_string(new_mode, 2) + 
               " disable_irq=" + std::to_string(disable_irq) + 
               " disable_fiq=" + std::to_string(disable_fiq));
}

// Coprocessor functions
void ARMCPU::arm_coprocessor_operation(uint32_t instruction) {
    // In GBA there are no coprocessors, so this should trigger undefined instruction handler
    arm_undefined(instruction);
}

void ARMCPU::arm_coprocessor_transfer(uint32_t instruction) {
    // In GBA there are no coprocessors, so this should trigger undefined instruction handler
    arm_undefined(instruction);
}

void ARMCPU::arm_coprocessor_register(uint32_t instruction) {
    // In GBA there are no coprocessors, so this should trigger undefined instruction handler
    arm_undefined(instruction);
}

void ARMCPU::arm_undefined(uint32_t instruction) {
    DEBUG_INFO("Undefined instruction encountered: 0x" + 
               debug_to_hex_string(instruction, 8));
    
    // Calculate the return address (PC+4)
    uint32_t return_address = parentCPU.R()[15] + 4;

    DEBUG_INFO("Return address for undefined instruction: 0x" + 
               debug_to_hex_string(return_address, 8));

    // Save user LR before exception for test validation
    uint32_t user_lr = parentCPU.R()[14];

    // Switch to undefined instruction mode and handle the exception
    handleException(0x00000004, 0x1B, true, false);
    exception_taken = true;

    // Restore user LR if we return to user mode (for test)
    if ((parentCPU.CPSR() & 0x1F) == 0x10) {
        parentCPU.R()[14] = user_lr;
    }

    // Prevent unused variable warnings in release builds
    UNUSED(instruction);
    UNUSED(return_address);
}

// ARM instruction helper functions
uint32_t ARMCPU::arm_apply_shift(uint32_t value, uint32_t shift_type, uint32_t shift_amount, uint32_t* carry_out) {
    // Use switch statement for shift operations (reverted from function pointer optimization)
    switch (shift_type) {
        case 0: // LSL
            if (shift_amount == 0) {
                *carry_out = (value >> 31) & 1;
                return value;
            } else if (shift_amount < 32) {
                *carry_out = (value >> (32 - shift_amount)) & 1;
                return value << shift_amount;
            } else if (shift_amount == 32) {
                *carry_out = value & 1;
                return 0;
            } else {
                *carry_out = 0;
                return 0;
            }
        case 1: // LSR
            if (shift_amount == 0) {
                // Special case: LSR #0 is interpreted as LSR #32
                *carry_out = (value >> 31) & 1;
                return 0;
            } else if (shift_amount < 32) {
                *carry_out = (value >> (shift_amount - 1)) & 1;
                return value >> shift_amount;
            } else {
                *carry_out = 0;
                return 0;
            }
        case 2: // ASR
            if (shift_amount == 0) {
                // Special case: ASR #0 is interpreted as ASR #32
                if (value & 0x80000000) {
                    *carry_out = 1;
                    return 0xFFFFFFFF;
                } else {
                    *carry_out = 0;
                    return 0;
                }
            } else if (shift_amount < 32) {
                *carry_out = (value >> (shift_amount - 1)) & 1;
                // Use arithmetic shift (sign extension)
                if (value & 0x80000000) {
                    return (value >> shift_amount) | (~0U << (32 - shift_amount));
                } else {
                    return value >> shift_amount;
                }
            } else {
                if (value & 0x80000000) {
                    *carry_out = 1;
                    return 0xFFFFFFFF;
                } else {
                    *carry_out = 0;
                    return 0;
                }
            }
            
        case 3: // ROR
            if (shift_amount == 0) {
                // Special case: ROR #0 is interpreted as RRX (rotate right with extend)
                uint32_t old_carry = (parentCPU.CPSR() >> 29) & 1;
                *carry_out = value & 1;
                return (value >> 1) | (old_carry << 31);
            } else {
                shift_amount %= 32; // Normalize rotation amount
                if (shift_amount == 0) {
                    // No rotation after normalization
                    return value;
                    // Carry unchanged
                } else {
                    *carry_out = (value >> (shift_amount - 1)) & 1;
                    return ror32(value, shift_amount);
                }
            }
    }
    
    return value; // Should never reach here
}

// Cached condition checking for performance
FORCE_INLINE bool ARMCPU::checkConditionCached(uint8_t condition) {
    uint32_t flags = parentCPU.CPSR() >> 28;
    switch (condition) {
        case 0x0: return (flags & 0x4) != 0;           // EQ: Z=1
        case 0x1: return (flags & 0x4) == 0;           // NE: Z=0
        case 0x2: return (flags & 0x2) != 0;           // CS: C=1
        case 0x3: return (flags & 0x2) == 0;           // CC: C=0
        case 0x4: return (flags & 0x8) != 0;           // MI: N=1
        case 0x5: return (flags & 0x8) == 0;           // PL: N=0
        case 0x6: return (flags & 0x1) != 0;           // VS: V=1
        case 0x7: return (flags & 0x1) == 0;           // VC: V=0
        case 0x8: return (flags & 0x2) && !(flags & 0x4); // HI: C=1 && Z=0
        case 0x9: return !(flags & 0x2) || (flags & 0x4); // LS: C=0 || Z=1
        case 0xA: return !((flags & 0x8) >> 3) == !((flags & 0x1)); // GE: N==V
        case 0xB: return !((flags & 0x8) >> 3) != !((flags & 0x1)); // LT: N!=V
        case 0xC: return !(flags & 0x4) && (!((flags & 0x8) >> 3) == !((flags & 0x1))); // GT: Z=0 && N==V
        case 0xD: return (flags & 0x4) || (!((flags & 0x8) >> 3) != !((flags & 0x1))); // LE: Z=1 || N!=V
        case 0xE: return true;                         // AL: Always
        case 0xF: return false;                        // NV: Never (undefined in ARMv4)
        default: return false;
    }
}

// Main instruction decoder
ARMCachedInstruction ARMCPU::decodeInstruction(uint32_t pc, uint32_t instruction) {
    DEBUG_LOG(std::string("[decodeInstruction] instruction: 0x") + debug_to_hex_string(instruction, 8));
    DEBUG_LOG(std::string("[decodeInstruction] bits<27,19>: ") + std::to_string(bits<27,19>(instruction)));
    UNUSED(pc); // PC is not used in this decode function, but could be useful for debugging
    ARMCachedInstruction decoded;

    // Add in common decodes ..
    decoded.instruction = instruction;
    
    DEBUG_LOG(std::string("[decodeInstruction] instruction: 0x") + debug_to_hex_string(decoded.instruction, 8));
    DEBUG_LOG(std::string("[decodeInstruction] bits<27,19>: ") + std::to_string(bits<27,19>(0xE0810002)));
    

    decoded.condition = bits<31, 28>(instruction);
    decoded.set_flags = bits<20, 20>(instruction);
    decoded.valid = true;
    decoded.pc_modified = (decoded.rd == 15);

    // Print the function name from the decode table for debug
    uint32_t index = bits<27,19>(instruction);

    // .. and call the decode handler based on the instruction type to handle the rest
    auto decode_func = arm_decode_table[index];
    if (decode_func == nullptr) {
        DEBUG_ERROR("[FATAL] arm_decode_table entry is nullptr for instruction 0x" + debug_to_hex_string(instruction, 8));
        // Optionally, set decoded.valid = false or handle error as needed
        return decoded;
    }
    (this->*decode_func)(decoded);
    return decoded;
}

FORCE_INLINE uint32_t ARMCPU::execOperand2imm(uint32_t imm, uint8_t rotate, uint32_t* carry_out) {
    if (rotate == 0) {
        *carry_out = (parentCPU.CPSR() >> 29) & 1;
        return imm;
    }
    if (rotate > 0) {
        uint32_t result = ror32(imm, rotate);
        *carry_out = (result >> 31) & 1;
        return result;
    }
    return imm;
}

FORCE_INLINE uint32_t ARMCPU::execOperand2reg(uint8_t rm, uint8_t rs, uint8_t shift_type, 
    bool reg_shift, uint32_t* carry_out) {
    
    uint32_t shift_amount = 0;
    if (rs == 0 && shift_type == 0) { // LSL #0
        *carry_out = (parentCPU.CPSR() >> 29) & 1;
        return rm;
    }
    if (reg_shift) {
        shift_amount = parentCPU.R()[rs] & 0xFF;
        if (shift_amount == 0) {
            *carry_out = (parentCPU.CPSR() >> 29) & 1;
            return rm;
        }
    } else {
        shift_amount = rs;
        if (shift_amount == 0) {
            // (shift_type == 0) handled above
            if (shift_type == 1) { // LSR #0 means LSR #32
                shift_amount = 32;
            } else if (shift_type == 2) { // ASR #0 means ASR #32
                shift_amount = 32;
            } else if (shift_type == 3) {
                uint32_t old_carry = (parentCPU.CPSR() >> 29) & 1;
                *carry_out = rm & 1;
                return (old_carry << 31) | (rm >> 1);
            }
        }
    }

    switch (shift_type) {
        case 0: // LSL
            if (shift_amount >= 32) {
                *carry_out = (shift_amount == 32) ? (rm & 1) : 0;
                return 0;
            }
            *carry_out = (rm >> (32 - shift_amount)) & 1;
            return rm << shift_amount;
        case 1: // LSR
            if (shift_amount == 32) {
                *carry_out = (rm >> 31) & 1;
                return 0;
            } else if (shift_amount > 32) {
                *carry_out = 0;
                return 0;
            }
            *carry_out = (rm >> (shift_amount - 1)) & 1;
            return rm >> shift_amount;
        case 2: // ASR
            if (shift_amount >= 32) {
                *carry_out = (rm >> 31) & 1;
                return (rm & 0x80000000) ? 0xFFFFFFFF : 0;
            }
            *carry_out = (rm >> (shift_amount - 1)) & 1;
            return static_cast<int32_t>(rm) >> shift_amount;
        case 3: // ROR
            if (shift_amount == 0) {
                *carry_out = (parentCPU.CPSR() >> 29) & 1;
                return rm;
            }
            shift_amount %= 32;
            *carry_out = (rm >> (shift_amount - 1)) & 1;
            return ror32(rm, shift_amount);
    }
    return rm;
}

// New execution functions for cached instructions
void ARMCPU::execute_arm_and_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_and_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] & result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_eor_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_eor_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] ^ result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_sub_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_sub_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] - result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = parentCPU.R()[cached.rn] >= result;
        bool v = ((parentCPU.R()[cached.rn] ^ result) & (parentCPU.R()[cached.rn] ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_rsb_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_rsb_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = result - parentCPU.R()[cached.rn];
    if (cached.set_flags && cached.rd != 15) {
        bool c = result >= parentCPU.R()[cached.rn];
        bool v = ((result ^ parentCPU.R()[cached.rn]) & (result ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_add_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_add_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] + result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = parentCPU.R()[cached.rd] < parentCPU.R()[cached.rn];
        bool v = ((parentCPU.R()[cached.rn] ^ result) & (parentCPU.R()[cached.rn] ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_adc_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_adc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    uint32_t op1 = parentCPU.R()[cached.rn] + carry_out; // Include carry from CPSR
    parentCPU.R()[cached.rd] = op1 + result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = (op1 + result) < op1; // Carry out if overflow
        bool v = ((op1 ^ result) & (op1 ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_sbc_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_sbc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    uint32_t op1 = parentCPU.R()[cached.rn] - carry_out; // Include carry from CPSR
    parentCPU.R()[cached.rd] = op1 - result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = op1 >= result; // Carry out if no borrow
        bool v = ((op1 ^ result) & (op1 ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_rsc_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_rsc_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = result - parentCPU.R()[cached.rn];
    if (cached.set_flags && cached.rd != 15) {
        bool c = result >= parentCPU.R()[cached.rn];
        bool v = ((result ^ parentCPU.R()[cached.rn]) & (result ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_tst_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_tst_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 & result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(res, carry_out);
    }
}

void ARMCPU::execute_arm_teq_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_teq_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 ^ result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(res, carry_out);
    }
}

void ARMCPU::execute_arm_cmp_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_cmp_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 - result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = op1 >= result; // Carry out if no borrow
        bool v = ((op1 ^ result) & (op1 ^ res) & 0x80000000) != 0;
        updateFlags(res, c, v);
    }
}

void ARMCPU::execute_arm_cmn_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_cmn_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 + result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = (op1 + result) < op1; // Carry out if overflow
        bool v = ((op1 ^ result) & (op1 ^ res) & 0x80000000) != 0;
        updateFlags(res, c, v);
    }
}

void ARMCPU::execute_arm_orr_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_orr_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] | result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_bic_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_bic_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] & ~result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_mvn_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_mvn_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    parentCPU.R()[cached.rd] = ~result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_mov_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_mov_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2imm(cached.imm, cached.rotate, &carry_out);

    if (cached.set_flags && cached.rd != 15) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N, Z
        if (result == 0) cpsr |= 0x40000000;   // Set Z if zero
        if (carry_out) cpsr |= 0x20000000;     // Set C from rotation
        parentCPU.CPSR() = cpsr;
    }
    parentCPU.R()[cached.rd] = result;
}

void ARMCPU::execute_arm_and_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_and_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] & result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(result, carry_out);
    }
}

void ARMCPU::execute_arm_eor_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_eor_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] ^ result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_sub_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_sub_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] - result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = parentCPU.R()[cached.rn] >= result;
        bool v = ((parentCPU.R()[cached.rn] ^ result) & (parentCPU.R()[cached.rn] ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_rsb_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_rsb_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = result - parentCPU.R()[cached.rn];
    if (cached.set_flags && cached.rd != 15) {
        bool c = result >= parentCPU.R()[cached.rn];
        bool v = ((result ^ parentCPU.R()[cached.rn]) & (result ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_add_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_add_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];
        
    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] + result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = parentCPU.R()[cached.rd] < parentCPU.R()[cached.rn];
        bool v = ((parentCPU.R()[cached.rn] ^ result) & (parentCPU.R()[cached.rn] ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_adc_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_adc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    uint32_t op1 = parentCPU.R()[cached.rn] + carry_out; // Include carry from CPSR
    parentCPU.R()[cached.rd] = op1 + result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = (op1 + result) < op1; // Carry out if overflow
        bool v = ((op1 ^ result) & (op1 ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_sbc_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_sbc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    uint32_t op1 = parentCPU.R()[cached.rn] - carry_out; // Include carry from CPSR
    parentCPU.R()[cached.rd] = op1 - result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = op1 >= result; // Carry out if no borrow
        bool v = ((op1 ^ result) & (op1 ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_rsc_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_rsc_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = result - parentCPU.R()[cached.rn];
    if (cached.set_flags && cached.rd != 15) {
        bool c = result >= parentCPU.R()[cached.rn];
        bool v = ((result ^ parentCPU.R()[cached.rn]) & (result ^ parentCPU.R()[cached.rd]) & 0x80000000) != 0;
        updateFlags(parentCPU.R()[cached.rd], c, v);
    }
}

void ARMCPU::execute_arm_tst_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_tst_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 & result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(res, carry_out);
    }
}

void ARMCPU::execute_arm_teq_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_teq_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 ^ result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(res, carry_out);
    }
}

void ARMCPU::execute_arm_cmp_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_cmp_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 - result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = op1 >= result; // Carry out if no borrow
        bool v = ((op1 ^ result) & (op1 ^ res) & 0x80000000) != 0;
        updateFlags(res, c, v);
    }
}

void ARMCPU::execute_arm_cmn_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_cmn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    uint32_t op1 = parentCPU.R()[cached.rn];
    uint32_t res = op1 + result;
    if (cached.set_flags && cached.rd != 15) {
        bool c = (op1 + result) < op1; // Carry out if overflow
        bool v = ((op1 ^ result) & (op1 ^ res) & 0x80000000) != 0;
        updateFlags(res, c, v);
    }
}

void ARMCPU::execute_arm_orr_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_orr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] | result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_bic_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_bic_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] & ~result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_mvn_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_mvn_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    parentCPU.R()[cached.rd] = ~result;
    if (cached.set_flags && cached.rd != 15) {
        updateFlagsLogical(parentCPU.R()[cached.rd], carry_out);
    }
}

void ARMCPU::execute_arm_mov_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_mov_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    uint32_t carry_out = 0;
    uint32_t result = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, &carry_out);
    result = parentCPU.R()[result];

    if (cached.set_flags && cached.rd != 15) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N, Z
        if (result == 0) cpsr |= 0x40000000;   // Set Z if zero
        if (carry_out) cpsr |= 0x20000000;     // Set C from rotation
        parentCPU.CPSR() = cpsr;
    }
    parentCPU.R()[cached.rd] = result;
}

void ARMCPU::execute_arm_str_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_str_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: STR (immediate offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = cached.imm;
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    parentCPU.getMemory().write32(address, parentCPU.R()[cached.rd]);
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.getMemory().write32(address, parentCPU.R()[cached.rd]);
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
    // PC-relative STR is not meaningful, so no special handling needed
    // Unaligned access: ARMv4 allows, but result is rotated; not implemented here
}

void ARMCPU::execute_arm_str_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_str_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: STR (register offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, nullptr);
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    parentCPU.getMemory().write32(address, parentCPU.R()[cached.rd]);
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.getMemory().write32(address, parentCPU.R()[cached.rd]);
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
}

void ARMCPU::execute_arm_ldr_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_ldr_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: LDR (immediate offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = cached.imm;
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    // PC-relative load (literal pool)
    if (cached.rn == 15) {
        address = (parentCPU.R()[15] & ~3) + offset;
    }
    parentCPU.R()[cached.rd] = parentCPU.getMemory().read32(address);
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.R()[cached.rd] = parentCPU.getMemory().read32(address);
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
    // Unaligned access: ARMv4 allows, but result is rotated; not implemented here
}

void ARMCPU::execute_arm_ldr_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_ldr_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: LDR (register offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, nullptr);
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    parentCPU.R()[cached.rd] = parentCPU.getMemory().read32(address);
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.R()[cached.rd] = parentCPU.getMemory().read32(address);
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
}

void ARMCPU::execute_arm_strb_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_strb_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: STRB (immediate offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = cached.imm;
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    parentCPU.getMemory().write8(address, static_cast<uint8_t>(parentCPU.R()[cached.rd]));
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.getMemory().write8(address, static_cast<uint8_t>(parentCPU.R()[cached.rd]));
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
}

void ARMCPU::execute_arm_strb_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_strb_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: STRB (register offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, nullptr);
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    parentCPU.getMemory().write8(address, static_cast<uint8_t>(parentCPU.R()[cached.rd]));
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.getMemory().write8(address, static_cast<uint8_t>(parentCPU.R()[cached.rd]));
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
}

void ARMCPU::execute_arm_ldrb_imm(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_ldr_b_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: LDRB (immediate offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = cached.imm;
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    parentCPU.R()[cached.rd] = static_cast<uint32_t>(parentCPU.getMemory().read8(address));
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.R()[cached.rd] = static_cast<uint32_t>(parentCPU.getMemory().read8(address));
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
}

void ARMCPU::execute_arm_ldrb_reg(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_ldr_b_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // ARM single data transfer: LDRB (register offset, all addressing modes)
    uint32_t base = parentCPU.R()[cached.rn];
    uint32_t offset = execOperand2reg(cached.rm, cached.rs, cached.shift_type, cached.reg_shift, nullptr);
    bool pre = cached.pre_index;
    bool up = cached.up;
    bool writeback = cached.writeback;
    uint32_t address = base;
    if (pre) {
        address = up ? (base + offset) : (base - offset);
    }
    parentCPU.R()[cached.rd] = static_cast<uint32_t>(parentCPU.getMemory().read8(address));
    if (!pre) {
        address = up ? (base + offset) : (base - offset);
        parentCPU.R()[cached.rd] = static_cast<uint32_t>(parentCPU.getMemory().read8(address));
    }
    if (writeback) {
        parentCPU.R()[cached.rn] = address;
    }
}

void ARMCPU::execute_arm_stm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("execute_arm_stm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    
    // Store multiple registers to memory
    uint32_t address = parentCPU.R()[decoded.rn];
    for (int i = 0; i < 16; ++i) {
        if (decoded.register_list & (1 << i)) {
            parentCPU.getMemory().write32(address, parentCPU.R()[i]);
            address += 4;
        }
    }
}

void ARMCPU::execute_arm_ldm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("execute_arm_ldm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    
    // Load multiple registers from memory
    uint32_t address = parentCPU.R()[decoded.rn];
    for (int i = 0; i < 16; ++i) {
        if (decoded.register_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(address);
            address += 4;
        }
    }
}

void ARMCPU::execute_arm_b(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_b: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Branch to the target address
    uint32_t offset = cached.offset_value << 2; // Shift left by 2 for word address
    parentCPU.R()[15] += offset;
}

void ARMCPU::execute_arm_bl(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_bl: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Branch with link to the target address
    uint32_t offset = cached.offset_value << 2; // Shift left by 2 for word address
    parentCPU.R()[14] = parentCPU.R()[15]; // Save return address in LR
    parentCPU.R()[15] += offset;
}

void ARMCPU::execute_arm_mul(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_mul: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Multiply two registers and store the result
    uint32_t op1 = parentCPU.R()[cached.rm];
    uint32_t op2 = parentCPU.R()[cached.rs];
    parentCPU.R()[cached.rd] = op1 * op2;
    
    if (cached.set_flags && cached.rd != 15) {
        // Update flags based on the result
        updateFlagsLogical(parentCPU.R()[cached.rd], 0); // No carry for multiplication
    }
}

void ARMCPU::execute_arm_mla(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_mla: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Multiply two registers and add to another register
    uint32_t op1 = parentCPU.R()[cached.rm];
    uint32_t op2 = parentCPU.R()[cached.rs];
    parentCPU.R()[cached.rd] = parentCPU.R()[cached.rn] + (op1 * op2);
    
    if (cached.set_flags && cached.rd != 15) {
        // Update flags based on the result
        updateFlagsLogical(parentCPU.R()[cached.rd], 0); // No carry for multiplication
    }
}

void ARMCPU::execute_arm_umull(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_umull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Unsigned multiply long - result in two arbitrary registers
    uint32_t rm = cached.rm;
    uint32_t rs = cached.rs;
    uint32_t rdLo = cached.rdLo;
    uint32_t rdHi = cached.rdHi;
    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
    if (cached.set_flags) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N and Z
        if (parentCPU.R()[rdHi] & 0x80000000) cpsr |= 0x80000000; // N
        if (parentCPU.R()[rdHi] == 0 && parentCPU.R()[rdLo] == 0) cpsr |= 0x40000000; // Z
        parentCPU.CPSR() = cpsr;
    }
}

void ARMCPU::execute_arm_umlal(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_umlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Unsigned multiply long accumulate - result in two arbitrary registers
    uint32_t rm = cached.rm;
    uint32_t rs = cached.rs;
    uint32_t rdLo = cached.rdLo;
    uint32_t rdHi = cached.rdHi;
    uint64_t acc = ((uint64_t)parentCPU.R()[rdHi] << 32) | parentCPU.R()[rdLo];
    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs] + acc;
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
    if (cached.set_flags) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N and Z
        if (parentCPU.R()[rdHi] & 0x80000000) cpsr |= 0x80000000; // N
        if (parentCPU.R()[rdHi] == 0 && parentCPU.R()[rdLo] == 0) cpsr |= 0x40000000; // Z
        parentCPU.CPSR() = cpsr;
    }
}

void ARMCPU::execute_arm_smull(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_smull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Signed multiply long - result in two arbitrary registers
    uint32_t rm = cached.rm;
    uint32_t rs = cached.rs;
    uint32_t rdLo = cached.rdLo;
    uint32_t rdHi = cached.rdHi;
    int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
    if (cached.set_flags) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N and Z
        if (parentCPU.R()[rdHi] & 0x80000000) cpsr |= 0x80000000; // N
        if (parentCPU.R()[rdHi] == 0 && parentCPU.R()[rdLo] == 0) cpsr |= 0x40000000; // Z
        parentCPU.CPSR() = cpsr;
    }
}

void ARMCPU::execute_arm_smlal(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_smlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Signed multiply long accumulate - result in two arbitrary registers
    uint32_t rm = cached.rm;
    uint32_t rs = cached.rs;
    uint32_t rdLo = cached.rdLo;
    uint32_t rdHi = cached.rdHi;
    int64_t acc = ((int64_t)((uint64_t)parentCPU.R()[rdHi] << 32) | parentCPU.R()[rdLo]);
    int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs] + acc;
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
    if (cached.set_flags) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N and Z
        if (parentCPU.R()[rdHi] & 0x80000000) cpsr |= 0x80000000; // N
        if (parentCPU.R()[rdHi] == 0 && parentCPU.R()[rdLo] == 0) cpsr |= 0x40000000; // Z
        parentCPU.CPSR() = cpsr;
    }
}

void ARMCPU::execute_arm_ldrh(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_ldrsh_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Load the halfword from memory and sign-extend it
    uint32_t address = parentCPU.R()[cached.rn] + cached.imm;
    int16_t value = static_cast<int16_t>(parentCPU.getMemory().read16(address));
    parentCPU.R()[cached.rd] = static_cast<uint32_t>(value);
}

void ARMCPU::execute_arm_strh(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_strh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Store the halfword in the register to memory
    uint32_t address = parentCPU.R()[cached.rn] + cached.imm;
    parentCPU.getMemory().write16(address, static_cast<uint16_t>(parentCPU.R()[cached.rd]));
}

void ARMCPU::execute_arm_ldrsb(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_ldrsb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Load the byte from memory and sign-extend it
    uint32_t address = parentCPU.R()[cached.rn] + cached.imm;
    int8_t value = static_cast<int8_t>(parentCPU.getMemory().read8(address));
    parentCPU.R()[cached.rd] = static_cast<uint32_t>(value);
}

void ARMCPU::execute_arm_ldrsh(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_ldrsh: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Load the halfword from memory and sign-extend it
    uint32_t address = parentCPU.R()[cached.rn] + cached.imm;
    int16_t value = static_cast<int16_t>(parentCPU.getMemory().read16(address));
    parentCPU.R()[cached.rd] = static_cast<uint32_t>(value);
}

void ARMCPU::execute_arm_swp(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_swp: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Swap the value in the register with the memory location
    uint32_t address = parentCPU.R()[cached.rn];
    uint32_t old_value = parentCPU.getMemory().read32(address);
    parentCPU.getMemory().write32(address, parentCPU.R()[cached.rd]);
    parentCPU.R()[cached.rd] = old_value; // Store old value back in rd
}

void ARMCPU::execute_arm_swpb(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_swpb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Swap the byte in the register with the memory location
    uint32_t address = parentCPU.R()[cached.rn];
    uint8_t old_value = parentCPU.getMemory().read8(address);
    parentCPU.getMemory().write8(address, static_cast<uint8_t>(parentCPU.R()[cached.rd]));
    parentCPU.R()[cached.rd] = static_cast<uint32_t>(old_value); // Store old value back in rd
}   

void ARMCPU::execute_arm_undefined(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_illegal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    
    // Handle illegal instruction
    // parentCPU.setException(ARMException::IllegalInstruction);
}

void ARMCPU::execute_arm_bx(ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("execute_arm_bx: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use cached.rm for BX
    uint32_t target_address = parentCPU.R()[cached.rm];
    bool switch_to_thumb = (target_address & 1) != 0;
    target_address &= ~1U; // Clear the LSB for ARM mode
    if (switch_to_thumb) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr |= 0x00000020; // Set T flag (bit 5)
        parentCPU.CPSR() = cpsr;
    }
    parentCPU.R()[15] = target_address;
}
