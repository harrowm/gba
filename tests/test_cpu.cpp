#include "gtest/gtest.h"
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"

std::string serializeCPUState(const CPU& cpu) {
    std::ostringstream oss;
    const auto& registers = cpu.R();
    for (size_t i = 0; i < registers.size(); ++i) {
        oss << "R" << i << ":" << registers[i] << ";";
    }
    oss << "CPSR:" << cpu.CPSR();
    return oss.str();
}

// Helper function to validate unchanged registers
void validateUnchangedRegisters(const CPU& cpu, const std::string& beforeState, const std::set<int>& changedRegisters) {
    const auto& registers = cpu.R();
    std::istringstream iss(beforeState);
    std::string token;

    // Deserialize the before state and compare unchanged registers
    for (size_t i = 0; i < registers.size(); ++i) {
        std::getline(iss, token, ';');
        if (changedRegisters.find(i) == changedRegisters.end()) {
            ASSERT_EQ(token, "R" + std::to_string(i) + ":" + std::to_string(registers[i]));
        }
    }
}

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
}

TEST(CPU, ADD_OFFSET) {
    std::string beforeState;

    // Test case 1: Simple addition with small offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 10;
        gba.getCPU().getMemory().write16(0x00000000, 0x1C48); // ADD R0, R1, #1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(11)); // 10 + 1 = 11
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
        
        registers[2] = static_cast<uint32_t>(-3); // 0xFFFFFFFD
        gba.getCPU().getMemory().write16(0x00000000, 0x1CD1); // ADD R1, R2, #3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0)); // -3 + 3 = 0
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
        gba.getCPU().getMemory().write16(0x00000000, 0x1C5A); // ADD R2, R3, #1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x80000001)); // -2147483648 + 1 = -2147483647
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
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
        
        registers[4] = 0xFFFFFFFC; // -4 unsigned, close to max
        gba.getCPU().getMemory().write16(0x00000000, 0x1DE3); // ADD R3, R4, #7
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(3)); // 0xFFFFFFFC + 7 = 3 (with carry)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
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
        
        registers[5] = 0x7FFFFFFC; // Close to maximum positive signed int
        gba.getCPU().getMemory().write16(0x00000000, 0x1D6C); // ADD R4, R5, #5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x80000001)); // 0x7FFFFFFC + 5 = 0x80000001 (overflow)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // No unsigned carry
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Signed overflow occurred
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 6: Addition with maximum offset (7)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[6] = 0x10;
        gba.getCPU().getMemory().write16(0x00000000, 0x1DF5); // ADD R5, R6, #7
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x17)); // 0x10 + 7 = 0x17
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 7: Addition with offset 0 (edge case)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x42;
        gba.getCPU().getMemory().write16(0x00000000, 0x1C3E); // ADD R6, R7, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x42)); // 0x42 + 0 = 0x42
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 8: Addition with same register (Rd = Rs case)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 100;
        gba.getCPU().getMemory().write16(0x00000000, 0x1D00); // ADD R0, R0, #4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(104)); // 100 + 4 = 104
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 9: Addition creating zero with zero source
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0;
        gba.getCPU().getMemory().write16(0x00000000, 0x1C08); // ADD R0, R1, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0)); // 0 + 0 = 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 10: Addition with all different registers (Rs != Rd)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[2] = 0x12345678;
        gba.getCPU().getMemory().write16(0x00000000, 0x1D97); // ADD R7, R2, #6
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x1234567E)); // 0x12345678 + 6 = 0x1234567E
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }

    // Test case 11: Addition with boundary condition - near maximum signed positive value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x7FFFFFFF; // Maximum positive signed int
        gba.getCPU().getMemory().write16(0x00000000, 0x1C5A); // ADD R2, R3, #1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x80000000)); // 0x7FFFFFFF + 1 = 0x80000000
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // No unsigned carry
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Signed overflow occurred
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 12: Addition with minimum negative + large offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x80000000; // Minimum negative signed int (-2147483648)
        gba.getCPU().getMemory().write16(0x00000000, 0x1DE3); // ADD R3, R4, #7
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x80000007)); // 0x80000000 + 7 = 0x80000007
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is still negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // No unsigned carry
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow (negative + positive = negative)
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(CPU, SUB_OFFSET) {
    std::string beforeState;

    // Test case 1: Simple subtraction with small offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 10;
        gba.getCPU().getMemory().write16(0x00000000, 0x1E48); // SUB R0, R1, #1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(9)); // 10 - 1 = 9
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
        
        registers[2] = 3;
        gba.getCPU().getMemory().write16(0x00000000, 0x1ED1); // SUB R1, R2, #3
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0)); // 3 - 3 = 0
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
        
        registers[3] = 0; // Zero value
        gba.getCPU().getMemory().write16(0x00000000, 0x1E5A); // SUB R2, R3, #1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xFFFFFFFF)); // 0 - 1 = -1 (0xFFFFFFFF)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow occurred
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: Subtraction with no borrow (large value)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0xFFFFFFFF; // Maximum unsigned value
        gba.getCPU().getMemory().write16(0x00000000, 0x1FE3); // SUB R3, R4, #7
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0xFFFFFFF8)); // 0xFFFFFFFF - 7 = 0xFFFFFFF8
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
        
        registers[5] = 0x80000000; // Minimum negative signed int (-2147483648)
        gba.getCPU().getMemory().write16(0x00000000, 0x1F6C); // SUB R4, R5, #5
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x7FFFFFFB)); // 0x80000000 - 5 = 0x7FFFFFFB (overflow)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Result is positive
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V)); // Signed overflow occurred
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 6: Subtraction with maximum offset (7)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[6] = 0x10;
        gba.getCPU().getMemory().write16(0x00000000, 0x1FF5); // SUB R5, R6, #7
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x09)); // 0x10 - 7 = 0x09
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 7: Subtraction with offset 0 (edge case)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x42;
        gba.getCPU().getMemory().write16(0x00000000, 0x1E3E); // SUB R6, R7, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x42)); // 0x42 - 0 = 0x42
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 8: Subtraction with same register (Rd = Rs case)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 100;
        gba.getCPU().getMemory().write16(0x00000000, 0x1F00); // SUB R0, R0, #4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(96)); // 100 - 4 = 96
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 9: Subtraction creating zero with zero source
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0;
        gba.getCPU().getMemory().write16(0x00000000, 0x1E08); // SUB R0, R1, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0)); // 0 - 0 = 0
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 10: Subtraction with all different registers (Rs != Rd)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[2] = 0x12345678;
        gba.getCPU().getMemory().write16(0x00000000, 0x1F97); // SUB R7, R2, #6
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x12345672)); // 0x12345678 - 6 = 0x12345672
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }

    // Test case 11: Subtraction with boundary condition - near minimum signed value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x80000001; // Minimum signed int + 1 (-2147483647)
        gba.getCPU().getMemory().write16(0x00000000, 0x1E5A); // SUB R2, R3, #1
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x80000000)); // 0x80000001 - 1 = 0x80000000
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 12: Subtraction with minimum value and maximum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x80000007; // Minimum negative + 7
        gba.getCPU().getMemory().write16(0x00000000, 0x1FE3); // SUB R3, R4, #7
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x80000000)); // 0x80000007 - 7 = 0x80000000
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N)); // Result is negative
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C)); // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V)); // No signed overflow
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(CPU, ALU_OPERATIONS) {}

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

// Updated CMP_IMM test case to include beforeState and validateUnchangedRegisters for each test case.

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

TEST(CPU, ALU_AND) {}
TEST(CPU, ALU_EOR) {}
TEST(CPU, ALU_LSL) {}
TEST(CPU, ALU_LSR) {}
TEST(CPU, ALU_ASR) {}
TEST(CPU, ALU_ADC) {}
TEST(CPU, ALU_SBC) {}
TEST(CPU, ALU_ROR) {}
TEST(CPU, ALU_TST) {}
TEST(CPU, ALU_NEG) {}
TEST(CPU, ALU_CMP) {}
TEST(CPU, ALU_CMN) {}
TEST(CPU, ALU_ORR) {}
TEST(CPU, ALU_MUL) {}
TEST(CPU, ALU_BIC) {}
TEST(CPU, ALU_MVN) {}

TEST(CPU, LDR) {
    std::string beforeState;

    // Test case 1: Simple PC-relative load
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup test data in memory - PC will be 0x00000002 when instruction executes
        // LDR R0, [PC, #4] will access address (0x00000002 + 4) = 0x00000006 (aligned to 0x00000004)
        gba.getCPU().getMemory().write32(0x00000004, 0x12345678);
        gba.getCPU().getMemory().write16(0x00000000, 0x4801); // LDR R0, [PC, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678)); // Should load the test data
        // LDR doesn't affect CPSR flags
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load zero value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup zero data in memory
        gba.getCPU().getMemory().write32(0x00000008, 0x00000000);
        gba.getCPU().getMemory().write16(0x00000000, 0x4902); // LDR R1, [PC, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x00000000)); // Should load zero
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: Load maximum 32-bit value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup maximum value in memory
        gba.getCPU().getMemory().write32(0x0000000C, 0xFFFFFFFF);
        gba.getCPU().getMemory().write16(0x00000000, 0x4A03); // LDR R2, [PC, #12]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xFFFFFFFF)); // Should load max value
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: Load with minimum offset (0)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
    // Test case 4: Load with minimum offset (0)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
    // Test case 4: Load with minimum offset (0)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test loading with offset 0: place instruction at 0x100, data at 0x100
        // The instruction itself will be overwritten but that's what we're testing
        gba.getCPU().getMemory().write32(0x00000100, 0xABCDEF01); // Data at 0x100
        gba.getCPU().getMemory().write16(0x00000100, 0x4B00); // LDR R3, [PC, #0] - overwrites lower 16 bits
        registers[15] = 0x00000100; // Set PC to 0x100
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // After execute: PC=0x102, aligned to 0x100, offset 0 -> load from 0x100
        // The data at 0x100 is now 0xABCD4B00 (instruction overwrote lower 16 bits)
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0xABCD4B00)); 
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
    }
    }

    // Test case 5: Load with medium offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data at offset 16 bytes from PC
        gba.getCPU().getMemory().write32(0x00000010, 0x87654321);
        gba.getCPU().getMemory().write16(0x00000000, 0x4C04); // LDR R4, [PC, #16]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x87654321)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 6: Load with large offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data at offset 32 bytes from PC
        gba.getCPU().getMemory().write32(0x00000020, 0x13579BDF);
        gba.getCPU().getMemory().write16(0x00000000, 0x4D08); // LDR R5, [PC, #32]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x13579BDF)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 7: Load with very large offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data at offset 64 bytes from PC
        gba.getCPU().getMemory().write32(0x00000040, 0xFEDCBA98);
        gba.getCPU().getMemory().write16(0x00000000, 0x4E10); // LDR R6, [PC, #64]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0xFEDCBA98)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 8: Load to different registers with same offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data and load into R7
        gba.getCPU().getMemory().write32(0x00000008, 0x24681357);
        gba.getCPU().getMemory().write16(0x00000000, 0x4F02); // LDR R7, [PC, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x24681357)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }

    // Test case 9: Load signed negative value (test sign extension not applied)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup negative value (should be loaded as-is, no sign extension for 32-bit loads)
        gba.getCPU().getMemory().write32(0x00000004, 0x80000001);
        gba.getCPU().getMemory().write16(0x00000000, 0x4801); // LDR R0, [PC, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x80000001)); // Should load exactly as stored
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 10: Load with boundary pattern
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup alternating bit pattern
        gba.getCPU().getMemory().write32(0x0000000C, 0xAAAA5555);
        gba.getCPU().getMemory().write16(0x00000000, 0x4A03); // LDR R2, [PC, #12]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xAAAA5555)); // Should load the pattern
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 11: Load preserves existing flags
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
        
        // Setup test data
        gba.getCPU().getMemory().write32(0x00000008, 0x11223344);
        gba.getCPU().getMemory().write16(0x00000000, 0x4902); // LDR R1, [PC, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x11223344)); // Should load the data
        // Flags should be preserved (LDR doesn't modify flags)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 12: Load with PC alignment (PC is word-aligned in calculation)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Place instruction at word boundary + 2 to test PC alignment
        // When instruction executes, PC=0x00000004, PC&~3 = 0x00000004
        gba.getCPU().getMemory().write32(0x00000008, 0x55667788);
        gba.getCPU().getMemory().write16(0x00000002, 0x4801); // LDR R0, [PC, #4] at address 0x02
        
        // Set PC to the instruction location
        registers[15] = 0x00000002;
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x55667788)); // Should load from aligned PC + offset
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }
}

TEST(CPU, STR_WORD) {}

TEST(CPU, LDR_WORD) {
    std::string beforeState;

    // Test case 1: Simple word load with register offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup: R1 = base address, R2 = offset, load from [R1 + R2]
        registers[1] = 0x00000800; // Base address within memory range
        registers[2] = 0x00000004; // Offset
        gba.getCPU().getMemory().write32(0x00000804, 0x12345678); // Data at target address
        gba.getCPU().getMemory().write16(0x00000000, 0x5888); // LDR R0, [R1, R2] - 0101100010001000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load zero value
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00000800;
        registers[4] = 0x00000008;
        gba.getCPU().getMemory().write32(0x00000808, 0x00000000);
        gba.getCPU().getMemory().write16(0x00000000, 0x593C); // LDR R4, [R3, R4] - 0101100100111100
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 3: Load maximum 32-bit value
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00000800;
        registers[6] = 0x0000000C;
        gba.getCPU().getMemory().write32(0x0000080C, 0xFFFFFFFF);
        gba.getCPU().getMemory().write16(0x00000000, 0x59AE); // LDR R6, [R5, R6] - 0101100110101110
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0xFFFFFFFF));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 4: Load with zero offset
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000800;
        registers[1] = 0x00000000; // Zero offset
        gba.getCPU().getMemory().write32(0x00000800, 0xABCDEF01);
        gba.getCPU().getMemory().write16(0x00000000, 0x5841); // LDR R1, [R0, R1] - 0101100001000001
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0xABCDEF01));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 5: Load with different register combinations
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00000800;
        registers[0] = 0x00000010;
        gba.getCPU().getMemory().write32(0x00000810, 0x87654321);
        gba.getCPU().getMemory().write16(0x00000000, 0x5838); // LDR R0, [R7, R0] - 0101100000111000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 6: Load preserves flags
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        registers[2] = 0x00000800;
        registers[3] = 0x00000014;
        gba.getCPU().getMemory().write32(0x00000814, 0x11223344);
        gba.getCPU().getMemory().write16(0x00000000, 0x58D3); // LDR R3, [R2, R3] - 0101100011010011
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x11223344));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(CPU, LDR_BYTE) {
    std::string beforeState;

    // Test case 1: Simple byte load with register offset
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00000800;
        registers[2] = 0x00000003;
        gba.getCPU().getMemory().write8(0x00000803, 0xAB);
        gba.getCPU().getMemory().write16(0x00000000, 0x5C88); // LDRB R0, [R1, R2] - 0101110010001000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x000000AB));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load zero byte
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00000800;
        registers[4] = 0x00000001;
        gba.getCPU().getMemory().write8(0x00000801, 0x00);
        gba.getCPU().getMemory().write16(0x00000000, 0x5D24); // LDRB R4, [R3, R4] - 0101110100100100
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 3: Load maximum byte value (255)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00000800;
        registers[6] = 0x00000002;
        gba.getCPU().getMemory().write8(0x00000802, 0xFF);
        gba.getCPU().getMemory().write16(0x00000000, 0x5DAE); // LDRB R6, [R5, R6] - 0101110110101110
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x000000FF));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 4: Load with zero offset
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000800;
        registers[1] = 0x00000000;
        gba.getCPU().getMemory().write8(0x00000800, 0x55);
        gba.getCPU().getMemory().write16(0x00000000, 0x5C41); // LDRB R1, [R0, R1] - 0101110001000001
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x00000055));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 5: Load byte preserves flags
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        registers[7] = 0x00000800;
        registers[0] = 0x00000005;
        gba.getCPU().getMemory().write8(0x00000805, 0x99);
        gba.getCPU().getMemory().write16(0x00000000, 0x5C38); // LDRB R0, [R7, R0] - 0101110000111000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00000099));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }
}

TEST(CPU, STR_BYTE) {}

TEST(CPU, STRH) {}

TEST(CPU, LDSB) {
    std::string beforeState;

    // Test case 1: Load positive signed byte
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00000800;
        registers[2] = 0x00000001;
        gba.getCPU().getMemory().write8(0x00000801, 0x7F); // Maximum positive signed byte
        gba.getCPU().getMemory().write16(0x00000000, 0x5650); // LDSB R0, [R1, R2] - 0101011001010000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x0000007F));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load negative signed byte with sign extension
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00000800;
        registers[4] = 0x00000002;
        gba.getCPU().getMemory().write8(0x00000802, 0x80); // Minimum negative signed byte (-128)
        gba.getCPU().getMemory().write16(0x00000000, 0x571C); // LDSB R4, [R3, R4] - 0101011100011100
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0xFFFFFF80)); // Sign-extended
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 3: Load zero byte
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00000800;
        registers[6] = 0x00000003;
        gba.getCPU().getMemory().write8(0x00000803, 0x00);
        gba.getCPU().getMemory().write16(0x00000000, 0x57F6); // LDSB R6, [R5, R6] - 0101011111110110
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 4: Load negative byte -1 (0xFF)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000800;
        registers[1] = 0x00000004;
        gba.getCPU().getMemory().write8(0x00000804, 0xFF);
        gba.getCPU().getMemory().write16(0x00000000, 0x5641); // LDSB R1, [R0, R1] - 0101011001000001
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0xFFFFFFFF)); // Sign-extended -1
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 5: Load with different register combinations
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00000800;
        registers[0] = 0x00000005;
        gba.getCPU().getMemory().write8(0x00000805, 0x8A); // Negative value
        gba.getCPU().getMemory().write16(0x00000000, 0x5638); // LDSB R0, [R7, R0] - 0101011000111000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xFFFFFF8A)); // Sign-extended
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 6: LDSB preserves flags
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        registers[2] = 0x00000800;
        registers[3] = 0x00000006;
        gba.getCPU().getMemory().write8(0x00000806, 0x42);
        gba.getCPU().getMemory().write16(0x00000000, 0x56D3); // LDSB R3, [R2, R3] - 0101011011010011
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x00000042));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(CPU, LDRH) {
    std::string beforeState;

    // Test case 1: Simple halfword load
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00000800;
        registers[2] = 0x00000002;
        gba.getCPU().getMemory().write16(0x00000802, 0x1234);
        gba.getCPU().getMemory().write16(0x00000000, 0x5A50); // LDRH R0, [R1, R2] - 0101101001010000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00001234));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load zero halfword
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00000800;
        registers[4] = 0x00000004;
        gba.getCPU().getMemory().write16(0x00000804, 0x0000);
        gba.getCPU().getMemory().write16(0x00000000, 0x5B3C); // LDRH R4, [R3, R4] - 0101101100111100
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 3: Load maximum halfword value
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00000800;
        registers[6] = 0x00000006;
        gba.getCPU().getMemory().write16(0x00000806, 0xFFFF);
        gba.getCPU().getMemory().write16(0x00000000, 0x5BAE); // LDRH R6, [R5, R6] - 0101101110101110
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x0000FFFF)); // No sign extension
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 4: Load with zero offset
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000800;
        registers[1] = 0x00000000;
        gba.getCPU().getMemory().write16(0x00000800, 0xABCD);
        gba.getCPU().getMemory().write16(0x00000000, 0x5A41); // LDRH R1, [R0, R1] - 0101101001000001
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x0000ABCD));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 5: Load halfword with high bit set (no sign extension)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00000800;
        registers[0] = 0x00000008;
        gba.getCPU().getMemory().write16(0x00000808, 0x8000); // High bit set
        gba.getCPU().getMemory().write16(0x00000000, 0x5A38); // LDRH R0, [R7, R0] - 0101101000111000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00008000)); // No sign extension for LDRH
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 6: LDRH preserves flags
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        registers[2] = 0x00000800;
        registers[3] = 0x0000000A;
        gba.getCPU().getMemory().write16(0x0000080A, 0x5678);
        gba.getCPU().getMemory().write16(0x00000000, 0x5AD3); // LDRH R3, [R2, R3] - 0101101011010011
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x00005678));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(CPU, LDSH) {
    std::string beforeState;

    // Test case 1: Load positive signed halfword
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00000800;
        registers[2] = 0x00000002;
        gba.getCPU().getMemory().write16(0x00000802, 0x7FFF); // Maximum positive signed halfword
        gba.getCPU().getMemory().write16(0x00000000, 0x5E50); // LDSH R0, [R1, R2] - 0101111001010000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00007FFF));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load negative signed halfword with sign extension
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00000800;
        registers[4] = 0x00000004;
        gba.getCPU().getMemory().write16(0x00000804, 0x8000); // Minimum negative signed halfword (-32768)
        gba.getCPU().getMemory().write16(0x00000000, 0x5F1C); // LDSH R4, [R3, R4] - 0101111100011100
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0xFFFF8000)); // Sign-extended
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 3: Load zero halfword
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00000800;
        registers[6] = 0x00000006;
        gba.getCPU().getMemory().write16(0x00000806, 0x0000);
        gba.getCPU().getMemory().write16(0x00000000, 0x5FF6); // LDSH R6, [R5, R6] - 0101111111110110
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 4: Load negative halfword -1 (0xFFFF)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000800;
        registers[1] = 0x00000008;
        gba.getCPU().getMemory().write16(0x00000808, 0xFFFF);
        gba.getCPU().getMemory().write16(0x00000000, 0x5E41); // LDSH R1, [R0, R1] - 0101111001000001
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0xFFFFFFFF)); // Sign-extended -1
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 5: Load moderately negative value
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00000800;
        registers[0] = 0x0000000A;
        gba.getCPU().getMemory().write16(0x0000080A, 0x8123); // Negative value
        gba.getCPU().getMemory().write16(0x00000000, 0x5E38); // LDSH R0, [R7, R0] - 0101111000111000
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xFFFF8123)); // Sign-extended
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 6: LDSH preserves flags
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        registers[2] = 0x00000800;
        registers[3] = 0x0000000C;
        gba.getCPU().getMemory().write16(0x0000080C, 0x1234);
        gba.getCPU().getMemory().write16(0x00000000, 0x5ED3); // LDSH R3, [R2, R3] - 0101111011010011
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x00001234));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 7: Edge case with 0x8001 (should be sign-extended)
    {
        GBA gba(true);
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00000800;
        registers[5] = 0x0000000E;
        gba.getCPU().getMemory().write16(0x0000080E, 0x8001);
        gba.getCPU().getMemory().write16(0x00000000, 0x5F65); // LDSH R5, [R4, R5] - 0101111101100101
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0xFFFF8001)); // Sign-extended
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }
}

TEST(CPU, B) {}

TEST(CPU, B_COND) {}

TEST(CPU, BL) {}

