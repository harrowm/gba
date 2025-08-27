// test_thumb03.cpp - Modern Thumb CPU test fixture for Format 3: Move/compare/add/subtract immediate
#include "thumb_test_base.h"

class ThumbCPUTest5 : public ThumbCPUTestBase {
    // Inherits all functionality from ThumbCPUTestBase
};

// ARM Thumb Format 3: Move/compare/add/subtract immediate
// Encoding: 001[Op)[Rd)[Offset8)
// Instructions: MOV, CMP, ADD, SUB with 8-bit immediate

// MOV Immediate Tests
TEST_F(ThumbCPUTest5, MOV_IMM_Basic) {
    // Test case: MOV R0, #1
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r0, #0x1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 1u);
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // MOV doesn't affect C
    EXPECT_FALSE(getFlag(CPU::FLAG_V));      // MOV doesn't affect V
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_Max) {
    // Test case: MOV R1, #255 (maximum 8-bit immediate)
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r1, #0xff", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(1), 255u);
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_Zero) {
    // Test case: MOV R2, #0 (sets Z flag)
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r2, #0x0", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_42) {
    // Test MOV R3, #42
    setup_registers({});
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r3, #0x2a", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(3), 42u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_127) {
    // Test MOV R4, #127
    setup_registers({});
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r4, #0x7f", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(4), 127u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_255_R5) {
    // Test MOV R5, #0xFF (duplicate coverage for register R5)
    setup_registers({});
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r5, #0xff", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(5), 255u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_Zero_R6) {
    // Test MOV R6, #0x00 (duplicate coverage for register R6)
    setup_registers({});
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r6, #0x0", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(6), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_128) {
    // Test MOV R7, #0x80 (128)
    setup_registers({});
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r7, #0x80", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(7), 128u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_IMM_FlagPreservation) {
    // Test MOV R7, #0x80 - test NCV flag preservation (only Z and N are affected)
    setup_registers({{7, 0}});
    
    // Set N, C, V flags initially
    cpu.CPSR() |= CPU::FLAG_N;
    cpu.CPSR() |= CPU::FLAG_C;
    cpu.CPSR() |= CPU::FLAG_V;
    
    ASSERT_TRUE(assembleAndWriteThumb("movs r7, #0x80", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(7), 128u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Z flag updated by MOV result
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // N flag updated by MOV result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // C flag preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // V flag preserved
    EXPECT_EQ(R(15), 0x00000002u);
}

// CMP Immediate Tests
TEST_F(ThumbCPUTest5, CMP_IMM_Equal) {
    // Test case: CMP equal values (R0 = 5, compare with 5)
    setup_registers({{0, 5}});
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r0, #0x5", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 5u);               // Register unchanged
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Equal -> Z set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow -> C set
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_IMM_Less) {
    // Test case: CMP less than (R1 = 0, compare with 1)
    setup_registers({{1, 0}});
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r1, #0x1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(1), 0u);               // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // 0 - 1 = negative
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // Borrow occurred -> C clear
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_IMM_Greater) {
    // Test case: CMP greater than (R2 = 10, compare with 5)
    setup_registers({{2, 10}});
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r2, #0x5", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 10u);              // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));      // 10 - 5 = positive
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow -> C set
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_IMM_Overflow) {
    // Test case: CMP with signed overflow (most negative - positive)
    setup_registers({{3, 0x80000000}});     // Most negative 32-bit signed int
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r3, #0xff", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(3), 0x80000000u);      // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));      // Result is positive
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_IMM_MaxValue) {
    // Test case: CMP with maximum value (0xFFFFFFFF - 255)
    setup_registers({{4, 0xFFFFFFFF}});
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r4, #0xff", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(4), 0xFFFFFFFFu);      // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Large positive result (negative in 2's complement)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

// ADD Immediate Tests
TEST_F(ThumbCPUTest5, ADD_IMM_Simple) {
    // Test case: Simple addition (R0 = 5, ADD #3)
    setup_registers({{0, 5}});
    
    ASSERT_TRUE(assembleAndWriteThumb("adds r0, #0x3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 8u);               // 5 + 3 = 8
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_IMM_Negative) {
    // Test case: Addition resulting in negative (large + large)
    setup_registers({{1, 0x80000000}});     // Large negative
    
    ASSERT_TRUE(assembleAndWriteThumb("adds r1, #0x1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(1), 0x80000001u);      // Still negative
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_IMM_Zero) {
    // Test case: Addition resulting in zero
    setup_registers({{2, static_cast<uint32_t>(-100)}});
    
    ASSERT_TRUE(assembleAndWriteThumb("adds r2, #0x64", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0u);               // -100 + 100 = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry from unsigned arithmetic
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_IMM_Overflow) {
    // Test case: Addition with signed overflow (positive + positive = negative)
    setup_registers({{3, 0x7FFFFFFF}});     // Maximum positive signed int
    
    ASSERT_TRUE(assembleAndWriteThumb("adds r3, #0x1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(3), 0x80000000u);      // Overflowed to negative
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_IMM_Carry) {
    // Test case: Addition with carry out
    setup_registers({{4, 0xFFFFFFFF}});     // Maximum unsigned value
    
    ASSERT_TRUE(assembleAndWriteThumb("adds r4, #0x1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(4), 0u);               // Wrapped to 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero result
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry out
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

// SUB Immediate Tests
TEST_F(ThumbCPUTest5, SUB_IMM_Simple) {
    // Test case: Simple subtraction (R0 = 10, SUB #3)
    setup_registers({{0, 10}});
    
    ASSERT_TRUE(assembleAndWriteThumb("subs r0, #0x3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 7u);               // 10 - 3 = 7
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow -> C set
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, SUB_IMM_Zero) {
    // Test case: Subtraction resulting in zero
    setup_registers({{1, 100}});
    
    ASSERT_TRUE(assembleAndWriteThumb("subs r1, #0x64", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(1), 0u);               // 100 - 100 = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, SUB_IMM_Negative) {
    // Test case: Subtraction resulting in negative (borrow)
    setup_registers({{2, 5}});
    
    ASSERT_TRUE(assembleAndWriteThumb("subs r2, #0xa", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), static_cast<uint32_t>(-5)); // 5 - 10 = -5
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // Borrow occurred -> C clear
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, SUB_IMM_Overflow) {
    // Test case: Subtraction with signed overflow (negative - positive = positive)
    setup_registers({{3, 0x80000000}});     // Most negative value
    
    ASSERT_TRUE(assembleAndWriteThumb("subs r3, #0x1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(3), 0x7FFFFFFFu);      // Overflowed to maximum positive
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));      // Positive result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest5, SUB_IMM_NoBorrow) {
    // Test case: Large subtraction with no borrow
    setup_registers({{4, 0xFFFFFFFF}});     // Maximum value
    
    ASSERT_TRUE(assembleAndWriteThumb("subs r4, #0x1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(4), 0xFFFFFFFEu);      // Still large positive
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative in 2's complement view
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}
