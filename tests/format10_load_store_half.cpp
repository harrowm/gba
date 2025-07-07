#include "test_cpu_common.h"

// ARM Thumb Format 10: Load/store halfword
// Encoding: 1000[L][Offset5][Rb][Rd]
// Instructions: STRH, LDRH
// L=0: STRH (Store), L=1: LDRH (Load)
// Offset = Offset5 * 2 (halfword-aligned)

TEST(Format10, STRH_IMMEDIATE_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: STRH R0, [R1, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[0] = 0x12345678; // Value to store (only lower 16 bits stored)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x8008); // STRH R0, [R1, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was stored at base
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001000);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0x5678));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: STRH R2, [R3, #2] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001000; // Base address
        registers[2] = 0x87654321; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x805A); // STRH R2, [R3, #2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was stored at base + 2
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001002);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0x4321));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: STRH R4, [R5, #4] - different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00001200; // Base address
        registers[4] = 0xAABBCCDD; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x80AC); // STRH R4, [R5, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was stored at base + 4
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001204);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0xCCDD));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: STRH with larger offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[6] = 0x00001000; // Base address
        registers[7] = 0x11223344; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x87F7); // STRH R7, [R6, #62] (offset5=31)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was stored at base + 62
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x0000103E);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0x3344));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: STRH all registers at different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00001000; // Base address (using R0 as base)
        
        // Store each register to a different offset
        for (int rd = 1; rd < 8; rd++) { // Skip R0 since it's the base
            registers[rd] = 0x1000 + rd; // Unique values
            
            // Calculate opcode: STRH Rd, [R0, #offset]
            int offset = rd * 2; // 2, 4, 6, 8, 10, 12, 14
            int offset5 = offset / 2; // 1, 2, 3, 4, 5, 6, 7
            uint16_t opcode = 0x8000 | (offset5 << 6) | rd;
            
            gba.getCPU().getMemory().write16(0x00000000, opcode);
            registers[15] = 0x00000000; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify the halfword was stored
            uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001000 + offset);
            ASSERT_EQ(stored_value, static_cast<uint16_t>(0x1000 + rd));
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }
}

TEST(Format10, LDRH_IMMEDIATE_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: LDRH R0, [R1, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        
        // Pre-store a halfword value
        gba.getCPU().getMemory().write16(0x00001000, 0x5678);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x8808); // LDRH R0, [R1, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x5678));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDRH R2, [R3, #2] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001000; // Base address
        
        // Pre-store a halfword value
        gba.getCPU().getMemory().write16(0x00001002, 0x4321);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x885A); // LDRH R2, [R3, #2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x4321));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: LDRH R4, [R5, #4] - different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00001200; // Base address
        
        // Pre-store a halfword value
        gba.getCPU().getMemory().write16(0x00001204, 0xCCDD);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x88AC); // LDRH R4, [R5, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0xCCDD));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 4: LDRH with larger offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[6] = 0x00001000; // Base address
        
        // Pre-store a halfword value
        gba.getCPU().getMemory().write16(0x0000103E, 0x3344);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x8FF7); // LDRH R7, [R6, #62] (offset5=31)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x3344));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }

    // Test case 5: LDRH all registers at different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00001000; // Base address (using R0 as base)
        
        // Load each register from a different offset
        for (int rd = 1; rd < 8; rd++) { // Skip R0 since it's the base
            int offset = rd * 2; // 2, 4, 6, 8, 10, 12, 14
            int offset5 = offset / 2; // 1, 2, 3, 4, 5, 6, 7
            uint16_t expected_value = 0x2000 + rd;
            
            // Pre-store unique halfword values
            gba.getCPU().getMemory().write16(0x00001000 + offset, expected_value);
            
            // Calculate opcode: LDRH Rd, [R0, #offset]
            uint16_t opcode = 0x8800 | (offset5 << 6) | rd;
            
            gba.getCPU().getMemory().write16(0x00000000, opcode);
            registers[15] = 0x00000000; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify the halfword was loaded
            ASSERT_EQ(registers[rd], static_cast<uint32_t>(expected_value));
            validateUnchangedRegisters(cpu, beforeState, {rd, 15});
        }
    }
}

TEST(Format10, STRH_LDRH_ROUNDTRIP) {
    std::string beforeState;

    // Test roundtrip: store then load the same value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0xABCD1234; // Value to store (lower 16 bits: 0x1234)
        
        // Store: STRH R2, [R1, #8]
        gba.getCPU().getMemory().write16(0x00000000, 0x810A); // STRH R2, [R1, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        validateUnchangedRegisters(cpu, beforeState, {15});
        
        // Clear the register
        registers[3] = 0;
        
        // Load: LDRH R3, [R1, #8]
        gba.getCPU().getMemory().write16(0x00000000, 0x890B); // LDRH R3, [R1, #8]
        registers[15] = 0x00000000; // Reset PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the loaded value matches the stored lower 16 bits
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x1234));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(Format10, OFFSET_RANGE_TESTS) {
    std::string beforeState;

    // Test all possible offset values (0-62 in steps of 2)
    for (int byte_offset = 0; byte_offset <= 62; byte_offset += 2) {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x3000 + byte_offset; // Unique value for each offset
        
        int offset5 = byte_offset / 2;
        
        // Test STRH
        uint16_t strh_opcode = 0x8000 | (offset5 << 6) | (1 << 3) | 2; // STRH R2, [R1, #offset]
        gba.getCPU().getMemory().write16(0x00000000, strh_opcode);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        validateUnchangedRegisters(cpu, beforeState, {15});
        
        // Verify storage
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001000 + byte_offset);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0x3000 + byte_offset));
        
        // Test LDRH
        registers[3] = 0; // Clear destination
        uint16_t ldrh_opcode = 0x8800 | (offset5 << 6) | (1 << 3) | 3; // LDRH R3, [R1, #offset]
        gba.getCPU().getMemory().write16(0x00000000, ldrh_opcode);
        registers[15] = 0x00000000; // Reset PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify load
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x3000 + byte_offset));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }
}

TEST(Format10, EDGE_CASES_AND_BOUNDARY_CONDITIONS) {
    std::string beforeState;

    // Test case 1: Store/load at memory boundary
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001FFE; // Near end of test memory (ends at 0x1FFF)
        registers[2] = 0xDEADBEEF; // Test value
        
        // Store at boundary: STRH R2, [R1, #0]
        gba.getCPU().getMemory().write16(0x00000000, 0x800A); // STRH R2, [R1, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        validateUnchangedRegisters(cpu, beforeState, {15});
        
        // Load back: LDRH R3, [R1, #0]
        registers[3] = 0;
        gba.getCPU().getMemory().write16(0x00000000, 0x880B); // LDRH R3, [R1, #0]
        registers[15] = 0x00000000; // Reset PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0xBEEF)); // Lower 16 bits
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 2: Maximum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00001000; // Base address
        registers[1] = 0x5555AAAA; // Test value
        
        // Store with max offset: STRH R1, [R0, #62] (offset5=31)
        gba.getCPU().getMemory().write16(0x00000000, 0x87C1); // STRH R1, [R0, #62]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        validateUnchangedRegisters(cpu, beforeState, {15});
        
        // Load back: LDRH R2, [R0, #62]
        registers[2] = 0;
        gba.getCPU().getMemory().write16(0x00000000, 0x8FC2); // LDRH R2, [R0, #62]
        registers[15] = 0x00000000; // Reset PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xAAAA)); // Lower 16 bits
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: Zero value handling
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001100; // Base address
        registers[4] = 0x12340000; // Value with zero lower 16 bits
        
        // Store zero lower halfword: STRH R4, [R3, #10]
        gba.getCPU().getMemory().write16(0x00000000, 0x815C); // STRH R4, [R3, #10]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        validateUnchangedRegisters(cpu, beforeState, {15});
        
        // Load back: LDRH R5, [R3, #10]
        registers[5] = 0xFFFFFFFF; // Pre-fill with non-zero
        gba.getCPU().getMemory().write16(0x00000000, 0x895D); // LDRH R5, [R3, #10]
        registers[15] = 0x00000000; // Reset PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x0000)); // Should be zero
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 4: Same register as source and base (when possible)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[6] = 0x00001200; // Base address and value (lower 16 bits: 0x1200)
        
        // Store: STRH R6, [R6, #4] - using same register
        gba.getCPU().getMemory().write16(0x00000000, 0x80B6); // STRH R6, [R6, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        validateUnchangedRegisters(cpu, beforeState, {15});
        
        // Verify storage
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001204);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0x1200));
        
        // Load into different register: LDRH R7, [R6, #4]
        registers[7] = 0;
        gba.getCPU().getMemory().write16(0x00000000, 0x88B7); // LDRH R7, [R6, #4]
        registers[15] = 0x00000000; // Reset PC
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x1200));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }
}
