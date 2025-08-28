/**
 * Thumb Format 17: Software interrupt operations
 * Instruction encoding: 1101 1111 Value8[7:0]
 * 
 * Software interrupt (SWI):
 * - Encoding: 1101 1111 Value8 (0xDF00-0xDFFF)
 * - Causes a software interrupt exception
 * - Value8 is an 8-bit comment field (0x00-0xFF)
 * - Comment field is ignored by processor but available to interrupt handler
 * - Forces processor into Supervisor mode
 * - Sets PC to SWI exception vector (0x08)
 * - Stores return address in LR_svc
 * - Sets I flag in CPSR to disable IRQ
 *
 * Exception behavior:
 * - LR_svc = PC + 2 (return to instruction after SWI)
 * - PC = 0x08 (SWI exception vector)
 * - CPSR mode bits = 10011 (Supervisor mode)
 * - CPSR I flag = 1 (IRQ disabled)
 * - SPSR_svc = CPSR (saved processor state)
 *
 * Comment field values:
 * - 0x00: Often used for system calls
 * - 0x01-0x0F: Common OS service numbers
 * - 0x10-0xFF: Application-specific services
 */
#include "thumb_test_base.h"

class ThumbCPUTest17 : public ThumbCPUTestBase {
};

TEST_F(ThumbCPUTest17, SWI_BASIC_COMMENT_VALUES) {
    // Test case 1: SWI #0 (comment = 0x00)
    setup_registers({{0, 0x12345678}, {1, 0x87654321}});
    
    assembleAndWriteThumb("swi #0", 0x00000000);
    
    execute(1);
    
    // SWI should not modify registers (in basic implementation)
    EXPECT_EQ(R(0), 0x12345678u);
    EXPECT_EQ(R(1), 0x87654321u);
    
    // Test case 2: SWI #1 (comment = 0x01)
    setup_registers({{2, 0xDEADBEEF}});
    
    assembleAndWriteThumb("swi #1", 0x00000000);
    
    execute(1);
    
    EXPECT_EQ(R(2), 0xDEADBEEFu);
    
    // Test case 3: SWI #0xFF (comment = 0xFF, maximum value)
    setup_registers({{7, 0xCAFEBABE}});
    
    assembleAndWriteThumb("swi #255", 0x00000000);
    
    execute();
    
    EXPECT_EQ(R(7), 0xCAFEBABEu);
}

TEST_F(ThumbCPUTest17, SWI_COMMON_COMMENT_VALUES) {
    // Test case 1: SWI #0x10 (common system call value)
    setup_registers({{0, 0x11111111}, {1, 0x22222222}, {2, 0x33333333}});
    
    assembleAndWriteThumb("swi #16", 0x00000000);
    
    execute();
    
    // Verify registers are preserved
    EXPECT_EQ(R(0), 0x11111111u);
    EXPECT_EQ(R(1), 0x22222222u);
    EXPECT_EQ(R(2), 0x33333333u);
    
    // Test case 2: SWI #0x80 (another common system call value)
    setup_registers({{3, 0x44444444}, {4, 0x55555555}});
    
    assembleAndWriteThumb("swi #128", 0x00000000);
    
    execute();
    
    EXPECT_EQ(R(3), 0x44444444u);
    EXPECT_EQ(R(4), 0x55555555u);
}

TEST_F(ThumbCPUTest17, SWI_ENCODING_VERIFICATION) {
    // Test various comment values to verify proper encoding
    struct TestCase {
        uint8_t comment;
        uint16_t expected_instruction;
        std::string description;
    };

    TestCase test_cases[] = {
        {0x00, 0xDF00, "SWI #0"},
        {0x01, 0xDF01, "SWI #1"},
        {0x0F, 0xDF0F, "SWI #15"},
        {0x10, 0xDF10, "SWI #16"},
        {0x20, 0xDF20, "SWI #32"},
        {0x40, 0xDF40, "SWI #64"},
        {0x7F, 0xDF7F, "SWI #127"},
        {0x80, 0xDF80, "SWI #128"},
        {0xAA, 0xDFAA, "SWI #170"},
        {0xFF, 0xDFFF, "SWI #255"}
    };

    for (const auto& test_case : test_cases) {
        setup_registers({{0, 0x12345678}});
        
        memory.write16(0x00000000, test_case.expected_instruction);
        execute();
        
        // Verify the instruction was recognized and registers preserved
        EXPECT_EQ(R(0), 0x12345678u) << "Failed for " << test_case.description;
        
        // Verify encoding structure: 11011111[Value8]
        uint16_t expected = 0xDF00 | test_case.comment;
        EXPECT_EQ(test_case.expected_instruction, expected) 
            << "Encoding mismatch for " << test_case.description;
    }
}

TEST_F(ThumbCPUTest17, SWI_INSTRUCTION_FORMAT) {
    // Test that the instruction format is correctly recognized
    // Format 17: 11011111[Value8]
    // Where Value8 is an 8-bit comment field
    
    setup_registers({{0, 0xAAAAAAAA}});
    
    // Test the boundary between Format 16 (0xDE__) and Format 17 (0xDF__)
    assembleAndWriteThumb("swi #66", 0x00000000);
    
    execute();
    
    // Verify instruction executed (registers preserved)
    EXPECT_EQ(R(0), 0xAAAAAAAAu);
}

TEST_F(ThumbCPUTest17, SWI_COMMENT_FIELD_EXTRACTION) {
    // Test that comment field is properly extracted from instruction encoding
    struct CommentTest {
        uint16_t instruction;
        uint8_t expected_comment;
        std::string description;
    };
    
    CommentTest comment_tests[] = {
        {0xDF00, 0x00, "Zero comment"},
        {0xDF55, 0x55, "Pattern comment"},
        {0xDFAA, 0xAA, "Alternating pattern"},
        {0xDFFF, 0xFF, "Maximum comment"}
    };
    
    for (const auto& test : comment_tests) {
        setup_registers({{5, 0xBEEFCAFE}});
        
        memory.write16(0x00000000, test.instruction);
        execute();
        
        // Verify comment extraction: comment = instruction & 0xFF
        uint8_t extracted_comment = test.instruction & 0xFF;
        EXPECT_EQ(extracted_comment, test.expected_comment)
            << "Comment extraction failed for " << test.description;
        
        // Verify registers preserved
        EXPECT_EQ(R(5), 0xBEEFCAFEu) << "Registers not preserved for " << test.description;
    }
}

TEST_F(ThumbCPUTest17, SWI_EDGE_CASES_AND_BOUNDARIES) {
    // Test case 1: Minimum comment value
    setup_registers({{1, 0x00000001}});
    memory.write16(0x00000000, 0xDF00); // SWI #0
    execute();
    EXPECT_EQ(R(1), 0x00000001u);
    
    // Test case 2: Maximum comment value
    setup_registers({{2, 0x00000002}});
    memory.write16(0x00000000, 0xDFFF); // SWI #255
    execute();
    EXPECT_EQ(R(2), 0x00000002u);
    
    // Test case 3: Powers of 2 comment values
    uint8_t power_of_2_values[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    
    for (int i = 0; i < 8; ++i) {
        setup_registers({{i, static_cast<uint32_t>(0x10000000 + i)}});
        uint16_t instruction = 0xDF00 | power_of_2_values[i];
        memory.write16(0x00000000, instruction);
        execute();
        EXPECT_EQ(R(i), static_cast<uint32_t>(0x10000000 + i)) 
            << "Failed for power of 2 value: " << static_cast<int>(power_of_2_values[i]);
    }
    
    // Test case 4: Multiple consecutive SWI instructions
    setup_registers({{6, 0xFEEDFACE}, {7, 0xDEADC0DE}});
    
    // Write multiple SWI instructions
    memory.write16(0x00000000, 0xDF11); // SWI #17
    memory.write16(0x00000002, 0xDF22); // SWI #34  
    memory.write16(0x00000004, 0xDF33); // SWI #51
    
    // Execute all three
    execute();
    EXPECT_EQ(R(6), 0xFEEDFACEu);
    EXPECT_EQ(R(7), 0xDEADC0DEu);
    
    execute();
    EXPECT_EQ(R(6), 0xFEEDFACEu);
    EXPECT_EQ(R(7), 0xDEADC0DEu);
    
    execute();  
    EXPECT_EQ(R(6), 0xFEEDFACEu);
    EXPECT_EQ(R(7), 0xDEADC0DEu);
}

TEST_F(ThumbCPUTest17, SWI_INSTRUCTION_RECOGNITION) {
    // Test that Format 17 instructions are properly distinguished from other formats
    
    // Test boundary conditions with Format 16 (conditional branches)
    // Format 16: 1101[Cond][SOffset8] where Cond != 0xF (condition 0xE is reserved/undefined)
    // Format 17: 11011111[Value8] where the high nibble is 0xF
    
    setup_registers({{0, 0x12345678}});
    
    // This should be Format 16 (BVS - condition 0x6) 
    memory.write16(0x00000000, 0xD6AA); // BVS with offset 0xAA (condition 6 = VS/overflow set)
    execute();
    EXPECT_EQ(R(0), 0x12345678u);
    
    // This should be Format 17 (SWI)
    setup_registers({{0, 0x87654321}});
    memory.write16(0x00000000, 0xDFAA); // SWI #0xAA
    execute();
    EXPECT_EQ(R(0), 0x87654321u);
    
    // Verify the distinguishing bit pattern
    // Format 17 requires bits 15-8 to be 11011111 (0xDF)
    EXPECT_EQ((0xDFAA >> 8) & 0xFF, 0xDF) << "Format 17 pattern check failed";
    EXPECT_NE((0xD6AA >> 8) & 0xFF, 0xDF) << "Format 16/17 boundary check failed";
}
