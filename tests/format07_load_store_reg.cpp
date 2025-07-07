#include "test_cpu_common.h"

// ARM Thumb Format 7: Load/store with register offset
// Encoding: 0101[L][B][0][Ro][Rb][Rd]
// Instructions: STR, STRB, LDR, LDRB
// L=0: Store, L=1: Load
// B=0: Word, B=1: Byte
// Effective address = Rb + Ro

TEST(Format7, STR_WORD_REGISTER_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: STR R0, [R1, R2] - basic register offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000008; // Offset
        registers[0] = 0x12345678; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5088); // STR R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at base + offset
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001008);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: STR R3, [R4, R5] - different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001200; // Base address
        registers[5] = 0x00000010; // Offset
        registers[3] = 0x87654321; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5163); // STR R3, [R4, R5]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at base + offset
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001210);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: STR with zero offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[6] = 0x00001400; // Base address
        registers[7] = 0x00000000; // Zero offset
        registers[1] = 0xAABBCCDD; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x51F1); // STR R1, [R6, R7]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at base address (no offset)
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001400);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0xAABBCCDD));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: STR all register combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001000; // Base address - use R3 to avoid conflicts
        registers[4] = 0x10; // Offset - use R4 to avoid conflicts
        
        for (int rd = 0; rd < 3; rd++) { // Only test rd 0-2 to avoid conflicts with base/offset registers
            uint32_t test_value = 0x12345600 + rd;
            registers[rd] = test_value;
            
            // STR Rd, [R3, R4]
            uint16_t opcode = 0x5000 | (4 << 6) | (3 << 3) | rd; // ro=4, rb=3, rd=rd
            gba.getCPU().getMemory().write16(rd * 4, opcode);
            registers[15] = rd * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify each value was stored correctly
            uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001010); // R3 + R4 = 0x00001000 + 0x10
            ASSERT_EQ(stored_value, test_value) << "Register R" << rd;
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 5: STR with different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[0] = 0x55555555; // Value to store
        
        uint32_t offsets[] = {0, 4, 8, 16, 32, 64, 128};
        
        for (size_t i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++) {
            registers[2] = offsets[i]; // Offset register
            
            // STR R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 4, 0x5088); // STR R0, [R1, R2]
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify value stored at correct address
            uint32_t expected_address = 0x00001000 + offsets[i];
            uint32_t stored_value = gba.getCPU().getMemory().read32(expected_address);
            ASSERT_EQ(stored_value, static_cast<uint32_t>(0x55555555)) << "Offset " << offsets[i];
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }
}

TEST(Format7, LDR_WORD_REGISTER_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: LDR R0, [R1, R2] - basic register offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000008; // Offset
        
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001008, 0x12345678);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5888); // LDR R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDR with different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001200; // Base address
        registers[5] = 0x00000020; // Offset
        registers[3] = 0xDEADBEEF; // Should be overwritten
        
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001220, 0x87654321);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5963); // LDR R3, [R4, R5]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 3: LDR all register combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001000; // Base address - use R3 to avoid conflicts
        registers[4] = 0x00000014; // Offset - use R4 to avoid conflicts
        
        for (int rd = 0; rd < 3; rd++) { // Only test rd 0-2 to avoid conflicts with base/offset registers
            uint32_t test_value = 0x30000000 + rd;
            // Pre-store value in memory
            gba.getCPU().getMemory().write32(0x00001014, test_value);
            
            registers[rd] = 0xDEADBEEF; // Reset destination register
            
            // LDR Rd, [R3, R4]
            uint16_t opcode = 0x5800 | (4 << 6) | (3 << 3) | rd; // L=1, ro=4, rb=3, rd=rd
            gba.getCPU().getMemory().write16(rd * 4, opcode);
            registers[15] = rd * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify each value was loaded correctly
            ASSERT_EQ(registers[rd], test_value) << "Register R" << rd;
            validateUnchangedRegisters(cpu, beforeState, {rd, 15});
        }
    }
}

TEST(Format7, STRB_BYTE_REGISTER_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: STRB R0, [R1, R2] - basic byte store
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000005; // Offset
        registers[0] = 0x123456AB; // Value to store (only LSB should be stored)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5488); // STRB R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify only the byte was stored
        uint8_t stored_value = gba.getCPU().getMemory().read8(0x00001005);
        ASSERT_EQ(stored_value, static_cast<uint8_t>(0xAB));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: STRB with different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001100; // Base address
        registers[4] = 0x00000007; // Offset
        registers[5] = 0xFFFFFF99; // Value to store (only LSB)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x551D); // STRB R5, [R3, R4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify only the byte was stored
        uint8_t stored_value = gba.getCPU().getMemory().read8(0x00001107);
        ASSERT_EQ(stored_value, static_cast<uint8_t>(0x99));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: STRB with different byte values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000008; // Offset
        
        uint8_t test_bytes[] = {0x00, 0x01, 0x7F, 0x80, 0xFF};
        
        for (size_t i = 0; i < sizeof(test_bytes)/sizeof(test_bytes[0]); i++) {
            registers[0] = 0x12345600 | test_bytes[i]; // Set LSB to test value
            registers[2] = 0x00000008 + i; // Different offset for each
            
            // STRB R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 4, 0x5488); // STRB R0, [R1, R2]
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify byte was stored correctly
            uint8_t stored_value = gba.getCPU().getMemory().read8(0x00001008 + i);
            ASSERT_EQ(stored_value, test_bytes[i]) << "Byte value " << (int)test_bytes[i];
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }
}

TEST(Format7, LDRB_BYTE_REGISTER_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: LDRB R0, [R1, R2] - basic byte load
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000003; // Offset
        registers[0] = 0xDEADBEEF; // Should be overwritten with zero-extended byte
        
        // Pre-store a byte value in memory
        gba.getCPU().getMemory().write8(0x00001003, 0xA5);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5C88); // LDRB R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the byte was loaded and zero-extended
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x000000A5));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDRB with different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[5] = 0x00001200; // Base address
        registers[6] = 0x0000000F; // Offset
        registers[7] = 0xFFFFFFFF; // Should be overwritten
        
        // Pre-store a byte value in memory
        gba.getCPU().getMemory().write8(0x0000120F, 0x7B);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5DAF); // LDRB R7, [R5, R6]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the byte was loaded and zero-extended
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x0000007B));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }

    // Test case 3: LDRB with different byte values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000010; // Base offset
        
        uint8_t test_bytes[] = {0x00, 0x01, 0x7F, 0x80, 0xFF};
        
        for (size_t i = 0; i < sizeof(test_bytes)/sizeof(test_bytes[0]); i++) {
            // Pre-store byte in memory
            gba.getCPU().getMemory().write8(0x00001010 + i, test_bytes[i]);
            
            registers[0] = 0xDEADBEEF; // Reset destination
            registers[2] = 0x00000010 + i; // Adjust offset
            
            // LDRB R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 4, 0x5C88); // LDRB R0, [R1, R2]
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify byte was loaded and zero-extended correctly
            ASSERT_EQ(registers[0], static_cast<uint32_t>(test_bytes[i])) << "Byte value " << (int)test_bytes[i];
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }
}

TEST(Format7, STR_LDR_ROUNDTRIP_TESTS) {
    std::string beforeState;

    // Test case 1: Store and load back same word value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000018; // Offset
        registers[0] = 0x13579BDF; // Test value
        
        // Store word: STR R0, [R1, R2]
        gba.getCPU().getMemory().write16(0x00000000, 0x5088); // STR R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Clear register and load back: LDR R3, [R1, R2]
        registers[3] = 0x00000000;
        gba.getCPU().getMemory().write16(0x00000002, 0x588B); // LDR R3, [R1, R2]
        cpu.execute(1);
        
        // Verify round-trip worked
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x13579BDF));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 2: Store and load back byte values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000020; // Offset
        
        uint8_t test_bytes[] = {0x12, 0xA5, 0xFF, 0x00, 0x7F};
        
        for (size_t i = 0; i < sizeof(test_bytes)/sizeof(test_bytes[0]); i++) {
            registers[0] = 0xABCD0000 | test_bytes[i]; // Set byte value
            registers[2] = 0x00000020 + i; // Different offset
            
            // Store byte: STRB R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 8, 0x5488); // STRB R0, [R1, R2]
            registers[15] = i * 8; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Load back: LDRB R3, [R1, R2]
            registers[3] = 0xDEADBEEF;
            gba.getCPU().getMemory().write16(i * 8 + 2, 0x5C8B); // LDRB R3, [R1, R2]
            cpu.execute(1);
            
            // Verify byte round-trip (zero-extended)
            ASSERT_EQ(registers[3], static_cast<uint32_t>(test_bytes[i])) << "Byte " << (int)test_bytes[i];
            validateUnchangedRegisters(cpu, beforeState, {3, 15});
        }
    }
}

TEST(Format7, EDGE_CASES_AND_BOUNDARY_CONDITIONS) {
    std::string beforeState;

    // Test case 1: Memory boundary conditions
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test near end of memory (0x0000 to 0x1FFF range)
        registers[1] = 0x00001FF0; // Base near end
        registers[2] = 0x0000000C; // Offset
        registers[0] = 0x99887766; // Test value
        
        // STR R0, [R1, R2] - Address will be 0x1FFC
        gba.getCPU().getMemory().write16(0x00000000, 0x5088); // STR R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify value stored correctly at boundary
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001FFC);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x99887766));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: Same register as base and offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00000800; // Base address (also will be used as offset)
        registers[0] = 0x55AA55AA; // Test value
        
        // STR R0, [R1, R1] - Address will be 0x800 + 0x800 = 0x1000
        gba.getCPU().getMemory().write16(0x00000000, 0x5048); // STR R0, [R1, R1]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify value stored at calculated address
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001000);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x55AA55AA));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: Store/load to same register
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000004; // Offset
        registers[0] = 0x12345678; // Value to store
        
        // Store R0: STR R0, [R1, R2]
        gba.getCPU().getMemory().write16(0x00000000, 0x5088); // STR R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Load back into same register: LDR R0, [R1, R2]
        registers[0] = 0xDEADBEEF; // Change value
        gba.getCPU().getMemory().write16(0x00000002, 0x5888); // LDR R0, [R1, R2]
        cpu.execute(1);
        
        // Should get original value back
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 4: Maximum register combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00001000; // Base (max register)
        registers[6] = 0x00000100; // Offset (max-1 register)
        registers[5] = 0xFEDCBA98; // Value (max-2 register)
        
        // STR R5, [R7, R6] - Use maximum register numbers
        gba.getCPU().getMemory().write16(0x00000000, 0x51BD); // STR R5, [R7, R6]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify value stored correctly
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001100);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0xFEDCBA98));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: Zero values and special patterns
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000000; // Zero offset
        
        uint32_t test_values[] = {0x00000000, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF, 0x55555555, 0xAAAAAAAA};
        
        for (size_t i = 0; i < sizeof(test_values)/sizeof(test_values[0]); i++) {
            registers[0] = test_values[i];
            
            // STR R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 8, 0x5088); // STR R0, [R1, R2]
            registers[15] = i * 8; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Load back and verify
            registers[3] = 0xDEADBEEF;
            gba.getCPU().getMemory().write16(i * 8 + 2, 0x588B); // LDR R3, [R1, R2]
            cpu.execute(1);
            
            ASSERT_EQ(registers[3], test_values[i]) << "Value 0x" << std::hex << test_values[i];
            validateUnchangedRegisters(cpu, beforeState, {3, 15});
        }
    }
}
