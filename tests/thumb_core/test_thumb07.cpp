
/**
 * test_thumb07.cpp - Format 7: Load/store with register offset instruction tests
 * 
 * This file tests Thumb Format 7 instructions which provide load/store operations
 * using register-based addressing with a register offset. Format 7 enables accessing
 * memory locations calculated by adding two registers together.
 *
 * Instruction Format:
 * |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
 * | 0| 1| 0| 1| L| B| 0|     Ro    |     Rb    | Rd |
 *
 * Format 7 Encoding Details:
 * - Bits [15:12] = 0101 (Format 7 identifier)
 * - Bit [11]     = L (Load/Store: 0=Store, 1=Load)
 * - Bit [10]     = B (Byte/Word: 0=Word, 1=Byte)
 * - Bit [9]      = 0 (reserved)
 * - Bits [8:6]   = Ro (offset register, R0-R7)
 * - Bits [5:3]   = Rb (base register, R0-R7)
 * - Bits [2:0]   = Rd (destination/source register, R0-R7)
 *
 * Supported Operations:
 * - STR Rd, [Rb, Ro]: Store word from Rd to memory[Rb + Ro]
 * - LDR Rd, [Rb, Ro]: Load word from memory[Rb + Ro] to Rd  
 * - STRB Rd, [Rb, Ro]: Store byte from Rd to memory[Rb + Ro]
 * - LDRB Rd, [Rb, Ro]: Load byte from memory[Rb + Ro] to Rd
 *
 * Effective Address Calculation:
 * - Address = Rb + Ro (both registers treated as unsigned values)
 * - No bounds checking performed by instruction
 * - Word operations must be word-aligned for proper behavior
 * - Byte operations work with any address alignment
 *
 * Test Infrastructure:
 * - Uses ThumbCPUTestBase for modern register access via R() method
 * - Uses assembleAndWriteThumb() for Keystone-based instruction assembly
 * - Uses execute() method for cycle-accurate instruction execution
 * - Comprehensive memory access pattern testing
 *
 * Coverage:
 * - All operation types (STR, LDR, STRB, LDRB)
 * - Various register combinations for base, offset, and data registers
 * - Memory boundary testing and alignment requirements
 * - Zero offset cases and maximum offset scenarios
 * - Data integrity verification for word and byte operations
 */

#include "thumb_test_base.h"

class ThumbCPUTest7 : public ThumbCPUTestBase {
};

// ARM Thumb Format 7: Load/store with register offset
// Encoding: 0101[L][B][0][Ro][Rb][Rd]
// Instructions: STR, STRB, LDR, LDRB
// L=0: Store, L=1: Load
// B=0: Word, B=1: Byte
// Effective address = Rb + Ro

TEST_F(ThumbCPUTest7, StrWordBasic) {
    // Test case: STR R0, [R1, R2] - basic register offset
    setup_registers({{1, 0x00000100}, {2, 0x00000008}, {0, 0x12345678}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("str r0, [r1, r2]", cpu.R()[15]));
    execute(1);
    
    // Verify the value was stored at base + offset
    uint32_t stored_value = memory.read32(0x00000108); // 0x100 + 0x8
    EXPECT_EQ(stored_value, 0x12345678u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u); // PC should advance
}

TEST_F(ThumbCPUTest7, StrWordDifferentRegisters) {
    // Test case: STR R3, [R4, R5] - different registers
    setup_registers({{4, 0x00000200}, {5, 0x00000010}, {3, 0x87654321}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("str r3, [r4, r5]", cpu.R()[15]));
    execute(1);
    
    // Verify the value was stored at base + offset
    uint32_t stored_value = memory.read32(0x00000210); // 0x200 + 0x10
    EXPECT_EQ(stored_value, 0x87654321u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, StrWordZeroOffset) {
    // Test case: STR with zero offset
    setup_registers({{6, 0x00000300}, {7, 0x00000000}, {1, 0xAABBCCDD}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("str r1, [r6, r7]", cpu.R()[15]));
    execute(1);
    
    // Verify the value was stored at base address (no offset)
    uint32_t stored_value = memory.read32(0x00000300);
    EXPECT_EQ(stored_value, 0xAABBCCDDu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, StrWordAllRegisterCombinations) {
    // Test all register combinations for destination register
    setup_registers({{3, 0x00000400}, {4, 0x00000010}});
    
    for (int rd = 0; rd < 3; rd++) { // Only test rd 0-2 to avoid conflicts with base/offset
        cpu.R()[rd] = 0x12345600 + rd;
        cpu.R()[15] = rd * 4; // Set PC for each test
        
        std::string asm_str = "str r" + std::to_string(rd) + ", [r3, r4]";
        ASSERT_TRUE(assembleAndWriteThumb(asm_str, cpu.R()[15]));
        execute(1);
        
        // Verify each value was stored correctly
        uint32_t stored_value = memory.read32(0x00000410); // R3 + R4
        EXPECT_EQ(stored_value, static_cast<uint32_t>(0x12345600 + rd)) << "Register R" << rd;
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(rd * 4 + 2));
    }
}

TEST_F(ThumbCPUTest7, StrWordDifferentOffsets) {
    // Test STR with different offset values
    setup_registers({{1, 0x00000500}, {0, 0x55555555}});
    
    uint32_t offsets[] = {0, 4, 8, 16, 32, 64, 128};
    
    for (size_t i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++) {
        cpu.R()[2] = offsets[i]; // Offset register
        cpu.R()[15] = i * 4; // Reset PC
        
        ASSERT_TRUE(assembleAndWriteThumb("str r0, [r1, r2]", cpu.R()[15]));
        execute(1);
        
        // Verify value stored at correct address
        uint32_t expected_address = 0x00000500 + offsets[i];
        uint32_t stored_value = memory.read32(expected_address);
        EXPECT_EQ(stored_value, 0x55555555u) << "Offset " << offsets[i];
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 4 + 2));
    }
}

TEST_F(ThumbCPUTest7, LdrWordBasic) {
    // Test case: LDR R0, [R1, R2] - basic register offset
    setup_registers({{1, 0x00000600}, {2, 0x00000008}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store test data
    memory.write32(0x00000608, 0x12345678); // Store at base + offset
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r0, [r1, r2]", cpu.R()[15]));
    execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x12345678u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, LdrWordDifferentRegisters) {
    // Test case: LDR R3, [R4, R5] - different registers
    setup_registers({{4, 0x00000700}, {5, 0x00000010}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store test data
    memory.write32(0x00000710, 0x87654321); // Store at base + offset
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r3, [r4, r5]", cpu.R()[15]));
    execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0x87654321u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, LdrWordZeroOffset) {
    // Test case: LDR with zero offset
    setup_registers({{6, 0x00000800}, {7, 0x00000000}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store test data
    memory.write32(0x00000800, 0xAABBCCDD); // Store at base address
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r1, [r6, r7]", cpu.R()[15]));
    execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0xAABBCCDDu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, LdrWordAllRegisterCombinations) {
    // Test all register combinations for destination register
    setup_registers({{3, 0x00000900}, {4, 0x00000010}});
    
    for (int rd = 0; rd < 3; rd++) { // Only test rd 0-2 to avoid conflicts
        uint32_t test_value = 0x12345600 + rd;
        memory.write32(0x00000910, test_value); // Store at R3 + R4
        
        cpu.R()[15] = rd * 4; // Set PC for each test
        
        std::string asm_str = "ldr r" + std::to_string(rd) + ", [r3, r4]";
        ASSERT_TRUE(assembleAndWriteThumb(asm_str, cpu.R()[15]));
        execute(1);
        
        EXPECT_EQ(cpu.R()[rd], test_value) << "Register R" << rd;
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(rd * 4 + 2));
    }
}

TEST_F(ThumbCPUTest7, LdrWordDifferentOffsets) {
    // Test LDR with different offset values
    setup_registers({{1, 0x00000A00}});
    
    uint32_t offsets[] = {0, 4, 8, 16, 32, 64, 128};
    
    for (size_t i = 0; i < sizeof(offsets)/sizeof(offsets[0]); i++) {
        cpu.R()[2] = offsets[i]; // Offset register
        cpu.R()[15] = i * 4; // Reset PC
        
        uint32_t test_value = 0x11111100 + i;
        uint32_t target_address = 0x00000A00 + offsets[i];
        memory.write32(target_address, test_value);
        
        ASSERT_TRUE(assembleAndWriteThumb("ldr r0, [r1, r2]", cpu.R()[15]));
        execute(1);
        
        EXPECT_EQ(cpu.R()[0], test_value) << "Offset " << offsets[i];
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 4 + 2));
    }
}

TEST_F(ThumbCPUTest7, StrbByteBasic) {
    // Test case: STRB R0, [R1, R2] - basic byte store with register offset
    setup_registers({{1, 0x00000B00}, {2, 0x00000008}, {0, 0x12345678}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("strb r0, [r1, r2]", cpu.R()[15]));
    execute(1);
    
    // Verify only the low byte was stored
    uint8_t stored_byte = memory.read8(0x00000B08); // 0xB00 + 0x8
    EXPECT_EQ(stored_byte, 0x78u); // Low byte of 0x12345678
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, StrbByteDifferentRegisters) {
    // Test case: STRB R3, [R4, R5] - different registers
    setup_registers({{4, 0x00000C00}, {5, 0x00000010}, {3, 0x87654321}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("strb r3, [r4, r5]", cpu.R()[15]));
    execute(1);
    
    // Verify only the low byte was stored
    uint8_t stored_byte = memory.read8(0x00000C10); // 0xC00 + 0x10
    EXPECT_EQ(stored_byte, 0x21u); // Low byte of 0x87654321
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, StrbByteZeroOffset) {
    // Test case: STRB with zero offset
    setup_registers({{6, 0x00000D00}, {7, 0x00000000}, {1, 0xAABBCCDD}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("strb r1, [r6, r7]", cpu.R()[15]));
    execute(1);
    
    // Verify only the low byte was stored
    uint8_t stored_byte = memory.read8(0x00000D00);
    EXPECT_EQ(stored_byte, 0xDDu); // Low byte of 0xAABBCCDD
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, StrbByteAllValues) {
    // Test STRB with all possible byte values
    setup_registers({{1, 0x00000E00}, {2, 0x00000000}});
    
    for (int byte_val = 0; byte_val <= 255; byte_val += 17) { // Sample values
        cpu.R()[0] = 0xFFFFFF00 | byte_val; // Set low byte
        cpu.R()[15] = (byte_val / 17) * 4; // Set PC for each test
        
        ASSERT_TRUE(assembleAndWriteThumb("strb r0, [r1, r2]", cpu.R()[15]));
        execute(1);
        
        uint8_t stored_byte = memory.read8(0x00000E00);
        EXPECT_EQ(stored_byte, static_cast<uint8_t>(byte_val)) << "Byte value " << byte_val;
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>((byte_val / 17) * 4 + 2));
    }
}

TEST_F(ThumbCPUTest7, LdrbByteBasic) {
    // Test case: LDRB R0, [R1, R2] - basic byte load with register offset
    setup_registers({{1, 0x00000F00}, {2, 0x00000008}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store test data (byte)
    memory.write8(0x00000F08, 0x78); // Store at base + offset
    
    ASSERT_TRUE(assembleAndWriteThumb("ldrb r0, [r1, r2]", cpu.R()[15]));
    execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x78u); // Should be zero-extended
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, LdrbByteDifferentRegisters) {
    // Test case: LDRB R3, [R4, R5] - different registers
    setup_registers({{4, 0x00001000}, {5, 0x00000010}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store test data (byte)
    memory.write8(0x00001010, 0x87); // Store at base + offset
    
    ASSERT_TRUE(assembleAndWriteThumb("ldrb r3, [r4, r5]", cpu.R()[15]));
    execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0x87u); // Should be zero-extended
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, LdrbByteZeroOffset) {
    // Test case: LDRB with zero offset
    setup_registers({{6, 0x00001100}, {7, 0x00000000}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store test data (byte)
    memory.write8(0x00001100, 0xAA); // Store at base address
    
    ASSERT_TRUE(assembleAndWriteThumb("ldrb r1, [r6, r7]", cpu.R()[15]));
    execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0xAAu); // Should be zero-extended
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, LdrbByteZeroExtension) {
    // Test LDRB zero extension with high bit set
    setup_registers({{1, 0x00001200}, {2, 0x00000000}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store byte with high bit set
    memory.write8(0x00001200, 0xFF);
    
    ASSERT_TRUE(assembleAndWriteThumb("ldrb r0, [r1, r2]", cpu.R()[15]));
    execute(1);
    
    // Should be zero-extended, not sign-extended
    EXPECT_EQ(cpu.R()[0], 0xFFu); // Not 0xFFFFFFFF
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest7, StrLdrRoundtrip) {
    // Test STR/LDR roundtrip to ensure no data corruption
    setup_registers({{1, 0x00001300}, {2, 0x00000000}});
    
    uint32_t test_values[] = {0x00000000, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF, 0x55555555, 0xAAAAAAAA};
    
    for (size_t i = 0; i < sizeof(test_values)/sizeof(test_values[0]); i++) {
        cpu.R()[0] = test_values[i];
        cpu.R()[15] = i * 8; // Reset PC with space for two instructions
        
        // Store the value
        ASSERT_TRUE(assembleAndWriteThumb("str r0, [r1, r2]", cpu.R()[15]));
        execute(1);
        
        // Load back into different register
        cpu.R()[3] = 0xDEADBEEF; // Initialize with different value
        ASSERT_TRUE(assembleAndWriteThumb("ldr r3, [r1, r2]", cpu.R()[15]));
        execute(1);
        
        EXPECT_EQ(cpu.R()[3], test_values[i]) << "Value 0x" << std::hex << test_values[i];
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 8 + 4)); // Should advance by 4 total (2 instructions)
    }
}

TEST_F(ThumbCPUTest7, StrbLdrbRoundtrip) {
    // Test STRB/LDRB roundtrip to ensure byte operations work correctly
    setup_registers({{1, 0x00001400}, {2, 0x00000000}});
    
    uint8_t test_bytes[] = {0x00, 0xFF, 0x80, 0x7F, 0x55, 0xAA, 0x01, 0xFE};
    
    for (size_t i = 0; i < sizeof(test_bytes)/sizeof(test_bytes[0]); i++) {
        cpu.R()[0] = 0xFFFFFF00 | test_bytes[i]; // Set byte in word context
        cpu.R()[15] = i * 8; // Reset PC with space for two instructions
        
        // Store the byte
        ASSERT_TRUE(assembleAndWriteThumb("strb r0, [r1, r2]", cpu.R()[15]));
        execute(1);
        
        // Load back into different register
        cpu.R()[3] = 0xDEADBEEF; // Initialize with different value
        ASSERT_TRUE(assembleAndWriteThumb("ldrb r3, [r1, r2]", cpu.R()[15]));
        execute(1);
        
        EXPECT_EQ(cpu.R()[3], static_cast<uint32_t>(test_bytes[i])) << "Byte value 0x" << std::hex << static_cast<int>(test_bytes[i]);
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 8 + 4)); // Should advance by 4 total
    }
}

TEST_F(ThumbCPUTest7, EdgeCasesAndBoundaryConditions) {
    // Test edge cases: unaligned access, boundary conditions, etc.
    
    // Test 1: Store/Load at memory boundary
    setup_registers({{1, 0x00001FF0}, {2, 0x0000000C}}); // Near end of 8KB test memory
    cpu.R()[15] = 0x00000000;
    
    cpu.R()[0] = 0x12345678;
    ASSERT_TRUE(assembleAndWriteThumb("str r0, [r1, r2]", cpu.R()[15])); // Store at 0x1FFC
    execute(1);
    
    uint32_t stored_value = memory.read32(0x00001FFC);
    EXPECT_EQ(stored_value, 0x12345678u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // Test 2: Unaligned word access (should still work on ARM)
    cpu.R()[15] = 0x00000010;
    setup_registers({{1, 0x00001500}, {2, 0x00000001}}); // Unaligned address (0x1501)
    cpu.R()[0] = 0x87654321;
    
    ASSERT_TRUE(assembleAndWriteThumb("str r0, [r1, r2]", cpu.R()[15]));
    execute(1);
    
    // ARM handles unaligned access by rotating
    // The exact behavior depends on implementation, but instruction should complete
    EXPECT_EQ(cpu.R()[15], 0x00000012u);
    
    // Test 3: Maximum offset value
    cpu.R()[15] = 0x00000020;
    setup_registers({{1, 0x00001000}, {2, 0x000007FF}}); // Large offset
    cpu.R()[0] = 0xDEADBEEF;
    
    ASSERT_TRUE(assembleAndWriteThumb("str r0, [r1, r2]", cpu.R()[15]));
    execute(1);
    
    uint32_t large_offset_value = memory.read32(0x000017FF);
    EXPECT_EQ(large_offset_value, 0xDEADBEEFu);
    EXPECT_EQ(cpu.R()[15], 0x00000022u);
}
