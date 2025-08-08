#include "test_cpu_common.h"

// ARM Thumb Format 2: Add/subtract
// Encoding: 00011[I][Op][Rn/Offset3][Rs][Rd]
// Instructions: ADD/SUB register, ADD/SUB immediate

TEST(CPU, ADD_REGISTER) {
    std::string beforeState;

    // Test case 1: Simple addition
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 5;
        registers[2] = 3;
        gba.getCPU().getMemory().write16(0x00000000, 0x1888); // ADD R0, R1, R2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(8)); // 5 + 3 = 8
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Addition resulting in zero
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 10;
        registers[3] = static_cast<uint32_t>(-10); // 0xFFFFFFF6
        gba.getCPU().getMemory().write16(0x00000000, 0x18C1); // ADD R1, R0, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0)); // 10 + (-10) = 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry out from unsigned addition
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: Addition resulting in negative
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x80000000; // Large negative value (-2147483648)
        registers[4] = 1; // Small positive value
        gba.getCPU().getMemory().write16(0x00000000, 0x191A); // ADD R2, R3, R4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x80000001)); // -2147483648 + 1 = -2147483647
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z)); // Result is not zero
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // No unsigned carry
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: Addition with carry out (unsigned overflow)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0xFFFFFFFF;
        registers[6] = 1;
        gba.getCPU().getMemory().write16(0x00000000, 0x19AB); // ADD R3, R5, R6
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0)); // 0xFFFFFFFF + 1 = 0 (with carry)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry flag set
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 5: Addition with signed overflow (positive + positive = negative)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x7FFFFFFF; // Maximum positive signed int
        registers[0] = 1;
        gba.getCPU().getMemory().write16(0x00000000, 0x183C); // ADD R4, R7, R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x80000000)); // 0x7FFFFFFF + 1 = 0x80000000
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // No unsigned carry
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Signed overflow occurred
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 6: Addition with both carry and overflow (negative + negative = positive)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x80000000; // Minimum negative signed int
        registers[2] = 0x80000000; // Minimum negative signed int
        gba.getCPU().getMemory().write16(0x00000000, 0x1888); // ADD R0, R1, R2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0)); // 0x80000000 + 0x80000000 = 0 (with carry and overflow)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Unsigned carry
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Signed overflow
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 7: Addition with maximum values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0xFFFFFFFF;
        registers[3] = 0xFFFFFFFF;
        gba.getCPU().getMemory().write16(0x00000000, 0x18C1); // ADD R1, R0, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0xFFFFFFFE)); // 0xFFFFFFFF + 0xFFFFFFFF = 0xFFFFFFFE (with carry)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // Carry flag set
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow (negative + negative = negative)
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 8: Addition with same register (Rd = Rs case)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 100;
        registers[4] = 50;
        gba.getCPU().getMemory().write16(0x00000000, 0x191B); // ADD R3, R3, R4 (Rs = Rd)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(150)); // 100 + 50 = 150
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(CPU, SUB_REGISTER) {
    std::string beforeState;

    // Test case 1: Simple subtraction
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 8;
        registers[2] = 3;
        gba.getCPU().getMemory().write16(0x00000000, 0x1A88); // SUB R0, R1, R2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(5)); // 8 - 3 = 5
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Subtraction resulting in zero
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 10;
        registers[3] = 10;
        gba.getCPU().getMemory().write16(0x00000000, 0x1AC1); // SUB R1, R0, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0)); // 10 - 10 = 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: Subtraction resulting in negative (borrow)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 5; // Small positive value
        registers[4] = 10; // Larger positive value
        gba.getCPU().getMemory().write16(0x00000000, 0x1B1A); // SUB R2, R3, R4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xFFFFFFFB)); // 5 - 10 = -5 (0xFFFFFFFB)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow occurred
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: Subtraction with no borrow (positive result)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0xFFFFFFFF;
        registers[6] = 1;
        gba.getCPU().getMemory().write16(0x00000000, 0x1BAB); // SUB R3, R5, R6
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0xFFFFFFFE)); // 0xFFFFFFFF - 1 = 0xFFFFFFFE
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative (in signed interpretation)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 5: Subtraction with signed overflow (negative - positive = positive)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x80000000; // Minimum negative signed int (-2147483648)
        registers[0] = 1;
        gba.getCPU().getMemory().write16(0x00000000, 0x1A3C); // SUB R4, R7, R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x7FFFFFFF)); // 0x80000000 - 1 = 0x7FFFFFFF
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Result is positive
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Signed overflow occurred
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 6: Subtraction with borrow and no overflow
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0; // Zero
        registers[2] = 1; // One
        gba.getCPU().getMemory().write16(0x00000000, 0x1A88); // SUB R0, R1, R2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xFFFFFFFF)); // 0 - 1 = -1 (0xFFFFFFFF)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow occurred
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 7: Subtraction with maximum values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0xFFFFFFFF;
        registers[3] = 0xFFFFFFFF;
        gba.getCPU().getMemory().write16(0x00000000, 0x1AC1); // SUB R1, R0, R3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0)); // 0xFFFFFFFF - 0xFFFFFFFF = 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 8: Subtraction with same register (Rd = Rs case)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 100;
        registers[4] = 30;
        gba.getCPU().getMemory().write16(0x00000000, 0x1B1B); // SUB R3, R3, R4 (Rs = Rd)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(70)); // 100 - 30 = 70
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test cases for ADD/SUB immediate (also part of Format 2)
    
    // ADD immediate test case
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 5;
        gba.getCPU().getMemory().write16(0x00000000, 0x1C88); // ADD R0, R1, #2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(7)); // 5 + 2 = 7
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }
    
    // SUB immediate test case
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 8;
        gba.getCPU().getMemory().write16(0x00000000, 0x1E88); // SUB R0, R1, #2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(6)); // 8 - 2 = 6
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }
}
