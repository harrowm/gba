#include "test_cpu_common.h"

// ARM Thumb Format 5: Hi register operations/branch exchange
// Encoding: 010001[Op][H1][H2][Rs/Hs][Rd/Hd]
// Instructions: ADD Rd, Rs; CMP Rd, Rs; MOV Rd, Rs; BX Rs

TEST(Format5, ADD_HI_REGISTER_OPERATIONS) {
    std::string beforeState;

    // Test case 1: ADD R0, R8 (low + high register)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x12345678;
        registers[8] = 0x87654321;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4440); // ADD R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678 + 0x87654321));
        // ADD with high registers does not affect flags
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: ADD R8, R0 (high + low register)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x11111111;
        registers[0] = 0x22222222;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4480); // ADD R8, R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[8], static_cast<uint32_t>(0x11111111 + 0x22222222));
        validateUnchangedRegisters(cpu, beforeState, {8, 15});
    }

    // Test case 3: ADD R8, R9 (high + high register)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0xAAAAAAAA;
        registers[9] = 0x55555555;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x44C8); // ADD R8, R9
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[8], static_cast<uint32_t>(0xAAAAAAAA + 0x55555555));
        validateUnchangedRegisters(cpu, beforeState, {8, 15});
    }

    // Test case 4: ADD R1, R2 (low + low register - valid when at least one is high)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x10203040;
        registers[2] = 0x01020304;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4411); // ADD R1, R2
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x10203040 + 0x01020304));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 5: ADD PC, LR (special case - PC modification)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[15] = 0x00000100; // PC
        registers[14] = 0x00000008; // LR
        
        gba.getCPU().getMemory().write16(0x00000100, 0x44F7); // ADD PC, LR
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // PC should be updated to LR + current PC + 4 (pipeline effect)
        uint32_t expected_pc = (0x00000100 + 4) + 0x00000008;
        ASSERT_EQ(registers[15], expected_pc);
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 6: ADD SP, R8 (stack pointer modification)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[8] = 0x00000100;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x44C5); // ADD SP, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001000 + 0x00000100));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 7: ADD with overflow (no flags affected)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0xFFFFFFFF;
        registers[9] = 0x00000001;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x44C8); // ADD R8, R9
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[8], static_cast<uint32_t>(0x00000000)); // Wraparound
        // Verify no flags are set (high register ADD doesn't affect flags)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        validateUnchangedRegisters(cpu, beforeState, {8, 15});
    }

    // Test case 8: ADD zero values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000000;
        registers[8] = 0x00000000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4440); // ADD R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }
}

TEST(Format5, CMP_HI_REGISTER_OPERATIONS) {
    std::string beforeState;

    // Test case 1: CMP R0, R8 (low vs high register) - equal values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x12345678;
        registers[8] = 0x12345678;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4540); // CMP R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Equal values should set Z flag and clear N, C, V
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: CMP R8, R0 (high vs low register) - first greater
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x12345679;
        registers[0] = 0x12345678;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4580); // CMP R8, R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // First > second should clear Z, set C, clear N and V
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: CMP R8, R9 (high vs high register) - first smaller
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x12345678;
        registers[9] = 0x12345679;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x45C8); // CMP R8, R9
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // First < second should clear Z and C, set N, clear V
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow occurred
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: CMP with negative result
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x00000001;
        registers[9] = 0x00000002;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x45C8); // CMP R8, R9
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // 1 - 2 = -1 (0xFFFFFFFF)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));  // Negative result
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C)); // Borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: CMP with overflow
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x80000000; // Most negative number
        registers[9] = 0x00000001; // Positive
        
        gba.getCPU().getMemory().write16(0x00000000, 0x45C8); // CMP R8, R9
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // 0x80000000 - 1 = 0x7FFFFFFF (overflow from negative to positive)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N)); // Result is positive
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // No borrow
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));  // Overflow occurred
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 6: CMP zero with zero
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000000;
        registers[8] = 0x00000000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4540); // CMP R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));  // Zero result
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 7: CMP with maximum values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0xFFFFFFFF;
        registers[9] = 0xFFFFFFFF;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x45C8); // CMP R8, R9
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));  // Equal
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));  // No borrow
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}

TEST(Format5, MOV_HI_REGISTER_OPERATIONS) {
    std::string beforeState;

    // Test case 1: MOV R0, R8 (high to low register)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x12345678;
        registers[0] = 0xDEADBEEF; // Should be overwritten
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4640); // MOV R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        // MOV with high registers does not affect flags
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: MOV R8, R0 (low to high register)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x87654321;
        registers[8] = 0xDEADBEEF; // Should be overwritten
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4680); // MOV R8, R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[8], static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {8, 15});
    }

    // Test case 3: MOV R8, R9 (high to high register)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[9] = 0xAAAABBBB;
        registers[8] = 0xCCCCDDDD; // Should be overwritten
        
        gba.getCPU().getMemory().write16(0x00000000, 0x46C8); // MOV R8, R9
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[8], static_cast<uint32_t>(0xAAAABBBB));
        validateUnchangedRegisters(cpu, beforeState, {8, 15});
    }

    // Test case 4: MOV PC, LR (branch using MOV)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[15] = 0x00000100; // Current PC
        registers[14] = 0x00000200; // LR (return address)
        
        gba.getCPU().getMemory().write16(0x00000100, 0x46F7); // MOV PC, LR
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // PC should be set to LR value
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000200));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: MOV SP, R12 (stack pointer manipulation)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[12] = 0x00001FFF; // New stack value
        registers[13] = 0x00001000; // Current SP
        
        gba.getCPU().getMemory().write16(0x00000000, 0x46E5); // MOV SP, R12
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001FFF));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 6: MOV LR, PC (save return address)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[15] = 0x00000500; // Current PC
        registers[14] = 0x00000000; // LR to be set
        
        gba.getCPU().getMemory().write16(0x00000500, 0x46FE); // MOV LR, PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // LR should get PC + 4 (pipeline effect)
        ASSERT_EQ(registers[14], static_cast<uint32_t>(0x00000500 + 4));
        validateUnchangedRegisters(cpu, beforeState, {14, 15});
    }

    // Test case 7: MOV with zero value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x00000000;
        registers[0] = 0xFFFFFFFF; // Should be overwritten
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4640); // MOV R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00000000));
        // MOV doesn't set flags for zero value
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 8: MOV with negative value (no sign extension)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[8] = 0x80000000; // Negative in signed interpretation
        registers[0] = 0x12345678;
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4640); // MOV R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x80000000));
        // MOV doesn't affect flags
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }
}

TEST(Format5, BX_BRANCH_EXCHANGE) {
    std::string beforeState;

    // Test case 1: BX R0 (branch to ARM mode - bit 0 clear)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
        
        registers[0] = 0x00000200; // Target address (ARM mode - bit 0 clear)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4700); // BX R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // PC should be set to target address
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000200));
        // T flag should be cleared (ARM mode)
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_T));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: BX R1 (branch to Thumb mode - bit 0 set)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = 0; // Start in ARM mode (T flag clear)
        
        registers[1] = 0x00000301; // Target address (Thumb mode - bit 0 set)
        
        gba.getCPU().getMemory().write32(0x00000000, 0xE12FFF11); // BX R1 (ARM encoding)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // PC should be set to target address with bit 0 cleared
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000300));
        // T flag should be set (Thumb mode)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_T));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: BX R8 (branch with high register)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
        
        registers[8] = 0x00000400; // Target address (ARM mode)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4740); // BX R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000400));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_T)); // ARM mode
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: BX LR (return from function)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
        
        registers[14] = 0x00000505; // Return address (Thumb mode)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4770); // BX LR
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000504)); // Bit 0 cleared
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_T)); // Thumb mode
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: BX PC (branch to current PC + pipeline offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
        
        registers[15] = 0x00000100; // Current PC
        
        gba.getCPU().getMemory().write16(0x00000100, 0x4778); // BX PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // PC should branch to itself + 4 (pipeline effect), ARM mode
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000104));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_T)); // ARM mode (bit 0 clear)
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 6: BX with address at memory boundary
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
        
        registers[0] = 0x00001FFF; // At memory boundary (Thumb mode)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4700); // BX R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00001FFE)); // Bit 0 cleared
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_T)); // Thumb mode
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 7: BX preserves other flags
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        registers[0] = 0x00000200; // ARM mode target
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4700); // BX R0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000200));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_T)); // Changed to ARM
        // Other flags should be preserved
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}

TEST(Format5, EDGE_CASES_AND_BOUNDARY_CONDITIONS) {
    std::string beforeState;

    // Test case 1: All register combinations for ADD
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        // Test all combinations of high/low registers
        struct {
            uint8_t rd, rs;
            uint16_t opcode;
            const char* description;
        } test_cases[] = {
            {0, 8, 0x4440, "ADD R0, R8"},
            {8, 0, 0x4480, "ADD R8, R0"},
            {8, 8, 0x44C0, "ADD R8, R8"},
            {15, 14, 0x44F7, "ADD PC, LR"},
            {13, 12, 0x46E5, "ADD SP, R12"}, // Actually MOV, but similar test
        };
        
        for (auto& test : test_cases) {
            if (test.rd == 13 && test.rs == 12) continue; // Skip MOV test here
            
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            registers[test.rd] = 0x10000000;
            registers[test.rs] = 0x01000000;
            
            // Handle the case where rd == rs (e.g., ADD R8, R8)
            uint32_t expected_result;
            if (test.rd == test.rs) {
                // For ADD Rd, Rs where Rd == Rs: result = 2 * initial_value
                expected_result = 0x02000000; // 2 * 0x01000000
            } else {
                expected_result = 0x11000000; // 0x10000000 + 0x01000000
            }
            
            gba.getCPU().getMemory().write16(0x00000000, test.opcode);
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            if (test.rd == 15) {
                // PC gets special handling
                ASSERT_NE(registers[15], static_cast<uint32_t>(0x10000000)) << test.description;
            } else {
                ASSERT_EQ(registers[test.rd], expected_result) << test.description;
            }
            validateUnchangedRegisters(cpu, beforeState, {test.rd, 15});
        }
    }

    // Test case 2: CMP with all flag combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        struct {
            uint32_t val1, val2;
            bool z, n, c, v;
            const char* description;
        } test_cases[] = {
            {0x00000000, 0x00000000, true, false, true, false, "Equal zero"},
            {0x12345678, 0x12345678, true, false, true, false, "Equal non-zero"},
            {0x12345679, 0x12345678, false, false, true, false, "First greater"},
            {0x12345678, 0x12345679, false, true, false, false, "First smaller"},
            {0x80000000, 0x00000001, false, false, true, true, "Overflow positive"},
            {0x7FFFFFFF, 0x80000000, false, true, false, true, "Overflow negative"},
        };
        
        for (auto& test : test_cases) {
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            registers[8] = test.val1;
            registers[9] = test.val2;
            
            gba.getCPU().getMemory().write16(0x00000000, 0x45C8); // CMP R8, R9
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            ASSERT_EQ(cpu.getFlag(CPU::FLAG_Z), test.z) << test.description << " - Z flag";
            ASSERT_EQ(cpu.getFlag(CPU::FLAG_N), test.n) << test.description << " - N flag";
            ASSERT_EQ(cpu.getFlag(CPU::FLAG_C), test.c) << test.description << " - C flag";
            ASSERT_EQ(cpu.getFlag(CPU::FLAG_V), test.v) << test.description << " - V flag";
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 3: MOV to PC with different addresses
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        uint32_t test_addresses[] = {
            0x00000000, 0x00000100, 0x00000800, 0x00001000, 0x00001FFE
        };
        
        for (auto addr : test_addresses) {
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            registers[8] = addr;
            registers[15] = 0x00000100; // Current PC
            
            gba.getCPU().getMemory().write16(0x00000100, 0x46C7); // MOV PC, R8
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            ASSERT_EQ(registers[15], addr) << "MOV PC failed for address 0x" << std::hex << addr;
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 4: BX mode switching patterns
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        struct {
            uint32_t initial_cpsr;
            uint32_t target_addr;
            bool expected_thumb;
            const char* description;
        } test_cases[] = {
            {CPU::FLAG_T, 0x00000200, false, "Thumb to ARM"},
            {CPU::FLAG_T, 0x00000201, true, "Thumb to Thumb"},
            {0, 0x00000200, false, "ARM to ARM"},
            {0, 0x00000201, true, "ARM to Thumb"},
        };
        
        for (auto& test : test_cases) {
            registers.fill(0);
            cpu.CPSR() = test.initial_cpsr;
            
            registers[0] = test.target_addr;
            
            // Use appropriate instruction encoding based on starting mode
            if (test.initial_cpsr & CPU::FLAG_T) {
                // Starting in Thumb mode - use Thumb BX instruction
                gba.getCPU().getMemory().write16(0x00000000, 0x4700); // BX R0 (Thumb)
            } else {
                // Starting in ARM mode - use ARM BX instruction  
                gba.getCPU().getMemory().write32(0x00000000, 0xE12FFF10); // BX R0 (ARM)
            }
            
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            ASSERT_EQ(registers[15], test.target_addr & ~1) << test.description << " - PC";
            ASSERT_EQ(cpu.getFlag(CPU::FLAG_T), test.expected_thumb) << test.description << " - T flag";
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 5: Stack operations with high registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test stack manipulation
        registers[13] = 0x00001000; // SP
        registers[8] = 0x00000100;  // Offset
        
        gba.getCPU().getMemory().write16(0x00000000, 0x44C5); // ADD SP, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x00001100));
        
        // Now move new value to another register
        gba.getCPU().getMemory().write16(0x00000002, 0x46E8); // MOV R8, SP
        cpu.execute(1);
        
        ASSERT_EQ(registers[8], static_cast<uint32_t>(0x00001100));
        validateUnchangedRegisters(cpu, beforeState, {8, 13, 15});
    }

    // Test case 6: Verify no unintended flag modifications
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        // Set all flags initially
        uint32_t initial_flags = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
        
        registers.fill(0);
        cpu.CPSR() = initial_flags;
        
        registers[8] = 0x12345678;
        registers[0] = 0x87654321;
        
        // Test ADD (should not affect flags)
        gba.getCPU().getMemory().write16(0x00000000, 0x4440); // ADD R0, R8
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Only T flag should remain (others preserved)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_T));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        
        // Test MOV (should not affect flags except when used as PC)
        gba.getCPU().getMemory().write16(0x00000002, 0x4640); // MOV R0, R8
        cpu.execute(1);
        
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }
}
