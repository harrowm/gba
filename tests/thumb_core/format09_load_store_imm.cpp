#include "test_cpu_common.h"

// ARM Thumb Format 9: Load/store with immediate offset  
// Encoding: 011[B][L][Offset5][Rb][Rd]
// Instructions: STR, LDR, STRB, LDRB
// B=0: Word operations (offset scaled by 4), B=1: Byte operations
// L=0: Store, L=1: Load
// Word effective address = Rb + (Offset5 * 4)
// Byte effective address = Rb + Offset5

TEST(Format9, STR_WORD_IMMEDIATE_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: STR R0, [R1, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[0] = 0x12345678; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x6008); // STR R0, [R1, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at base address
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001000);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: STR R2, [R3, #4] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001200; // Base address
        registers[2] = 0x87654321; // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x605A); // STR R2, [R3, #4] (offset5=1)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at base + 4
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001204);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: STR with larger offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001000; // Base address
        registers[5] = 0xAABBCCDD; // Value to store
        
        // Test different offset values
        uint32_t test_offsets[] = {8, 16, 32, 64, 124}; // Byte offsets
        uint32_t offset5_values[] = {2, 4, 8, 16, 31}; // Corresponding offset5 values
        
        for (size_t i = 0; i < sizeof(test_offsets)/sizeof(test_offsets[0]); i++) {
            registers[5] = 0x30000000 + i; // Unique value per test
            
            // STR R5, [R4, #offset]
            uint16_t opcode = 0x6000 | (offset5_values[i] << 6) | (4 << 3) | 5;
            gba.getCPU().getMemory().write16(i * 4, opcode);
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify value stored at correct address
            uint32_t expected_address = 0x00001000 + test_offsets[i];
            uint32_t stored_value = gba.getCPU().getMemory().read32(expected_address);
            ASSERT_EQ(stored_value, static_cast<uint32_t>(0x30000000 + i)) 
                << "Offset " << test_offsets[i] << " (offset5=" << offset5_values[i] << ")";
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 4: STR all registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00001000; // Base address (using R7 to avoid conflicts)
        
        for (int rd = 0; rd < 8; rd++) {
            uint32_t test_value = 0x40000000 + rd;
            
            // Set the source register, but preserve base register if they're the same
            if (rd == 7) {
                // Skip testing R7 as source since it's our base register
                continue;
            } else {
                registers[rd] = test_value;
            }
            
            // STR Rd, [R7, #12] (offset5=3)
            uint16_t opcode = 0x6000 | (3 << 6) | (7 << 3) | rd;
            gba.getCPU().getMemory().write16(rd * 4, opcode);
            registers[15] = rd * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify each store overwrites the same location
            uint32_t stored_value = gba.getCPU().getMemory().read32(0x0000100C);
            ASSERT_EQ(stored_value, test_value) << "Register R" << rd;
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 5: STR maximum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00001000; // Base address
        registers[7] = 0xFEDCBA98; // Value to store
        
        // STR R7, [R0, #124] (offset5=31, maximum)
        gba.getCPU().getMemory().write16(0x00000000, 0x67C7); // STR R7, [R0, #124]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify value stored at maximum offset
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x0000107C);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0xFEDCBA98));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}

TEST(Format9, LDR_WORD_IMMEDIATE_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: LDR R0, [R1, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[0] = 0xDEADBEEF; // Should be overwritten
        
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001000, 0x12345678);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x6808); // LDR R0, [R1, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDR R3, [R4, #8] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001300; // Base address
        registers[3] = 0xFFFFFFFF; // Should be overwritten
        
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001308, 0x87654321);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x68A3); // LDR R3, [R4, #8] (offset5=2)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 3: LDR with different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        
        uint32_t test_offsets[] = {12, 20, 48, 80, 124}; // Byte offsets
        uint32_t offset5_values[] = {3, 5, 12, 20, 31}; // Corresponding offset5 values
        
        for (size_t i = 0; i < sizeof(test_offsets)/sizeof(test_offsets[0]); i++) {
            uint32_t test_value = 0x50000000 + i;
            
            // Pre-store value in memory
            gba.getCPU().getMemory().write32(0x00001000 + test_offsets[i], test_value);
            
            registers[0] = 0xDEADBEEF; // Reset destination
            
            // LDR R0, [R1, #offset]
            uint16_t opcode = 0x6800 | (offset5_values[i] << 6) | (1 << 3) | 0;
            gba.getCPU().getMemory().write16(i * 4, opcode);
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify value loaded correctly
            ASSERT_EQ(registers[0], test_value) 
                << "Offset " << test_offsets[i] << " (offset5=" << offset5_values[i] << ")";
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }

    // Test case 4: LDR all registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        uint32_t test_value = 0x60000000;
        
        // Pre-store value in memory
        gba.getCPU().getMemory().write32(0x00001010, test_value);
        
        for (int rd = 0; rd < 8; rd++) {
            registers.fill(0); // Reset all registers for each test
            registers[1] = 0x00001000; // Base address (must be set after reset)
            
            // LDR Rd, [R1, #16] (offset5=4)
            uint16_t opcode = 0x6800 | (4 << 6) | (1 << 3) | rd;
            gba.getCPU().getMemory().write16(rd * 4, opcode);
            registers[15] = rd * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify each register loaded the same value
            ASSERT_EQ(registers[rd], test_value) << "Register R" << rd;
            // Validate that only the destination register and PC changed
            std::set<int> changed_regs = {rd, 15};
            if (rd == 1) {
                // When rd is the base register, both base and result are valid
                changed_regs.insert(1);
            }
            validateUnchangedRegisters(cpu, beforeState, changed_regs);
        }
    }
}

TEST(Format9, STRB_BYTE_IMMEDIATE_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: STRB R0, [R1, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[0] = 0x123456AB; // Value to store (only LSB should be stored)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x7008); // STRB R0, [R1, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify only the byte was stored
        uint8_t stored_value = gba.getCPU().getMemory().read8(0x00001000);
        ASSERT_EQ(stored_value, static_cast<uint8_t>(0xAB));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: STRB R2, [R3, #5] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001200; // Base address
        registers[2] = 0xFFFFFF99; // Value to store (only LSB)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x715A); // STRB R2, [R3, #5] (offset5=5)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify only the byte was stored at base + 5
        uint8_t stored_value = gba.getCPU().getMemory().read8(0x00001205);
        ASSERT_EQ(stored_value, static_cast<uint8_t>(0x99));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: STRB with different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001000; // Base address
        
        uint8_t test_offsets[] = {1, 7, 15, 23, 31}; // Byte offsets (same as offset5)
        uint8_t test_bytes[] = {0x11, 0x22, 0x33, 0x44, 0x55};
        
        for (size_t i = 0; i < sizeof(test_offsets)/sizeof(test_offsets[0]); i++) {
            registers[5] = 0x12340000 | test_bytes[i]; // Set byte value
            
            // STRB R5, [R4, #offset]
            uint16_t opcode = 0x7000 | (test_offsets[i] << 6) | (4 << 3) | 5;
            gba.getCPU().getMemory().write16(i * 4, opcode);
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify byte stored at correct address
            uint8_t stored_value = gba.getCPU().getMemory().read8(0x00001000 + test_offsets[i]);
            ASSERT_EQ(stored_value, test_bytes[i]) 
                << "Offset " << (int)test_offsets[i] << " byte 0x" << std::hex << (int)test_bytes[i];
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 4: STRB different byte values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        
        uint8_t test_bytes[] = {0x00, 0x01, 0x7F, 0x80, 0xFF};
        
        for (size_t i = 0; i < sizeof(test_bytes)/sizeof(test_bytes[0]); i++) {
            registers[0] = 0xABCD0000 | test_bytes[i]; // Set byte value
            
            // STRB R0, [R1, #offset] (use i+10 as offset to avoid overlap)
            uint16_t opcode = 0x7000 | ((i + 10) << 6) | (1 << 3) | 0;
            gba.getCPU().getMemory().write16(i * 4, opcode);
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify byte was stored correctly
            uint8_t stored_value = gba.getCPU().getMemory().read8(0x00001000 + i + 10);
            ASSERT_EQ(stored_value, test_bytes[i]) << "Byte value 0x" << std::hex << (int)test_bytes[i];
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 5: STRB maximum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[6] = 0x00001000; // Base address
        registers[7] = 0x12345677; // Value to store
        
        // STRB R7, [R6, #31] (offset5=31, maximum)
        gba.getCPU().getMemory().write16(0x00000000, 0x77F7); // STRB R7, [R6, #31]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify byte stored at maximum offset
        uint8_t stored_value = gba.getCPU().getMemory().read8(0x0000101F);
        ASSERT_EQ(stored_value, static_cast<uint8_t>(0x77));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}

TEST(Format9, LDRB_BYTE_IMMEDIATE_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: LDRB R0, [R1, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[0] = 0xDEADBEEF; // Should be overwritten with zero-extended byte
        
        // Pre-store a byte value in memory
        gba.getCPU().getMemory().write8(0x00001000, 0xA5);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x7808); // LDRB R0, [R1, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the byte was loaded and zero-extended
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x000000A5));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDRB R3, [R4, #7] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001300; // Base address
        registers[3] = 0xFFFFFFFF; // Should be overwritten
        
        // Pre-store a byte value in memory
        gba.getCPU().getMemory().write8(0x00001307, 0x7B);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x79E3); // LDRB R3, [R4, #7] (offset5=7)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the byte was loaded and zero-extended
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x0000007B));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 3: LDRB with different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        
        uint8_t test_offsets[] = {2, 9, 16, 25, 31}; // Byte offsets (same as offset5)
        uint8_t test_bytes[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
        
        for (size_t i = 0; i < sizeof(test_offsets)/sizeof(test_offsets[0]); i++) {
            // Pre-store byte in memory
            gba.getCPU().getMemory().write8(0x00001000 + test_offsets[i], test_bytes[i]);
            
            registers[0] = 0xDEADBEEF; // Reset destination
            
            // LDRB R0, [R1, #offset]
            uint16_t opcode = 0x7800 | (test_offsets[i] << 6) | (1 << 3) | 0;
            gba.getCPU().getMemory().write16(i * 4, opcode);
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify byte loaded and zero-extended correctly
            ASSERT_EQ(registers[0], static_cast<uint32_t>(test_bytes[i])) 
                << "Offset " << (int)test_offsets[i] << " byte 0x" << std::hex << (int)test_bytes[i];
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }

    // Test case 4: LDRB different byte values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[2] = 0x00001000; // Base address
        
        uint8_t test_bytes[] = {0x00, 0x01, 0x7F, 0x80, 0xFF};
        
        for (size_t i = 0; i < sizeof(test_bytes)/sizeof(test_bytes[0]); i++) {
            // Pre-store byte in memory
            gba.getCPU().getMemory().write8(0x00001000 + i + 5, test_bytes[i]);
            
            registers[1] = 0xDEADBEEF; // Reset destination
            
            // LDRB R1, [R2, #offset] (use i+5 as offset)
            uint16_t opcode = 0x7800 | ((i + 5) << 6) | (2 << 3) | 1;
            gba.getCPU().getMemory().write16(i * 4, opcode);
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify byte was loaded and zero-extended correctly (no sign extension)
            ASSERT_EQ(registers[1], static_cast<uint32_t>(test_bytes[i])) 
                << "Byte value 0x" << std::hex << (int)test_bytes[i] << " should be zero-extended";
            validateUnchangedRegisters(cpu, beforeState, {1, 15});
        }
    }
}

TEST(Format9, STR_LDR_ROUNDTRIP_TESTS) {
    std::string beforeState;

    // Test case 1: Word round-trip tests
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        
        uint32_t test_values[] = {0x00000000, 0x12345678, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF};
        uint32_t test_offsets[] = {0, 8, 16, 32, 64}; // Byte offsets
        uint32_t offset5_values[] = {0, 2, 4, 8, 16}; // Corresponding offset5 values
        
        for (size_t i = 0; i < sizeof(test_values)/sizeof(test_values[0]); i++) {
            registers[0] = test_values[i];
            
            // Store word: STR R0, [R1, #offset]
            uint16_t store_opcode = 0x6000 | (offset5_values[i] << 6) | (1 << 3) | 0;
            gba.getCPU().getMemory().write16(i * 8, store_opcode);
            registers[15] = i * 8; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Load back: LDR R2, [R1, #offset]
            registers[2] = 0xDEADBEEF;
            uint16_t load_opcode = 0x6800 | (offset5_values[i] << 6) | (1 << 3) | 2;
            gba.getCPU().getMemory().write16(i * 8 + 2, load_opcode);
            cpu.execute(1);
            
            // Verify word round-trip
            ASSERT_EQ(registers[2], test_values[i]) 
                << "Word 0x" << std::hex << test_values[i] << " at offset " << test_offsets[i];
            validateUnchangedRegisters(cpu, beforeState, {2, 15});
        }
    }

    // Test case 2: Byte round-trip tests
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001100; // Base address
        
        uint8_t test_bytes[] = {0x00, 0x12, 0x7F, 0x80, 0xFF};
        uint8_t test_offsets[] = {0, 3, 7, 15, 31}; // Byte offsets (same as offset5)
        
        for (size_t i = 0; i < sizeof(test_bytes)/sizeof(test_bytes[0]); i++) {
            registers[4] = 0xABCD0000 | test_bytes[i]; // Set byte value
            
            // Store byte: STRB R4, [R3, #offset]
            uint16_t store_opcode = 0x7000 | (test_offsets[i] << 6) | (3 << 3) | 4;
            gba.getCPU().getMemory().write16(i * 8, store_opcode);
            registers[15] = i * 8; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Load back: LDRB R5, [R3, #offset]
            registers[5] = 0xDEADBEEF;
            uint16_t load_opcode = 0x7800 | (test_offsets[i] << 6) | (3 << 3) | 5;
            gba.getCPU().getMemory().write16(i * 8 + 2, load_opcode);
            cpu.execute(1);
            
            // Verify byte round-trip (zero-extended)
            ASSERT_EQ(registers[5], static_cast<uint32_t>(test_bytes[i])) 
                << "Byte 0x" << std::hex << (int)test_bytes[i] << " at offset " << (int)test_offsets[i];
            validateUnchangedRegisters(cpu, beforeState, {5, 15});
        }
    }
}

TEST(Format9, EDGE_CASES_AND_BOUNDARY_CONDITIONS) {
    std::string beforeState;

    // Test case 1: Memory boundary conditions
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test near end of memory (0x0000 to 0x1FFF range)
        registers[1] = 0x00001F80; // Base near end
        registers[0] = 0x99887766; // Test value
        
        // STR R0, [R1, #124] - Address will be 0x1FFC (within bounds)
        gba.getCPU().getMemory().write16(0x00000000, 0x67C8); // STR R0, [R1, #124] (offset5=31)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify value stored correctly at boundary
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001FFC);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x99887766));
        validateUnchangedRegisters(cpu, beforeState, {15});
        
        // Test byte at very end
        registers[2] = 0x00001FFF; // Base at very end
        registers[3] = 0x12345655; // Byte value
        
        // STRB R3, [R2, #0] - Address will be 0x1FFF (last byte)
        gba.getCPU().getMemory().write16(0x00000002, 0x7013); // STRB R3, [R2, #0]
        beforeState = serializeCPUState(cpu); // Capture state before second instruction
        cpu.execute(1);
        
        uint8_t stored_byte = gba.getCPU().getMemory().read8(0x00001FFF);
        ASSERT_EQ(stored_byte, static_cast<uint8_t>(0x55));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: Zero offset operations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001500; // Base address
        registers[5] = 0x11223344; // Test value
        
        // STR R5, [R4, #0] - Zero offset (base address only)
        gba.getCPU().getMemory().write16(0x00000000, 0x6025); // STR R5, [R4, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify stored at base address
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001500);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x11223344));
        
        // Load back with zero offset
        registers[6] = 0xDEADBEEF;
        gba.getCPU().getMemory().write16(0x00000002, 0x6826); // LDR R6, [R4, #0]
        cpu.execute(1);
        
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x11223344));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 3: Maximum register combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00001000; // Base (max register)
        registers[6] = 0xFEDCBA98; // Value (max-1 register)
        
        // STR R6, [R7, #120] (offset5=30) - Use maximum register numbers
        gba.getCPU().getMemory().write16(0x00000000, 0x67BE); // STR R6, [R7, #120]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify value stored correctly
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001078);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0xFEDCBA98));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: Word alignment effects
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test that word operations use word-aligned offsets (offset * 4)
        registers[1] = 0x00001000; // Base address
        registers[0] = 0x12345678; // Test value
        
        // Test offset5=1 should give byte offset 4, offset5=5 should give byte offset 20
        uint32_t test_cases[][3] = {
            {1, 4, 0x6048},   // offset5=1 -> byte offset 4 -> opcode 0x6048
            {5, 20, 0x6148},  // offset5=5 -> byte offset 20 -> opcode 0x6148  
            {10, 40, 0x6288}, // offset5=10 -> byte offset 40 -> opcode 0x6288
        };
        
        for (size_t i = 0; i < 3; i++) {
            uint32_t offset5 = test_cases[i][0];
            uint32_t expected_byte_offset = test_cases[i][1];
            uint32_t expected_opcode = test_cases[i][2];
            
            registers[0] = 0x70000000 + i; // Unique value
            
            // STR R0, [R1, #offset]
            gba.getCPU().getMemory().write16(i * 4, expected_opcode);
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify stored at correct byte address
            uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001000 + expected_byte_offset);
            ASSERT_EQ(stored_value, static_cast<uint32_t>(0x70000000 + i)) 
                << "offset5=" << offset5 << " should give byte offset " << expected_byte_offset;
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 5: Base register modification
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Initial base
        registers[0] = 0x11111111; // Test value
        
        // Store: STR R0, [R1, #8]
        gba.getCPU().getMemory().write16(0x00000000, 0x6088); // STR R0, [R1, #8] (offset5=2)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify stored at original base + 8
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001008);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x11111111));
        
        // Modify base register
        registers[1] = 0x00001200;
        
        // Store again with same instruction
        registers[0] = 0x22222222;
        gba.getCPU().getMemory().write16(0x00000002, 0x6088); // STR R0, [R1, #8]
        cpu.execute(1);
        
        // Verify stored at new base + 8
        uint32_t stored_value2 = gba.getCPU().getMemory().read32(0x00001208);
        ASSERT_EQ(stored_value2, static_cast<uint32_t>(0x22222222));
        
        // Original location should be unchanged
        uint32_t original_value = gba.getCPU().getMemory().read32(0x00001008);
        ASSERT_EQ(original_value, static_cast<uint32_t>(0x11111111));
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 15});
    }
}
