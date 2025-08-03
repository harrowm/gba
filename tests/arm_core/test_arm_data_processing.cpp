
// test_arm_data_processing.cpp
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

class ARMDataProcessingTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;

    ARMDataProcessingTest() : cpu(memory, interrupts), arm_cpu(cpu) {}

    void SetUp() override {
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        cpu.CPSR() = 0x10; // User mode, no flags set
    }
};

// AND: Rd = Rn & Operand2
TEST_F(ARMDataProcessingTest, AND_Basic) {
    cpu.R()[0] = 0xF0F0F0F0; // Rn
    cpu.R()[1] = 0x0F0F0F0F; // Rm
    cpu.R()[15] = 0x00000000;
    // AND r2, r0, r1 (Rd=2, Rn=0, Operand2=R1)
    uint32_t instr = 0xE0001002; // AND r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// AND with all bits set
TEST_F(ARMDataProcessingTest, AND_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF; // Rn
    cpu.R()[1] = 0x12345678; // Rm
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0002001; // AND r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

// AND with zero
TEST_F(ARMDataProcessingTest, AND_Zero) {
    cpu.R()[0] = 0x0; // Rn
    cpu.R()[1] = 0xFFFFFFFF; // Rm
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0002001; // AND r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

// AND with S bit set (flags)
TEST_F(ARMDataProcessingTest, ANDS_SetsFlags) {
    cpu.R()[0] = 0x80000000; // Rn
    cpu.R()[1] = 0xFFFFFFFF; // Rm
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0102001; // ANDS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
}

// ANDS with zero result (Z flag)
TEST_F(ARMDataProcessingTest, ANDS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0; // Rn
    cpu.R()[1] = 0x0; // Rm
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0102001; // ANDS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

// AND with immediate operand
TEST_F(ARMDataProcessingTest, AND_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE200200F; // AND r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
}

// AND with shifted operand (LSL #4)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0002281; // AND r2, r0, r1, LSL #5 (shift=5)
    // 0x0000000F << 5 = 0x000001E0, 0xFFFF00FF & 0x1E0 = 0x000000E0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x000000E0u);
}

// AND with shifted operand (LSR #4)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE00021A1; // AND r2, r0, r1, LSR #3 (shift=3)
    // 0xF0000000 >> 3 = 0x1E000000, 0x0F0F0F0F & 0x1E000000 = 0x0E000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0E000000u);
}

// AND with shifted operand (ASR #8)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE00021C1; // AND r2, r0, r1, ASR #3 (shift=3)
    // 0x80000000 >> 3 (arithmetic) = 0xF0000000, 0xFFFFFFFF & 0xF0000000 = 0xF0000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0000000u);
}

// AND with shifted operand (ROR #4)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0002161; // AND r2, r0, r1, ROR #2 (shift=2)
    // 0x0000000F ror 2 = 0xC0000003, 0xFF00FF00 & 0xC0000003 = 0xC0000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);
}

// ANDS with carry out from shifter (LSR #1, S=1)
TEST_F(ARMDataProcessingTest, ANDS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    uint32_t instr = 0xe01021a1 ; // ANDS r2, r0, r1, LSR #3 (shift=3, S=1)
    // 0x3 >> 3 = 0x0, carry out is bit 2 (should be 0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    // C flag should be 0
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

// AND with S=0 (flags unchanged)
TEST_F(ARMDataProcessingTest, AND_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0002001; // AND r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    // N and C should remain set
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

// ANDS with Rd=15 (PC), S=1 (should not update CPSR in user mode)
TEST_F(ARMDataProcessingTest, ANDS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xe21ff001; // ANDS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // CPSR should remain unchanged
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

// AND with condition code (NE, should not execute)
TEST_F(ARMDataProcessingTest, AND_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x10002001; // ANDNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Should not execute, r2 unchanged
    EXPECT_EQ(cpu.R()[2], 0u);
}

// AND with edge values
TEST_F(ARMDataProcessingTest, AND_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0002001; // AND r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

// AND a register with itself
TEST_F(ARMDataProcessingTest, AND_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0002000; // AND r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

// AND with register-shifted register (LSL by register)
TEST_F(ARMDataProcessingTest, AND_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4; // shift amount in r3
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0002311; // AND r2, r0, r1, LSL r3
    // 0x0000000F << 4 = 0x000000F0, 0xFFFF00FF & 0xF0 = 0x000000F0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x000000F0u);
}

// AND with RRX (rotate right with extend)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0002171; // AND r2, r0, r1, ROR #0 (==RRX)
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);
}

// AND with rotated immediate (e.g., #0xFF000000)
TEST_F(ARMDataProcessingTest, AND_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    // #0xFF000000 is encoded as 0xFF rotated right by 8 (imm=0xFF, rot=4)
    uint32_t instr = 0xE20024FF; // AND r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF000000u);
}

