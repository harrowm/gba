
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

ARMCPU::ARMCPU(CPU& cpu) : parentCPU(cpu) {
    DEBUG_INFO("Initializing ARMCPU with parent CPU");
}

ARMCPU::~ARMCPU() {
    // Cleanup logic if necessary
}

void ARMCPU::execute(uint32_t cycles) {
    // Use lazy evaluation for debug logs
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

        // Use lazy evaluation for instruction fetch debug logs
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

// Add an optimized inline version of calculateOperand2
FORCE_INLINE uint32_t ARMCPU::calculateOperand2(uint32_t instruction, uint32_t* carry_out) {
    // Fast path for common case: immediate operand with minimal rotation
    if (instruction & 0x02000000) {
        // Immediate operand
        uint32_t imm = instruction & 0xFF;
        uint32_t rotate_field = (instruction >> 8) & 0xF;
        uint32_t rotate = rotate_field * 2;
        if (rotate == 0) {
            *carry_out = (parentCPU.CPSR() >> 29) & 1;
            return imm;
        }
        if (rotate > 0) {
            uint32_t result = (imm >> rotate) | (imm << (32 - rotate));
            *carry_out = (result >> 31) & 1;
            return result;
        }
        return imm;
    } else {
        uint32_t rm = parentCPU.R()[instruction & 0xF];
        uint32_t shift_type = (instruction >> 5) & 3;
        uint32_t shift_amount = 0;
        if (!(instruction & 0xFF0)) {
            *carry_out = (parentCPU.CPSR() >> 29) & 1;
            return rm;
        }
        if (instruction & 0x10) {
            uint32_t rs = (instruction >> 8) & 0xF;
            shift_amount = parentCPU.R()[rs] & 0xFF;
            if (shift_amount == 0) {
                *carry_out = (parentCPU.CPSR() >> 29) & 1;
                return rm;
            }
        } else {
            shift_amount = (instruction >> 7) & 0x1F;
            if (shift_amount == 0) {
                if (shift_type == 0) {
                    *carry_out = (parentCPU.CPSR() >> 29) & 1;
                    return rm;
                } else if (shift_type == 1) { // LSR #0 means LSR #32
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
        // Debug print for shifter operand
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
                return (rm >> shift_amount) | (rm << (32 - shift_amount));
        }
        return rm;
    }
}

bool ARMCPU::executeWithCache(uint32_t pc, uint32_t instruction) {
    ARMCachedInstruction* cached = instruction_cache.lookup(pc, instruction);
    
    if (cached) {
        if (!checkConditionCached(cached->condition)) return false;
        executeCachedInstruction(*cached);
        if (exception_taken) return true; // treat as PC modified
        return cached->pc_modified;
    } else {
        DEBUG_INFO("CACHE MISS: PC=0x" + debug_to_hex_string(pc, 8) + 
                   " Instruction=0x" + debug_to_hex_string(instruction, 8));
        ARMCachedInstruction decoded = decodeInstruction(pc, instruction);
        instruction_cache.insert(pc, decoded);
        if (!checkConditionCached(decoded.condition)) return false;
        DEBUG_INFO("Executing decoded instruction: PC=0x" + debug_to_hex_string(pc, 8) + 
                   " Instruction=0x" + debug_to_hex_string(decoded.instruction, 8));
        executeCachedInstruction(decoded);
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

// Special optimized implementations for the most common operations
void ARMCPU::arm_and(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] & operand2;
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        updateFlagsLogical(result, carry_out);
    }
}

void ARMCPU::arm_eor(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] ^ operand2;
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        updateFlagsLogical(result, carry_out);
    }
}

void ARMCPU::arm_adc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t carry_in = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = op1 + operand2 + carry_in;
    
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        // Check for carry and overflow
        bool carry = (result < op1) || (result == op1 && carry_in);
        bool overflow = ((op1 ^ result) & (operand2 ^ result) & 0x80000000) != 0;
        
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_sbc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t carry_in = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = op1 - operand2 - (1 - carry_in);
    
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        // For SBC, carry is set if no borrow required (op1 >= operand2 + !carry_in)
        bool carry = carry_in ? (op1 >= operand2) : (op1 > operand2);
        bool overflow = ((op1 ^ operand2) & (op1 ^ result) & 0x80000000) != 0;
        
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_rsc(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t carry_in = (parentCPU.CPSR() >> 29) & 1;
    uint32_t result = operand2 - op1 - (1 - carry_in);
    
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        // For RSC, carry is set if no borrow required (operand2 >= op1 + !carry_in)
        bool carry = carry_in ? (operand2 >= op1) : (operand2 > op1);
        bool overflow = ((operand2 ^ op1) & (operand2 ^ result) & 0x80000000) != 0;
        
        updateFlags(result, carry, overflow);
    }
}

void ARMCPU::arm_tst(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(rd);     // TST doesn't write to a register
    UNUSED(set_flags); // TST always updates flags
    
    uint32_t result = parentCPU.R()[rn] & operand2;
    updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_teq(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(rd);     // TEQ doesn't write to a register
    UNUSED(set_flags); // TEQ always updates flags
    
    uint32_t result = parentCPU.R()[rn] ^ operand2;
    updateFlagsLogical(result, carry_out);
}

void ARMCPU::arm_cmn(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(rd);     // CMN doesn't write to a register
    UNUSED(set_flags); // CMN always updates flags
    UNUSED(carry_out);
    
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t result = op1 + operand2;
    
    // Check for carry and overflow
    bool carry = result < op1;
    bool overflow = ((op1 ^ result) & (operand2 ^ result) & 0x80000000) != 0;
    
    updateFlags(result, carry, overflow);
}

void ARMCPU::arm_orr(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] | operand2;
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        updateFlagsLogical(result, carry_out);
    }
}

void ARMCPU::arm_bic(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    uint32_t result = parentCPU.R()[rn] & ~operand2;
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        updateFlagsLogical(result, carry_out);
    }
}

void ARMCPU::arm_mvn(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(rn); // MVN doesn't use Rn
    
    uint32_t result = ~operand2;
    parentCPU.R()[rd] = result;
    
    if (set_flags) {
        updateFlagsLogical(result, carry_out);
    }
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
                    return (value >> shift_amount) | (value << (32 - shift_amount));
                }
            }
    }
    
    return value; // Should never reach here
}

// Implementation of arm_mov with FORCE_INLINE
FORCE_INLINE void ARMCPU::arm_mov(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(rn); // MOV doesn't use Rn
    parentCPU.R()[rd] = operand2;
    if (set_flags && rd != 15) { // PC updates need special handling
        updateFlagsLogical(operand2, carry_out);
    }
}

// Implementation of arm_sub with FORCE_INLINE
FORCE_INLINE void ARMCPU::arm_sub(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t result = op1 - operand2;
    parentCPU.R()[rd] = result;
    
    if (set_flags && rd != 15) { // PC updates need special handling
        bool carry = op1 >= operand2; // Carry out is set if no borrow required
        bool overflow = ((op1 ^ operand2) & (op1 ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

// Implementation of arm_add with FORCE_INLINE
FORCE_INLINE void ARMCPU::arm_add(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t result = op1 + operand2;
    parentCPU.R()[rd] = result;
    
    if (set_flags && rd != 15) { // PC updates need special handling
        bool carry = result < op1; // If result < op1, then there was an overflow, meaning carry
        bool overflow = ((op1 ^ result) & (operand2 ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

// Implementation of arm_cmp with FORCE_INLINE
FORCE_INLINE void ARMCPU::arm_cmp(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(rd); // CMP doesn't write to a register
    UNUSED(set_flags); // CMP always updates flags
    UNUSED(carry_out);
    
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t result = op1 - operand2;
    
    // CMP always sets flags
    bool carry = op1 >= operand2; // Carry out is set if no borrow required
    bool overflow = ((op1 ^ operand2) & (op1 ^ result) & 0x80000000) != 0;
    updateFlags(result, carry, overflow);
}

// Implementation of arm_rsb with FORCE_INLINE
FORCE_INLINE void ARMCPU::arm_rsb(uint32_t rd, uint32_t rn, uint32_t operand2, bool set_flags, uint32_t carry_out) {
    UNUSED(carry_out);
    uint32_t op1 = parentCPU.R()[rn];
    uint32_t result = operand2 - op1;
    parentCPU.R()[rd] = result;
    
    if (set_flags && rd != 15) { // PC updates need special handling
        bool carry = operand2 >= op1;
        bool overflow = ((operand2 ^ op1) & (operand2 ^ result) & 0x80000000) != 0;
        updateFlags(result, carry, overflow);
    }
}

// Fast-path ALU operations for function pointer dispatch optimization
FORCE_INLINE void ARMCPU::fastALU_ADD(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry) {
    UNUSED(rn);
    UNUSED(carry);
    uint32_t result = op1 + rm;
    parentCPU.R()[rd] = result;
    if (set_flags && rd != 15) {
        bool c = result < op1;
        bool v = ((op1 ^ result) & (rm ^ result) & 0x80000000) != 0;
        updateFlags(result, c, v);
    }
}

FORCE_INLINE void ARMCPU::fastALU_SUB(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry) {
    UNUSED(rn);
    UNUSED(carry);
    uint32_t result = op1 - rm;
    parentCPU.R()[rd] = result;
    if (set_flags && rd != 15) {
        bool c = op1 >= rm;
        bool v = ((op1 ^ rm) & (op1 ^ result) & 0x80000000) != 0;
        updateFlags(result, c, v);
    }
}

FORCE_INLINE void ARMCPU::fastALU_MOV(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry) {
    UNUSED(rn);
    UNUSED(op1);
    parentCPU.R()[rd] = rm;
    if (set_flags && rd != 15) {
        updateFlagsLogical(rm, carry);
    }
}

FORCE_INLINE void ARMCPU::fastALU_ORR(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry) {
    UNUSED(rn);
    UNUSED(carry);
    uint32_t result = op1 | rm;
    parentCPU.R()[rd] = result;
    if (set_flags && rd != 15) {
        updateFlagsLogical(result, carry);
    }
}

FORCE_INLINE void ARMCPU::fastALU_AND(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry) {
    UNUSED(rn);
    UNUSED(carry);
    uint32_t result = op1 & rm;
    parentCPU.R()[rd] = result;
    if (set_flags && rd != 15) {
        updateFlagsLogical(result, carry);
    }
}

FORCE_INLINE void ARMCPU::fastALU_CMP(uint32_t rd, uint32_t rn, uint32_t op1, uint32_t rm, bool set_flags, uint32_t carry) {
    UNUSED(rd);
    UNUSED(rn);
    UNUSED(set_flags);
    UNUSED(carry);
    uint32_t result = op1 - rm;
    bool c = op1 >= rm;
    bool v = ((op1 ^ rm) & (op1 ^ result) & 0x80000000) != 0;
    updateFlags(result, c, v);
}

// ==================== ARM INSTRUCTION CACHE IMPLEMENTATION ====================

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

// Execute cached instruction using function pointer dispatch (optimized)
void ARMCPU::executeCachedInstruction(const ARMCachedInstruction& cached) {
    // Always handle undefined instructions via exception handler
    if (cached.type == ARMInstructionType::UNDEFINED) {
        arm_undefined(cached.instruction);
        return;
    }
    // Use function pointer for direct dispatch - eliminates switch overhead
    if (cached.execute_func) {
        (this->*cached.execute_func)(cached);
    } else {
        // Fallback to original instruction execution for undefined instructions
        arm_undefined(cached.instruction);
    }
}

// Main instruction decoder
ARMCachedInstruction ARMCPU::decodeInstruction(uint32_t pc, uint32_t instruction) {
    // DEBUG: Log instruction and mask for MSR immediate diagnosis
    ARMCachedInstruction decoded;
    decoded.instruction = instruction;
    decoded.condition = (instruction >> 28) & 0xF;
    decoded.valid = true;

    // Add in common decodes here .. instruction handlers can add to these
    decoded.set_flags = (instruction & 0x00100000) != 0;
    decoded.pc_modified = (decoded.rd == 15);
    


    // Determine instruction format
    uint32_t format = ARM_GET_FORMAT(instruction);

    // --- New decode table migration: MOV example ---
    // MOV_REG opcode  index 0x068-0x06B in the 9-bit table
    uint32_t decode_index = (instruction >> 19) & 0x1FF;
    if (decode_index >= 0x068 && decode_index <= 0x06B) {
        (this->*arm_decode_table[decode_index])(decoded);
        return decoded;
    }

    // Special case handling first
    // Undefined instruction pattern (ARMv4: 0xE7F000F0 and similar)
    // The correct match is (instruction & 0x0FF000F0) == 0x07F000F0
    if ((instruction & 0x0FF000F0) == 0x07F000F0) {
        decoded.type = ARMInstructionType::UNDEFINED;
        decoded.pc_modified = false;
        decoded.execute_func = nullptr;
    }
    // PSR transfer (MRS/MSR, register or immediate)
    else if (
        // MRS (register form)
        (instruction & 0x0FBF0FFF) == 0x010F0000 ||
        // MSR (register form, ignore bits 19-16 and 3-0)
        (instruction & 0x0FBFFFF0) == 0x012FF000 ||
        // MSR (immediate form)
        ((instruction & 0x0FB00000) == 0x03200000)
    ) {
        decoded.type = ARMInstructionType::PSR_TRANSFER;
        decoded.rd = (instruction >> 12) & 0xF;
        decoded.pc_modified = (!(instruction & 0x00200000)) && (decoded.rd == 15); // MRS to PC
        decoded.execute_func = &ARMCPU::executeCachedPSRTransfer;
    }
    // ARM multiply-long: bits 27-23=00001, bits 7-4=1001 (UMULL, UMLAL, SMULL, SMLAL)
    else if (format == 0 && (instruction & 0x0F8000F0) == 0x00800090) {
        // Multiply-long instruction
        DEBUG_INFO("Decoding ARM multiply-long instruction: 0x" + debug_to_hex_string(instruction, 8));
        decoded.type = ARMInstructionType::MULTIPLY;
        decoded.rdHi = (instruction >> 16) & 0xF;
        decoded.rdLo = (instruction >> 12) & 0xF;
        decoded.rs = (instruction >> 8) & 0xF;
        decoded.rm = instruction & 0xF;
        decoded.accumulate = (instruction & 0x00200000) != 0;
        decoded.set_flags = (instruction & 0x00100000) != 0;
        decoded.signed_op = (instruction & 0x00400000) != 0;
        decoded.pc_modified = (decoded.rdHi == 15 || decoded.rdLo == 15);
        decoded.execute_func = &ARMCPU::executeCachedMultiplyLong;
    }
    // ARM multiply/MLA: bits 27-22=000000, bits 7-4=1001 (ignore accumulate/set flags bits)
    else if (format == 0 && ((instruction & 0x0FE000F0) == 0x00000090 || (instruction & 0x0FE000F0) == 0x00200090)) {
        // Multiply or MLA instruction
        decoded.type = ARMInstructionType::MULTIPLY;
        decoded.rd = (instruction >> 16) & 0xF;
        decoded.rn = (instruction >> 12) & 0xF;
        decoded.rs = (instruction >> 8) & 0xF;
        decoded.rm = instruction & 0xF;
        decoded.accumulate = (instruction & 0x00200000) != 0;
        decoded.set_flags = (instruction & 0x00100000) != 0;
        decoded.pc_modified = (decoded.rd == 15);
        decoded.execute_func = &ARMCPU::executeCachedMultiply;
    }
    else if (format == 0 && (instruction & 0x0FFFFFF0) == 0x012FFF10) {
        // BX instruction
        decoded.type = ARMInstructionType::BX;
        decoded.rm = instruction & 0xF;
        decoded.pc_modified = true;
        decoded.execute_func = &ARMCPU::executeCachedBX;
    }
    // ...existing code...
    else {
        // Standard format-based decoding
        switch (format) {
            case 0:
            case 1:
                decoded = decodeDataProcessing(pc, instruction);
                break;
            case 2:
            case 3:
                decoded = decodeSingleDataTransfer(pc, instruction);
                break;
            case 4:
                decoded = decodeBlockDataTransfer(pc, instruction);
                break;
            case 5:
                decoded = decodeBranch(pc, instruction);
                break;
            case 6:
                decoded.type = ARMInstructionType::COPROCESSOR_OP;
                decoded.pc_modified = false;
                decoded.execute_func = &ARMCPU::executeCachedCoprocessor;
                break;
            case 7:
                if ((instruction & 0x0F000000) == 0x0F000000) {
                    decoded.type = ARMInstructionType::SOFTWARE_INTERRUPT;
                    decoded.pc_modified = true;
                    decoded.execute_func = &ARMCPU::executeCachedSoftwareInterrupt;
                } else {
                    decoded.type = ARMInstructionType::COPROCESSOR_REGISTER;
                    decoded.load = (instruction >> 20) & 1;
                    decoded.rd = (instruction >> 12) & 0xF;
                    decoded.pc_modified = decoded.load && (decoded.rd == 15);
                    decoded.execute_func = &ARMCPU::executeCachedCoprocessor;
                }
                break;
            default:
                decoded.type = ARMInstructionType::UNDEFINED;
                decoded.pc_modified = false;
                decoded.execute_func = nullptr;
                break;
        }
    }
    
    return decoded;
}

// Decode data processing instructions with optimizations
ARMCachedInstruction ARMCPU::decodeDataProcessing(uint32_t pc, uint32_t instruction) {
    (void)pc; // Parameter used for future extensions
    ARMCachedInstruction decoded;
    decoded.instruction = instruction;
    decoded.condition = (instruction >> 28) & 0xF;
    decoded.type = ARMInstructionType::DATA_PROCESSING;
    decoded.valid = true;
    
    // Extract common fields
    decoded.dp_op = static_cast<ARMDataProcessingOp>((instruction >> 21) & 0xF);
    decoded.set_flags = (instruction >> 20) & 1;
    decoded.rn = (instruction >> 16) & 0xF;
    decoded.rd = (instruction >> 12) & 0xF;
    decoded.immediate = (instruction >> 25) & 1;
    
    // Determine if PC is modified
    decoded.pc_modified = (decoded.rd == 15);
    
    // Pre-compute immediate operand if applicable
    if (decoded.immediate) {
        uint32_t imm = instruction & 0xFF;
        uint32_t rotate = ((instruction >> 8) & 0xF) * 2;
        
        if (rotate == 0) {
            decoded.imm_value = imm;
            decoded.imm_carry = (parentCPU.CPSR() >> 29) & 1; // Preserve carry
        } else {
            decoded.imm_value = (imm >> rotate) | (imm << (32 - rotate));
            decoded.imm_carry = (decoded.imm_value >> 31) & 1;
        }
        decoded.imm_valid = true;
    } else {
        decoded.rm = instruction & 0xF;
        decoded.imm_valid = false;
    }
    
    decoded.execute_func = &ARMCPU::executeCachedDataProcessing;
    return decoded;
}

// Decode single data transfer instructions
ARMCachedInstruction ARMCPU::decodeSingleDataTransfer(uint32_t pc, uint32_t instruction) {
    (void)pc; // Parameter used for future extensions
    ARMCachedInstruction decoded;
    decoded.instruction = instruction;
    decoded.condition = (instruction >> 28) & 0xF;
    decoded.type = ARMInstructionType::SINGLE_DATA_TRANSFER;
    decoded.valid = true;
    
    decoded.load = (instruction >> 20) & 1;
    decoded.rd = (instruction >> 12) & 0xF;
    decoded.rn = (instruction >> 16) & 0xF;
    decoded.immediate = !((instruction >> 25) & 1); // I bit is inverted for LDR/STR
    
    decoded.pc_modified = decoded.load && (decoded.rd == 15);
    
    // Pre-compute offset for immediate addressing
    if (decoded.immediate) {
        decoded.offset_value = instruction & 0xFFF;
        if (!((instruction >> 23) & 1)) { // U bit
            decoded.offset_value = -decoded.offset_value;
        }
    } else {
        decoded.rm = instruction & 0xF;
        decoded.offset_type = (instruction >> 5) & 3; // Shift type
    }
    
    decoded.execute_func = &ARMCPU::executeCachedSingleDataTransfer;
    return decoded;
}

// Decode branch instructions
ARMCachedInstruction ARMCPU::decodeBranch(uint32_t pc, uint32_t instruction) {
    (void)pc; // Parameter used for future extensions
    ARMCachedInstruction decoded;
    decoded.instruction = instruction;
    decoded.condition = (instruction >> 28) & 0xF;
    decoded.type = ARMInstructionType::BRANCH;
    decoded.valid = true;
    decoded.pc_modified = true;
    
    decoded.link = (instruction >> 24) & 1;
    
    // Pre-compute branch target
    int32_t offset = instruction & 0xFFFFFF;
    if (offset & 0x800000) { // Sign extend
        offset |= 0xFF000000;
    }
    decoded.branch_offset = (offset << 2) + 8; // ARM branches are relative to PC+8
    
    decoded.execute_func = &ARMCPU::executeCachedBranch;
    return decoded;
}

// Decode block data transfer instructions
ARMCachedInstruction ARMCPU::decodeBlockDataTransfer(uint32_t pc, uint32_t instruction) {
    (void)pc; // Parameter used for future extensions
    ARMCachedInstruction decoded;
    decoded.instruction = instruction;
    decoded.condition = (instruction >> 28) & 0xF;
    decoded.type = ARMInstructionType::BLOCK_DATA_TRANSFER;
    decoded.valid = true;
    
    decoded.load = (instruction >> 20) & 1;
    decoded.rn = (instruction >> 16) & 0xF;
    decoded.register_list = instruction & 0xFFFF;
    decoded.addressing_mode = (instruction >> 23) & 3; // P and U bits
    
    decoded.pc_modified = decoded.load && (decoded.register_list & (1 << 15));
    
    decoded.execute_func = &ARMCPU::executeCachedBlockDataTransfer;
    return decoded;
}

// Placeholder cached execution functions (these would call the original implementations)
void ARMCPU::executeCachedDataProcessing(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedDataProcessing: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use pre-decoded cached values to avoid redundant bit extraction
    const uint32_t opcode = static_cast<uint32_t>(cached.dp_op);
    const uint32_t rd = cached.rd;
    const uint32_t rn = cached.rn;
    const bool set_flags = cached.set_flags;
    
    // Fast path for immediate operands (pre-computed during decode)
    if (cached.immediate && cached.imm_valid) {
        // Use pre-computed immediate value and carry from cache
        const uint32_t operand2 = cached.imm_value;
        const uint32_t carry_out = cached.imm_carry;
        
        // Super-optimized MOV immediate path (very common)
        if (opcode == 0xD) {  // MOV
            if (set_flags && rd != 15) {
                uint32_t cpsr = parentCPU.CPSR();
                cpsr &= ~(0x80000000 | 0x40000000); // Clear N, Z
                if (operand2 == 0) cpsr |= 0x40000000;   // Set Z if zero
                if (carry_out) cpsr |= 0x20000000;       // Set C from rotation
                parentCPU.CPSR() = cpsr;
            }
            parentCPU.R()[rd] = operand2;
            return;
        }
        
        // Fast dispatch table for immediate operations
        typedef void (ARMCPU::*DataProcessingFunc)(uint32_t, uint32_t, uint32_t, bool, uint32_t);
        static const DataProcessingFunc funcTable[16] = {
            &ARMCPU::arm_and,  // 0000 - AND
            &ARMCPU::arm_eor,  // 0001 - EOR 
            &ARMCPU::arm_sub,  // 0010 - SUB
            &ARMCPU::arm_rsb,  // 0011 - RSB
            &ARMCPU::arm_add,  // 0100 - ADD
            &ARMCPU::arm_adc,  // 0101 - ADC
            &ARMCPU::arm_sbc,  // 0110 - SBC
            &ARMCPU::arm_rsc,  // 0111 - RSC
            &ARMCPU::arm_tst,  // 1000 - TST
            &ARMCPU::arm_teq,  // 1001 - TEQ
            &ARMCPU::arm_cmp,  // 1010 - CMP
            &ARMCPU::arm_cmn,  // 1011 - CMN
            &ARMCPU::arm_orr,  // 1100 - ORR
            &ARMCPU::arm_mov,  // 1101 - MOV
            &ARMCPU::arm_bic,  // 1110 - BIC
            &ARMCPU::arm_mvn   // 1111 - MVN
        };
        
        // Direct function call with cached values
        (this->*funcTable[opcode])(rd, rn, operand2, set_flags, carry_out);
        return;
    }
    
    // Register operand path - optimize for common no-shift case
    if ((cached.instruction & 0x02000FF0) == 0) {  // Register operand with no shift
        const uint32_t rm = parentCPU.R()[cached.rm];
        const uint32_t op1 = parentCPU.R()[rn];
        const uint32_t carry = (parentCPU.CPSR() >> 29) & 1;
        
        // Fast-path dispatch table for common ALU operations
        typedef void (ARMCPU::*FastALUFunc)(uint32_t, uint32_t, uint32_t, uint32_t, bool, uint32_t);
        static const FastALUFunc fastALUTable[16] = {
            &ARMCPU::fastALU_AND, // 0x0 - AND
            nullptr,              // 0x1 - EOR (not in fast path)
            &ARMCPU::fastALU_SUB, // 0x2 - SUB
            nullptr,              // 0x3 - RSB (not in fast path)
            &ARMCPU::fastALU_ADD, // 0x4 - ADD
            nullptr,              // 0x5 - ADC (not in fast path)
            nullptr,              // 0x6 - SBC (not in fast path)
            nullptr,              // 0x7 - RSC (not in fast path)
            nullptr,              // 0x8 - TST (not in fast path)
            nullptr,              // 0x9 - TEQ (not in fast path)
            &ARMCPU::fastALU_CMP, // 0xA - CMP
            nullptr,              // 0xB - CMN (not in fast path)
            &ARMCPU::fastALU_ORR, // 0xC - ORR
            &ARMCPU::fastALU_MOV, // 0xD - MOV
            nullptr,              // 0xE - BIC (not in fast path)
            nullptr               // 0xF - MVN (not in fast path)
        };
        
        // Use function pointer table for branchless dispatch
        FastALUFunc fastFunc = fastALUTable[opcode];
        if (fastFunc) {
            (this->*fastFunc)(rd, rn, op1, rm, set_flags, carry);
            return;
        }
    }
    
    // Fallback to standard path for complex cases
    uint32_t carry_out = 0;
    uint32_t operand2 = calculateOperand2(cached.instruction, &carry_out);
    
    // Use function dispatch table
    typedef void (ARMCPU::*DataProcessingFunc)(uint32_t, uint32_t, uint32_t, bool, uint32_t);
    static const DataProcessingFunc funcTable[16] = {
        &ARMCPU::arm_and,  // 0000 - AND
        &ARMCPU::arm_eor,  // 0001 - EOR 
        &ARMCPU::arm_sub,  // 0010 - SUB
        &ARMCPU::arm_rsb,  // 0011 - RSB
        &ARMCPU::arm_add,  // 0100 - ADD
        &ARMCPU::arm_adc,  // 0101 - ADC
        &ARMCPU::arm_sbc,  // 0110 - SBC
        &ARMCPU::arm_rsc,  // 0111 - RSC
        &ARMCPU::arm_tst,  // 1000 - TST
        &ARMCPU::arm_teq,  // 1001 - TEQ
        &ARMCPU::arm_cmp,  // 1010 - CMP
        &ARMCPU::arm_cmn,  // 1011 - CMN
        &ARMCPU::arm_orr,  // 1100 - ORR
        &ARMCPU::arm_mov,  // 1101 - MOV
        &ARMCPU::arm_bic,  // 1110 - BIC
        &ARMCPU::arm_mvn   // 1111 - MVN
    };
    
    (this->*funcTable[opcode])(rd, rn, operand2, set_flags, carry_out);
}

void ARMCPU::executeCachedSingleDataTransfer(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedSingleDataTransfer: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use cached fields to perform single data transfer without re-decoding
    const bool load = cached.load;
    const uint32_t rd = cached.rd;
    const uint32_t rn = cached.rn;
    const bool immediate = cached.immediate;
    uint32_t address = parentCPU.R()[rn];
    uint32_t offset = 0;
    if (immediate) {
        offset = cached.offset_value;
    } else {
        uint32_t rm = cached.rm;
        uint32_t shift_type = cached.offset_type;
        uint32_t shift_amount = (cached.instruction >> 7) & 0x1F;
        uint32_t carry_out = 0;
        offset = arm_apply_shift(parentCPU.R()[rm], shift_type, shift_amount, &carry_out);
        if (!((cached.instruction >> 23) & 1)) { // U bit
            offset = -offset;
        }
    }
    uint32_t effective_address = address + offset;
    if (load) {
        parentCPU.R()[rd] = parentCPU.getMemory().read32(effective_address);
    } else {
        parentCPU.getMemory().write32(effective_address, parentCPU.R()[rd]);
    }
    // Write-back for pre-indexed addressing with write-back (P and W bits set)
    bool pre_indexing = (cached.instruction & 0x01000000) != 0; // P bit
    bool write_back = (cached.instruction & 0x00200000) != 0;   // W bit
    if (pre_indexing && write_back) {
        parentCPU.R()[rn] = effective_address;
    }
}

void ARMCPU::executeCachedBranch(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedBranch: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use pre-decoded branch_offset and link from cache
    if (cached.link) {
        parentCPU.R()[14] = parentCPU.R()[15] + 4;
    }
    parentCPU.R()[15] = parentCPU.R()[15] + cached.branch_offset;
}

void ARMCPU::executeCachedBlockDataTransfer(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedBlockDataTransfer: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use pre-decoded cached values to avoid redundant bit extraction
    const uint32_t rn = cached.rn;
    const uint16_t register_list = cached.register_list;
    const uint8_t addressing_mode = cached.addressing_mode;
    const bool load = cached.load;
    
    // Extract addressing mode bits (pre-computed in cache)
    const bool pre_indexing = (addressing_mode & 0x2) != 0;  // P bit
    const bool add_offset = (addressing_mode & 0x1) != 0;    // U bit
    const bool write_back = (cached.instruction & 0x00200000) != 0; // W bit
    
    uint32_t address = parentCPU.R()[rn];
    const uint32_t old_address = address;
    
    // Fast path for empty register list (edge case)
    if (register_list == 0) {
        if (write_back) {
            parentCPU.R()[rn] = old_address + (add_offset ? 0x40 : -0x40);
        }
        return;
    }
    
    // Pre-calculate register count
    const uint32_t num_registers = __builtin_popcount(register_list);
    const int32_t address_step = add_offset ? 4 : -4;
    
    // Pre-indexing address adjustment (combine with address increment)
    if (pre_indexing) {
        address += address_step;
    }
    
    // Specialized paths for load vs store to improve branch prediction
    if (load) {
        // Fast paths for common register combinations
        if (num_registers >= 4 && add_offset) {
            // Process registers sequentially in chunks of 4 if possible
            // This optimizes cache access patterns
            Memory& memory = parentCPU.getMemory();
            uint16_t remaining_registers = register_list;
            
            while (remaining_registers != 0) {
                const uint32_t reg_index = __builtin_ctz(remaining_registers);
                const uint32_t loaded_value = memory.read32(address);
                
                // Avoid overwriting base register during load with writeback
                if (reg_index != rn || !write_back) {
                    parentCPU.R()[reg_index] = loaded_value;
                }
                
                // Update address and clear the processed bit in one step
                address += address_step;
                remaining_registers &= ~(1 << reg_index);
            }
        } else {
            // General case for load operations
            uint16_t remaining_registers = register_list;
            while (remaining_registers != 0) {
                const uint32_t reg_index = __builtin_ctz(remaining_registers);
                const uint32_t loaded_value = parentCPU.getMemory().read32(address);
                
                if (reg_index != rn || !write_back) {
                    parentCPU.R()[reg_index] = loaded_value;
                }
                
                address += address_step;
                remaining_registers &= ~(1 << reg_index);
            }
        }
    } else {
        // Store operations - no need to check for base register writeback conflicts
        Memory& memory = parentCPU.getMemory();
        uint16_t remaining_registers = register_list;
        
        while (remaining_registers != 0) {
            const uint32_t reg_index = __builtin_ctz(remaining_registers);
            memory.write32(address, parentCPU.R()[reg_index]);
            
            address += address_step;
            remaining_registers &= ~(1 << reg_index);
        }
    }
    
    // Write-back optimization - avoid the second condition check when possible
    if (write_back) {
        if (!load || !(register_list & (1 << rn))) {
            parentCPU.R()[rn] = add_offset ? 
                old_address + (num_registers << 2) : 
                old_address - (num_registers << 2);
        }
    }
}

void ARMCPU::executeCachedMultiply(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedMultiply: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use cached fields to perform multiply/MLA without re-decoding
    uint32_t rd = cached.rd;
    uint32_t rn = cached.rn;
    uint32_t rs = cached.rs;
    uint32_t rm = cached.rm;
    bool accumulate = cached.accumulate;
    bool set_flags = cached.set_flags;

    uint32_t result = parentCPU.R()[rm] * parentCPU.R()[rs];
    if (accumulate) {
        result += parentCPU.R()[rn];
    }

    parentCPU.R()[rd] = result;

    // Set flags if requested
    if (set_flags) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N and Z flags
        cpsr |= (result & 0x80000000);      // Set N flag if result is negative
        if (result == 0) {
            cpsr |= 0x40000000;             // Set Z flag if result is zero
        }
        parentCPU.CPSR() = cpsr;
    }
}

void ARMCPU::executeCachedMultiplyLong(const ARMCachedInstruction& cached) {
    DEBUG_LOG("executeCachedMultiplyLong: pc=0x" + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // For UMULL, UMLAL, SMULL, SMLAL
    uint32_t rdLo = cached.rdLo;
    uint32_t rdHi = cached.rdHi;
    uint32_t rm = cached.rm;
    uint32_t rs = cached.rs;
    bool accumulate = cached.accumulate;
    bool set_flags = cached.set_flags;
    bool signed_op = cached.signed_op;

    if (signed_op) {
        int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs];
        if (accumulate) {
            int64_t acc = ((int64_t)((uint64_t)parentCPU.R()[rdHi] << 32 | parentCPU.R()[rdLo]));
            result += acc;
        }
        if (rdLo == rdHi) {
            parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
            parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
        } else {
            parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
            parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
        }
        DEBUG_LOG("[MUL-LONG] (signed) RdLo=" + DEBUG_TO_HEX_STRING(parentCPU.R()[rdLo], 8) + ", RdHi=" + DEBUG_TO_HEX_STRING(parentCPU.R()[rdHi], 8));
    } else {
        uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs];
        if (accumulate) {
            uint64_t acc = ((uint64_t)parentCPU.R()[rdHi] << 32) | parentCPU.R()[rdLo];
            result += acc;
        }
        if (rdLo == rdHi) {
            parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
            parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
        } else {
            parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
            parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);
        }
        DEBUG_LOG("[MUL-LONG] (unsigned) RdLo=" + DEBUG_TO_HEX_STRING(parentCPU.R()[rdLo], 8) + ", RdHi=" + DEBUG_TO_HEX_STRING(parentCPU.R()[rdHi], 8));
    }

    if (set_flags) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr &= ~(0x80000000 | 0x40000000); // Clear N and Z
        if (parentCPU.R()[rdHi] & 0x80000000) cpsr |= 0x80000000; // N
        if (parentCPU.R()[rdHi] == 0 && parentCPU.R()[rdLo] == 0) cpsr |= 0x40000000; // Z
        parentCPU.CPSR() = cpsr;
    }
}

void ARMCPU::executeCachedBX(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedBX: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use cached.rm for BX
    uint32_t target_address = parentCPU.R()[cached.rm];
    bool switch_to_thumb = (target_address & 1) != 0;
    target_address &= ~1U;
    if (switch_to_thumb) {
        uint32_t cpsr = parentCPU.CPSR();
        cpsr |= 0x00000020; // Set T flag (bit 5)
        parentCPU.CPSR() = cpsr;
    }
    parentCPU.R()[15] = target_address;
}

void ARMCPU::executeCachedSoftwareInterrupt(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedSoftwareInterrupt: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Use cached.instruction for SWI number
    uint32_t swi_number = cached.instruction & 0x00FFFFFF;
    uint32_t return_address = parentCPU.R()[15] + 4;
    handleException(0x00000008, 0x13, true, false);
    DEBUG_INFO("ARM Software Interrupt: number=0x" + 
               debug_to_hex_string(swi_number, 6) + 
               " return_address=0x" + debug_to_hex_string(return_address, 8));
    UNUSED(swi_number);
    UNUSED(return_address);
}

void ARMCPU::executeCachedPSRTransfer(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedPSRTransfer: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    // Inline arm_psr_transfer logic using only cached fields
    bool is_cpsr = ((cached.instruction & 0x00400000) == 0); // 1=CPSR (bit 22=0), 0=SPSR (bit 22=1)
    bool is_mrs = (cached.instruction & 0x00200000) == 0; // 0=MSR, 1=MRS

    if (!is_mrs) {
        // MSR: Move from register/immediate to PSR
        uint32_t field_mask = (cached.instruction >> 16) & 0xF;
        uint32_t value;
        if (cached.instruction & 0x02000000) {
            // Immediate operand
            uint32_t imm = cached.instruction & 0xFF;
            uint32_t rotate_field = (cached.instruction >> 8) & 0xF;
            uint32_t rotate = rotate_field * 2;
            value = imm;
            if (rotate) {
                value = ror32(value, rotate);
            }
        } else {
            // Register operand
            uint32_t rm = cached.instruction & 0xF;
            value = parentCPU.R()[rm];
        }
        uint32_t psr = parentCPU.CPSR();
        if (field_mask & 1) psr = (psr & ~0x000000FF) | ((value & 0xFF) << 0);
        if (field_mask & 2) psr = (psr & ~0x0000FF00) | (((value >> 8) & 0xFF) << 8);
        if (field_mask & 4) psr = (psr & ~0x00FF0000) | (((value >> 16) & 0xFF) << 16);
        if (field_mask & 8) psr = (psr & ~0xF0000000) | (((value >> 28) & 0xF) << 28);
        if (is_cpsr) {
            parentCPU.CPSR() = psr;
        }
        // SPSR not supported
    } else {
        // MRS: Move from PSR to register
        uint32_t rd = (cached.instruction >> 12) & 0xF;
        uint32_t cpsr_val = parentCPU.CPSR();
        if (is_cpsr) {
            parentCPU.R()[rd] = cpsr_val;
        } else {
            parentCPU.R()[rd] = cpsr_val; // SPSR not supported
        }
    }
}

void ARMCPU::executeCachedCoprocessor(const ARMCachedInstruction& cached) {
    DEBUG_LOG(std::string("executeCachedCoprocessor: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(cached.instruction, 8));
    switch (cached.type) {
        case ARMInstructionType::COPROCESSOR_OP:
            arm_coprocessor_operation(cached.instruction);
            break;
        case ARMInstructionType::COPROCESSOR_TRANSFER:
            arm_coprocessor_transfer(cached.instruction);
            break;
        case ARMInstructionType::COPROCESSOR_REGISTER:
            arm_coprocessor_register(cached.instruction);
            break;
        default:
            arm_undefined(cached.instruction);
            break;
    }
}

