/*
 * Thumb Format 4 ALU Operations Tests
 * 
 * This file tests the Thumb Format 4 (ALU operations) instruction format,
 * which includes register-to-register ALU operations that affect flags.
 * 
 * Instructions tested:
 * - AND: Bitwise AND with flag updates  
 * - EOR: Exclusive OR (XOR) with flag updates
 * - LSL: Logical Shift Left with carry out
 * - LSR: Logical Shift Right with carry out  
 * - ASR: Arithmetic Shift Right with sign extension
 * - ADC: Add with Carry
 * - SBC: Subtract with Carry (borrow)
 * - ROR: Rotate Right with carry
 * - TST: Test (AND without result storage, flags only)
 * - NEG: Negate (two's complement)
 * - CMP: Compare (subtract without result storage, flags only)
 * - CMN: Compare Negative (add without result storage, flags only)
 * - ORR: Bitwise OR with flag updates
 * - MUL: Multiply with flag updates
 * - BIC: Bit Clear (AND with complement)
 * - MVN: Move NOT (bitwise complement)
 * 
 * Also includes Thumb Format 5 PC-relative load tests (LDR Rd, [PC, #imm]).
 * 
 * All tests use the modern ThumbCPUTestBase infrastructure with assembly-based
 * instruction generation via Keystone assembler, using the 's' suffix syntax
 * (e.g., 'ands', 'eors') required for proper Thumb instruction encoding.
 */

#include "thumb_test_base.h"

class ThumbCPUTest6 : public ThumbCPUTestBase {
};

// AND Tests
TEST_F(ThumbCPUTest6, AND_Basic) {
    // Test case: Basic AND operation
    setup_registers({{0, 0xFF00FF00}, {1, 0xF0F0F0F0}});
    
    ASSERT_TRUE(assembleAndWriteThumb("ands r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0xF000F000u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N)); // Result is negative
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // C is unaffected by AND
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, AND_ResultZero) {
    // Test case: AND resulting in zero
    setup_registers({{2, 0xAAAAAAAA}, {3, 0x55555555}});
    
    ASSERT_TRUE(assembleAndWriteThumb("ands r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

// EOR (XOR) Tests  
TEST_F(ThumbCPUTest6, EOR_Basic) {
    // Test case: Basic XOR operation
    setup_registers({{0, 0xFF00FF00}, {1, 0xF0F0F0F0}});
    
    ASSERT_TRUE(assembleAndWriteThumb("eors r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0x0FF00FF0u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, EOR_SelfZero) {
    // Test case: XOR with itself (should result in zero)
    setup_registers({{4, 0x12345678}});
    
    ASSERT_TRUE(assembleAndWriteThumb("eors r4, r4", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(4), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

// LSL Tests
TEST_F(ThumbCPUTest6, LSL_Basic) {
    // Test case: Basic logical shift left (no carry out)
    setup_registers({{0, 0x00000001}, {1, 2}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsls r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0x00000004u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, LSL_CarryOut) {
    // Test case: LSL with carry out
    setup_registers({{2, 0x80000000}, {3, 1}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsls r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

// LSR Tests
TEST_F(ThumbCPUTest6, LSR_Basic) {
    // Test case: Basic logical shift right (no carry out)
    setup_registers({{0, 0x00000010}, {1, 2}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsrs r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0x00000004u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, LSR_CarryOut) {
    // Test case: LSR with carry out
    setup_registers({{2, 0x00000001}, {3, 1}});
    
    ASSERT_TRUE(assembleAndWriteThumb("lsrs r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

// ASR Tests
TEST_F(ThumbCPUTest6, ASR_Basic) {
    // Test case: Basic arithmetic shift right (positive number, no carry out)
    setup_registers({{0, 0x00000010}, {1, 2}});    
    ASSERT_TRUE(assembleAndWriteThumb("asrs r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0x00000004u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, ASR_Negative) {
    // Test case: ASR with negative number (sign extension)
    setup_registers({{2, 0x80000000}, {3, 4}});    
    ASSERT_TRUE(assembleAndWriteThumb("asrs r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0xF8000000u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(R(15), 0x00000002u);
}

// TST Tests
TEST_F(ThumbCPUTest6, TST_NonZero) {
    // Test case: TST with non-zero result
    setup_registers({{0, 0xFF00FF00}, {1, 0xF0F0F0F0}});    
    ASSERT_TRUE(assembleAndWriteThumb("tst r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    // TST doesn't modify the destination register, only flags
    EXPECT_EQ(R(0), 0xFF00FF00u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, TST_Zero) {
    // Test case: TST with zero result
    setup_registers({{2, 0xAAAAAAAA}, {3, 0x55555555}});    
    ASSERT_TRUE(assembleAndWriteThumb("tst r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0xAAAAAAAAu);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

// NEG Tests
TEST_F(ThumbCPUTest6, NEG_Basic) {
    // Test case: Basic negation
    setup_registers({{0, 5}});    
    ASSERT_TRUE(assembleAndWriteThumb("negs r0, r0", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0xFFFFFFFBu); // -5 in two's complement
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // Borrow occurred
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, NEG_Zero) {
    // Test case: Negation of zero
    setup_registers({{1, 0}});    
    ASSERT_TRUE(assembleAndWriteThumb("negs r1, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(1), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // No borrow for 0-0
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

// CMP Tests
TEST_F(ThumbCPUTest6, CMP_Equal) {
    // Test case: CMP with equal values
    setup_registers({{0, 10}, {1, 10}});    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 10u); // CMP doesn't modify registers
    EXPECT_EQ(R(1), 10u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, CMP_Less) {
    // Test case: CMP with first < second
    setup_registers({{2, 5}, {3, 10}});    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 5u);
    EXPECT_EQ(R(3), 10u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // Borrow occurred
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

// ORR Tests
TEST_F(ThumbCPUTest6, ORR_Basic) {
    // Test case: Basic OR operation
    setup_registers({{0, 0x00FF00FF}, {1, 0xFF0000FF}});    
    ASSERT_TRUE(assembleAndWriteThumb("orrs r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0xFFFF00FFu);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

// MUL Tests
TEST_F(ThumbCPUTest6, MUL_Basic) {
    // Test case: Basic multiplication
    setup_registers({{0, 6}, {1, 7}});    
    ASSERT_TRUE(assembleAndWriteThumb("muls r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 42u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, MUL_Zero) {
    // Test case: Multiplication resulting in zero
    setup_registers({{2, 0}, {3, 999}});    
    ASSERT_TRUE(assembleAndWriteThumb("muls r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

// BIC Tests
TEST_F(ThumbCPUTest6, BIC_Basic) {
    // Test case: Basic bit clear operation
    setup_registers({{0, 0xFFFFFFFF}, {1, 0xF0F0F0F0}});    
    ASSERT_TRUE(assembleAndWriteThumb("bics r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0x0F0F0F0Fu);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

// MVN Tests
TEST_F(ThumbCPUTest6, MVN_Basic) {
    // Test case: Basic move NOT operation
    setup_registers({{0, 0xF0F0F0F0}, {1, 0}});    
    ASSERT_TRUE(assembleAndWriteThumb("mvns r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0xFFFFFFFFu);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(R(15), 0x00000002u);
}

// ADC Tests
TEST_F(ThumbCPUTest6, ADC_NoCarry) {
    // Test case: Add with carry, no previous carry
    setup_registers({{0, 5}, {1, 3}});    cpu.CPSR() &= ~CPU::FLAG_C; // Clear carry flag
    
    ASSERT_TRUE(assembleAndWriteThumb("adcs r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 8u); // 5 + 3 + 0 (no carry)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, ADC_WithCarry) {
    // Test case: Add with carry, previous carry set
    setup_registers({{2, 5}, {3, 3}});    cpu.CPSR() |= CPU::FLAG_C; // Set carry flag
    
    ASSERT_TRUE(assembleAndWriteThumb("adcs r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 9u); // 5 + 3 + 1 (carry)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

// SBC Tests  
TEST_F(ThumbCPUTest6, SBC_NoBorrow) {
    // Test case: Subtract with carry, no borrow
    setup_registers({{0, 10}, {1, 3}});    cpu.CPSR() |= CPU::FLAG_C; // Set carry flag (no borrow)
    
    ASSERT_TRUE(assembleAndWriteThumb("sbcs r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 7u); // 10 - 3 - 0 (no borrow)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest6, SBC_WithBorrow) {
    // Test case: Subtract with carry, with borrow
    setup_registers({{2, 5}, {3, 3}});    cpu.CPSR() &= ~CPU::FLAG_C; // Clear carry flag (borrow)
    
    ASSERT_TRUE(assembleAndWriteThumb("sbcs r2, r3", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(2), 1u); // 5 - 3 - 1 (borrow)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

// ROR Tests
TEST_F(ThumbCPUTest6, ROR_Basic) {
    // Test case: Basic rotate right (carry flag set from rotated bit)
    setup_registers({{0, 0x80000001}, {1, 1}});    
    ASSERT_TRUE(assembleAndWriteThumb("rors r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 0xC0000000u); // Bit 0 rotated to bit 31
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N)); // Result is negative
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // Bit rotated out to carry
    EXPECT_EQ(R(15), 0x00000002u);
}

// CMN Tests
TEST_F(ThumbCPUTest6, CMN_Basic) {
    // Test case: Compare negative (CMN) - equivalent to ADD for flags
    setup_registers({{0, 5}, {1, 7}});    
    ASSERT_TRUE(assembleAndWriteThumb("cmn r0, r1", R(15)));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(R(0), 5u); // CMN doesn't modify registers
    EXPECT_EQ(R(1), 7u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // 5 + 7 = 12, not zero
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Result is positive
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // No carry
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}
