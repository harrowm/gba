#include "test_cpu_common.h"

// ARM Thumb Format 18: Long branch with link
// Encoding: 1111[H][Offset] (two-instruction sequence)
// Instructions: BL

TEST(CPU, BL) {
    std::string beforeState;

    // Test case 1: Simple forward BL
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup: BL +4 (branch to 0x04, return address should be 0x04)
        gba.getCPU().getMemory().write16(0x00000000, 0xF000); // BL +4 high part (verified encoding)
        gba.getCPU().getMemory().write16(0x00000002, 0xF802); // BL +4 low part (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1); // Execute first part
        cpu.execute(1); // Execute second part
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000008)); // PC = 0x04 + (2*2)
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = PC + 1 (with Thumb bit)
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }

    // Test case 2: Backward BL
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup: Start at 0x100, BL -4
        registers[15] = 0x00000100;
        gba.getCPU().getMemory().write16(0x00000100, 0xF7FF); // BL -4 high part (verified encoding)
        gba.getCPU().getMemory().write16(0x00000102, 0xFFFE); // BL -4 low part (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1); // Execute first part
        cpu.execute(1); // Execute second part
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000100)); // PC = 0x104 + (-2*2)
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000105)); // LR = 0x104 + 1
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }

    // Test case 3: BL with larger offset
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup: BL +100
        gba.getCPU().getMemory().write16(0x00000000, 0xF000); // BL +100 high part (verified encoding)
        gba.getCPU().getMemory().write16(0x00000002, 0xF832); // BL +100 low part (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1); // Execute first part
        cpu.execute(1); // Execute second part
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000068)); // PC = 0x04 + (50*2)
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = 0x04 + 1
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }

    // Test case 4: BL preserves flags
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xF000); // BL +4 high part
        gba.getCPU().getMemory().write16(0x00000002, 0xF802); // BL +4 low part
        beforeState = serializeCPUState(cpu);
        cpu.execute(1); // Execute first part
        cpu.execute(1); // Execute second part
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000008)); // PC = 0x04 + (2*2)
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = 0x04 + 1
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }

    // Test case 5: BL with existing LR value (should overwrite)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[14] = 0xABCDEF01; // Existing LR value
        gba.getCPU().getMemory().write16(0x00000000, 0xF000); // BL +4 high part
        gba.getCPU().getMemory().write16(0x00000002, 0xF802); // BL +4 low part
        beforeState = serializeCPUState(cpu);
        cpu.execute(1); // Execute first part
        cpu.execute(1); // Execute second part
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000008)); // PC = 0x04 + (2*2)
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR overwritten
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }

    // Test case 6: BL zero offset
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xF000); // BL +0 high part (verified encoding)
        gba.getCPU().getMemory().write16(0x00000002, 0xF800); // BL +0 low part (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1); // Execute first part
        cpu.execute(1); // Execute second part
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000004)); // PC = 0x04 + (0*2)
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = 0x04 + 1
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }

    // Test case 7: BL large backward offset within memory bounds
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Start at 0x400, BL -100
        registers[15] = 0x00000400;
        gba.getCPU().getMemory().write16(0x00000400, 0xF7FF); // BL -100 high part (verified encoding)
        gba.getCPU().getMemory().write16(0x00000402, 0xFFCE); // BL -100 low part (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1); // Execute first part
        cpu.execute(1); // Execute second part
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x000003A0)); // PC = 0x404 + (-100) = 0x3A0
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000405)); // LR = 0x404 + 1
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }
}
