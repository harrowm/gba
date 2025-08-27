//
// ARM Thumb Instruction Set Test Suite - Format 6: PC-relative load operations
//
// This file tests the ARM Thumb instruction format for PC-relative load operations.
// Format 6 covers: LDR Rd, [PC, #imm]
// Encoding: 01001[Rd][Word8]
//
// The instruction performs: Rd = MEMORY[(PC + 4 & ~3) + imm*4]
// Where imm is an 8-bit unsigned value (Word8) representing words (0-1020 bytes)
//

#include <gtest/gtest.h>
#include <keystone/keystone.h>
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "arm_cpu.h"
#include "thumb_cpu.h"

// Test fixture for Thumb Format 6 PC-relative load operations
class ThumbCPUTest6 : public ::testing::Test {
protected:
    GBA gba{true}; // Test mode
    CPU& cpu = gba.getCPU();
    Memory& memory = cpu.getMemory();
    ThumbCPU& thumb_cpu = cpu.getThumbCPU();
    ks_engine* ks = nullptr;

    void SetUp() override {
        // Reset CPU state
        cpu.R().fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode
        cpu.R()[15] = 0x00000000; // Set PC to start of memory like original tests

        // Initialize Keystone for Thumb mode
        if (ks) ks_close(ks);
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for Thumb mode";
        }
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }

    // Helper: assemble Thumb instruction and write to memory
    bool assemble_and_write_thumb(const std::string& asm_code, uint32_t addr, std::vector<uint8_t>* out_bytes = nullptr) {
        unsigned char* encode = nullptr;
        size_t size, count;
        
        int err = ks_asm(ks, asm_code.c_str(), addr, &encode, &size, &count);
        if ((ks_err)err != KS_ERR_OK) {
            fprintf(stderr, "Keystone Thumb error: %s\n", ks_strerror((ks_err)err));
            return false;
        }
                
        if (size != 2 || count != 1) {
            fprintf(stderr, "Expected 2-byte Thumb instruction, got %zu bytes\n", size);
            if (encode) ks_free(encode);
            return false;
        }
        
        // Write instruction to memory
        uint16_t instruction = encode[0] | (encode[1] << 8);
        memory.write16(addr, instruction);
        
        if (out_bytes) {
            out_bytes->clear();
            for (size_t i = 0; i < size; i++) {
                out_bytes->push_back(encode[i]);
            }
        }
        
        ks_free(encode);
        return true;
    }

    // Helper: set register values from initializer list
    void setup_registers(const std::vector<std::pair<int, uint32_t>>& reg_values) {
        for (const auto& [reg, val] : reg_values) {
            if (reg >= 0 && reg < 16) {
                cpu.R()[reg] = val;
            }
        }
    }

    // Helper: check flag state
    bool getFlag(uint32_t flag) const {
        return cpu.CPSR() & flag;
    }
};

TEST_F(ThumbCPUTest6, SimplePCRelativeLoad) {
    // Test case 1: Simple PC-relative load
    // LDR R0, [PC, #4] - load from (PC+4 & ~3) + 4 bytes forward
    cpu.R()[15] = 0x00000000;  // PC within valid test memory range
    memory.write32(0x00000008, 0x12345678);  // Target data at PC+4+4
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r0, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x12345678u);
}

TEST_F(ThumbCPUTest6, LoadZeroValue) {
    // Test case 2: Load zero value
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000008, 0x00000000);
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r1, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0x00000000u);
}

TEST_F(ThumbCPUTest6, LoadMaximumValue) {
    // Test case 3: Load maximum 32-bit value
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000008, 0xFFFFFFFF);
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r2, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ThumbCPUTest6, MinimumOffset) {
    // Test case 4: Load with minimum offset (0)
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000004, 0xDEADBEEF);  // At PC+4 aligned
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r3, [pc, #0]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0xDEADBEEFu);
}

TEST_F(ThumbCPUTest6, MediumOffset) {
    // Test case 5: Load with medium offset
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000044, 0xCAFEBABE);  // At PC+4+64 bytes
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r4, [pc, #64]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0xCAFEBABEu);
}

TEST_F(ThumbCPUTest6, LargeOffset) {
    // Test case 6: Load with large offset
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000104, 0x11223344);  // At PC+4+256 bytes
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r5, [pc, #256]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[5], 0x11223344u);
}

TEST_F(ThumbCPUTest6, VeryLargeOffset) {
    // Test case 7: Load with very large offset
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000204, 0x55667788);  // At PC+4+512 bytes
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r6, [pc, #512]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[6], 0x55667788u);
}

TEST_F(ThumbCPUTest6, DifferentRegisters) {
    // Test case 8: Load to different registers with same offset
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000024, 0x99AABBCC);  // At PC+4+32 bytes
    
    // Test with different destination registers
    for (int rd = 0; rd < 8; rd++) {
        SetUp(); // Reset state
        cpu.R()[15] = 0x00000000;
        memory.write32(0x00000024, 0x99AABBCC);
        
        std::string instr = "ldr r" + std::to_string(rd) + ", [pc, #32]";
        ASSERT_TRUE(assemble_and_write_thumb(instr, cpu.R()[15]));
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[rd], 0x99AABBCCu);
    }
}

TEST_F(ThumbCPUTest6, SignedNegativeValue) {
    // Test case 9: Load signed negative value (test sign extension not applied)
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000008, 0x80000000);  // Negative in signed interpretation
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r7, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[7], 0x80000000u);  // No sign extension for 32-bit loads
}

TEST_F(ThumbCPUTest6, BoundaryPattern) {
    // Test case 10: Load with boundary pattern
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000008, 0x55555555);
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r0, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x55555555u);
}

TEST_F(ThumbCPUTest6, FlagsPreservation) {
    // Test case 11: Load preserves existing flags
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000008, 0x12345678);
    
    // Set all condition flags
    cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_N | CPU::FLAG_Z | CPU::FLAG_C | CPU::FLAG_V;
    uint32_t initial_cpsr = cpu.CPSR();
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r1, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0x12345678u);
    EXPECT_EQ(cpu.CPSR(), initial_cpsr);
}

TEST_F(ThumbCPUTest6, PCAlignment) {
    // Test case 12: Load with PC alignment (PC is word-aligned in calculation)
    cpu.R()[15] = 0x00000002;  // Start at odd address
    memory.write32(0x00000008, 0xABCDEF01);  // Target at (0x00000002+4)&~3 + 4 = 0x00000004 + 4 = 0x00000008
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r2, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0xABCDEF01u);
}

TEST_F(ThumbCPUTest6, MaximumOffset) {
    // Test case 13: Maximum offset (1020 bytes = 255 words)
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000400, 0x87654321);  // At PC+4+1020 bytes
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r3, [pc, #1020]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0x87654321u);
}

TEST_F(ThumbCPUTest6, AllRegistersWithSameOffset) {
    // Test case 14: All registers with same offset
    
    // Test each register 0-7
    for (int rd = 0; rd < 8; rd++) {
        SetUp(); // Reset state
        cpu.R()[15] = 0x00000000;
        memory.write32(0x00000014, 0x13579BDF);  // At PC+4+16 bytes
        
        std::string instr = "ldr r" + std::to_string(rd) + ", [pc, #16]";
        ASSERT_TRUE(assemble_and_write_thumb(instr, cpu.R()[15])) << "Failed for register R" << rd;
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[rd], 0x13579BDFu) << "Failed for register R" << rd;
    }
}

TEST_F(ThumbCPUTest6, MemoryNearUpperBoundary) {
    // Test case 15: Load from memory near upper boundary
    cpu.R()[15] = 0x00000000;
    memory.write32(0x000003FC, 0x24681ACE);  // Near maximum offset
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r4, [pc, #1016]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0x24681ACEu);
}

TEST_F(ThumbCPUTest6, ZeroOffsetEdgeCase) {
    // Test case 16: Zero offset edge case
    cpu.R()[15] = 0x00000004;  // Start at word-aligned address
    memory.write32(0x00000008, 0xFEDCBA98);  // At (PC+4)&~3 + 0 = PC+4
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r5, [pc, #0]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[5], 0xFEDCBA98u);
}

TEST_F(ThumbCPUTest6, PCAlignmentOddAddresses) {
    // Test case 17: PC alignment with odd addresses
    cpu.R()[15] = 0x00000006;  // Start at address ending in 6
    // PC calculation: (0x00000006 + 4) & ~3 = 0x0000000A & ~3 = 0x00000008
    memory.write32(0x00000014, 0x369CF258);  // At 0x00000008 + 12 = 0x00000014
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r6, [pc, #12]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[6], 0x369CF258u);
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
        cpu.R()[15] = 0x00000000;
        memory.write32(tests[i].expected_addr, tests[i].test_value);
        
        std::string instr = "ldr r7, [pc, #" + std::to_string(tests[i].offset) + "]";
        ASSERT_TRUE(assemble_and_write_thumb(instr, cpu.R()[15]));
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[7], tests[i].test_value) 
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
    cpu.R()[15] = 0x00000000;
    ASSERT_TRUE(assemble_and_write_thumb("ldr r0, [pc, #4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Execute second load at PC=0x2 (PC advanced by 2)
    cpu.R()[15] = 0x00000002;
    ASSERT_TRUE(assemble_and_write_thumb("ldr r1, [pc, #8]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Execute third load at PC=0x4 (PC advanced by 4 total)
    cpu.R()[15] = 0x00000004;
    ASSERT_TRUE(assemble_and_write_thumb("ldr r2, [pc, #12]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xAAAAAAAAu);
    EXPECT_EQ(cpu.R()[1], 0xBBBBBBBBu);
    EXPECT_EQ(cpu.R()[2], 0xCCCCCCCCu);
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
        cpu.R()[15] = 0x00000000;
        memory.write32(0x00000008 + i * 4, patterns[i].pattern);
        
        std::string instr = "ldr r0, [pc, #" + std::to_string(4 + i * 4) + "]";
        ASSERT_TRUE(assemble_and_write_thumb(instr, cpu.R()[15]));
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[0], patterns[i].pattern) 
            << "Failed for pattern: " << patterns[i].description;
    }
}

TEST_F(ThumbCPUTest6, LoadFromInstructionLocation) {
    // Test case 21: Edge case - load from instruction location
    cpu.R()[15] = 0x00000200;
    memory.write32(0x00000204, 0xABCD4800);  // Data includes instruction encoding
    
    ASSERT_TRUE(assemble_and_write_thumb("ldr r0, [pc, #0]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xABCD4800u);
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
        cpu.R()[15] = 0x00000000;
        memory.write32(0x00000010, 0x12345678);
        
        cpu.CPSR() = flag_tests[i].flags;
        uint32_t initial_cpsr = cpu.CPSR();
        
        ASSERT_TRUE(assemble_and_write_thumb("ldr r0, [pc, #12]", cpu.R()[15]));
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[0], 0x12345678u) << "Data load failed for: " << flag_tests[i].description;
        EXPECT_EQ(cpu.CPSR(), initial_cpsr) << "Flags changed for: " << flag_tests[i].description;
    }
}
