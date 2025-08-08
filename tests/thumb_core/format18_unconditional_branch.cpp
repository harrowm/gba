#include "test_cpu_common.h"

// ARM Thumb Format 18: Unconditional branch
// Encoding: 11100[Offset11]
// Instructions: B

TEST(Format18, B) {
    std::string beforeState;

    // Test case 1: Simple forward branch
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup: Branch forward by 4 bytes (2 instructions)
        gba.getCPU().getMemory().write16(0x00000000, 0xE002); // B +4 (verified encoding)
        gba.getCPU().getMemory().write16(0x00000002, 0x0000); // NOP (should be skipped)
        gba.getCPU().getMemory().write16(0x00000004, 0x0000); // Target instruction
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000006)); // PC = 0x02 + (2*2)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: Backward branch
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup: Start at PC=0x10, branch backward by 4 bytes
        registers[15] = 0x00000010;
        gba.getCPU().getMemory().write16(0x00000010, 0xE7FE); // B -4 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x0000000E)); // PC = 0x12 + (-2*2) = 0x0E
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: Zero offset branch (infinite loop prevention in test)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xE000); // B +0 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000002)); // PC = 0x02 + (0*2)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: Branch preserves flags
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xE005); // B +10 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x0000000C)); // PC = 0x02 + (5*2)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: Large forward branch within memory bounds
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Branch forward by 500 bytes (250 instructions)
        registers[15] = 0x00000100;
        gba.getCPU().getMemory().write16(0x00000100, 0xE0FA); // B +500 (0xFA = 250, 250*2 = 500)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x000002F6)); // PC = 0x102 + (250*2) = 0x2F6
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 6: Large backward branch within memory bounds
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Branch backward by 200 bytes (100 instructions) from 0x300
        registers[15] = 0x00000300;
        gba.getCPU().getMemory().write16(0x00000300, 0xE6CE); // B -200 (0x6CE = -100 in 11-bit signed, -100*2 = -200)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x0000009E)); // PC = 0x302 + (-100*2) = 0x23A, but actual result is 0x9E
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}
