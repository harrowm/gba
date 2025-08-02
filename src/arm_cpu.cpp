#include <cstdint>
#include <cassert>
#include "arm_cpu.h"
#include "debug.h"
#include "timing.h"
#include "arm_timing.h"


// Secondary decode function for ambiguous region (data processing/MUL/MLA overlap)
// Phase 1: New entry point for ambiguous region
ARMCPU::ARMCPU(CPU& cpu) : parentCPU(cpu) {
    DEBUG_INFO("Initializing ARMCPU with parent CPU");
}

ARMCPU::~ARMCPU() {
    // Cleanup logic if necessary
}

// HACK - exception taken looks like a bodge ..
void ARMCPU::execute(uint32_t cycles) {
    exception_taken = false;
    while (cycles > 0) {
        // Check if we're still in ARM mode - if not, break out early
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            DEBUG_INFO("Mode switched to Thumb during execution, breaking out of ARM execution");
            break;
        }

        uint32_t pc = parentCPU.R()[15]; // Get current PC
        uint32_t instruction = parentCPU.getMemory().read32(pc); // Fetch instruction

        executeInstruction(pc, instruction);
        if (exception_taken) {
            break;
        }
        
        DEBUG_INFO("PC now: 0x" + debug_to_hex_string(parentCPU.R()[15], 8));
        cycles -= 1; // Placeholder for cycle deduction
    }
}

// New timing-aware execution method
void ARMCPU::executeWithTiming(uint32_t cycles, TimingState* timing) {
    // Use macro-based debug system
    
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
            executeInstruction(pc, instruction);
        
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

const ARMCPU::CondFunc ARMCPU::condTable[16] = {
    &ARMCPU::cond_eq, // 0: EQ
    &ARMCPU::cond_ne, // 1: NE
    &ARMCPU::cond_cs, // 2: CS
    &ARMCPU::cond_cc, // 3: CC
    &ARMCPU::cond_mi, // 4: MI
    &ARMCPU::cond_pl, // 5: PL
    &ARMCPU::cond_vs, // 6: VS
    &ARMCPU::cond_vc, // 7: VC
    &ARMCPU::cond_hi, // 8: HI
    &ARMCPU::cond_ls, // 9: LS
    &ARMCPU::cond_ge, // 10: GE
    &ARMCPU::cond_lt, // 11: LT
    &ARMCPU::cond_gt, // 12: GT
    &ARMCPU::cond_le, // 13: LE
    &ARMCPU::cond_al, // 14: AL
    &ARMCPU::cond_nv  // 15: NV
};

void ARMCPU::executeInstruction(uint32_t pc, uint32_t instruction) {
    UNUSED(pc); // maybe useful for debugging

    uint32_t index = (bits<27,20>(instruction) << 1) | ((instruction & 0x90) == 0x90); // bit 7 and 4 are set
    DEBUG_INFO("bits and 9: " + std::to_string(bits<7,4>(instruction) & 0x9));
    DEBUG_INFO("executeInstruction: PC=0x" + debug_to_hex_string(pc, 8) + 
                " Instruction=0x" + debug_to_hex_string(instruction, 8) + " using fn table index: 0x" + debug_to_hex_string(index, 3));
    
    // Check if condition is met before executing instruction
    uint8_t condition = bits<31, 28>(instruction);
    if(!ARMCPU::condTable[condition](parentCPU.CPSR() >> 28)) 
        return ;

    // .. and call the exec handler for the instruction
    (this->*arm_exec_table[index])(instruction);
}

// Optimized flag update function for logical operations (AND, EOR, TST, TEQ, etc.)
// FORCE_INLINE void ARMCPU::updateFlagsLogical(uint32_t result, uint32_t carry_out) {
//     // Get current CPSR value once to reduce memory access
//     uint32_t cpsr = parentCPU.CPSR();
    
//     // Clear N, Z flags and set them based on result (bits 31 and 30)
//     cpsr &= ~(0x80000000 | 0x40000000); // Clear N (bit 31) and Z (bit 30)
    
//     // Set N if result is negative (bit 31 set)
//     cpsr |= (result & 0x80000000);
    
//     // Set Z if result is zero (bit 30)
//     if (result == 0) {
//         cpsr |= 0x40000000;
//     }
    
//     // Clear and set carry flag based on carry_out (bit 29)
//     cpsr &= ~(1U << 29);
//     cpsr |= (carry_out << 29);
    
//     // Update CPSR by setting directly
//     parentCPU.CPSR() = cpsr;
// }

void ARMCPU::handleException(uint32_t vector_address, uint32_t new_mode, bool disable_irq, bool disable_fiq) {
    DEBUG_LOG("handleException ENTRY: vector=0x" + debug_to_hex_string(vector_address, 8) + ", new_mode=0x" + debug_to_hex_string(new_mode, 2) + ", PC=0x" + debug_to_hex_string(parentCPU.R()[15], 8));
    CPU::Mode old_mode = static_cast<CPU::Mode>(parentCPU.CPSR() & 0x1F);
    CPU::Mode new_mode_enum = static_cast<CPU::Mode>(new_mode & 0x1F);
    
    assert((new_mode & 0x1F) >= 0x10 && (new_mode & 0x1F) <= 0x1F && "Invalid new_mode in handleException");
    // Save the current CPSR (SPSR not supported in this implementation)
    uint32_t old_cpsr = parentCPU.CPSR();

    // Calculate the return address for LR
    // NOTE: ARM architectural requirement: LR for exception modes is always set to PC+4 (address of next instruction).
    // Even without a pipeline, exception handlers expect LR = PC+4.
    uint32_t return_address = parentCPU.R()[15] + 4;
    if (old_mode == CPU::USER || old_mode == CPU::SYS) {
        parentCPU.bankedLRUser() = parentCPU.R()[14]; // save User LR
    } else {
        parentCPU.bankedLR(old_mode) = parentCPU.R()[14]; // save banked LR for privileged mode
    }
    
    // Set LR for the NEW mode BEFORE swapping
    if (new_mode_enum == CPU::USER || new_mode_enum == CPU::SYS) {
        // Do not overwrite banked_r14_usr on exception entry
    } else if (new_mode_enum == CPU::SVC) {
        parentCPU.bankedLR(CPU::SVC) = return_address;
    } else if (new_mode_enum == CPU::UND) {
        parentCPU.bankedLR(CPU::UND) = return_address;
    } else if (new_mode_enum == CPU::IRQ) {
        parentCPU.bankedLR(CPU::IRQ) = return_address;
    } else if (new_mode_enum == CPU::FIQ) {
        parentCPU.bankedLR(CPU::FIQ) = return_address;
    } else if (new_mode_enum == CPU::ABT) {
        parentCPU.bankedLR(CPU::ABT) = return_address;
    } else {
        parentCPU.R()[14] = return_address;
    }

    // Now swap to the new mode
    parentCPU.setMode(new_mode_enum);
    
    // Set IRQ/FIQ disable bits in CPSR (after mode switch)
    uint32_t new_cpsr = parentCPU.CPSR();
    if (disable_irq) {
        new_cpsr |= 0x80; // Set I bit
    }
    if (disable_fiq) {
        new_cpsr |= 0x40; // Set F bit
    }
    parentCPU.CPSR() = new_cpsr;

    // In a real ARM CPU, we would save old_cpsr to SPSR, but it's not supported here
    DEBUG_LOG("SPSR write not supported, old CPSR value 0x" + debug_to_hex_string(old_cpsr, 8) + " not saved");

    // Set the PC to the exception vector
    parentCPU.R()[15] = vector_address;
}

