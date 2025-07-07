#include "test_cpu_common.h"

// ARM Thumb Format 3: Move/compare/add/subtract immediate
// Encoding: 001[Op][Rd][Offset8]
// Instructions: MOV, CMP, ADD, SUB with 8-bit immediate

TEST(CPU, MOV_IMM) {
    // Setup test GBA with test RAM region
    GBA gba(true); // Pass true to indicate test mode

    // Initialize CPU state
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0); // Reset all registers to zero
    cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode

    // Test MOV R0, #1
    gba.getCPU().getMemory().write16(0x00000000, 0x2001); // MOV R0, #1
    std::string beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[0], static_cast<unsigned int>(1));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(2)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {0, 15});

    // Test MOV R1, #255
    gba.getCPU().getMemory().write16(0x00000002, 0x21FF); // MOV R1, #255
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[1], static_cast<unsigned int>(255));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(4)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {1, 15});

    // Test MOV R2, #0
    gba.getCPU().getMemory().write16(0x00000004, 0x2200); // MOV R2, #0
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[2], static_cast<unsigned int>(0));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(6)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {2, 15});

    // Test MOV R3, #42
    gba.getCPU().getMemory().write16(0x00000006, 0x232A); // MOV R3, #42 (42 in decimal is 0x2A in hex)
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[3], static_cast<unsigned int>(42));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(8)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {3, 15});

    // Test MOV R4, #127
    gba.getCPU().getMemory().write16(0x00000008, 0x247F); // MOV R4, #127
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[4], static_cast<unsigned int>(127));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(10)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {4, 15});

    // Test MOV R5, #0xFF
    gba.getCPU().getMemory().write16(0x0000000A, 0x25FF); // MOV R5, #255
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[5], static_cast<unsigned int>(255));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(12)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {5, 15});

    // Test MOV R6, #0x00
    gba.getCPU().getMemory().write16(0x0000000C, 0x2600); // MOV R6, #0
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[6], static_cast<unsigned int>(0));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(14)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {6, 15});

    // Test MOV R7, #0x80
    gba.getCPU().getMemory().write16(0x0000000E, 0x2780); // MOV R7, #128
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[7], static_cast<unsigned int>(128));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(16)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {7, 15});

    // Test MOV R7, #0x80 - test NCV flag not altered
    cpu.CPSR() |= CPU::FLAG_N; // set N
    cpu.CPSR() |= CPU::FLAG_C; // set C
    cpu.CPSR() |= CPU::FLAG_V; // set V
    
    gba.getCPU().getMemory().write16(0x00000010, 0x2780); // MOV R7, #128
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[7], static_cast<unsigned int>(128));
    ASSERT_EQ(registers[15], static_cast<unsigned int>(18)); // PC should increment by 2
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // N flag is cleared by MOV
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // C flag is preserved
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));  // V flag is preserved
    validateUnchangedRegisters(cpu, beforeState, {7, 15});
}

TEST(CPU, CMP_IMM) {
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode

    // Test case 1: Zero result
    registers[0] = 5; // R0
    gba.getCPU().getMemory().write16(0x00000000, 0x2805); // CMP R0, #5
    std::string beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {15});

    // Test case 2: Negative result
    registers[1] = 0; // R1
    gba.getCPU().getMemory().write16(0x00000002, 0x2901); // CMP R1, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));  // Negative flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Carry flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {15});

    // Test case 3: Carry set
    registers[2] = 10; // R2
    gba.getCPU().getMemory().write16(0x00000004, 0x2A05); // CMP R2, #5
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // Carry flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {15});

    // Test case 4: Overflow
    registers[3] = 0x80000000; // R3 (minimum signed int)
    gba.getCPU().getMemory().write16(0x00000006, 0x2BFF); // CMP R3, #255
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));  // Negative flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // Carry flag should be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));  // Overflow flag should be set
    validateUnchangedRegisters(cpu, beforeState, {15});

    // Test case 5: Boundary values (maximum value)
    registers[4] = 0xFFFFFFFF; // R4
    gba.getCPU().getMemory().write16(0x00000008, 0x2CFF); // CMP R4, #255
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // Carry flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // V flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {15});
}

TEST(CPU, ADD_IMM) {
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode

    // Test case 1: Simple addition
    registers[0] = 5; // R0
    gba.getCPU().getMemory().write16(0x00000000, 0x3005); // ADD R0, #5
    std::string beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[0], static_cast<unsigned int>(10)); // R0 = 5 + 5
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Carry flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {0, 15});

    // Test case 2: Addition resulting in negative
    registers[1] = 0xFFFFFFF0; // R1
    gba.getCPU().getMemory().write16(0x00000002, 0x310F); // ADD R1, #15
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[1], static_cast<unsigned int>(0xFFFFFFFF)); // R1 = 0xFFFFFFF0 + 15
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Carry flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {1, 15});

    // Test case 3: Addition resulting in zero
    registers[2] = 0; // R2
    gba.getCPU().getMemory().write16(0x00000004, 0x3200); // ADD R2, #0
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[2], static_cast<unsigned int>(0)); // R2 = 0 + 0
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Carry flag should not be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {2, 15});

    // Test case 4: Addition with overflow
    registers[3] = 0x7FFFFFFF; // R3 (maximum positive signed int)
    gba.getCPU().getMemory().write16(0x00000006, 0x3301); // ADD R3, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[3], static_cast<unsigned int>(0x80000000)); // R3 = 0x7FFFFFFF + 1
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Carry flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should be set
    validateUnchangedRegisters(cpu, beforeState, {3, 15});

    // Test case 5: Addition with carry
    registers[4] = 0xFFFFFFFF; // R4
    gba.getCPU().getMemory().write16(0x00000008, 0x3401); // ADD R4, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[4], static_cast<unsigned int>(0)); // R4 = 0xFFFFFFFF + 1
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // Zero flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Negative flag should not be set
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry flag should be set
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // Overflow flag should not be set
    validateUnchangedRegisters(cpu, beforeState, {4, 15});
}

TEST(CPU, SUB_IMM) {
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode

    // Test case 1: Simple subtraction (no borrow)
    registers[0] = 10;
    gba.getCPU().getMemory().write16(0x00000000, 0x3805); // SUB R0, #5
    std::string beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[0], static_cast<unsigned int>(5));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {0, 15});

    // Test case 2: Subtraction resulting in zero
    registers[1] = 5;
    gba.getCPU().getMemory().write16(0x00000002, 0x3905); // SUB R1, #5
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[1], static_cast<unsigned int>(0));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {1, 15});

    // Test case 3: Subtraction resulting in negative (borrow)
    registers[2] = 5;
    gba.getCPU().getMemory().write16(0x00000004, 0x3A0A); // SUB R2, #10
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[2], static_cast<unsigned int>(0xFFFFFFFB));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow occurred
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {2, 15});

    // Test case 4: Subtraction with overflow
    registers[3] = 0x80000000; // -2147483648
    gba.getCPU().getMemory().write16(0x00000006, 0x3B01); // SUB R3, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[3], static_cast<unsigned int>(0x7FFFFFFF)); // Result is 2147483647
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Overflow
    validateUnchangedRegisters(cpu, beforeState, {3, 15});

    // Test case 5: Boundary (no borrow)
    registers[4] = 0xFFFFFFFF;
    gba.getCPU().getMemory().write16(0x00000008, 0x3C01); // SUB R4, #1
    beforeState = serializeCPUState(cpu);
    cpu.execute(1);
    ASSERT_EQ(registers[4], static_cast<unsigned int>(0xFFFFFFFE));
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
    ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
    ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
    validateUnchangedRegisters(cpu, beforeState, {4, 15});
}
