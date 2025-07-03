#include "gtest/gtest.h"
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"

TEST(CPU, SimpleProgram) {
    // Setup test GBA with test RAM region
    GBA gba(true); // Pass true to indicate test mode

    // Debug: Log before writing to memory
    Debug::log::info("Writing Thumb instruction MOV R1, #27 to memory at address 0x00000000");

    // Load a simple program into RAM
    // Program: MOV R1, #27 (Thumb instruction: 0x213B)
    gba.getCPU().getMemory().write16(0x00000000, 0x213B); // MOV R1, #27

    // Debug: Log after writing to memory
    Debug::log::info("Thumb instruction written successfully to memory");

    // Initialize CPU state
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0); // Reset all registers to zero
    cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode

    // Print initial CPU state
    cpu.printCPUState();
    
    // Run the program
    cpu.execute(1); // Execute one instruction - hack should be the number of cycles

    // Print final CPU state
    cpu.printCPUState();
   
    // Verify CPU state
    ASSERT_EQ(registers[1], static_cast<unsigned int>(27)); // R1 should contain 27
    for (size_t i = 0; i < registers.size(); ++i) {
        if (i != 1) {
            ASSERT_EQ(registers[i], static_cast<unsigned int>(0)); // Other registers should remain unchanged
        }
    }
    ASSERT_EQ(cpu.CPSR(), CPU::FLAG_T); // CPSR should only have the Thumb flag set
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
