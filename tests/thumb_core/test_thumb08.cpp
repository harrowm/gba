// test_thumb08.cpp - Modern Thumb CPU test fixture for Format 8: Load/store sign-extended byte/halfword
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <initializer_list>

extern "C" {
#include <keystone/keystone.h>
}

class ThumbCPUTest8 : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks; // Keystone handle

    ThumbCPUTest8() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu), ks(nullptr) {}

    void SetUp() override {
        // Initialize all registers to 0
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        
        // Set Thumb mode (T flag) and User mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
        
        // Initialize Keystone for Thumb mode
        if (ks) ks_close(ks);
        // Use KS_MODE_THUMB + KS_MODE_V8 to ensure we get 16-bit Thumb instructions
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for Thumb mode";
        }
        
        // Try to set syntax to prefer 16-bit Thumb instructions
        if (ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL) != KS_ERR_OK) {
            // This might not be necessary, but let's try
        }
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }

    // Helper function to assemble and write instruction
    bool assemble_and_write_thumb(const std::string& assembly, uint32_t address) {
        unsigned char *machine_code;
        size_t machine_size;
        size_t statement_count;
        
        int err = ks_asm(ks, assembly.c_str(), address, &machine_code, &machine_size, &statement_count);
        if ((ks_err)err != KS_ERR_OK) {
            fprintf(stderr, "Keystone Thumb error: %s\n", ks_strerror((ks_err)err));
            return false;
        }

        if (machine_size >= 2) {
            // Write instruction to memory (Thumb instructions are 16-bit)
            uint16_t instruction = (machine_code[1] << 8) | machine_code[0];
            memory.write16(address, instruction);            
        }

        ks_free(machine_code);
        return true;
    }

    void setup_registers(std::initializer_list<std::pair<int, uint32_t>> reg_values) {
        // Clear all registers first
        cpu.R().fill(0);
        
        // Set specified register values
        for (const auto& pair : reg_values) {
            cpu.R()[pair.first] = pair.second;
        }
    }
};

// Format 8: Load/store sign-extended byte/halfword
// Encoding: 0101[H][S][1][Ro][Rb][Rd]
// H=0,S=0: STRH (Store Halfword) - 0x52xx
// H=0,S=1: LDRSB (Load Register Signed Byte) - 0x56xx  
// H=1,S=0: LDRH (Load Halfword) - 0x5Axx
// H=1,S=1: LDRSH (Load Register Signed Halfword) - 0x5Exx

TEST_F(ThumbCPUTest8, StrhHalfwordBasic) {
    // Test case: STRH R0, [R1, R2] - basic halfword store
    setup_registers({{1, 0x00000100}, {2, 0x00000006}, {0, 0x12345678}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("strh r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify only the lower 16 bits were stored as halfword
    uint16_t stored = memory.read16(0x00000106);
    EXPECT_EQ(stored, 0x5678);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, StrhHalfwordDifferentRegisters) {
    // Test case: STRH with different register combinations
    setup_registers({{3, 0x00000200}, {4, 0x0000000A}, {5, 0xFFFFABCD}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("strh r5, [r3, r4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify only the lower 16 bits were stored
    uint16_t stored = memory.read16(0x0000020A);
    EXPECT_EQ(stored, 0xABCD);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, StrhHalfwordZeroOffset) {
    // Test case: STRH with zero offset register
    setup_registers({{6, 0x00000300}, {7, 0x00000000}, {1, 0x0000BEEF}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("strh r1, [r6, r7]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // With zero offset, should store at base address
    uint16_t stored = memory.read16(0x00000300);
    EXPECT_EQ(stored, 0xBEEF);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, StrhHalfwordBoundaryValues) {
    // Test various boundary values for halfword store
    struct TestCase {
        uint32_t value;
        uint16_t expected;
    };
    
    TestCase test_cases[] = {
        {0x00000000, 0x0000}, // Zero
        {0x0000FFFF, 0xFFFF}, // Max 16-bit
        {0x12345678, 0x5678}, // Normal value
        {0xFFFFFFFF, 0xFFFF}, // All ones
        {0x80008000, 0x8000}, // Sign bit pattern
        {0x7FFF7FFF, 0x7FFF}, // Max positive
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        cpu.R()[0] = test_cases[i].value;
        cpu.R()[1] = 0x00000400;
        cpu.R()[2] = 0x00000000;
        cpu.R()[15] = i * 4;
        
        ASSERT_TRUE(assemble_and_write_thumb("strh r0, [r1, r2]", cpu.R()[15]));
        thumb_cpu.execute(1);
        
        uint16_t stored = memory.read16(0x00000400);
        EXPECT_EQ(stored, test_cases[i].expected) 
            << "Test case " << i << ", input value 0x" << std::hex << test_cases[i].value;
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 4 + 2));
    }
}

TEST_F(ThumbCPUTest8, LdsbPositiveByte) {
    // Test case: LDRSB R0, [R1, R2] - positive byte (no sign extension)
    setup_registers({{1, 0x00000500}, {2, 0x00000003}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store a positive byte value in memory
    memory.write8(0x00000503, 0x7F); // Positive byte
    
    ASSERT_TRUE(assemble_and_write_thumb("ldrsb r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify the byte was loaded and zero-extended (positive)
    EXPECT_EQ(cpu.R()[0], 0x0000007Fu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, LdsbNegativeByte) {
    // Test case: LDRSB with negative byte (sign extension)
    setup_registers({{3, 0x00000600}, {4, 0x00000007}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store a negative byte value in memory
    memory.write8(0x00000607, 0x80); // Negative byte (0x80 = -128)
    
    ASSERT_TRUE(assemble_and_write_thumb("ldrsb r5, [r3, r4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify the byte was loaded and sign-extended (negative)
    EXPECT_EQ(cpu.R()[5], 0xFFFFFF80u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, LdsbVariousByteValues) {
    // Test various byte values with proper sign extension
    setup_registers({{1, 0x00000700}, {2, 0x00000000}});
    
    struct TestCase {
        uint8_t byte_val;
        uint32_t expected;
    };
    
    TestCase test_cases[] = {
        {0x00, 0x00000000}, // Zero
        {0x01, 0x00000001}, // Small positive
        {0x7F, 0x0000007F}, // Max positive signed byte
        {0x80, 0xFFFFFF80}, // Min negative signed byte
        {0xFF, 0xFFFFFFFF}, // -1
        {0xFE, 0xFFFFFFFE}, // Small negative
        {0x55, 0x00000055}, // Pattern positive
        {0xAA, 0xFFFFFFAA}, // Pattern negative
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        memory.write8(0x00000700, test_cases[i].byte_val);
        cpu.R()[15] = i * 4; // Set PC for each test
        
        ASSERT_TRUE(assemble_and_write_thumb("ldrsb r0, [r1, r2]", cpu.R()[15]));
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[0], test_cases[i].expected) 
            << "Byte value 0x" << std::hex << static_cast<int>(test_cases[i].byte_val);
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 4 + 2));
    }
}

TEST_F(ThumbCPUTest8, LdrhHalfwordBasic) {
    // Test case: LDRH R0, [R1, R2] - basic halfword load
    setup_registers({{1, 0x00000800}, {2, 0x00000006}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store a halfword value in memory
    memory.write16(0x00000806, 0x1234);
    
    ASSERT_TRUE(assemble_and_write_thumb("ldrh r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify the halfword was loaded and zero-extended
    EXPECT_EQ(cpu.R()[0], 0x00001234u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, LdrhHalfwordDifferentRegisters) {
    // Test case: LDRH with different register combinations
    setup_registers({{4, 0x00000900}, {5, 0x00000008}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store a halfword value in memory
    memory.write16(0x00000908, 0xABCD);
    
    ASSERT_TRUE(assemble_and_write_thumb("ldrh r6, [r4, r5]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify the halfword was loaded into correct register
    EXPECT_EQ(cpu.R()[6], 0x0000ABCDu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, LdrhHalfwordBoundaryValues) {
    // Test various boundary values for halfword load
    setup_registers({{1, 0x00000A00}, {2, 0x00000000}});
    
    struct TestCase {
        uint16_t halfword_val;
        uint32_t expected;
    };
    
    TestCase test_cases[] = {
        {0x0000, 0x00000000}, // Zero
        {0x0001, 0x00000001}, // Small value
        {0x7FFF, 0x00007FFF}, // Max positive
        {0x8000, 0x00008000}, // High bit set (but zero-extended, not sign-extended)
        {0xFFFF, 0x0000FFFF}, // Max value
        {0x5555, 0x00005555}, // Pattern
        {0xAAAA, 0x0000AAAA}, // Pattern
        {0x1234, 0x00001234}, // Random value
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        memory.write16(0x00000A00, test_cases[i].halfword_val);
        cpu.R()[15] = i * 4; // Set PC for each test
        
        ASSERT_TRUE(assemble_and_write_thumb("ldrh r0, [r1, r2]", cpu.R()[15]));
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[0], test_cases[i].expected) 
            << "Halfword value 0x" << std::hex << test_cases[i].halfword_val;
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 4 + 2));
    }
}

TEST_F(ThumbCPUTest8, LdshPositiveHalfword) {
    // Test case: LDRSH R0, [R1, R2] - positive halfword (no sign extension)
    setup_registers({{1, 0x00000B00}, {2, 0x00000008}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store a positive halfword value in memory
    memory.write16(0x00000B08, 0x7FFF); // Maximum positive
    
    ASSERT_TRUE(assemble_and_write_thumb("ldrsh r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify the halfword was loaded and zero-extended (positive)
    EXPECT_EQ(cpu.R()[0], 0x00007FFFu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, LdshNegativeHalfword) {
    // Test case: LDRSH with negative halfword (sign extension)
    setup_registers({{3, 0x00000C00}, {4, 0x0000000A}});
    cpu.R()[15] = 0x00000000;
    
    // Pre-store a negative halfword value in memory
    memory.write16(0x00000C0A, 0x8000); // Minimum negative
    
    ASSERT_TRUE(assemble_and_write_thumb("ldrsh r5, [r3, r4]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // Verify the halfword was loaded and sign-extended (negative)
    EXPECT_EQ(cpu.R()[5], 0xFFFF8000u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest8, LdshVariousHalfwordValues) {
    // Test various halfword values with proper sign extension
    setup_registers({{1, 0x00000D00}, {2, 0x00000000}});
    
    struct TestCase {
        uint16_t halfword_val;
        uint32_t expected;
    };
    
    TestCase test_cases[] = {
        {0x0000, 0x00000000}, // Zero
        {0x0001, 0x00000001}, // Small positive
        {0x7FFF, 0x00007FFF}, // Max positive signed halfword
        {0x8000, 0xFFFF8000}, // Min negative signed halfword
        {0xFFFF, 0xFFFFFFFF}, // -1
        {0xFFFE, 0xFFFFFFFE}, // Small negative
        {0x5555, 0x00005555}, // Pattern positive
        {0xAAAA, 0xFFFFAAAA}, // Pattern negative
        {0x1234, 0x00001234}, // Random positive
        {0x9876, 0xFFFF9876}, // Random negative
    };
    
    for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
        memory.write16(0x00000D00, test_cases[i].halfword_val);
        cpu.R()[15] = i * 4; // Set PC for each test
        
        ASSERT_TRUE(assemble_and_write_thumb("ldrsh r0, [r1, r2]", cpu.R()[15]));
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[0], test_cases[i].expected) 
            << "Halfword value 0x" << std::hex << test_cases[i].halfword_val;
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(i * 4 + 2));
    }
}

TEST_F(ThumbCPUTest8, StrhLdrhRoundtrip) {
    // Test storing and loading halfwords to verify consistency
    setup_registers({{1, 0x00000E00}, {2, 0x00000000}});
    
    struct TestValue {
        uint32_t original;
        uint16_t expected_stored;
        uint32_t expected_loaded;
    };
    
    TestValue test_values[] = {
        {0x12345678, 0x5678, 0x00005678},
        {0xFFFFABCD, 0xABCD, 0x0000ABCD},
        {0x00000000, 0x0000, 0x00000000},
        {0x0000FFFF, 0xFFFF, 0x0000FFFF},
        {0x87654321, 0x4321, 0x00004321},
        {0xFEDCBA98, 0xBA98, 0x0000BA98},
    };
    
    for (size_t i = 0; i < sizeof(test_values)/sizeof(test_values[0]); i++) {
        // Store the value
        cpu.R()[0] = test_values[i].original;
        cpu.R()[15] = i * 8; // Different PC for each store
        
        ASSERT_TRUE(assemble_and_write_thumb("strh r0, [r1, r2]", cpu.R()[15]));
        thumb_cpu.execute(1);
        
        // Verify stored value
        uint16_t stored = memory.read16(0x00000E00);
        EXPECT_EQ(stored, test_values[i].expected_stored);
        
        // Load it back
        cpu.R()[3] = 0x00000000; // Clear destination
        cpu.R()[15] = i * 8 + 2; // Next instruction
        
        ASSERT_TRUE(assemble_and_write_thumb("ldrh r3, [r1, r2]", cpu.R()[15]));
        thumb_cpu.execute(1);
        
        // Verify loaded value
        EXPECT_EQ(cpu.R()[3], test_values[i].expected_loaded);
    }
}

TEST_F(ThumbCPUTest8, AllRegisterCombinations) {
    // Test different register combinations to verify encoding
    setup_registers({{3, 0x00000F00}, {4, 0x00000010}});
    
    // Test with different destination registers
    for (int rd = 0; rd < 3; rd++) {
        cpu.R()[rd] = 0x1000 + rd; // Different value for each register
        cpu.R()[15] = rd * 4;
        
        std::string instruction = "strh r" + std::to_string(rd) + ", [r3, r4]";
        ASSERT_TRUE(assemble_and_write_thumb(instruction, cpu.R()[15]));
        thumb_cpu.execute(1);
        
        // Verify correct value was stored
        uint16_t stored = memory.read16(0x00000F10);
        EXPECT_EQ(stored, static_cast<uint16_t>(0x1000 + rd));
        EXPECT_EQ(cpu.R()[15], static_cast<uint32_t>(rd * 4 + 2));
    }
}

TEST_F(ThumbCPUTest8, EdgeCasesAndBoundaryConditions) {
    // Test 1: Maximum address calculation
    cpu.R()[1] = 0x0000FFFF; // Large base
    cpu.R()[2] = 0x0000FFFF; // Large offset
    cpu.R()[0] = 0x1234;
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("strh r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // ARM handles address wraparound - instruction should complete
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // Test 2: Unaligned access (ARM typically allows this)
    cpu.R()[15] = 0x00000010;
    cpu.R()[1] = 0x00001100; // Base address
    cpu.R()[2] = 0x00000001; // Odd offset (unaligned for halfword)
    cpu.R()[0] = 0x5678;
    
    ASSERT_TRUE(assemble_and_write_thumb("strh r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // ARM handles unaligned access - instruction should complete
    EXPECT_EQ(cpu.R()[15], 0x00000012u);
    
    // Test 3: Sign extension boundary for LDRSB
    cpu.R()[15] = 0x00000020;
    setup_registers({{1, 0x00001200}, {2, 0x00000000}});
    
    // Test byte value 0x7F (positive) vs 0x80 (negative)
    memory.write8(0x00001200, 0x7F);
    ASSERT_TRUE(assemble_and_write_thumb("ldrsb r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x0000007Fu); // Should be positive
    
    cpu.R()[15] = 0x00000030;
    memory.write8(0x00001200, 0x80);
    ASSERT_TRUE(assemble_and_write_thumb("ldrsb r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFF80u); // Should be sign-extended negative
    
    // Test 4: Sign extension boundary for LDRSH
    cpu.R()[15] = 0x00000040;
    memory.write16(0x00001200, 0x7FFF);
    ASSERT_TRUE(assemble_and_write_thumb("ldrsh r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x00007FFFu); // Should be positive
    
    cpu.R()[15] = 0x00000050;
    memory.write16(0x00001200, 0x8000);
    ASSERT_TRUE(assemble_and_write_thumb("ldrsh r0, [r1, r2]", cpu.R()[15]));
    thumb_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFF8000u); // Should be sign-extended negative
}
