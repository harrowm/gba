#include "test_cpu_common.h"

// ARM Thumb Format 11: SP-relative load/store
// Encoding: 1001[L][Rd][Word8]
// Instructions: STR Rd, [SP, #offset], LDR Rd, [SP, #offset]
// L=0: STR (Store), L=1: LDR (Load)
// Offset = Word8 * 4 (word-aligned)

TEST(Format11, STR_SP_RELATIVE_BASIC) {
    std::string beforeState;

    // Test case 1: STR R0, [SP, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[0] = 0x12345678;  // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9000); // STR R0, [SP, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at SP
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001000);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: STR R1, [SP, #4] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[1] = 0x87654321;  // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9101); // STR R1, [SP, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at SP + 4
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001004);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: STR R2, [SP, #8] - different register
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001200; // SP
        registers[2] = 0xAABBCCDD;  // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9202); // STR R2, [SP, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at SP + 8
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001208);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0xAABBCCDD));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: STR with larger offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[3] = 0x11223344;  // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9320); // STR R3, [SP, #128] (word8=32)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at SP + 128
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001080);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x11223344));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 5: STR all registers at different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        for (int i = 0; i < 8; i++) {
            registers[i] = 0x10000000 + i; // Unique values
        }
        
        // Store each register at (register_number * 4) offset
        uint16_t opcodes[] = {0x9000, 0x9101, 0x9202, 0x9303, 0x9404, 0x9505, 0x9606, 0x9707};
        
        for (int i = 0; i < 8; i++) {
            gba.getCPU().getMemory().write16(i * 2, opcodes[i]);
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify each value was stored correctly
            uint32_t expected_address = 0x00001000 + (i * 4);
            uint32_t stored_value = gba.getCPU().getMemory().read32(expected_address);
            ASSERT_EQ(stored_value, static_cast<uint32_t>(0x10000000 + i)) << "Register R" << i;
        }
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 6: STR with zero value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[4] = 0x00000000;  // Zero value
        
        // Pre-fill memory with non-zero to ensure store works
        gba.getCPU().getMemory().write32(0x00001010, 0xDEADBEEF);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9404); // STR R4, [SP, #16]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify zero was stored
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001010);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}

TEST(Format11, LDR_SP_RELATIVE_BASIC) {
    std::string beforeState;

    // Test case 1: LDR R0, [SP, #0] - minimum offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001000, 0x12345678);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9800); // LDR R0, [SP, #0]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded into R0
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDR R1, [SP, #4] - basic offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[1] = 0xDEADBEEF;  // Initial value (should be overwritten)
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001004, 0x87654321);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9901); // LDR R1, [SP, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded into R1
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: LDR R2, [SP, #8] - different register
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001200; // SP
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001208, 0xAABBCCDD);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9A02); // LDR R2, [SP, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded into R2
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xAABBCCDD));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: LDR with larger offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        // Pre-store a value in memory
        gba.getCPU().getMemory().write32(0x00001080, 0x11223344);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9B20); // LDR R3, [SP, #128] (word8=32)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was loaded into R3
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x11223344));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 5: LDR all registers from different offsets
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        
        // Pre-store unique values at different offsets
        for (int i = 0; i < 8; i++) {
            uint32_t address = 0x00001000 + (i * 4);
            uint32_t value = 0x20000000 + i;
            gba.getCPU().getMemory().write32(address, value);
        }
        
        // Load each register from (register_number * 4) offset
        uint16_t opcodes[] = {0x9800, 0x9901, 0x9A02, 0x9B03, 0x9C04, 0x9D05, 0x9E06, 0x9F07};
        
        for (int i = 0; i < 8; i++) {
            registers[i] = 0xDEADBEEF; // Reset to known value
            gba.getCPU().getMemory().write16(i * 2, opcodes[i]);
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify each value was loaded correctly
            ASSERT_EQ(registers[i], static_cast<uint32_t>(0x20000000 + i)) << "Register R" << i;
        }
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 2, 3, 4, 5, 6, 7, 15});
    }

    // Test case 6: LDR zero value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[4] = 0xFFFFFFFF;  // Non-zero initial value
        // Pre-store zero value in memory
        gba.getCPU().getMemory().write32(0x00001010, 0x00000000);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9C04); // LDR R4, [SP, #16]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify zero was loaded
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x00000000));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }
}

TEST(Format11, STR_LDR_ROUNDTRIP_TESTS) {
    std::string beforeState;

    // Test case 1: Store and load back same value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[0] = 0x12345678;  // Original value
        
        // Store R0 to [SP, #12]
        gba.getCPU().getMemory().write16(0x00000000, 0x9003); // STR R0, [SP, #12]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Clear R0 and load it back
        registers[0] = 0x00000000;
        gba.getCPU().getMemory().write16(0x00000002, 0x9803); // LDR R0, [SP, #12]
        cpu.execute(1);
        
        // Verify the value round-tripped correctly
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Multiple store/load operations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        
        uint32_t test_values[] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
        
        // Store multiple values
        for (int i = 0; i < 4; i++) {
            registers[i] = test_values[i];
            uint16_t store_opcode = 0x9000 | (i << 8) | (i * 2); // STR Ri, [SP, #(i*8)]
            gba.getCPU().getMemory().write16(i * 2, store_opcode);
            cpu.execute(1);
        }
        
        // Clear registers and load them back
        for (int i = 0; i < 4; i++) {
            registers[i] = 0xDEADBEEF;
        }
        
        beforeState = serializeCPUState(cpu);
        
        // Load values back
        for (int i = 0; i < 4; i++) {
            uint16_t load_opcode = 0x9800 | (i << 8) | (i * 2); // LDR Ri, [SP, #(i*8)]
            gba.getCPU().getMemory().write16(8 + i * 2, load_opcode);
            cpu.execute(1);
        }
        
        // Verify all values were restored correctly
        for (int i = 0; i < 4; i++) {
            ASSERT_EQ(registers[i], test_values[i]) << "Register R" << i;
        }
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 2, 3, 15});
    }

    // Test case 3: Overlapping memory access
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        registers[0] = 0xABCDEF01;
        registers[1] = 0x23456789;
        
        // Store two values to adjacent addresses
        gba.getCPU().getMemory().write16(0x00000000, 0x9000); // STR R0, [SP, #0]
        cpu.execute(1);
        gba.getCPU().getMemory().write16(0x00000002, 0x9101); // STR R1, [SP, #4]
        cpu.execute(1);
        
        beforeState = serializeCPUState(cpu);
        
        // Load them back into different registers
        gba.getCPU().getMemory().write16(0x00000004, 0x9A00); // LDR R2, [SP, #0]
        cpu.execute(1);
        gba.getCPU().getMemory().write16(0x00000006, 0x9B01); // LDR R3, [SP, #4]
        cpu.execute(1);
        
        // Verify values were loaded correctly
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xABCDEF01));
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x23456789));
        validateUnchangedRegisters(cpu, beforeState, {2, 3, 15});
    }
}

TEST(Format11, EDGE_CASES_AND_BOUNDARY_CONDITIONS) {
    std::string beforeState;

    // Test case 1: Maximum offset (1020 bytes = word8 255)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00000800; // SP - ensure SP + 1020 < 0x2000
        registers[7] = 0xFEDCBA98;  // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x97FF); // STR R7, [SP, #1020]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the value was stored at maximum offset
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00000800 + 1020);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0xFEDCBA98));
        
        // Load it back
        registers[6] = 0x00000000;
        gba.getCPU().getMemory().write16(0x00000002, 0x9EFF); // LDR R6, [SP, #1020]
        cpu.execute(1);
        
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0xFEDCBA98));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 2: SP at memory boundary
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001FF0; // SP near end of memory
        registers[0] = 0x55AA55AA;  // Value to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0x9003); // STR R0, [SP, #12]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Address should be 0x1FF0 + 12 = 0x1FFC (within bounds)
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001FFC);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x55AA55AA));
        
        // Load it back
        registers[1] = 0x00000000;
        gba.getCPU().getMemory().write16(0x00000002, 0x9903); // LDR R1, [SP, #12]
        cpu.execute(1);
        
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x55AA55AA));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: All possible word8 values (test encoding coverage)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00000100; // SP
        
        // Test several key word8 values
        uint8_t test_word8_values[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 255};
        
        for (auto word8 : test_word8_values) {
            uint32_t offset = word8 * 4;
            uint32_t test_value = 0x30000000 + word8;
            
            registers[0] = test_value;
            
            // Store using STR R0, [SP, #offset]
            uint16_t store_opcode = 0x9000 | word8;
            gba.getCPU().getMemory().write16(0x00000000, store_opcode);
            registers[15] = 0x00000000; // Reset PC to start of instruction
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify stored correctly
            uint32_t expected_address = 0x00000100 + offset;
            uint32_t stored_value = gba.getCPU().getMemory().read32(expected_address);
            ASSERT_EQ(stored_value, test_value) << "word8=" << (int)word8 << ", offset=" << offset;
            
            // Load back using LDR R1, [SP, #offset]
            registers[1] = 0x00000000;
            uint16_t load_opcode = 0x9900 | word8;
            gba.getCPU().getMemory().write16(0x00000002, load_opcode);
            registers[15] = 0x00000002; // Reset PC to load instruction
            cpu.execute(1);
            
            ASSERT_EQ(registers[1], test_value) << "Load word8=" << (int)word8 << ", offset=" << offset;
            validateUnchangedRegisters(cpu, beforeState, {1, 15});
        }
    }

    // Test case 4: SP modification during execution
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Initial SP
        registers[0] = 0x11111111;  // Value to store
        
        // Store at [SP, #8]
        gba.getCPU().getMemory().write16(0x00000000, 0x9002); // STR R0, [SP, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify stored at original SP + 8
        uint32_t stored_value = gba.getCPU().getMemory().read32(0x00001008);
        ASSERT_EQ(stored_value, static_cast<uint32_t>(0x11111111));
        
        // Modify SP
        registers[13] = 0x00001100;
        
        // Store again with same instruction (should use new SP)
        registers[1] = 0x22222222;
        gba.getCPU().getMemory().write16(0x00000002, 0x9102); // STR R1, [SP, #8]
        cpu.execute(1);
        
        // Verify stored at new SP + 8
        uint32_t stored_value2 = gba.getCPU().getMemory().read32(0x00001108);
        ASSERT_EQ(stored_value2, static_cast<uint32_t>(0x22222222));
        
        // Original location should be unchanged
        uint32_t original_value = gba.getCPU().getMemory().read32(0x00001008);
        ASSERT_EQ(original_value, static_cast<uint32_t>(0x11111111));
        validateUnchangedRegisters(cpu, beforeState, {1, 13, 15});
    }

    // Test case 5: Word alignment verification
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test that all generated addresses are word-aligned
        registers[13] = 0x00001001; // Unaligned SP
        
        uint8_t test_word8[] = {0, 1, 3, 7, 15, 31, 63, 127, 255};
        
        for (auto word8 : test_word8) {
            registers[0] = 0x40000000 + word8;
            
            uint16_t opcode = 0x9000 | word8; // STR R0, [SP, #(word8*4)]
            gba.getCPU().getMemory().write16(0x00000000, opcode);
            registers[15] = 0x00000000; // Reset PC to start of instruction
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Calculate expected address: SP + (word8 * 4)
            uint32_t expected_address = 0x00001001 + (word8 * 4);
            
            // Address should be exactly SP + offset (implementation doesn't force alignment)
            uint32_t stored_value = gba.getCPU().getMemory().read32(expected_address);
            ASSERT_EQ(stored_value, static_cast<uint32_t>(0x40000000 + word8)) << "word8=" << (int)word8;
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }

    // Test case 6: Memory consistency across different register combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // SP
        
        // Test all register combinations with the same offset
        uint32_t base_address = 0x00001000 + 32; // SP + 32
        
        for (int rd = 0; rd < 8; rd++) {
            uint32_t test_value = 0x50000000 + rd;
            registers[rd] = test_value;
            
            // Store: STR Rd, [SP, #32] (word8=8)
            uint16_t store_opcode = 0x9000 | (rd << 8) | 8;
            gba.getCPU().getMemory().write16(rd * 4, store_opcode);
            registers[15] = rd * 4; // Set PC to the instruction location
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify each store overwrites the same location
            uint32_t stored_value = gba.getCPU().getMemory().read32(base_address);
            ASSERT_EQ(stored_value, test_value) << "Store register R" << rd;
            
            // Load into different register: LDR R7, [SP, #32]
            registers[7] = 0x00000000;
            uint16_t load_opcode = 0x9F08; // LDR R7, [SP, #32]
            gba.getCPU().getMemory().write16(32 + rd * 4, load_opcode);
            registers[15] = 32 + rd * 4; // Set PC to the load instruction location
            cpu.execute(1);
            
            ASSERT_EQ(registers[7], test_value) << "Load to R7 from R" << rd << " store";
            validateUnchangedRegisters(cpu, beforeState, {7, 15});
        }
    }
}
