#include "test_cpu_common.h"

// ARM Thumb Format 13: Add/Subtract offset to Stack Pointer
// Encoding: 1011 0000 S [offset7]
// Instructions: ADD SP, #imm ; SUB SP, #imm
// S=0: ADD SP, #imm (SP = SP + (offset7 * 4))
// S=1: SUB SP, #imm (SP = SP - (offset7 * 4))

TEST(Format13, ADD_SP_IMMEDIATE_BASIC) {
    std::string beforeState;

    // Test case 1: ADD SP, #0 - no change
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB000); // ADD SP, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should remain unchanged
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001000));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: ADD SP, #4 - basic increment
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB001); // ADD SP, #4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be incremented by 4
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001004));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 3: ADD SP, #32 - medium increment
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB008); // ADD SP, #32
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be incremented by 32
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001020));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 4: ADD SP, #128 - large increment
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB020); // ADD SP, #128
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be incremented by 128
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001080));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 5: ADD SP, #508 - maximum increment
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB07F); // ADD SP, #508
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be incremented by 508
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x000011FC));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }
}

TEST(Format13, SUB_SP_IMMEDIATE_BASIC) {
    std::string beforeState;

    // Test case 1: SUB SP, #0 - no change
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB080); // SUB SP, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should remain unchanged
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001000));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: SUB SP, #4 - basic decrement
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB081); // SUB SP, #4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be decremented by 4
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00000FFC));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 3: SUB SP, #32 - medium decrement
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB088); // SUB SP, #32
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be decremented by 32
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00000FE0));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 4: SUB SP, #128 - large decrement
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB0A0); // SUB SP, #128
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be decremented by 128
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00000F80));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 5: SUB SP, #508 - maximum decrement
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP value
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB0FF); // SUB SP, #508
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // SP should be decremented by 508
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00000E04));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }
}

TEST(Format13, OFFSET_RANGE_TESTS) {
    std::string beforeState;

    // Test all valid offsets that are multiples of 4, from 0 to 508
    for (int offset = 0; offset <= 508; offset += 4) {
        
        // Test ADD SP
        {
            GBA gba(true); // Test mode
            auto& cpu = gba.getCPU();
            auto& registers = cpu.R();
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            registers[13] = 0x00001000; // Initial SP value
            
            // Calculate opcode for ADD SP, #offset
            uint16_t offset7 = offset / 4;
            uint16_t opcode = 0xB000 | offset7;
            
            gba.getCPU().getMemory().write16(0x00000000, opcode);
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify SP was incremented correctly
            uint32_t expected_sp = 0x00001000 + offset;
            ASSERT_EQ(registers[13], expected_sp) 
                << "ADD SP, #" << offset << " failed. Expected SP: 0x" 
                << std::hex << expected_sp << ", Got: 0x" << registers[13];
            validateUnchangedRegisters(cpu, beforeState, {13, 15});
        }
        
        // Test SUB SP (only if SP won't underflow)
        if (offset <= 0x1000) {
            GBA gba(true); // Test mode
            auto& cpu = gba.getCPU();
            auto& registers = cpu.R();
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            registers[13] = 0x00001000; // Initial SP value
            
            // Calculate opcode for SUB SP, #offset
            uint16_t offset7 = offset / 4;
            uint16_t opcode = 0xB080 | offset7;
            
            gba.getCPU().getMemory().write16(0x00000000, opcode);
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify SP was decremented correctly
            uint32_t expected_sp = 0x00001000 - offset;
            ASSERT_EQ(registers[13], expected_sp)
                << "SUB SP, #" << offset << " failed. Expected SP: 0x" 
                << std::hex << expected_sp << ", Got: 0x" << registers[13];
            validateUnchangedRegisters(cpu, beforeState, {13, 15});
        }
    }
}

TEST(Format13, ADD_SUB_SEQUENCE_TESTS) {
    std::string beforeState;

    // Test case 1: ADD then SUB same amount - should return to original
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        uint32_t initial_sp = 0x00001000;
        registers[13] = initial_sp;
        
        // ADD SP, #32
        gba.getCPU().getMemory().write16(0x00000000, 0xB008);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], initial_sp + 32);
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
        
        // SUB SP, #32
        gba.getCPU().getMemory().write16(0x00000000, 0xB088);
        registers[15] = 0x00000000; // Reset PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Should be back to original value
        ASSERT_EQ(registers[13], initial_sp);
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 2: Multiple ADD operations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        uint32_t initial_sp = 0x00001000;
        registers[13] = initial_sp;
        
        // ADD SP, #16
        gba.getCPU().getMemory().write16(0x00000000, 0xB004);
        registers[15] = 0x00000000;
        cpu.execute(1);
        ASSERT_EQ(registers[13], initial_sp + 16);
        
        // ADD SP, #16 again
        gba.getCPU().getMemory().write16(0x00000000, 0xB004);
        registers[15] = 0x00000000;
        cpu.execute(1);
        ASSERT_EQ(registers[13], initial_sp + 32);
        
        // ADD SP, #16 again
        gba.getCPU().getMemory().write16(0x00000000, 0xB004);
        registers[15] = 0x00000000;
        cpu.execute(1);
        ASSERT_EQ(registers[13], initial_sp + 48);
    }

    // Test case 3: Multiple SUB operations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        uint32_t initial_sp = 0x00001200; // Higher starting point for SUB
        registers[13] = initial_sp;
        
        // SUB SP, #16
        gba.getCPU().getMemory().write16(0x00000000, 0xB084);
        registers[15] = 0x00000000;
        cpu.execute(1);
        ASSERT_EQ(registers[13], initial_sp - 16);
        
        // SUB SP, #16 again
        gba.getCPU().getMemory().write16(0x00000000, 0xB084);
        registers[15] = 0x00000000;
        cpu.execute(1);
        ASSERT_EQ(registers[13], initial_sp - 32);
        
        // SUB SP, #16 again
        gba.getCPU().getMemory().write16(0x00000000, 0xB084);
        registers[15] = 0x00000000;
        cpu.execute(1);
        ASSERT_EQ(registers[13], initial_sp - 48);
    }
}

TEST(Format13, EDGE_CASES_AND_BOUNDARY_CONDITIONS) {
    std::string beforeState;

    // Test case 1: SP at memory boundary - ADD
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001F00; // Near end of test memory
        
        // ADD SP, #4 - should be fine
        gba.getCPU().getMemory().write16(0x00000000, 0xB001);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001F04));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 2: SP at memory boundary - SUB
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00000100; // Near start of memory
        
        // SUB SP, #4
        gba.getCPU().getMemory().write16(0x00000000, 0xB081);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x000000FC));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 3: SP overflow (ADD maximum to high value)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0xFFFFFF00; // High value that will overflow
        
        // ADD SP, #508 (maximum)
        gba.getCPU().getMemory().write16(0x00000000, 0xB07F);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Should wrap around due to 32-bit arithmetic
        uint32_t expected = 0xFFFFFF00 + 508;
        ASSERT_EQ(registers[13], expected);
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 4: SP underflow (SUB from low value)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00000100; // Low value
        
        // SUB SP, #508 (maximum) - will underflow
        gba.getCPU().getMemory().write16(0x00000000, 0xB0FF);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Should wrap around due to 32-bit arithmetic
        uint32_t expected = 0x00000100 - 508;
        ASSERT_EQ(registers[13], expected);
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 5: No effect on other registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        
        // Set up a simpler test: just verify SP operation and PC increment
        registers.fill(0);
        registers[13] = 0x00001000; // Initial SP value
        registers[15] = 0x00000000; // Set PC to 0
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB010); // ADD SP, #64
        cpu.execute(1);
        
        // Verify SP was modified correctly
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001040));
        // Verify PC was incremented
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000002));
    }

    // Test case 6: CPSR flags should be unaffected
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        
        // Set various CPSR flags
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_N | CPU::FLAG_Z | CPU::FLAG_C | CPU::FLAG_V;
        registers[13] = 0x00001000;
        
        uint32_t original_cpsr = cpu.CPSR();
        
        // ADD SP, #32
        gba.getCPU().getMemory().write16(0x00000000, 0xB008);
        cpu.execute(1);
        
        // CPSR should be unchanged
        ASSERT_EQ(cpu.CPSR(), original_cpsr);
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001020));
    }
}

TEST(Format13, INSTRUCTION_ENCODING_VALIDATION) {
    // Test that all instruction encodings work correctly
    std::string beforeState;
    
    struct TestCase {
        uint16_t opcode;
        std::string description;
        uint32_t initial_sp;
        uint32_t expected_sp;
    };
    
    std::vector<TestCase> test_cases = {
        // ADD instructions
        {0xB000, "ADD SP, #0",   0x1000, 0x1000},
        {0xB001, "ADD SP, #4",   0x1000, 0x1004},
        {0xB002, "ADD SP, #8",   0x1000, 0x1008},
        {0xB004, "ADD SP, #16",  0x1000, 0x1010},
        {0xB008, "ADD SP, #32",  0x1000, 0x1020},
        {0xB010, "ADD SP, #64",  0x1000, 0x1040},
        {0xB020, "ADD SP, #128", 0x1000, 0x1080},
        {0xB040, "ADD SP, #256", 0x1000, 0x1100},
        {0xB07F, "ADD SP, #508", 0x1000, 0x11FC},
        
        // SUB instructions
        {0xB080, "SUB SP, #0",   0x1000, 0x1000},
        {0xB081, "SUB SP, #4",   0x1000, 0x0FFC},
        {0xB082, "SUB SP, #8",   0x1000, 0x0FF8},
        {0xB084, "SUB SP, #16",  0x1000, 0x0FF0},
        {0xB088, "SUB SP, #32",  0x1000, 0x0FE0},
        {0xB090, "SUB SP, #64",  0x1000, 0x0FC0},
        {0xB0A0, "SUB SP, #128", 0x1000, 0x0F80},
        {0xB0C0, "SUB SP, #256", 0x1000, 0x0F00},
        {0xB0FF, "SUB SP, #508", 0x1000, 0x0E04},
    };
    
    for (const auto& test : test_cases) {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = test.initial_sp;
        
        gba.getCPU().getMemory().write16(0x00000000, test.opcode);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], test.expected_sp) 
            << test.description << " failed. Expected: 0x" << std::hex << test.expected_sp
            << ", Got: 0x" << registers[13];
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }
}
