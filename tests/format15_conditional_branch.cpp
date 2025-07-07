#include "test_cpu_common.h"

// ARM Thumb Format 15: Conditional branch
// Encoding: 1101[Cond][SOffset8]
// Instructions: Bcc (conditional branch)

TEST(CPU, B_COND) {
    std::string beforeState;

    // Test case 1: BEQ taken (Z flag set)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z; // Set Z flag
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD001); // BEQ +2 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000004)); // Branch taken: PC = 0x02 + (1*2)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // Flags preserved
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: BEQ not taken (Z flag clear)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Z flag clear
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD001); // BEQ +2 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000002)); // Branch not taken: PC = 0x02
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: BNE taken (Z flag clear)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Z flag clear
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD102); // BNE +4 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000006)); // Branch taken: PC = 0x02 + (2*2)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: BNE not taken (Z flag set)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z; // Set Z flag
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD102); // BNE +4 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000002)); // Branch not taken: PC = 0x02
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: BMI taken (N flag set)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_N; // Set N flag
        
        registers[15] = 0x00000010;
        gba.getCPU().getMemory().write16(0x00000010, 0xD4FF); // BMI -2 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000010)); // Branch taken: PC = 0x12 + (-1*2)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Flags preserved
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 6: BPL taken (N flag clear)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // N flag clear
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD503); // BPL +6 (verified encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000008)); // Branch taken: PC = 0x02 + (3*2)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 7: BCS taken (C flag set)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_C; // Set C flag
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD204); // BCS +8 
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x0000000A)); // Branch taken: PC = 0x02 + (4*2)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Flags preserved
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 8: BCC taken (C flag clear)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // C flag clear
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD305); // BCC +10
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x0000000C)); // Branch taken: PC = 0x02 + (5*2)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 9: BVS taken (V flag set)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_V; // Set V flag
        
        gba.getCPU().getMemory().write16(0x00000000, 0xD603); // BVS +6
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000008)); // Branch taken: PC = 0x02 + (3*2)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Flags preserved
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 10: Multiple flags combination - BGE (N == V)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_N | CPU::FLAG_V; // N=1, V=1 (N==V)
        
        gba.getCPU().getMemory().write16(0x00000000, 0xDA02); // BGE +4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000006)); // Branch taken: PC = 0x02 + (2*2)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 11: BGE not taken (N != V)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_N; // N=1, V=0 (N!=V)
        
        gba.getCPU().getMemory().write16(0x00000000, 0xDA02); // BGE +4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000002)); // Branch not taken: PC = 0x02
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 12: Large backward conditional branch
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z; // Set condition for BEQ
        
        registers[15] = 0x00000200;
        gba.getCPU().getMemory().write16(0x00000200, 0xD080); // BEQ -256 (max backward)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000102)); // PC = 0x202 + (-128*2)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}
