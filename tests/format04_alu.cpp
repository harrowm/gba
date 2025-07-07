#include "test_cpu_common.h"

// ARM Thumb Format 4: ALU operations
// Encoding: 010000[Op][Rs][Rd]
// Instructions: AND, EOR, LSL, LSR, ASR, ADC, SBC, ROR, TST, NEG, CMP, CMN, ORR, MUL, BIC, MVN

TEST(Format4, ALU_AND) {
    std::string beforeState;

    // Test case 1: Basic AND operation
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0xFF00FF00;
        registers[1] = 0xF0F0F0F0;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4008); // AND R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xF000F000));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // C is unaffected by AND
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: AND resulting in zero
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0xAAAAAAAA;
        registers[3] = 0x55555555;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x401A); // AND R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: AND with all bits set
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x12345678;
        registers[5] = 0xFFFFFFFF;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x402C); // AND R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x12345678)); // Should remain unchanged
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_EOR) {
    std::string beforeState;

    // Test case 1: Basic XOR operation
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0xFF00FF00;
        registers[1] = 0xF0F0F0F0;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4048); // EOR R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x0FF00FF0));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: XOR with itself (should result in zero)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x12345678;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4052); // EOR R2, R2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: XOR with negative result
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[3] = 0x7FFFFFFF;
        registers[4] = 0xFFFFFFFF;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4063); // EOR R3, R4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x80000000));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(Format4, ALU_LSL) {
    std::string beforeState;

    // Test case 1: Simple left shift
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00000001;
        registers[1] = 2; // Shift by 2
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4088); // LSL R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00000004));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Shift with carry out
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x80000000;
        registers[3] = 1; // Shift by 1
        
        gba.getCPU().getMemory().write16(0x00000000, 0x409A); // LSL R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 31 shifted out
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Shift by 0 (no change)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x12345678;
        registers[5] = 0; // Shift by 0
        
        gba.getCPU().getMemory().write16(0x00000000, 0x40AC); // LSL R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x12345678));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_LSR) {
    std::string beforeState;

    // Test case 1: Simple right shift
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00000004;
        registers[1] = 2; // Shift by 2
        
        gba.getCPU().getMemory().write16(0x00000000, 0x40C8); // LSR R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00000001));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Shift with carry out
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x00000001;
        registers[3] = 1; // Shift by 1
        
        gba.getCPU().getMemory().write16(0x00000000, 0x40DA); // LSR R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit 0 shifted out
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Logical shift of negative number
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x80000000;
        registers[5] = 1; // Shift by 1
        
        gba.getCPU().getMemory().write16(0x00000000, 0x40EC); // LSR R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x40000000));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Result is positive
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_ASR) {
    std::string beforeState;

    // Test case 1: Arithmetic shift of positive number
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x7FFFFFFC;
        registers[1] = 2; // Shift by 2
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4108); // ASR R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x1FFFFFFF));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // ASR carry behavior may differ
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Arithmetic shift of negative number
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x80000000;
        registers[3] = 1; // Shift by 1
        
        gba.getCPU().getMemory().write16(0x00000000, 0x411A); // ASR R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xC0000000)); // Sign extended
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Still negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Shift resulting in -1
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x80000001;
        registers[5] = 31; // Large shift
        
        gba.getCPU().getMemory().write16(0x00000000, 0x412C); // ASR R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0xFFFFFFFF)); // -1
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_TST) {
    std::string beforeState;

    // Test case 1: TST with non-zero result
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0xFF00FF00;
        registers[1] = 0xF0F0F0F0;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4208); // TST R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // TST doesn't modify Rd, only sets flags
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xFF00FF00)); // Unchanged
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // F000F000 != 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        validateUnchangedRegisters(cpu, beforeState, {15}); // No registers should change
    }

    // Test case 2: TST with zero result
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0xAAAAAAAA;
        registers[3] = 0x55555555;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x421A); // TST R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xAAAAAAAA)); // Unchanged
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // Result is zero
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {15}); // No registers should change
    }
}

TEST(Format4, ALU_NEG) {
    std::string beforeState;

    // Test case 1: Negate positive number
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00000001;
        registers[1] = 0; // NEG uses Rs as source, Rd as destination
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4248); // NEG R0, R1 (R0 = -R1, but R1=0, so R0 = 0)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0)); // -0 = 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // NEG of 0 sets carry
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Negate to get negative result
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0; // Will be overwritten
        registers[3] = 0x00000001;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x425A); // NEG R2, R3 (R2 = -R3)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xFFFFFFFF)); // -1
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Negate maximum positive value (overflow case)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0; // Will be overwritten
        registers[5] = 0x80000000; // Most negative number
        
        gba.getCPU().getMemory().write16(0x00000000, 0x426C); // NEG R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x80000000)); // -(-2147483648) overflows to same value
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Overflow occurred
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_CMP) {
    std::string beforeState;

    // Test case 1: Compare equal values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x12345678;
        registers[1] = 0x12345678;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4288); // CMP R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // CMP doesn't modify registers, only sets flags
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678)); // Unchanged
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // Equal values
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        validateUnchangedRegisters(cpu, beforeState, {15}); // No registers should change
    }

    // Test case 2: Compare with first operand smaller
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x00000001;
        registers[3] = 0x00000002;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x429A); // CMP R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x00000001)); // Unchanged
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Not equal
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Negative result (1-2 = -1)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow occurred
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: Compare with first operand larger
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x00000002;
        registers[5] = 0x00000001;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x42AC); // CMP R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x00000002)); // Unchanged
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Not equal
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Positive result
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}

TEST(Format4, ALU_ORR) {
    std::string beforeState;

    // Test case 1: Basic OR operation
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0xFF00FF00;
        registers[1] = 0x00FF00FF;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4308); // ORR R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xFFFFFFFF));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: OR with zero (no change)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x12345678;
        registers[3] = 0x00000000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x431A); // ORR R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x12345678)); // Unchanged
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: OR resulting in zero (both operands zero)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x00000000;
        registers[5] = 0x00000000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x432C); // ORR R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_MUL) {
    std::string beforeState;

    // Test case 1: Basic multiplication
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 6;
        registers[1] = 7;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4348); // MUL R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(42));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Multiplication resulting in zero
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0;
        registers[3] = 0x12345678;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x435A); // MUL R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Multiplication with negative result (due to overflow)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x10000;
        registers[5] = 0x10000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x436C); // MUL R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0)); // Overflow wraps to 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_BIC) {
    std::string beforeState;

    // Test case 1: Basic bit clear operation
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0xFFFFFFFF;
        registers[1] = 0xF0F0F0F0;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4388); // BIC R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x0F0F0F0F)); // Clear bits set in R1
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: BIC resulting in zero
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x12345678;
        registers[3] = 0xFFFFFFFF;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x439A); // BIC R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0)); // All bits cleared
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: BIC with no bits to clear
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x12345678;
        registers[5] = 0x00000000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x43AC); // BIC R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x12345678)); // No change
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_MVN) {
    std::string beforeState;

    // Test case 1: Move NOT of positive value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0; // Will be overwritten
        registers[1] = 0x00000000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x43C8); // MVN R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xFFFFFFFF)); // NOT 0 = 0xFFFFFFFF
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Move NOT resulting in zero
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0; // Will be overwritten
        registers[3] = 0xFFFFFFFF;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x43DA); // MVN R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x00000000)); // NOT 0xFFFFFFFF = 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Move NOT of pattern
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0; // Will be overwritten
        registers[5] = 0xAAAAAAAA;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x43EC); // MVN R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x55555555)); // NOT 0xAAAAAAAA = 0x55555555
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_ADC) {
    std::string beforeState;

    // Test case 1: ADC with carry clear
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T; // Carry clear
        registers.fill(0);
        
        registers[0] = 5;
        registers[1] = 7;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4148); // ADC R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(12)); // 5 + 7 + 0 = 12
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: ADC with carry set
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_C; // Carry set
        registers.fill(0);
        
        registers[2] = 5;
        registers[3] = 7;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x415A); // ADC R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(13)); // 5 + 7 + 1 = 13
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: ADC with overflow and carry out
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_C; // Carry set
        registers.fill(0);
        
        registers[4] = 0xFFFFFFFF;
        registers[5] = 0x00000001;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x416C); // ADC R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(1)); // 0xFFFFFFFF + 1 + 1 = 1 (with carry out)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry out
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_SBC) {
    std::string beforeState;

    // Test case 1: SBC with carry set (no borrow)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_C; // Carry set (no borrow)
        registers.fill(0);
        
        registers[0] = 10;
        registers[1] = 3;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4188); // SBC R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(7)); // 10 - 3 - 0 = 7
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: SBC with carry clear (borrow)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T; // Carry clear (borrow)
        registers.fill(0);
        
        registers[2] = 10;
        registers[3] = 3;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x419A); // SBC R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(6)); // 10 - 3 - 1 = 6
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow for this result
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: SBC resulting in negative (borrow)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T; // Carry clear (borrow)
        registers.fill(0);
        
        registers[4] = 3;
        registers[5] = 10;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x41AC); // SBC R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // 3 - 10 - 1 = -8 = 0xFFFFFFF8
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0xFFFFFFF8));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Negative result
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow occurred
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_ROR) {
    std::string beforeState;

    // Test case 1: Simple rotate right
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x80000001;
        registers[1] = 1; // Rotate by 1
        
        gba.getCPU().getMemory().write16(0x00000000, 0x41C8); // ROR R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xC0000000)); // Bit 0 rotated to bit 31
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Bit rotated out to carry
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Rotate by 0 (no change)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x12345678;
        registers[3] = 0; // Rotate by 0
        
        gba.getCPU().getMemory().write16(0x00000000, 0x41DA); // ROR R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x12345678)); // No change
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Full rotation (32 bits)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x12345678;
        registers[5] = 32; // Full rotation
        
        gba.getCPU().getMemory().write16(0x00000000, 0x41EC); // ROR R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x12345678)); // Back to original
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format4, ALU_CMN) {
    std::string beforeState;

    // Test case 1: CMN with positive result
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 5;
        registers[1] = 7;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x42C8); // CMN R0, R1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // CMN doesn't modify registers, only sets flags (R0 + R1 = 12)
        ASSERT_EQ(registers[0], static_cast<uint32_t>(5)); // Unchanged
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // 5 + 7 = 12 != 0
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Positive result
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // No carry
        validateUnchangedRegisters(cpu, beforeState, {15}); // No registers should change
    }

    // Test case 2: CMN resulting in zero (negative + positive)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0xFFFFFFFF; // -1
        registers[3] = 0x00000001; // +1
        
        gba.getCPU().getMemory().write16(0x00000000, 0x42DA); // CMN R2, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xFFFFFFFF)); // Unchanged
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // -1 + 1 = 0
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry out
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: CMN with carry out and negative result
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[4] = 0x80000000; // Large negative
        registers[5] = 0x80000000; // Large negative
        
        gba.getCPU().getMemory().write16(0x00000000, 0x42EC); // CMN R4, R5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x80000000)); // Unchanged
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z)); // Result wraps to 0
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry out
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Overflow
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}
