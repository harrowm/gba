#include <gtest/gtest.h>
#include "gba.h"
#include "debug.h"

TEST(Format11Debug, SP_RELATIVE_SIMPLE_TEST) {
    // Simple test to debug the issue
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Set up test scenario
    registers[13] = 0x00000100; // SP
    registers[0] = 0x30000001;  // Test value (exactly what's failing)
    
    // Store using STR R0, [SP, #4] (word8=1)
    uint16_t store_opcode = 0x9001; // 0x9000 | 1
    gba.getCPU().getMemory().write16(0x00000000, store_opcode);
    
    std::cout << "Before execution:" << std::endl;
    std::cout << "SP = 0x" << std::hex << registers[13] << std::endl;
    std::cout << "R0 = 0x" << std::hex << registers[0] << std::endl;
    std::cout << "Instruction = 0x" << std::hex << store_opcode << std::endl;
    
    cpu.execute(1);
    
    // Check what was actually stored
    uint32_t expected_address = 0x00000100 + 4; // 0x104
    uint32_t stored_value = gba.getCPU().getMemory().read32(expected_address);
    
    std::cout << "After execution:" << std::endl;
    std::cout << "Expected address = 0x" << std::hex << expected_address << std::endl;
    std::cout << "Stored value = 0x" << std::hex << stored_value << std::endl;
    std::cout << "Expected value = 0x" << std::hex << registers[0] << std::endl;
    
    ASSERT_EQ(stored_value, static_cast<uint32_t>(0x30000001));
}
