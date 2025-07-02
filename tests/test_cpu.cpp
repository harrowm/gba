#include "gtest/gtest.h"
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"

TEST(CPU, SimpleProgram) {
    // Setup test RAM and GBA
    Memory memory(0x40000); // Pass size to Memory constructor
    GBA gba; // Use default constructor

    // Load a simple program into RAM
    // Program: MOV R1, #27 (Thumb instruction: 0x213B)
    memory.write16(0x00000000, 0x213B); // MOV R1, #27

    // Initialize CPU state
    CPUState initialState;
    for (int i = 0; i < 16; ++i) {
        initialState.registers[i] = 0;
    }
    initialState.cpsr = CPU::FLAG_THUMB; // Set Thumb mode
    initialState.bigEndian = false;
    gba.getCPU().setCPUState(initialState); // Use getCPU to access CPU

    // Print initial CPU state
    gba.getCPU().printCPUState();

    // Run the program
    gba.getCPU().execute(1); // Execute one instruction

    // Print final CPU state
    gba.getCPU().printCPUState();

    // Verify CPU state
    CPUState finalState = gba.getCPU().getCPUState();
    for (int i = 0; i < 16; ++i) {
        if (i == 1) {
            ASSERT_EQ(finalState.registers[i], static_cast<unsigned int>(27)); // R1 should contain 27
        } else {
            ASSERT_EQ(finalState.registers[i], static_cast<unsigned int>(0)); // Other registers should remain unchanged
        }
    }
    ASSERT_EQ(finalState.cpsr, CPU::FLAG_THUMB); // CPSR should only have the Thumb flag set
    ASSERT_EQ(finalState.cpsr & CPU::FLAG_ZERO, 0u); // Zero flag should not be set
    ASSERT_EQ(finalState.cpsr & CPU::FLAG_NEGATIVE, 0u); // Negative flag should not be set
    ASSERT_EQ(finalState.cpsr & CPU::FLAG_THUMB, CPU::FLAG_THUMB); // Thumb flag should be set
}
