// test_thumb01.cpp - Modern Thumb CPU test fixture for Format 1: Move shifted register
#include "thumb_test_base.h"

class ThumbCPUTest : public ThumbCPUTestBase {
};

// ARM Thumb Format 1: Move shifted register
// Encoding: 000[op][offset5][Rs][Rd]
// Instructions: LSL, LSR, ASR

TEST_F(ThumbCPUTest, LSL_Basic) {
    // Test case: LSL R0, R0, #2 (shift 0b1 left by 2 positions)
    setup_registers({{0, 0b1}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsls r0, r0, #2", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), static_cast<unsigned int>(0b100));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u); // Thumb instructions are 2 bytes
}

TEST_F(ThumbCPUTest, LSL_CarryOut) {
    // Test case: LSL with carry out (shift 0xC0000000 left by 1)
    setup_registers({{1, 0xC0000000}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsls r1, r1, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(1), 0x80000000u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));  // Result is negative
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // Carry out from bit 31
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSL_ZeroResult) {
    // Test case: LSL resulting in zero
    setup_registers({{2, 0x80000000}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsls r2, r2, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));  // Not negative
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSL_ShiftByZero) {
    // Test case: Shift by 0 (special case - no operation, carry unaffected)
    setup_registers({{3, 0xABCD}, {15, 0x00000000}});
    cpu.CPSR() |= CPU::FLAG_C; // Pre-set carry flag
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r3, r3", R(15))); // LSL #0 is MOV in UAL
    execute(1);
    
    EXPECT_EQ(R(3), 0xABCDu);      // Value unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry flag not affected
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSL_MaxShift) {
    // Test case: Maximum shift amount (31)
    setup_registers({{4, 0b11}, {15, 0x00000000}});
    
    // Use hardcoded instruction for LSL #31 since Keystone may not support it
    writeInstruction(R(15), 0x07E4); // LSL R4, #31 (based on format01 original)
    execute(1);
    
    EXPECT_EQ(R(4), 0x80000000u);  // 0b11 << 31, bit 1 -> bit 32 (carry), bit 0 -> bit 31
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));   // Result is negative (bit 31 set)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Bit 0 of original (1) shifted out
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_Basic) {
    // Test case: LSR R0, R0, #2 (logical shift right)
    setup_registers({{0, 0b1100}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsrs r0, r0, #2", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), static_cast<unsigned int>(0b11));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_CarryOut) {
    // Test case: LSR with carry out
    setup_registers({{1, 0b101}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsrs r1, r1, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(1), static_cast<unsigned int>(0b10));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from bit 0
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_ZeroResult) {
    // Test case: LSR resulting in zero
    setup_registers({{2, 0x1}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsrs r2, r2, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from LSB
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_ShiftByZero) {
    // Test case: Shift by 0 (special case, treated as LSR #32)
    setup_registers({{3, 0x80000000}, {15, 0x00000000}});
    cpu.CPSR() &= ~CPU::FLAG_C; // Pre-clear carry flag
    
    // Use hardcoded instruction for LSR #0 (treated as LSR #32)
    writeInstruction(R(15), 0x081B); // LSR R3, #0 -> LSR R3, #32 (from format01 original)
    execute(1);
    
    EXPECT_EQ(R(3), 0u);           // LSR #32 results in 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Bit 31 was 1
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_MaxShift) {
    // Test case: Maximum explicit shift amount (31)
    setup_registers({{4, 0xFFFFFFFF}, {15, 0x00000000}});
    
    // Use hardcoded instruction for LSR #31
    writeInstruction(R(15), 0x0FE4); // LSR R4, #31 (from format01 original)
    execute(1);
    
    EXPECT_EQ(R(4), 1u);           // 0xFFFFFFFF >> 31 = 1
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Bit 30 was 1
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_Basic) {
    // Test case: ASR (arithmetic shift right) - positive number
    setup_registers({{0, 0x80}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("asrs r0, r0, #2", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0x20u);        // 0x80 >> 2 = 0x20
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_NegativeNumber) {
    // Test case: ASR with negative number (sign extension)
    setup_registers({{1, 0x80000000}, {15, 0x00000000}});  // Most negative 32-bit number
    
    ASSERT_TRUE(assembleAndWriteThumb("asrs r1, r1, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(1), 0xC0000000u);  // Sign bit extended
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));   // Still negative
    EXPECT_FALSE(getFlag(CPU::FLAG_C));  // No carry from bit 0
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_CarryOut) {
    // Test case: ASR with carry out from negative number
    setup_registers({{2, 0x80000001}, {15, 0x00000000}});  // Negative number with LSB set
    
    ASSERT_TRUE(assembleAndWriteThumb("asrs r2, r2, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(2), 0xC0000000u);  // Sign extended with carry
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));   // Negative result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from LSB
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_ZeroResult) {
    // Test case: ASR resulting in zero
    setup_registers({{2, 0x1}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("asrs r2, r2, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from LSB
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_ShiftByZero) {
    // Test case: Shift by 0 (special case, treated as ASR #32)
    setup_registers({{3, 0x80000000}, {15, 0x00000000}});
    cpu.CPSR() &= ~CPU::FLAG_C; // Pre-clear carry flag
    
    // Use hardcoded instruction for ASR #0 (treated as ASR #32)
    writeInstruction(R(15), 0x101B); // ASR R3, #0 -> ASR R3, #32 (from format01 original)
    execute(1);
    
    EXPECT_EQ(R(3), 0xFFFFFFFFu); // ASR #32 of negative = all 1s (sign extended)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));  // Negative result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // Bit 31 was 1
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_MaxShift) {
    // Test case: Maximum shift amount (31)
    setup_registers({{4, 0xFFFFFFFF}, {15, 0x00000000}});
    
    // Use hardcoded instruction for ASR #31
    writeInstruction(R(15), 0x17E4); // ASR R4, #31 (from format01 original)
    execute(1);
    
    EXPECT_EQ(R(4), 0xFFFFFFFFu); // Sign-extended (all 1s remain)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N)); // Negative result
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // Bit 0 was 1
    EXPECT_EQ(R(15), 0x00000002u);
}

// Test different source and destination registers
TEST_F(ThumbCPUTest, LSL_DifferentRegisters) {
    // Test case: LSL with different source and destination (Rd != Rs)
    setup_registers({{3, 0x5}, {4, 0}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsls r4, r3, #1", R(15)));
    execute(1);
    
    EXPECT_EQ(R(3), 0x5u);         // Source unchanged
    EXPECT_EQ(R(4), 0xAu);         // Destination = 0x5 << 1
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}
