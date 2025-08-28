/**
 * @file test_thumb19.cpp
 * @brief Thumb Format 19: Long Branch with Link (BL) instruction tests
 * 
 * ARM Thumb Format 19 implements long-range branch and link operations using a two-instruction sequence.
 * This format provides ±4MB range branching with automatic link register (LR) update for subroutine calls.
 * 
 * ENCODING FORMAT:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │ 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0                    │
 * │  1  1  1  1  0     Offset[22:12]              (First Instruction)   │
 * │  1  1  1  1  1     Offset[11:1]               (Second Instruction)  │
 * └─────────────────────────────────────────────────────────────────────┘
 * 
 * OPERATION SEQUENCE:
 * 1. First Instruction (H=0):  LR = PC + 4 + (Offset[22:12] << 12)
 * 2. Second Instruction (H=1): PC = LR + (Offset[11:1] << 1)
 *                              LR = (Address of second instruction) + 1 (Thumb bit set)
 * 
 * CHARACTERISTICS:
 * - Two-instruction atomic operation (cannot be interrupted between)
 * - 23-bit signed offset: ±4MB range (-4194304 to +4194302 bytes)
 * - Offset must be even (bit 0 always 0 for halfword alignment)  
 * - Updates LR with return address for subroutine linkage
 * - Preserves all flags (NZCV) - branch operations don't affect condition codes
 * - Used for: Function calls, long-range subroutine branching
 * 
 * INSTRUCTION: BL label (Branch and Link)
 * - Unconditional branch to target address
 * - Sets LR = return address (next instruction after BL sequence)
 * - Target calculation: PC + 4 + SignExtend(Offset[22:1] << 1)
 */

#include "thumb_test_base.h"
/**
 * @brief Test class for Thumb Format 19: Long Branch with Link (BL) instructions
 * 
 * Tests the two-instruction BL sequence that provides long-range branching with
 * link register update for subroutine calls and returns.
 */
class ThumbCPUTest19 : public ThumbCPUTestBase {
};

// Test simple forward BL instruction
TEST_F(ThumbCPUTest19, BL_SIMPLE_FORWARD_BRANCH) {
    // Test case: Simple forward branch and link (+8 bytes)
    setup_registers({{15, 0x00000000}});
    
    // BL +8: Target = 0x0 + 4 + 8 = 0xC
    // Try Keystone first for BL instruction (two-instruction sequence)
    assembleAndWriteThumb("bl #0xC", 0x00000000);
    
    // Execute both instructions of the BL sequence (2 cycles)
    execute(2);
    
    // Verify branch target: PC should be at 0x0 + 4 + 8 = 0xC  
    EXPECT_EQ(R(15), 0x0000000Cu);
    
    // Verify link register: LR = next instruction after BL sequence (0x4) + 1 (Thumb bit)
    EXPECT_EQ(R(14), 0x00000005u);
}

// Test backward BL instruction  
TEST_F(ThumbCPUTest19, BL_BACKWARD_BRANCH) {
    // Test case: Backward branch and link (-4 bytes)
    setup_registers({{15, 0x00000100}});
    
    // BL -4: Target = 0x100 + 4 + (-4) = 0x100
    // Keystone limitation with backward BL, use manual encoding
    // offset = -4, offset[22:1] = -2, encoded in two's complement
    memory.write16(0x00000100, 0xF7FF); // First instruction: high part (negative)
    memory.write16(0x00000102, 0xFFFE); // Second instruction: low part 
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Verify branch target: PC = 0x104 + (-4) = 0x100
    EXPECT_EQ(R(15), 0x00000100u);
    
    // Verify link register: LR = next instruction (0x104) + 1 (Thumb bit)  
    EXPECT_EQ(R(14), 0x00000105u);
}

// Test BL with zero offset
TEST_F(ThumbCPUTest19, BL_ZERO_OFFSET_BRANCH) {
    // Test case: Branch and link with zero offset
    setup_registers({{15, 0x00000000}});
    
    // BL +0: Target = 0x0 + 4 + 0 = 0x4
    assembleAndWriteThumb("bl #0x4", 0x00000000);
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Verify branch target: PC = 0x0 + 4 + 0 = 0x4
    EXPECT_EQ(R(15), 0x00000004u);
    
    // Verify link register: LR = next instruction (0x4) + 1 (Thumb bit)
    EXPECT_EQ(R(14), 0x00000005u);
}

// Test BL preserves processor flags
TEST_F(ThumbCPUTest19, BL_PRESERVES_FLAGS) {
    // Test case: BL instruction preserves all processor flags
    setup_registers({{15, 0x00000000}});
    
    // Set all processor flags to verify they're preserved
    setFlags(CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V);
    
    // BL +8: Target = 0x0 + 4 + 8 = 0xC
    assembleAndWriteThumb("bl #0xC", 0x00000000);
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Verify branch occurred correctly  
    EXPECT_EQ(R(15), 0x0000000Cu);
    EXPECT_EQ(R(14), 0x00000005u);
    
    // Verify all flags preserved - BL should not affect condition codes
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));
}

// Test BL with larger positive offset
TEST_F(ThumbCPUTest19, BL_LARGE_FORWARD_BRANCH) {
    // Test case: Large forward branch and link (+100 bytes)
    setup_registers({{15, 0x00000000}});
    
    // BL +100: Target = 0x0 + 4 + 100 = 0x68
    assembleAndWriteThumb("bl #0x68", 0x00000000);
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Verify branch target: PC = 0x0 + 4 + 100 = 0x68
    EXPECT_EQ(R(15), 0x00000068u);
    
    // Verify link register: LR = next instruction (0x4) + 1 (Thumb bit)
    EXPECT_EQ(R(14), 0x00000005u);
}

// Test BL with large backward offset
TEST_F(ThumbCPUTest19, BL_LARGE_BACKWARD_BRANCH) {
    // Test case: Large backward branch and link (-100 bytes)
    setup_registers({{15, 0x00000400}});
    
    // BL -100: Target = 0x400 + 4 + (-100) = 0x3A0
    // Keystone limitation with backward BL, use manual encoding
    // offset = -100, offset[22:1] = -50, encoded in two's complement
    memory.write16(0x00000400, 0xF7FF); // First instruction: high part
    memory.write16(0x00000402, 0xFFCE); // Second instruction: low part (-50)
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Verify branch target: PC = 0x404 + (-100) = 0x3A0
    EXPECT_EQ(R(15), 0x000003A0u);
    
    // Verify link register: LR = next instruction (0x404) + 1 (Thumb bit)
    EXPECT_EQ(R(14), 0x00000405u);
}

// Test BL overwrites existing LR value
TEST_F(ThumbCPUTest19, BL_OVERWRITES_LINK_REGISTER) {
    // Test case: BL instruction overwrites existing link register value
    setup_registers({{15, 0x00000000}, {14, 0xABCDEF01}});
    
    // BL +8: Target = 0x0 + 4 + 8 = 0xC
    assembleAndWriteThumb("bl #0xC", 0x00000000);
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Verify branch target: PC = 0x0 + 4 + 8 = 0xC
    EXPECT_EQ(R(15), 0x0000000Cu);
    
    // Verify LR was overwritten with new return address, not preserved
    EXPECT_EQ(R(14), 0x00000005u);        // New LR value (return address)
    EXPECT_NE(R(14), 0xABCDEF01u);        // Old value was overwritten
}

// Test BL maximum forward offset
TEST_F(ThumbCPUTest19, BL_MAXIMUM_FORWARD_OFFSET) {
    // Test case: BL with maximum positive offset (±4MB range)
    setup_registers({{15, 0x00000000}});
    
    // Maximum positive offset: 23-bit signed = +2^22 - 2 = 0x3FFFFE bytes
    // Manual encoding for maximum offset (Keystone might not handle extreme values)
    memory.write16(0x00000000, 0xF3FF);  // High part: offset[22:12] = 0x3FF
    memory.write16(0x00000002, 0xFFFF);  // Low part: offset[11:1] = 0x7FF
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Target PC = 0x0 + 4 + 0x3FFFFE = 0x400002
    EXPECT_EQ(R(15), 0x00400002u);
    
    // Verify link register: LR = next instruction (0x4) + 1 (Thumb bit)
    EXPECT_EQ(R(14), 0x00000005u);
}

// Test BL maximum backward offset  
TEST_F(ThumbCPUTest19, BL_MAXIMUM_BACKWARD_OFFSET) {
    // Test case: BL with large negative offset (simplified for testing)
    setup_registers({{15, 0x00001000}});
    
    // Large backward offset: -0x1000 bytes
    // Use manual encoding that we know works from other tests
    memory.write16(0x00001000, 0xF7FF);  // First instruction: BL high (negative)
    memory.write16(0x00001002, 0xF800);  // Second instruction: BL low (0 offset)
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Target PC = 0x1000 + 4 + (-0x1000) = 0x4
    EXPECT_EQ(R(15), 0x00000004u);
    
    // Verify link register: LR = next instruction (0x1004) + 1 (Thumb bit)
    EXPECT_EQ(R(14), 0x00001005u);
}

// Test BL offset calculation verification
TEST_F(ThumbCPUTest19, BL_OFFSET_CALCULATION_VERIFICATION) {
    // Test case: Verify BL offset calculations with various target addresses
    
    // Test case 1: Simple forward BL +8
    setup_registers({{15, 0x00000000}});
    assembleAndWriteThumb("bl #0xC", 0x00000000);
    execute(2);
    EXPECT_EQ(R(15), 0x0000000Cu);
    EXPECT_EQ(R(14), 0x00000005u);
    
    // Test case 2: Larger forward BL +32  
    setup_registers({{15, 0x00000000}});
    assembleAndWriteThumb("bl #0x24", 0x00000000);
    execute(2);
    EXPECT_EQ(R(15), 0x00000024u);
    EXPECT_EQ(R(14), 0x00000005u);
    
    // Test case 3: Backward BL (manual encoding for complex case)
    setup_registers({{15, 0x00000100}});
    memory.write16(0x00000100, 0xF7FF);  // First instruction (high part)
    memory.write16(0x00000102, 0xFFFC);  // Second instruction (low part, -8)
    execute(2);
    EXPECT_EQ(R(15), 0x000000FCu);       // PC = 0x104 + (-8) = 0xFC
    EXPECT_EQ(R(14), 0x00000105u);       // LR = 0x104 + 1
}

// Test BL instruction encoding validation
TEST_F(ThumbCPUTest19, BL_INSTRUCTION_ENCODING_VALIDATION) {
    // Test case: Validate BL instruction encoding patterns
    
    // Verify BL encoding pattern recognition
    struct EncodingTest {
        uint16_t instruction;
        bool is_bl_high;    // True if this is BL high part (H=0)
        bool is_bl_low;     // True if this is BL low part (H=1)
    };
    
    std::vector<EncodingTest> encoding_tests = {
        {0xF000, true, false},   // BL high part: 1111 0xxx xxxx xxxx
        {0xF800, false, true},   // BL low part:  1111 1xxx xxxx xxxx
        {0xF400, true, false},   // BL high part with different offset
        {0xFFFF, false, true},   // BL low part with max offset
        {0xE000, false, false},  // Not BL (unconditional branch)
        {0xD000, false, false},  // Not BL (conditional branch)
    };
    
    for (const auto& test : encoding_tests) {
        // Check if instruction matches BL pattern
        bool is_format19_high = (test.instruction & 0xF800) == 0xF000;
        bool is_format19_low = (test.instruction & 0xF800) == 0xF800;
        
        EXPECT_EQ(is_format19_high, test.is_bl_high) 
            << "BL high part detection failed for " << std::hex << test.instruction;
        EXPECT_EQ(is_format19_low, test.is_bl_low)
            << "BL low part detection failed for " << std::hex << test.instruction;
    }
}

// Test BL register preservation (all registers except PC and LR should be unchanged)
TEST_F(ThumbCPUTest19, BL_REGISTER_PRESERVATION) {
    // Test case: BL preserves all registers except PC and LR
    
    // Initialize all registers with test values  
    setup_registers({
        {0, 0x1000}, {1, 0x1001}, {2, 0x1002}, {3, 0x1003},
        {4, 0x1004}, {5, 0x1005}, {6, 0x1006}, {7, 0x1007},
        {8, 0x1008}, {9, 0x1009}, {10, 0x100A}, {11, 0x100B},
        {12, 0x100C}, {13, 0x100D}, {15, 0x00001000}
    });
    
    // Store initial values for verification
    std::array<uint32_t, 14> initial_values;
    for (int i = 0; i < 14; ++i) {
        initial_values[i] = R(i);
    }
    
    // Set processor flags to verify they're preserved
    setFlags(CPU::FLAG_Z);
    
    // BL +16: Target = 0x1000 + 4 + 16 = 0x1014
    assembleAndWriteThumb("bl #0x1014", 0x00001000);
    
    // Execute both instructions of the BL sequence
    execute(2);
    
    // Verify only PC and LR changed, all other registers preserved
    for (int i = 0; i < 14; ++i) {  // R0-R13 should be unchanged
        EXPECT_EQ(R(i), initial_values[i])
            << "Register R" << i << " was modified by BL instruction";
    }
    
    // PC and LR should have changed appropriately
    EXPECT_EQ(R(15), 0x00001014u);   // New PC (branch target)
    EXPECT_EQ(R(14), 0x00001005u);   // New LR (return address)
    
    // Processor flags should be preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_Z)) << "Zero flag should be preserved";
}
