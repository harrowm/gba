/**
 * Thumb Format 16: Conditional branch operations
 * Instruction encoding: 1101 Cond[3:0] SOffset8[7:0]
 * 
 * Conditional branch (Bcc):
 * - Encoding: 1101 Cond SOffset8 (0xD000-0xDEFF, 0xDF00-0xDFFF reserved for SWI)
 * - Branches if condition Cond is true
 * - Target address = PC + 4 + (sign_extend(SOffset8) << 1)
 * - SOffset8 is an 8-bit signed offset in halfwords (-256 to +254 bytes)
 * - PC points to instruction after branch (current PC + 2)
 *
 * Condition codes (same as ARM):
 * - 0000 (EQ): Equal (Z=1)
 * - 0001 (NE): Not Equal (Z=0) 
 * - 0010 (CS/HS): Carry Set (C=1)
 * - 0011 (CC/LO): Carry Clear (C=0)
 * - 0100 (MI): Negative (N=1)
 * - 0101 (PL): Positive (N=0)
 * - 0110 (VS): Overflow Set (V=1)
 * - 0111 (VC): Overflow Clear (V=0)
 * - 1000 (HI): Higher (C=1 AND Z=0)
 * - 1001 (LS): Lower or Same (C=0 OR Z=1)
 * - 1010 (GE): Greater or Equal (N=V)
 * - 1011 (LT): Less Than (N≠V)
 * - 1100 (GT): Greater Than (Z=0 AND N=V)
 * - 1101 (LE): Less or Equal (Z=1 OR N≠V)
 * - 1110: Always (AL) - Reserved, use Format 18 (B) instead
 * - 1111: Never (NV) - Reserved for SWI
 *
 * Branch range: PC - 256 to PC + 254 bytes (±127 halfwords)
 */
#include "thumb_test_base.h"

class ThumbCPUTest16 : public ThumbCPUTestBase {
};

TEST_F(ThumbCPUTest16, BEQ_CONDITIONAL_BRANCH) {
    // Test case 1: BEQ taken (Z flag set)
    setFlags(CPU::FLAG_Z); // Set Z flag
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("beq #0x6", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (1*2) = 0x04
    EXPECT_EQ(R(15), 0x00000004u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z)); // Flags preserved
    
    // Test case 2: BEQ not taken (Z flag clear)
    setFlags(0); // Z flag clear
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("beq #0x6", 0x00000000);
    
    execute(1);
    
    // Branch not taken: PC = 0x02
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest16, BNE_CONDITIONAL_BRANCH) {
    // Test case 1: BNE taken (Z flag clear)
    setFlags(0); // Z flag clear
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bne #0x8", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (2*2) = 0x06
    EXPECT_EQ(R(15), 0x00000006u);
    
    // Test case 2: BNE not taken (Z flag set)
    setFlags(CPU::FLAG_Z); // Set Z flag
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bne #0x8", 0x00000000);
    
    execute(1);
    
    // Branch not taken: PC = 0x02
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest16, BMI_BPL_CONDITIONAL_BRANCH) {
    // Test case 1: BMI taken (N flag set) - backward branch
    setFlags(CPU::FLAG_N); // Set N flag
    R(15) = 0x00000010;
    
    // Backward branch: D4FF means offset -1. For backwards, need negative offset
    // Current PC 0x10, want to branch to 0x10 (same location creates loop)
    // This is a special case - keep manual encoding for backwards branch
    memory.write16(0x00000010, 0xD4FF); // BMI -2 (manual)
    
    execute(1);
    
    // Branch taken: PC = 0x12 + (-1*2) = 0x10
    EXPECT_EQ(R(15), 0x00000010u);
    
    // Test case 2: BPL taken (N flag clear)
    setFlags(0); // N flag clear
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bpl #0xA", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (3*2) = 0x08
    EXPECT_EQ(R(15), 0x00000008u);
}

TEST_F(ThumbCPUTest16, BCS_BCC_CONDITIONAL_BRANCH) {
    // Test case 1: BCS taken (C flag set)
    setFlags(CPU::FLAG_C); // Set C flag
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bcs #0xC", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (4*2) = 0x0A
    EXPECT_EQ(R(15), 0x0000000Au);
    
    // Test case 2: BCC taken (C flag clear)
    setFlags(0); // C flag clear
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bcc #0xE", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (5*2) = 0x0C
    EXPECT_EQ(R(15), 0x0000000Cu);
}

TEST_F(ThumbCPUTest16, BVS_CONDITIONAL_BRANCH) {
    // Test case: BVS taken (V flag set)
    setFlags(CPU::FLAG_V); // Set V flag
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bvs #0xA", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (3*2) = 0x08
    EXPECT_EQ(R(15), 0x00000008u);
    EXPECT_TRUE(getFlag(CPU::FLAG_V)); // Flags preserved
}

TEST_F(ThumbCPUTest16, BGE_CONDITIONAL_BRANCH) {
    // Test case 1: BGE taken (N == V, both set)
    setFlags(CPU::FLAG_N | CPU::FLAG_V); // N=1, V=1 (N==V)
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bge #0x8", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (2*2) = 0x06
    EXPECT_EQ(R(15), 0x00000006u);
    
    // Test case 2: BGE not taken (N != V)
    setFlags(CPU::FLAG_N); // N=1, V=0 (N!=V)
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bge #0x8", 0x00000000);
    
    execute(1);
    
    // Branch not taken: PC = 0x02
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test case 3: BGE taken (N == V, both clear)
    setFlags(0); // N=0, V=0 (N==V)
    R(15) = 0x00000000;
    
    assembleAndWriteThumb("bge #0x8", 0x00000000);
    
    execute(1);
    
    // Branch taken: PC = 0x02 + (2*2) = 0x06
    EXPECT_EQ(R(15), 0x00000006u);
}

TEST_F(ThumbCPUTest16, BACKWARD_BRANCH_MAXIMUM) {
    // Test case: Large backward conditional branch (maximum range)
    setFlags(CPU::FLAG_Z); // Set condition for BEQ
    R(15) = 0x00000200;
    
    // BEQ with maximum backward offset (-128*2 = -256)
    memory.write16(0x00000200, 0xD080); // BEQ -256 (max backward)
    
    execute(1);
    
    // PC = 0x202 + (-128*2) = 0x102
    EXPECT_EQ(R(15), 0x00000102u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z)); // Flags preserved
}

TEST_F(ThumbCPUTest16, INSTRUCTION_ENCODING_VALIDATION) {
    // Test various conditional branch encodings
    struct TestCase {
        uint16_t encoding;
        std::string description;
        uint8_t condition;
        int8_t offset;
    };
    
    std::vector<TestCase> test_cases = {
        {0xD001, "BEQ +2", 0x0, 1},    // EQ
        {0xD102, "BNE +4", 0x1, 2},    // NE  
        {0xD204, "BCS +8", 0x2, 4},    // CS/HS
        {0xD305, "BCC +10", 0x3, 5},   // CC/LO
        {0xD4FF, "BMI -2", 0x4, -1},   // MI
        {0xD503, "BPL +6", 0x5, 3},    // PL
        {0xD603, "BVS +6", 0x6, 3},    // VS
        {0xDA02, "BGE +4", 0xA, 2},    // GE
        {0xD080, "BEQ -256", 0x0, -128} // Maximum backward
    };
    
    for (const auto& test_case : test_cases) {
        // Verify encoding structure: 1101[Cond][SOffset8]
        uint16_t expected = 0xD000; // Base pattern
        expected |= (test_case.condition & 0xF) << 8; // Condition bits
        expected |= (test_case.offset & 0xFF); // 8-bit signed offset
        
        EXPECT_EQ(test_case.encoding, expected) 
            << "Encoding mismatch for " << test_case.description;
        
        // Verify offset extraction
        int8_t extracted_offset = static_cast<int8_t>(test_case.encoding & 0xFF);
        EXPECT_EQ(extracted_offset, test_case.offset)
            << "Offset extraction mismatch for " << test_case.description;
    }
}

TEST_F(ThumbCPUTest16, EDGE_CASES_AND_BOUNDARIES) {
    // Test case 1: Zero offset branch (branch to next instruction)
    setFlags(CPU::FLAG_Z);
    R(15) = 0x00000000;
    
    // D000 = offset 0 - Keystone works for this
    assembleAndWriteThumb("beq #0x4", 0x00000000);
    execute(1);
    
    // Branch to same location: PC = 0x02 + (0*2) = 0x02
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test case 2: Maximum forward branch
    setFlags(CPU::FLAG_Z);
    R(15) = 0x00000000;
    
    // D07F = offset 127, target = 4 + 127*2 = 258 = 0x102
    // Keystone fails for large offsets (switches to BL instruction at ~0x80+)
    // Keep manual encoding for maximum range edge cases
    memory.write16(0x00000000, 0xD07F); // BEQ +254 (max forward)
    execute(1);
    
    // PC = 0x02 + (127*2) = 0x100
    EXPECT_EQ(R(15), 0x00000100u);
    
    // Test case 3: Branch condition evaluation with multiple flags
    setFlags(CPU::FLAG_Z | CPU::FLAG_C | CPU::FLAG_N); // Multiple flags set
    R(15) = 0x00000000;
    
    // D001 = offset 1 - Keystone works fine for this
    assembleAndWriteThumb("beq #0x6", 0x00000000);
    execute(1);
    
    EXPECT_EQ(R(15), 0x00000004u); // Branch taken
    // Verify all flags preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
}
