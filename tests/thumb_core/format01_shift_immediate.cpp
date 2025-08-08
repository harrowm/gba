#include "test_cpu_common.h"

// ARM Thumb Format 1: Move shifted register
// Encoding: 000[op][offset5][Rs][Rd]
// Instructions: LSL, LSR, ASR

TEST(CPU, LSL) {
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    std::string beforeState;

    // Test case 1: Simple shift
    registers[0] = 0b1;
    gba.getCPU().getMemory().write16(0x00000000, 0x0080); // LSL R0, #2
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[0], static_cast<unsigned int>(0b100));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // V flag is unaffected
    validateUnchangedRegisters(cpu, beforeState, {0, 15});

    // Test case 2: Shift resulting in negative, with carry out
    registers[1] = 0xC0000000;
    gba.getCPU().getMemory().write16(0x00000002, 0x0049); // LSL R1, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[1], static_cast<unsigned int>(0x80000000));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 31 was shifted out
    validateUnchangedRegisters(cpu, beforeState, {1, 15});

    // Test case 3: Shift resulting in zero
    registers[2] = 0x80000000;
    gba.getCPU().getMemory().write16(0x00000004, 0x0052); // LSL R2, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[2], static_cast<unsigned int>(0));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 31 was shifted out
    validateUnchangedRegisters(cpu, beforeState, {2, 15});

    // Test case 4: Shift by 0
    registers[3] = 0xABCD;
    cpu.CPSR() |= CPU::FLAG_C; // Pre-set carry flag
    gba.getCPU().getMemory().write16(0x00000006, 0x001B); // LSL R3, #0
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[3], static_cast<unsigned int>(0xABCD));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // C flag is not affected
    validateUnchangedRegisters(cpu, beforeState, {3, 15});

    // Test case 5: Max shift
    registers[4] = 0b11;
    gba.getCPU().getMemory().write16(0x00000008, 0x07E4); // LSL R4, #31
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[4], static_cast<unsigned int>(1 << 31));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 0 of original value is 1
    validateUnchangedRegisters(cpu, beforeState, {4, 15});
}

TEST(CPU, LSR) {
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    std::string beforeState;

    // Test case 1: Simple shift
    registers[0] = 0b100;
    gba.getCPU().getMemory().write16(0x00000000, 0x0880); // LSR R0, #2
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[0], static_cast<uint32_t>(0b1));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Bit 1 was 0
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // V flag is unaffected
    validateUnchangedRegisters(cpu, beforeState, {0, 15});

    // Test case 2: Shift with carry out
    registers[1] = 0b101;
    gba.getCPU().getMemory().write16(0x00000002, 0x0849); // LSR R1, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[1], static_cast<uint32_t>(0b10));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 0 was 1
    validateUnchangedRegisters(cpu, beforeState, {1, 15});

    // Test case 3: Shift resulting in zero
    registers[2] = 0b1;
    gba.getCPU().getMemory().write16(0x00000004, 0x0852); // LSR R2, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[2], static_cast<uint32_t>(0));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 0 was 1
    validateUnchangedRegisters(cpu, beforeState, {2, 15});

    // Test case 4: Shift by 0 (special case, treated as LSR #32)
    registers[3] = 0x80000000;
    cpu.CPSR() &= ~CPU::FLAG_C; // Pre-clear carry flag
    gba.getCPU().getMemory().write16(0x00000006, 0x081B); // LSR R3, #0 -> LSR R3, #32
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[3], static_cast<uint32_t>(0));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 31 was 1
    validateUnchangedRegisters(cpu, beforeState, {3, 15});

    // Test case 5: Max shift
    registers[4] = 0xFFFFFFFF;
    gba.getCPU().getMemory().write16(0x00000008, 0x0FE4); // LSR R4, #31
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[4], static_cast<uint32_t>(1));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 30 was 1
    validateUnchangedRegisters(cpu, beforeState, {4, 15});
}

TEST(CPU, ASR) {
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    std::string beforeState;

    // Test case 1: Simple shift
    registers[0] = 0b100;
    gba.getCPU().getMemory().write16(0x00000000, 0x1080); // ASR R0, #2
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[0], static_cast<uint32_t>(0b1));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Bit 1 was 0
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // V flag is unaffected
    validateUnchangedRegisters(cpu, beforeState, {0, 15});

    // Test case 2: Shift with carry out
    registers[1] = 0b101;
    gba.getCPU().getMemory().write16(0x00000002, 0x1049); // ASR R1, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[1], static_cast<uint32_t>(0b10));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 0 was 1
    validateUnchangedRegisters(cpu, beforeState, {1, 15});

    // Test case 3: Shift resulting in zero
    registers[2] = 0b1;
    gba.getCPU().getMemory().write16(0x00000004, 0x1052); // ASR R2, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[2], static_cast<uint32_t>(0));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 0 was 1
    validateUnchangedRegisters(cpu, beforeState, {2, 15});

    // Test case 4: Shift by 0 (special case, treated as ASR #32)
    registers[3] = 0x80000000;
    cpu.CPSR() &= ~CPU::FLAG_C; // Pre-clear carry flag
    gba.getCPU().getMemory().write16(0x00000006, 0x101B); // ASR R3, #0 -> ASR R3, #32
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[3], static_cast<uint32_t>(0xFFFFFFFF)); // Sign-extended
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 31 was 1
    validateUnchangedRegisters(cpu, beforeState, {3, 15});

    // Test case 5: Max shift
    registers[4] = 0xFFFFFFFF;
    gba.getCPU().getMemory().write16(0x00000008, 0x17E4); // ASR R4, #31
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[4], static_cast<uint32_t>(0xFFFFFFFF)); // Sign-extended
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 0 was 1
    validateUnchangedRegisters(cpu, beforeState, {4, 15});
}
