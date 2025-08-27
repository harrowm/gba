
/**
 * test_thumb06.cpp - Format 6: PC-relative load instruction tests
 * 
 * This file tests Thumb Format 6 instructions which implement PC-relative loads.
 * Format 6 provides load word operations using PC-relative addressing with
 * immediate offsets, commonly used for loading constants and accessing literal pools.
 *
 * Instruction Format:
 * |15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
 * | 0| 1| 0| 0| 1|     Rd    |      Word8         |
 *
 * Format 6 Encoding Details:
 * - Bits [15:11] = 01001 (Format 6 identifier)
 * - Bits [10:8]  = Rd (destination register, R0-R7)
 * - Bits [7:0]   = Word8 (8-bit immediate offset, word-aligned)
 *
 * Operation: LDR Rd, [PC, #(Word8 << 2)]
 * - Effective address = (PC + 4) & ~3 + (Word8 << 2)
 * - PC is aligned to word boundary before adding offset
 * - Word8 is automatically shifted left by 2 for word alignment
 * - Offset range: 0 to 1020 bytes (0x000 to 0x3FC)
 *
 * Test Infrastructure:
 * - Uses ThumbCPUTestBase for modern register access via R() method
 * - Uses assembleAndWriteThumb() for Keystone-based instruction assembly
 * - Uses execute() method for cycle-accurate instruction execution
 * - Tests use hex literals for Keystone assembler compatibility (decimal literals
 *   cause generation of Thumb-2 F8DF opcodes instead of proper Thumb-1 Format 6)
 *
 * Coverage:
 * - All 8 destination registers (R0-R7)
 * - Full offset range testing (minimum to maximum)
 * - PC alignment verification with odd addresses
 * - Flag preservation during load operations
 * - Edge cases: zero offset, maximum offset, boundary conditions
 * - Memory pattern verification with alternating bit patterns
 */
#include "thumb_test_base.h"

class ThumbCPUTest6 : public ThumbCPUTestBase {
};

// ARM Thumb Format 6: PC-relative load
// Encoding: 01001[Rd][Word8]
// Instructions: LDR Rd, [PC, #imm]

TEST_F(ThumbCPUTest6, SimplePCRelativeLoad) {
    // Test case 1: Simple PC-relative load
    // LDR R0, [PC, #4] - load from (PC+4 & ~3) + 4 bytes forward
    setup_registers({{15, 0x00000000}});  // PC within valid test memory range
    memory.write32(0x00000008, 0x12345678);  // Target data at PC+4+4
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r0, [pc, #0x4]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0x12345678u);
}

TEST_F(ThumbCPUTest6, LoadZeroValue) {
    // Test case 2: Load zero value
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000008, 0x00000000);
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r1, [pc, #0x4]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(1), 0x00000000u);
}

TEST_F(ThumbCPUTest6, LoadMaximumValue) {
    // Test case 3: Load maximum 32-bit value
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000008, 0xFFFFFFFF);
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r2, [pc, #0x4]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(2), 0xFFFFFFFFu);
}

TEST_F(ThumbCPUTest6, MinimumOffset) {
    // Test case 4: Load with minimum offset (0)
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000004, 0xDEADBEEF);  // At PC+4 aligned
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r3, [pc, #0x0]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(3), 0xDEADBEEFu);
}

TEST_F(ThumbCPUTest6, MediumOffset) {
    // Test case 5: Load with medium offset
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000044, 0xCAFEBABE);  // At PC+4+64 bytes
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r4, [pc, #0x40]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(4), 0xCAFEBABEu);
}

TEST_F(ThumbCPUTest6, LargeOffset) {
    // Test case 6: Load with large offset
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000104, 0x11223344);  // At PC+4+256 bytes
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r5, [pc, #0x100]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(5), 0x11223344u);
}

TEST_F(ThumbCPUTest6, VeryLargeOffset) {
    // Test case 7: Load with very large offset
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000204, 0x55667788);  // At PC+4+512 bytes
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r6, [pc, #0x200]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(6), 0x55667788u);
}

TEST_F(ThumbCPUTest6, DifferentRegisters) {
    // Test case 8: Load to different registers with same offset
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000024, 0x99AABBCC);  // At PC+4+32 bytes
    
    // Test with different destination registers
    for (int rd = 0; rd < 8; rd++) {
        SetUp(); // Reset state
        setup_registers({{15, 0x00000000}});
        memory.write32(0x00000024, 0x99AABBCC);
        
        std::string instr = "ldr r" + std::to_string(rd) + ", [pc, #0x20]";
        ASSERT_TRUE(assembleAndWriteThumb(instr, R(15)));
        execute(1);
        
        EXPECT_EQ(R(rd), 0x99AABBCCu);
    }
}

TEST_F(ThumbCPUTest6, SignedNegativeValue) {
    // Test case 9: Load signed negative value (test sign extension not applied)
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000008, 0x80000000);  // Negative in signed interpretation
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r7, [pc, #0x4]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(7), 0x80000000u);  // No sign extension for 32-bit loads
}

TEST_F(ThumbCPUTest6, BoundaryPattern) {
    // Test case 10: Load with boundary pattern
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000008, 0x55555555);
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r0, [pc, #0x4]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0x55555555u);
}

TEST_F(ThumbCPUTest6, FlagsPreservation) {
    // Test case 11: Load preserves existing flags
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000008, 0x12345678);
    
    // Set all condition flags
    cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_N | CPU::FLAG_Z | CPU::FLAG_C | CPU::FLAG_V;
    uint32_t initial_cpsr = cpu.CPSR();
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r1, [pc, #0x4]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(1), 0x12345678u);
    EXPECT_EQ(cpu.CPSR(), initial_cpsr);
}

TEST_F(ThumbCPUTest6, PCAlignment) {
    // Test case 12: Load with PC alignment (PC is word-aligned in calculation)
    setup_registers({{15, 0x00000002}});  // Start at odd address
    memory.write32(0x00000008, 0xABCDEF01);  // Target at (0x00000002+4)&~3 + 4 = 0x00000004 + 4 = 0x00000008
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r2, [pc, #0x4]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(2), 0xABCDEF01u);
}

TEST_F(ThumbCPUTest6, MaximumOffset) {
    // Test case 13: Maximum offset (1020 bytes = 255 words)
    setup_registers({{15, 0x00000000}});
    memory.write32(0x00000400, 0x87654321);  // At PC+4+1020 bytes
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r3, [pc, #0x3fc]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(3), 0x87654321u);
}

TEST_F(ThumbCPUTest6, AllRegistersWithSameOffset) {
    // Test case 14: All registers with same offset
    
    // Test each register 0-7
    for (int rd = 0; rd < 8; rd++) {
        SetUp(); // Reset state
        setup_registers({{15, 0x00000000}});
        memory.write32(0x00000014, 0x13579BDF);  // At PC+4+16 bytes
        
        std::string instr = "ldr r" + std::to_string(rd) + ", [pc, #0x10]";
        ASSERT_TRUE(assembleAndWriteThumb(instr, R(15))) << "Failed for register R" << rd;
        execute(1);
        
        EXPECT_EQ(R(rd), 0x13579BDFu) << "Failed for register R" << rd;
    }
}

TEST_F(ThumbCPUTest6, MemoryNearUpperBoundary) {
    // Test case 15: Load from memory near upper boundary
    setup_registers({{15, 0x00000000}});
    memory.write32(0x000003FC, 0x24681ACE);  // Near maximum offset
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r4, [pc, #0x3f8]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(4), 0x24681ACEu);
}

TEST_F(ThumbCPUTest6, ZeroOffsetEdgeCase) {
    // Test case 16: Zero offset edge case
    setup_registers({{15, 0x00000004}});  // Start at word-aligned address
    memory.write32(0x00000008, 0xFEDCBA98);  // At (PC+4)&~3 + 0 = PC+4
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r5, [pc, #0x0]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(5), 0xFEDCBA98u);
}

TEST_F(ThumbCPUTest6, PCAlignmentOddAddresses) {
    // Test case 17: PC alignment with odd addresses
    setup_registers({{15, 0x00000006}});  // Start at address ending in 6
    // PC calculation: (0x00000006 + 4) & ~3 = 0x0000000A & ~3 = 0x00000008
    memory.write32(0x00000014, 0x369CF258);  // At 0x00000008 + 12 = 0x00000014
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r6, [pc, #0xc]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(6), 0x369CF258u);
}

TEST_F(ThumbCPUTest6, BoundaryOffsetsPattern) {
    // Test case 18: Boundary offsets pattern
    struct OffsetTest {
        uint16_t offset;
        uint32_t expected_addr;
        uint32_t test_value;
    } tests[] = {
        {4, 0x00000008, 0x11111111},
        {8, 0x0000000C, 0x22222222},
        {12, 0x00000010, 0x33333333},
        {64, 0x00000044, 0x44444444},
        {128, 0x00000084, 0x55555555},
        {256, 0x00000104, 0x66666666},
        {512, 0x00000204, 0x77777777},
        {1020, 0x00000400, 0x88888888}
    };
    
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        SetUp(); // Reset state
        setup_registers({{15, 0x00000000}});
        memory.write32(tests[i].expected_addr, tests[i].test_value);
        
        std::stringstream ss;
        ss << "ldr r7, [pc, #0x" << std::hex << tests[i].offset << "]";
        std::string instr = ss.str();
        ASSERT_TRUE(assembleAndWriteThumb(instr, R(15)));
        execute(1);
        
        EXPECT_EQ(R(7), tests[i].test_value) 
            << "Failed for offset " << tests[i].offset 
            << " (expected addr 0x" << std::hex << tests[i].expected_addr << ")";
    }
}

TEST_F(ThumbCPUTest6, MultipleConsecutiveLoads) {
    // Test case 19: Multiple consecutive loads
    // Test PC advancement with consecutive loads using proper word-aligned offsets
    
    // Setup data at different addresses for each load
    memory.write32(0x00000008, 0xAAAAAAAA);  // First load target: (PC=0x0+4)&~3 + 4 = 0x8
    memory.write32(0x0000000C, 0xBBBBBBBB);  // Second load target: (PC=0x2+4)&~3 + 8 = 0xC  
    memory.write32(0x00000014, 0xCCCCCCCC);  // Third load target: (PC=0x4+4)&~3 + 12 = 0x14
    
    // Execute first load at PC=0x0
    setup_registers({{15, 0x00000000}});
    ASSERT_TRUE(assembleAndWriteThumb("ldr r0, [pc, #0x4]", R(15)));
    execute(1);
    
    // Execute second load at PC=0x2 (PC advanced by 2)
    setup_registers({{15, 0x00000002}});
    ASSERT_TRUE(assembleAndWriteThumb("ldr r1, [pc, #0x8]", R(15)));
    execute(1);
    
    // Execute third load at PC=0x4 (PC advanced by 4 total)
    setup_registers({{15, 0x00000004}});
    ASSERT_TRUE(assembleAndWriteThumb("ldr r2, [pc, #0xc]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0xAAAAAAAAu);
    EXPECT_EQ(R(1), 0xBBBBBBBBu);
    EXPECT_EQ(R(2), 0xCCCCCCCCu);
}

TEST_F(ThumbCPUTest6, AlternatingBitPatterns) {
    // Test case 20: Load with alternating bit patterns
    struct PatternTest {
        uint32_t pattern;
        const char* description;
    } patterns[] = {
        {0xAAAAAAAA, "Alternating 10101010"},
        {0x55555555, "Alternating 01010101"},
        {0x0F0F0F0F, "Nibble alternating"},
        {0xF0F0F0F0, "Nibble alternating inverted"},
        {0x00FF00FF, "Byte alternating"},
        {0xFF00FF00, "Byte alternating inverted"}
    };
    
    for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        SetUp(); // Reset state
        setup_registers({{15, 0x00000000}});
        memory.write32(0x00000008 + i * 4, patterns[i].pattern);
        
        std::stringstream ss;
        ss << "ldr r0, [pc, #0x" << std::hex << (4 + i * 4) << "]";
        std::string instr = ss.str();
        ASSERT_TRUE(assembleAndWriteThumb(instr, R(15)));
        execute(1);
        
        EXPECT_EQ(R(0), patterns[i].pattern) 
            << "Failed for pattern: " << patterns[i].description;
    }
}

TEST_F(ThumbCPUTest6, LoadFromInstructionLocation) {
    // Test case 21: Edge case - load from instruction location
    setup_registers({{15, 0x00000200}});
    memory.write32(0x00000204, 0xABCD4800);  // Data includes instruction encoding
    
    ASSERT_TRUE(assembleAndWriteThumb("ldr r0, [pc, #0x0]", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0xABCD4800u);
}

TEST_F(ThumbCPUTest6, AllFlagPreservation) {
    // Test case 22: Verify all flag preservation with different initial flag states
    struct FlagTest {
        uint32_t flags;
        const char* description;
    } flag_tests[] = {
        {CPU::FLAG_T, "Only Thumb"},
        {CPU::FLAG_T | CPU::FLAG_Z, "Thumb + Zero"},
        {CPU::FLAG_T | CPU::FLAG_N, "Thumb + Negative"},
        {CPU::FLAG_T | CPU::FLAG_C, "Thumb + Carry"},
        {CPU::FLAG_T | CPU::FLAG_V, "Thumb + Overflow"},
        {CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V, "All flags"}
    };
    
    for (size_t i = 0; i < sizeof(flag_tests) / sizeof(flag_tests[0]); i++) {
        SetUp(); // Reset state
        setup_registers({{15, 0x00000000}});
        memory.write32(0x00000010, 0x12345678);
        
        cpu.CPSR() = flag_tests[i].flags;
        uint32_t initial_cpsr = cpu.CPSR();
        
        ASSERT_TRUE(assembleAndWriteThumb("ldr r0, [pc, #0xc]", R(15)));
        execute(1);
        
        EXPECT_EQ(R(0), 0x12345678u) << "Data load failed for: " << flag_tests[i].description;
        EXPECT_EQ(cpu.CPSR(), initial_cpsr) << "Flags changed for: " << flag_tests[i].description;
    }
}
