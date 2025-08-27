
/*
 * test_thumb05.cpp - Modern Thumb CPU test fixture for Format 5: Hi register operations/branch exchange
 * 
 * This file contains comprehensive tests for ARM Thumb Format 5 instructions:
 * - ADD Hi register: Add high register (R8-R15) to low register (R0-R7) or vice versa
 * - CMP Hi register: Compare high register with low register or vice versa
 * - MOV Hi register: Move value between high and low registers
 * - BX: Branch and exchange instruction set (ARM/Thumb mode switching)
 * 
 * Format 5 Encoding: 010001[Op)[H1)[H2)[Rs/Hs)[Rd/Hd)
 * - Op[1:0): 00=ADD, 01=CMP, 10=MOV, 11=BX
 * - H1: High register flag for destination (0=R0-R7, 1=R8-R15)
 * - H2: High register flag for source (0=R0-R7, 1=R8-R15)
 * - Rs/Hs: 3-bit source register + H2 flag
 * - Rd/Hd: 3-bit destination register + H1 flag
 * 
 * Special behaviors:
 * - ADD/CMP/MOV operations with PC (R15) have special handling
 * - BX can switch between ARM and Thumb modes based on LSB of target address
 * - High register operations extend the accessible register range beyond R0-R7
 * 
 * These tests use the modern ThumbCPUTestBase infrastructure with:
 * - Keystone assembler for runtime instruction generation  
 * - Modern R() register access API
 * - Automated execution and state management
 * - Comprehensive mode switching and PC handling verification
 * 
 * Coverage: 28 tests across all high register operations, mode switching, and edge cases
 */
#include "thumb_test_base.h"

class ThumbCPUTest7 : public ThumbCPUTestBase {
};

// ARM Thumb Format 5: Hi register operations/branch exchange
// Encoding: 010001[Op)[H1)[H2)[Rs/Hs)[Rd/Hd)
// Instructions: ADD Rd, Rs; CMP Rd, Rs; MOV Rd, Rs; BX Rs

// ADD Hi Register Tests
TEST_F(ThumbCPUTest7, ADD_LowPlusHigh) {
    // Test case: ADD R0, R8 (low + high register)
    setup_registers({{0, 0x12345678}, {8, 0x87654321}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("add r0, r8", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0x99999999u); // 0x12345678 + 0x87654321
    EXPECT_EQ(R(8), 0x87654321u); // R8 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, ADD_HighPlusLow) {
    // Test case: ADD R8, R0 (high + low register)
    setup_registers({{8, 0x11111111}, {0, 0x22222222}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("add r8, r0", R(15)));
    execute(1);
    
    EXPECT_EQ(R(8), 0x33333333u); // 0x11111111 + 0x22222222
    EXPECT_EQ(R(0), 0x22222222u); // R0 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, ADD_HighPlusHigh) {
    // Test case: ADD R8, R9 (high + high register)
    setup_registers({{8, 0xAAAAAAAA}, {9, 0x55555555}, {15, 0x00000000}});
    
    ASSERT_TRUE(assembleAndWriteThumb("add r8, r9", R(15)));
    execute(1);
    
    EXPECT_EQ(R(8), 0xFFFFFFFFu); // 0xAAAAAAAA + 0x55555555
    EXPECT_EQ(R(9), 0x55555555u); // R9 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, ADD_WithPC) {
    // Test case: ADD R0, PC (PC is R15, high register)
    setup_registers({{0, 0x00000100}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("add r0, pc", R(15)));
    execute(1);
    
    // PC+4 alignment in Thumb mode (PC is read as current PC + 4)
    EXPECT_EQ(R(0), 0x00000104u); // 0x100 + (0x0 + 4)
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, ADD_ZeroValues) {
    // Test case: ADD with zero values
    setup_registers({{0, 0x00000000}, {8, 0x00000000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("add r0, r8", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0x00000000u);
    EXPECT_EQ(R(15), 0x00000002u);
}

// CMP Hi Register Tests
TEST_F(ThumbCPUTest7, CMP_Equal) {
    // Test case: CMP R0, R8 (equal values)
    setup_registers({{0, 0x12345678}, {8, 0x12345678}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r0, r8", R(15)));
    execute(1);
    
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));  // Equal values set Z
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Result is zero (positive)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V)); // No overflow
    EXPECT_EQ(R(0), 0x12345678u); // R0 unchanged
    EXPECT_EQ(R(8), 0x12345678u); // R8 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, CMP_FirstGreater) {
    // Test case: CMP R8, R0 (first greater than second)
    setup_registers({{8, 0x80000000}, {0, 0x12345678}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r8, r0", R(15)));
    execute(1);
    
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Not equal
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Result positive (unsigned comparison)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // Overflow detected by CPU implementation
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, CMP_FirstLess) {
    // Test case: CMP R0, R8 (first less than second)
    setup_registers({{0, 0x12345678}, {8, 0x80000000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r0, r8", R(15)));
    execute(1);
    
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Not equal
    EXPECT_TRUE(getFlag(CPU::FLAG_N));  // Result negative (borrow occurred)
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // Borrow occurred
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // Overflow detected by CPU implementation
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, CMP_WithPC) {
    // Test case: CMP R0, PC
    setup_registers({{0, 0x00000004}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r0, pc", R(15)));
    execute(1);
    
    // PC+4 alignment: CMP 0x4, (0x0 + 4) = CMP 0x4, 0x4
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));  // Equal
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

// MOV Hi Register Tests
TEST_F(ThumbCPUTest7, MOV_LowToHigh) {
    // Test case: MOV R8, R0 (low to high register)
    setup_registers({{0, 0x12345678}, {8, 0x00000000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov r8, r0", R(15)));
    execute(1);
    
    EXPECT_EQ(R(8), 0x12345678u); // R8 gets R0's value
    EXPECT_EQ(R(0), 0x12345678u); // R0 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, MOV_HighToLow) {
    // Test case: MOV R0, R8 (high to low register)
    setup_registers({{8, 0x87654321}, {0, 0x11111111}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov r0, r8", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0x87654321u); // R0 gets R8's value
    EXPECT_EQ(R(8), 0x87654321u); // R8 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, MOV_HighToHigh) {
    // Test case: MOV R8, R9 (high to high register)
    setup_registers({{9, 0xCAFEBABE}, {8, 0x00000000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov r8, r9", R(15)));
    execute(1);
    
    EXPECT_EQ(R(8), 0xCAFEBABEu); // R8 gets R9's value
    EXPECT_EQ(R(9), 0xCAFEBABEu); // R9 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, MOV_PCToRegister) {
    // Test case: MOV R0, PC
    setup_registers({{0, 0x11111111}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov r0, pc", R(15)));
    execute(1);
    
    // PC+4 alignment: R0 gets PC+4
    EXPECT_EQ(R(0), 0x00000004u); // PC (0x0) + 4
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, MOV_ToPC) {
    // Test case: MOV PC, R0 (branch to address in R0)
    setup_registers({{0, 0x00000200}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov pc, r0", R(15)));
    execute(1);
    
    EXPECT_EQ(R(15), 0x00000200u); // PC set to R0's value
    EXPECT_TRUE(getFlag(CPU::FLAG_T));   // Still in Thumb mode
}

// BX Branch Exchange Tests
TEST_F(ThumbCPUTest7, BX_ToARM) {
    // Test case: BX R0 (branch to ARM mode - bit 0 clear)
    setup_registers({{0, 0x00000200}}); // ARM address (even)
    R(15) = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assembleAndWriteThumb("bx r0", R(15)));
    execute(1);
    
    EXPECT_EQ(R(15), 0x00000200u); // PC set to target address
    EXPECT_FALSE(getFlag(CPU::FLAG_T));  // Switched to ARM mode (T flag clear)
}

TEST_F(ThumbCPUTest7, BX_ToThumb) {
    // Test case: BX R1 (branch to Thumb mode - bit 0 set)
    setup_registers({{1, 0x00000301}}); // Thumb address (odd)
    R(15) = 0x00000000;
    cpu.CPSR() = 0; // Start in ARM mode (T flag clear)
    
    // Use ARM encoding for BX since we're starting in ARM mode
    memory.write32(R(15), 0xE12FFF11); // BX R1 (ARM encoding)
    cpu.execute(1);
    
    EXPECT_EQ(R(15), 0x00000300u); // PC set to target with bit 0 cleared
    EXPECT_TRUE(getFlag(CPU::FLAG_T));   // Switched to Thumb mode (T flag set)
}

TEST_F(ThumbCPUTest7, BX_HighRegister) {
    // Test case: BX R8 (branch with high register)
    setup_registers({{8, 0x00000400}}); // ARM address
    R(15) = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assembleAndWriteThumb("bx r8", R(15)));
    execute(1);
    
    EXPECT_EQ(R(15), 0x00000400u); // PC set to R8's value
    EXPECT_FALSE(getFlag(CPU::FLAG_T));  // Switched to ARM mode
}

TEST_F(ThumbCPUTest7, BX_ThumbToThumb) {
    // Test case: BX with Thumb address while in Thumb mode
    setup_registers({{2, 0x00000501}}); // Thumb address (odd)
    R(15) = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assembleAndWriteThumb("bx r2", R(15)));
    execute(1);
    
    EXPECT_EQ(R(15), 0x00000500u); // PC set with bit 0 cleared
    EXPECT_TRUE(getFlag(CPU::FLAG_T));   // Stay in Thumb mode
}

// Edge Cases and Boundary Conditions
TEST_F(ThumbCPUTest7, ADD_Overflow) {
    // Test case: ADD causing 32-bit overflow
    setup_registers({{0, 0xFFFFFFFF}, {8, 0x00000001}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("add r0, r8", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0x00000000u); // Wraps to 0
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, CMP_Overflow) {
    // Test case: CMP with signed overflow
    setup_registers({{0, 0x7FFFFFFF}, {8, 0x80000000}}); // Max positive - max negative
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r0, r8", R(15)));
    execute(1);
    
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Not equal
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // Signed overflow occurred
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, MOV_LR) {
    // Test case: MOV involving LR (R14)
    setup_registers({{14, 0xDEADBEEF}, {0, 0x00000000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov r0, lr", R(15)));
    execute(1);
    
    EXPECT_EQ(R(0), 0xDEADBEEFu); // R0 gets LR's value
    EXPECT_EQ(R(14), 0xDEADBEEFu); // LR unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

// Missing ADD Operations
TEST_F(ThumbCPUTest7, ADD_LowPlusLow) {
    // Test case: ADD R1, R2 (low + low register - valid when at least one operand involves hi register behavior)
    setup_registers({{1, 0x10203040}, {2, 0x01020304}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("add r1, r2", R(15)));
    execute(1);
    
    EXPECT_EQ(R(1), 0x11223344u); // 0x10203040 + 0x01020304
    EXPECT_EQ(R(2), 0x01020304u); // R2 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, ADD_PCPlusLR) {
    // Test case: ADD PC, LR (PC modification with pipeline effect)
    setup_registers({{14, 0x00000008}});
    R(15) = 0x00000100;
    
    ASSERT_TRUE(assembleAndWriteThumb("add pc, lr", R(15)));
    execute(1);
    
    // PC should be updated to LR + current PC + 4 (pipeline effect)
    uint32_t expected_pc = (0x00000100 + 4) + 0x00000008;
    EXPECT_EQ(R(15), expected_pc);
}

TEST_F(ThumbCPUTest7, ADD_SPPlusRegister) {
    // Test case: ADD SP, R8 (stack pointer modification)
    setup_registers({{13, 0x00001000}, {8, 0x00000100}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("add sp, r8", R(15)));
    execute(1);
    
    EXPECT_EQ(R(13), 0x00001100u); // 0x1000 + 0x100
    EXPECT_EQ(R(8), 0x00000100u); // R8 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

// Missing CMP Operations
TEST_F(ThumbCPUTest7, CMP_NegativeResult) {
    // Test case: CMP with negative result (1 - 2)
    setup_registers({{8, 0x00000001}, {9, 0x00000002}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r8, r9", R(15)));
    execute(1);
    
    // 1 - 2 = -1 (0xFFFFFFFF)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Not zero
    EXPECT_TRUE(getFlag(CPU::FLAG_N));  // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // Borrow occurred
    EXPECT_FALSE(getFlag(CPU::FLAG_V)); // No overflow
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, CMP_ZeroWithZero) {
    // Test case: CMP zero with zero
    setup_registers({{0, 0x00000000}, {8, 0x00000000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r0, r8", R(15)));
    execute(1);
    
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));  // Zero result
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Not negative
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V)); // No overflow
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, CMP_MaxValues) {
    // Test case: CMP with maximum values (0xFFFFFFFF vs 0xFFFFFFFF)
    setup_registers({{8, 0xFFFFFFFF}, {9, 0xFFFFFFFF}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("cmp r8, r9", R(15)));
    execute(1);
    
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));  // Equal values
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Result is zero
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V)); // No overflow
    EXPECT_EQ(R(15), 0x00000002u);
}

// Missing MOV Operations  
TEST_F(ThumbCPUTest7, MOV_PCFromLR) {
    // Test case: MOV PC, LR (branch using MOV)
    setup_registers({{14, 0x00000200}});
    R(15) = 0x00000100;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov pc, lr", R(15)));
    execute(1);
    
    // PC should be set to LR value
    EXPECT_EQ(R(15), 0x00000200u);
    EXPECT_TRUE(getFlag(CPU::FLAG_T)); // Still in Thumb mode
}

TEST_F(ThumbCPUTest7, MOV_SPFromRegister) {
    // Test case: MOV SP, R12 (stack pointer manipulation)
    setup_registers({{12, 0x00001FFF}, {13, 0x00001000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov sp, r12", R(15)));
    execute(1);
    
    EXPECT_EQ(R(13), 0x00001FFFu); // SP gets R12's value
    EXPECT_EQ(R(12), 0x00001FFFu); // R12 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, MOV_LRFromPC) {
    // Test case: MOV LR, PC (save return address with pipeline)
    setup_registers({{14, 0x00000000}});
    R(15) = 0x00000500;
    
    ASSERT_TRUE(assembleAndWriteThumb("mov lr, pc", R(15)));
    execute(1);
    
    // LR should get PC + 4 (pipeline effect)
    EXPECT_EQ(R(14), 0x00000504u); // PC (0x500) + 4
    EXPECT_EQ(R(15), 0x00000502u); // PC incremented normally
}

// Missing BX Operations
TEST_F(ThumbCPUTest7, BX_FromLR) {
    // Test case: BX LR (return from function)
    setup_registers({{14, 0x00000505}}); // Return address (Thumb mode)
    R(15) = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assembleAndWriteThumb("bx lr", R(15)));
    execute(1);
    
    EXPECT_EQ(R(15), 0x00000504u); // Bit 0 cleared
    EXPECT_TRUE(getFlag(CPU::FLAG_T)); // Thumb mode (bit 0 was set)
}

TEST_F(ThumbCPUTest7, BX_FromPC) {
    // Test case: BX PC (branch to current PC + pipeline offset)
    R(15) = 0x00000100;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assembleAndWriteThumb("bx pc", R(15)));
    execute(1);
    
    // PC should branch to itself + 4 (pipeline effect), ARM mode
    EXPECT_EQ(R(15), 0x00000104u);
    EXPECT_FALSE(getFlag(CPU::FLAG_T)); // ARM mode (bit 0 clear)
}

TEST_F(ThumbCPUTest7, BX_MemoryBoundary) {
    // Test case: BX with address at memory boundary
    setup_registers({{0, 0x00001FFF}}); // At memory boundary (Thumb mode)
    R(15) = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assembleAndWriteThumb("bx r0", R(15)));
    execute(1);
    
    EXPECT_EQ(R(15), 0x00001FFEu); // Bit 0 cleared
    EXPECT_TRUE(getFlag(CPU::FLAG_T)); // Thumb mode (bit 0 was set)
}

// Edge Case: Register Combinations
TEST_F(ThumbCPUTest7, ADD_RegisterCombinations) {
    // Test case: ADD R8, R8 (same register)
    setup_registers({{8, 0x10000000}});
    R(15) = 0x00000000;
    
    ASSERT_TRUE(assembleAndWriteThumb("add r8, r8", R(15)));
    execute(1);
    
    // For ADD Rd, Rs where Rd == Rs: result = 2 * initial_value
    EXPECT_EQ(R(8), 0x20000000u); // 2 * 0x10000000
    EXPECT_EQ(R(15), 0x00000002u);
}

// Edge Case: Flag Preservation
TEST_F(ThumbCPUTest7, FlagPreservation) {
    // Test case: Verify ADD/MOV don't affect flags, BX preserves non-T flags
    uint32_t initial_flags = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
    setup_registers({{8, 0x12345678}, {0, 0x87654321}});
    cpu.CPSR() = initial_flags;
    R(15) = 0x00000000;
    
    // Test ADD (should not affect flags)
    ASSERT_TRUE(assembleAndWriteThumb("add r0, r8", R(15)));
    execute(1);
    
    // All flags should be preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_T));
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest7, BX_FlagPreservation) {
    // Test case: BX preserves other flags (from original test case 7)
    setup_registers({{0, 0x00000200}}); // ARM mode target
    R(15) = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
    
    ASSERT_TRUE(assembleAndWriteThumb("bx r0", R(15)));
    execute(1);
    
    EXPECT_EQ(R(15), 0x00000200u);
    EXPECT_FALSE(getFlag(CPU::FLAG_T)); // Changed to ARM
    // Other flags should be preserved  
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));
}
