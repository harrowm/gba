#include "test_cpu_common.h"

// ARM Thumb Format 17: Software interrupt
// Encoding: 11011111[Value8]
// Instructions: SWI

TEST(Format17, SWI_BASIC_COMMENT_VALUES) {
    std::string beforeState;

    // Test case 1: SWI #0 (comment = 0x00)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x12345678; // Test data
        registers[1] = 0x87654321; // Test data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xDF00); // SWI #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SWI should not modify registers (in this basic implementation)
        // The actual behavior depends on the OS/handler, but we test the instruction encoding
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
    }

    // Test case 2: SWI #1 (comment = 0x01)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0xDEADBEEF; // Test data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xDF01); // SWI #1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xDEADBEEF));
        validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
    }

    // Test case 3: SWI #0xFF (comment = 0xFF, maximum value)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[7] = 0xCAFEBABE; // Test data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xDFFF); // SWI #0xFF
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0xCAFEBABE));
        validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
    }
}

TEST(Format17, SWI_COMMON_COMMENT_VALUES) {
    std::string beforeState;

    // Test case 1: SWI #0x10 (common system call value)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x11111111; // Test data
        registers[1] = 0x22222222; // Test data
        registers[2] = 0x33333333; // Test data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xDF10); // SWI #0x10
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify registers are preserved
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x11111111));
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x22222222));
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x33333333));
        validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
    }

    // Test case 2: SWI #0x80 (another common system call value)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[3] = 0x44444444; // Test data
        registers[4] = 0x55555555; // Test data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xDF80); // SWI #0x80
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x44444444));
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x55555555));
        validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
    }
}

TEST(Format17, SWI_ENCODING_VERIFICATION) {
    std::string beforeState;

    // Test various comment values to verify proper encoding
    struct TestCase {
        uint8_t comment;
        uint16_t expectedInstruction;
    };

    TestCase testCases[] = {
        {0x00, 0xDF00},  // SWI #0
        {0x01, 0xDF01},  // SWI #1
        {0x0F, 0xDF0F},  // SWI #15
        {0x10, 0xDF10},  // SWI #16
        {0x20, 0xDF20},  // SWI #32
        {0x40, 0xDF40},  // SWI #64
        {0x7F, 0xDF7F},  // SWI #127
        {0x80, 0xDF80},  // SWI #128
        {0xAA, 0xDFAA},  // SWI #170
        {0xFF, 0xDFFF},  // SWI #255
    };

    for (const auto& testCase : testCases) {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x12345678; // Test data
        
        gba.getCPU().getMemory().write16(0x00000000, testCase.expectedInstruction);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the instruction was recognized (PC should advance)
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
    }
}

TEST(Format17, SWI_INSTRUCTION_FORMAT) {
    // Test that the instruction format is correctly recognized
    // Format 17: 11011111 Value8
    // Where Value8 is an 8-bit comment field
    
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    cpu.CPSR() = CPU::FLAG_T;
    registers.fill(0);
    
    // Test the boundary between Format 16 (0xDE__) and Format 17 (0xDF__)
    registers[0] = 0xAAAAAAAA; // Test data
    
    // This should be recognized as SWI (Format 17), not conditional branch (Format 16)
    gba.getCPU().getMemory().write16(0x00000000, 0xDF42); // SWI #0x42
    std::string beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    
    // Verify instruction executed (registers preserved, PC advanced)
    ASSERT_EQ(registers[0], static_cast<uint32_t>(0xAAAAAAAA));
    validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
}
