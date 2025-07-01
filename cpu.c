#include "cpu.h"
#include "debug.h" // Include debug utilities
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// CPU state structure
CPUState cpu;


// Initialize the CPU
void cpu_init() {
    cpu.r[15] = 0; // Program Counter (PC)
    cpu.r[14] = 0; // Link Register (LR)
    cpu.r[13] = 0; // Stack Pointer (SP)
    cpu.cpsr = 0;  // Current Program Status Register
    cpu.mode = ARM_MODE; // Start in ARM mode

    // Initialize the hash tables for arm instructions
    arm_init_hash_tables();
}

// Fetch, decode, and execute instructions
void cpu_step(uint32_t cycles) {
    while (cycles > 0) {
        CPUMode mode = get_cpu_state(&cpu); // Get the current CPU mode

        if (mode == ARM_MODE) {
            uint32_t instruction = memory_read_32(cpu.r[15]); // Fetch ARM instruction
            uint32_t instruction_cycles = arm_decode_and_execute(instruction); // Decode and execute ARM instruction
            cycles -= instruction_cycles; // Deduct cycles
        } else if (mode == THUMB_MODE) {
            uint16_t instruction = memory_read_16(cpu.r[15]); // Fetch Thumb instruction
            uint32_t instruction_cycles = thumb_decode_and_execute(instruction); // Decode and execute Thumb instruction
            cycles -= instruction_cycles; // Deduct cycles
        }

        // Handle interrupts if necessary
        if (check_interrupts()) {
            handle_interrupts();
        }
    }
}


// Replace magic numbers with CPSR flag constants
int check_condition_codes(uint8_t condition) {
    switch (condition) {
        case 0: // EQ: Equal (Z == 1)
            return (cpu.cpsr & CPSR_Z_FLAG) != 0;
        case 1: // NE: Not Equal (Z == 0)
            return (cpu.cpsr & CPSR_Z_FLAG) == 0;
        case 2: // CS/HS: Carry Set/Unsigned Higher or Same (C == 1)
            return (cpu.cpsr & CPSR_C_FLAG) != 0;
        case 3: // CC/LO: Carry Clear/Unsigned Lower (C == 0)
            return (cpu.cpsr & CPSR_C_FLAG) == 0;
        case 4: // MI: Negative (N == 1)
            return (cpu.cpsr & CPSR_N_FLAG) != 0;
        case 5: // PL: Positive or Zero (N == 0)
            return (cpu.cpsr & CPSR_N_FLAG) == 0;
        case 6: // VS: Overflow (V == 1)
            return (cpu.cpsr & CPSR_V_FLAG) != 0;
        case 7: // VC: No Overflow (V == 0)
            return (cpu.cpsr & CPSR_V_FLAG) == 0;
        case 8: // HI: Unsigned Higher (C == 1 && Z == 0)
            return ((cpu.cpsr & CPSR_C_FLAG) != 0) && ((cpu.cpsr & CPSR_Z_FLAG) == 0);
        case 9: // LS: Unsigned Lower or Same (C == 0 || Z == 1)
            return ((cpu.cpsr & CPSR_C_FLAG) == 0) || ((cpu.cpsr & CPSR_Z_FLAG) != 0);
        case 10: // GE: Signed Greater or Equal (N == V)
            return ((cpu.cpsr & CPSR_N_FLAG) >> 31) == ((cpu.cpsr & CPSR_V_FLAG) >> 28);
        case 11: // LT: Signed Less Than (N != V)
            return ((cpu.cpsr & CPSR_N_FLAG) >> 31) != ((cpu.cpsr & CPSR_V_FLAG) >> 28);
        case 12: // GT: Signed Greater Than (Z == 0 && N == V)
            return ((cpu.cpsr & CPSR_Z_FLAG) == 0) && (((cpu.cpsr & CPSR_N_FLAG) >> 31) == ((cpu.cpsr & CPSR_V_FLAG) >> 28));
        case 13: // LE: Signed Less Than or Equal (Z == 1 || N != V)
            return ((cpu.cpsr & CPSR_Z_FLAG) != 0) || (((cpu.cpsr & CPSR_N_FLAG) >> 31) != ((cpu.cpsr & CPSR_V_FLAG) >> 28));
        case 14: // AL: Always (no condition)
            return 1;
        default:
            return 0; // Invalid condition
    }
}

// Centralized CPSR update logic in `decode_and_execute`
void update_cpsr_flags(uint32_t result, uint8_t carry_out) {
    if (result == 0) {
        cpu.cpsr |= CPSR_Z_FLAG; // Set Zero flag if result is zero
    } else {
        cpu.cpsr &= ~CPSR_Z_FLAG; // Clear Zero flag
    }
    if (result & CPSR_N_FLAG) {
        cpu.cpsr |= CPSR_N_FLAG; // Set Negative flag if result is negative
    } else {
        cpu.cpsr &= ~CPSR_N_FLAG; // Clear Negative flag
    }
    if (carry_out) {
        cpu.cpsr |= CPSR_C_FLAG; // Set Carry flag if applicable
    } else {
        cpu.cpsr &= ~CPSR_C_FLAG; // Clear Carry flag
    }
}

// Function to set the CPU mode
void set_cpu_mode(CPUState* state, CPUMode mode) {
    state->mode = mode; // Update the mode in the CPU state structure
    LOG_INFO("CPU mode set to %s", mode == ARM_MODE ? "ARM" : "Thumb");
}

// Function to get the current CPU mode from a given CPU state
CPUMode get_cpu_state(CPUState* state) {
    return state->mode;
}

// Placeholder for interrupt handling
static int check_interrupts() {
    // Implement interrupt checking logic
    return 0;
}

static void handle_interrupts() {
    // Implement interrupt handling logic
}
