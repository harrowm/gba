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

// ===================== EOR Tests =====================
// EOR: Rd = Rn ^ Operand2
TEST_F(ARMDataProcessingTest, EOR_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202001; // EOR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, EOR_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202001; // EOR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xEDCBA987u);
}

TEST_F(ARMDataProcessingTest, EOR_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202001; // EOR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, EORS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0302001; // EORS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x7FFFFFFFu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, EORS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0302001; // EORS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

TEST_F(ARMDataProcessingTest, EOR_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE220200F; // EOR r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0FFu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0202281; // EOR r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xffff011fu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE02021A1; // EOR r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x110F0F0Fu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE02021C1; // EOR r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0FFFFFFFu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202161; // EOR r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x3F00FF03u);
}

TEST_F(ARMDataProcessingTest, EORS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    uint32_t instr = 0xE03021A1; // EORS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, EOR_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0202001; // EOR r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, EORS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE23FF001; // EORS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, EOR_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x12002001; // EORNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, EOR_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202001; // EOR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, EOR_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202000; // EOR r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202311; // EOR r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu ^ 0x000000F0u);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0202171; // EOR r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu ^ 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, EOR_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE22024FF; // EOR r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00FFFFFFu);
}

// ===================== SUB Tests =====================
// SUB: Rd = Rn - Operand2
TEST_F(ARMDataProcessingTest, SUB_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0402001; // SUB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFu);
}

TEST_F(ARMDataProcessingTest, SUB_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0402001; // SUB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xEDCBA987u);
}

TEST_F(ARMDataProcessingTest, SUB_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0402001; // SUB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
}

TEST_F(ARMDataProcessingTest, SUBS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0502001; // SUBS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000001u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, SUBS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x1;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0502001; // SUBS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, SUB_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE240200F; // SUB r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0E1u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0402281; // SUB r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - 0x1E0u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE04021A1; // SUB r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu - 0x1E000000u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE04021C1; // SUB r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xF0000000u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0402161; // SUB r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u - 0xC0000003u);
}

TEST_F(ARMDataProcessingTest, SUBS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    uint32_t instr = 0xe05021a1 ; // SUBS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x0u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); 
}

TEST_F(ARMDataProcessingTest, SUBS_CarryOutFromShifter_CarrySet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x4; // binary 0100
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    uint32_t instr = 0xE05021A1; // SUBS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - (0x4u >> 3)); // 0xFFFFFFFF - 0
    // Carry-out is bit 2 of r1 (0x4), which is 1
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C flag should be set
}

TEST_F(ARMDataProcessingTest, SUB_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0402001; // SUB r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, SUBS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE25FF001; // SUBS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, SUB_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x14002001; // SUBNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, SUB_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0402001; // SUB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, SUB_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0402000; // SUB r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0402311; // SUB r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - 0x000000F0u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0402171; // SUB r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, SUB_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE24024FF; // SUB r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFF000000u);
}

// ===================== RSB Tests =====================
// RSB: Rd = Operand2 - Rn
TEST_F(ARMDataProcessingTest, RSB_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0602001 ; // RSB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u - 0x10u);
}

TEST_F(ARMDataProcessingTest, RSB_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0602001; // RSB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, RSB_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0602001; // RSB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x0u);
}

TEST_F(ARMDataProcessingTest, RSBS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0702001; // RSBS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x80000000u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, RSBS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x1;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0702001; // RSBS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, RSB_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE260200F; // RSB r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFu - 0xF0F0F0F0u);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0602281; // RSB r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 5) - 0xFFFF00FFu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE06021A1; // RSB r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0xF0000000 >> 3) - 0x0F0F0F0Fu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE06021C1; // RSB r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((int32_t)0x80000000 >> 3) - 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0602161; // RSB r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((0x0000000F >> 2) | (0x0000000F << 30)) - 0xFF00FF00u);
}

TEST_F(ARMDataProcessingTest, RSBS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    uint32_t instr = 0xE07021A1; // RSBS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x3u >> 3) - 0xFFFFFFFFu);
    // C flag: borrow occurred, so C should be 0
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, RSB_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0602001; // RSB r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u - 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, RSBS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE27FF001; // RSBS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, RSB_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x16002001; // RSBNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, RSB_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0602001; // RSB r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x7FFFFFFFu - 0x80000000u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, RSB_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0602000; // RSB r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0x12345678u);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0602311; // RSB r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 4) - 0xFFFF00FFu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0602171; // RSB r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xC0000000u - 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, RSB_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE26024FF; // RSB r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF000000u - 0xFFFFFFFFu);
}

// ===================== ADD Tests =====================
// ADD: Rd = Rn + Operand2
TEST_F(ARMDataProcessingTest, ADD_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802001; // ADD r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x11u);
}

TEST_F(ARMDataProcessingTest, ADD_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802001; // ADD r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345677u);
}

TEST_F(ARMDataProcessingTest, ADD_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802001; // ADD r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ADDS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0902001; // ADDS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u); // Result should be 0
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set (carry out)
    EXPECT_TRUE(cpu.CPSR() & (1u << 28)); // V set (overflow)
}

TEST_F(ARMDataProcessingTest, ADDS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0902001; // ADDS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, ADD_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE280200F; // ADD r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0FFu);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802281; // ADD r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + 0x1E0u);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE08021A1; // ADD r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu + 0x1E000000u);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE08021C1; // ADD r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + ((int32_t)0x80000000 >> 3));
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802161; // ADD r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u + ((0x0000000F >> 2) | (0x0000000F << 30)));
}

TEST_F(ARMDataProcessingTest, ADDS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    uint32_t instr = 0xE09021A1; // ADDS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + (0x3u >> 3));
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, ADD_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0802001; // ADD r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, ADDS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE29FF001; // ADDS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, ADD_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x18002001; // ADDNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, ADD_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802001; // ADD r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFEu);
}

TEST_F(ARMDataProcessingTest, ADD_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802000; // ADD r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u + 0x12345678u);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0802311; // ADD r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + (0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0802171; // ADD r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, ADD_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE28024FF; // ADD r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xFF000000u);
}

// ===================== ADC Tests =====================
// ADC: Rd = Rn + Operand2 + C
TEST_F(ARMDataProcessingTest, ADC_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A01002; // ADC r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 0x11u);
}

TEST_F(ARMDataProcessingTest, ADC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A02001; // ADC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0x12345678u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x0; // C flag clear
    uint32_t instr = 0xE0A02001; // ADC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ADCS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0B02001; // ADCS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set (carry out)
    EXPECT_FALSE(cpu.CPSR() & (1u << 28)); // V set (overflow)
}

TEST_F(ARMDataProcessingTest, ADCS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0B02001; // ADCS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, ADC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2A0200F; // ADC r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u + 0xFu + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A02281; // ADC r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + 0x1E0u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A021A1; // ADC r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu + 0x1E000000u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A021C1; // ADC r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + ((int32_t)0x80000000 >> 3) + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A02161; // ADC r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u + ((0x0000000F >> 2) | (0x0000000F << 30)) + 1u);
}

TEST_F(ARMDataProcessingTest, ADCS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0B021A1; // ADCS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + (0x3u >> 3) + 1u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, ADC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0A02001; // ADC r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, ADCS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE2BFF001; // ADCS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, ADC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x1A002001; // ADCNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, ADC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A02001; // ADC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u + 0x7FFFFFFFu + 1u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xFFFFFFFFu + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A02000; // ADC r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u + 0x12345678u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A02311; // ADC r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + (0x0000000F << 4) + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0A02171; // ADC r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xC0000000u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2A024FF; // ADC r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xFF000000u + 1u);
}

// ===================== SBC Tests =====================
// SBC: Rd = Rn - Operand2 - (1 - C)
TEST_F(ARMDataProcessingTest, SBC_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C01002; // SBC r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 0x10u - 0x1u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C02001; // SBC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x12345678u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x0; // C flag clear
    uint32_t instr = 0xE0C02001; // SBC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u - 0xFFFFFFFFu - 1u);
}

TEST_F(ARMDataProcessingTest, SBCS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0D02001; // SBCS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set (no borrow)
    EXPECT_FALSE(cpu.CPSR() & (1u << 28)); // V clear (no overflow)
}

TEST_F(ARMDataProcessingTest, SBCS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x1;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0D02001; // SBCS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, SBC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2C0200F; // SBC r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u - 0xFu - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C02281; // SBC r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - 0x1E0u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C021A1; // SBC r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu - 0x1E000000u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C021C1; // SBC r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - ((int32_t)0x80000000 >> 3) - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C02161; // SBC r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u - ((0x0000000F >> 2) | (0x0000000F << 30)) - 0u);
}

TEST_F(ARMDataProcessingTest, SBCS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2D021A1; // SBCS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - (0x3u >> 3) - 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, SBC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0C02001; // SBC r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, SBCS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE2DFF001; // SBCS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, SBC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x1C002001; // SBCNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, SBC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C02001; // SBC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u - 0x7FFFFFFFu - 0u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C02000; // SBC r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0x12345678u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C02311; // SBC r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - (0x0000000F << 4) - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0C02171; // SBC r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xC0000000u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2C024FF; // SBC r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFF000000u - 0u);
}

// ===================== RSC Tests =====================
// RSC: Rd = Operand2 - Rn - (1 - C)
TEST_F(ARMDataProcessingTest, RSC_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E01002; // RSC r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 0x1u - 0x10u - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E02001; // RSC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x0; // C flag clear
    uint32_t instr = 0xE0E02001; // RSC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x0u - 1u);
}

TEST_F(ARMDataProcessingTest, RSCS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0F02001; // RSCS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u - 0x80000000u - 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set (no borrow)
    EXPECT_FALSE(cpu.CPSR() & (1u << 28)); // V clear (no overflow)
}

TEST_F(ARMDataProcessingTest, RSCS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x1;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0F02001; // RSCS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u - 0x1u - 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, RSC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2E0200F; // RSC r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFu - 0xF0F0F0F0u - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E02281; // RSC r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 5) - 0xFFFF00FFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E021A1; // RSC r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0xF0000000 >> 3) - 0x0F0F0F0Fu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E021C1; // RSC r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((int32_t)0x80000000 >> 3) - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E02161; // RSC r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((0x0000000F >> 2) | (0x0000000F << 30)) - 0xFF00FF00u - 0u);
}

TEST_F(ARMDataProcessingTest, RSCS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2F021A1; // RSCS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x3u >> 3) - 0xFFFFFFFFu - 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, RSC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE0E02001; // RSC r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u - 0xFFFFFFFFu - 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, RSCS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE2FFF001; // RSCS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, RSC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x1E002001; // RSCNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, RSC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E02001; // RSC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x7FFFFFFFu - 0x80000000u - 0u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E02000; // RSC r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0x12345678u - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E02311; // RSC r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 4) - 0xFFFF00FFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE0E02171; // RSC r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xC0000000u - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE2E024FF; // RSC r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF000000u - 0xFFFFFFFFu - 0u);
}

// ===================== TST Tests =====================
// TST: updates flags as if AND, result not written
TEST_F(ARMDataProcessingTest, TST_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100001; // TST r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set (result is 0)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100001; // TST r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100001; // TST r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE310000F; // TST r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100281; // TST r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE11001A1; // TST r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE11001C1; // TST r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set (result is negative)
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100161; // TST r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TSTS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE11001A1; // TST r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // C flag should be 0 (bit 2 of r1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, TST_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0x11000001; // TSTNE r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Should not execute, flags unchanged
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, TST_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100001; // TST r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100000; // TST r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1100311; // TST r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1100171; // TST r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
}

TEST_F(ARMDataProcessingTest, TST_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE31024FF; // TST r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

// ===================== TEQ Tests =====================
// TEQ: updates flags as if EOR, result not written
TEST_F(ARMDataProcessingTest, TEQ_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300001; // TEQ r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear (result is 0xFFFFFFFF)
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set (result is negative)
}

TEST_F(ARMDataProcessingTest, TEQ_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300001; // TEQ r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set (result is 0)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TEQ_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300001; // TEQ r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
}

TEST_F(ARMDataProcessingTest, TEQ_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE330000F; // TEQ r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300281; // TEQ r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE13001A1; // TEQ r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE13001C1; // TEQ r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear (result is positive)
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300161; // TEQ r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE13001A1; // TEQ r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // C flag should be 0 (bit 2 of r1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, TEQ_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0x13000001; // TEQNE r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Should not execute, flags unchanged
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, TEQ_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300001; // TEQ r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, TEQ_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300000; // TEQ r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1300311; // TEQ r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1300171; // TEQ r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
}

TEST_F(ARMDataProcessingTest, TEQ_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE33024FF; // TEQ r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

// ===================== CMP Tests =====================
// CMP: updates flags as if SUB, result not written
TEST_F(ARMDataProcessingTest, CMP_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500001; // CMP r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0x10 - 0x1 = 0xF, N clear, Z clear, C set (no borrow)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set
}

TEST_F(ARMDataProcessingTest, CMP_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500001; // CMP r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0xFFFFFFFF - 0xFFFFFFFF = 0, Z set, C set
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set
}

TEST_F(ARMDataProcessingTest, CMP_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500001; // CMP r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0 - 0xFFFFFFFF = 1, N clear, Z clear, C clear (borrow)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C clear
}

TEST_F(ARMDataProcessingTest, CMP_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE350000F; // CMP r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0xF0F0F0F0 - 0xF
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500281; // CMP r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE15001A1; // CMP r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE15001C1; // CMP r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500161; // CMP r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMPS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE15001A1; // CMP r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // C flag should be 0 (borrow)
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, CMP_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0x15000001; // CMPNE r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Should not execute, flags unchanged
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, CMP_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500001; // CMP r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, CMP_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500000; // CMP r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1500311; // CMP r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1500171; // CMP r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE35024FF; // CMP r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

// ===================== CMN Tests =====================
// CMN: updates flags as if ADD, result not written
TEST_F(ARMDataProcessingTest, CMN_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700001; // CMN r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0x10 + 0x1 = 0x11, N clear, Z clear, C clear (no carry)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700001; // CMN r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0xFFFFFFFF + 0xFFFFFFFF = 0xFFFFFFFE, C set (carry out)
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set
}

TEST_F(ARMDataProcessingTest, CMN_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700001; // CMN r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0 + 0 = 0, Z set
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, CMN_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE370000F; // CMN r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0xF0F0F0F0 + 0xF
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700281; // CMN r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE17001A1; // CMN r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE17001C1; // CMN r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700161; // CMN r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMNS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE17001A1; // CMN r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // C flag should be set if result < either operand
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, CMN_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0x17000001; // CMNNE r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Should not execute, flags unchanged
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, CMN_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700001; // CMN r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700000; // CMN r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1700311; // CMN r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1700171; // CMN r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE37024FF; // CMN r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

// ===================== ORR Tests =====================
// ORR: Rd = Rn | Operand2
TEST_F(ARMDataProcessingTest, ORR_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1801002; // ORR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1802001; // ORR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1802001; // ORR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORRS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1902001; // ORRS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, ORRS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1902001; // ORRS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, ORR_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE380200F; // ORR r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0FFu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1802281; // ORR r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF01FFu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE18021A1; // ORR r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1F0F0F0Fu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE18021C1; // ORR r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1802161; // ORR r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF0Fu);
}

TEST_F(ARMDataProcessingTest, ORRS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE19021A1; // ORRS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, ORR_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE1802001; // ORR r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, ORRS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE19FF001; // ORRS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, ORR_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x18002001; // ORRNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, ORR_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1802001; // ORR r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1802000; // ORR r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1802311; // ORR r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu | (0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1802171; // ORR r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu | 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, ORR_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE38024FF; // ORR r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

// ===================== MOV Tests =====================
// MOV: Rd = Operand2
TEST_F(ARMDataProcessingTest, MOV_Basic) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x12345678;
    uint32_t instr = 0xE1A02001; // MOV r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

TEST_F(ARMDataProcessingTest, MOV_AllBitsSet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xFFFFFFFF;
    uint32_t instr = 0xE1A02001; // MOV r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, MOV_Zero) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    uint32_t instr = 0xE1A02001; // MOV r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, MOVS_SetsFlags) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    uint32_t instr = 0xE1B02001; // MOVS r2, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, MOVS_ResultZeroSetsZ) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    uint32_t instr = 0xE1B02001; // MOVS r2, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, MOV_ImmediateOperand) {
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE3A020FF; // MOV r2, #0xFF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFu);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_LSL) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    uint32_t instr = 0xE1A02281; // MOV r2, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1E0u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_LSR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xF0000000;
    uint32_t instr = 0xE1A021A1; // MOV r2, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1E000000u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_ASR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    uint32_t instr = 0xE1A021C1; // MOV r2, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0000000u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_ROR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    uint32_t instr = 0xE1A02161; // MOV r2, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xC0000003u);
}

TEST_F(ARMDataProcessingTest, MOVS_CarryOutFromShifter) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x3;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1B021A1; // MOVS r2, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, MOV_FlagsUnchangedWhenS0) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE1A02001; // MOV r2, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, MOVS_Rd15_S1_UserMode) {
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE1BFF000; // MOVS pc, pc (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, MOV_ConditionCodeNotMet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x1;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x11A02001; // MOVNE r2, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, MOV_EdgeValues) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    uint32_t instr = 0xE1A02001; // MOV r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    cpu.R()[1] = 0xFFFFFFFF;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, MOV_Self) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[2] = 0x12345678;
    uint32_t instr = 0xE1A02002; // MOV r2, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedRegister_LSL_Reg) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    uint32_t instr = 0xE1A02311; // MOV r2, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_RRX) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000001;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1A02171; // MOV r2, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, MOV_ImmediateRotated) {
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE3A024FF; // MOV r2, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF000000u);
}

// ===================== BIC Tests =====================
// BIC: Rd = Rn & ~Operand2
TEST_F(ARMDataProcessingTest, BIC_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02001; // BIC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u & ~0x0F0F0F0Fu);
}

TEST_F(ARMDataProcessingTest, BIC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02001; // BIC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, BIC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02001; // BIC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, BICS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1D02001; // BICS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, BICS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1D02001; // BICS r2, r0, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, BIC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE3C0200F; // BIC r2, r0, #0xF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u & ~0xFu);
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02281; // BIC r2, r0, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu & ~(0x0000000F << 5));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C021A1; // BIC r2, r0, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu & ~(0xF0000000 >> 3));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C021C1; // BIC r2, r0, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~(0x80000000 >> 3 | 0xE0000000));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02161; // BIC r2, r0, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u & ~(0x0000000F >> 2 | 0xC0000000));
}

TEST_F(ARMDataProcessingTest, BICS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1D021A1; // BICS r2, r0, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~(0x3 >> 3));
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, BIC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE1C02001; // BIC r2, r0, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, BICS_Rd15_S1_UserMode) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE1DFF001; // BICS pc, pc, #1 (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, BIC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x11C02001; // BICNE r2, r0, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, BIC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02001; // BIC r2, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u & ~0x7FFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, BIC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02000; // BIC r2, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u & ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C02311; // BIC r2, r0, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu & ~(0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1C02171; // BIC r2, r0, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0xC0000000u);
}

TEST_F(ARMDataProcessingTest, BIC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE3C024FF; // BIC r2, r0, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0xFF000000u);
}

// ===================== MVN Tests =====================
// MVN: Rd = ~Operand2
TEST_F(ARMDataProcessingTest, MVN_Basic) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x12345678;
    uint32_t instr = 0xE1E02001; // MVN r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, MVN_AllBitsSet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xFFFFFFFF;
    uint32_t instr = 0xE1E02001; // MVN r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
}

TEST_F(ARMDataProcessingTest, MVN_Zero) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    uint32_t instr = 0xE1E02001; // MVN r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, MVNS_SetsFlags) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x7FFFFFFF;
    uint32_t instr = 0xE1F02001; // MVNS r2, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x7FFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, MVNS_ResultZeroSetsZ) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xFFFFFFFF;
    uint32_t instr = 0xE1F02001; // MVNS r2, r1 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, MVN_ImmediateOperand) {
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE3E020FF; // MVN r2, #0xFF
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0xFFu);
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_LSL) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    uint32_t instr = 0xE1E02281; // MVN r2, r1, LSL #5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)~(0x0000000F << 5));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_LSR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xF0000000;
    uint32_t instr = 0xE1E021A1; // MVN r2, r1, LSR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~(0xF0000000 >> 3));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_ASR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    uint32_t instr = 0xE1E021C1; // MVN r2, r1, ASR #3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~(0x80000000 >> 3 | 0xE0000000));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_ROR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    uint32_t instr = 0xE1E02161; // MVN r2, r1, ROR #2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~(0x0000000F >> 2 | 0xC0000000));
}

TEST_F(ARMDataProcessingTest, MVNS_CarryOutFromShifter) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x3;
    cpu.CPSR() = 0;
    uint32_t instr = 0xE1F021A1; // MVNS r2, r1, LSR #3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)~(0x3 >> 3));
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, MVN_FlagsUnchangedWhenS0) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    cpu.CPSR() = 0xA0000000; // N and C set
    uint32_t instr = 0xE1E02001; // MVN r2, r1 (S=0)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, MVNS_Rd15_S1_UserMode) {
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    uint32_t instr = 0xE1FFF000; // MVNS pc, pc (Rd=15, S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, MVN_ConditionCodeNotMet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x1;
    cpu.CPSR() = 0x40000000; // Z flag set
    uint32_t instr = 0x11E02001; // MVNNE r2, r1 (cond=0001, NE)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, MVN_EdgeValues) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    uint32_t instr = 0xE1E02001; // MVN r2, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x80000000u);
    cpu.R()[1] = 0xFFFFFFFF;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
}

TEST_F(ARMDataProcessingTest, MVN_Self) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[2] = 0x12345678;
    uint32_t instr = 0xE1E02002; // MVN r2, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedRegister_LSL_Reg) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    uint32_t instr = 0xE1E02311; // MVN r2, r1, LSL r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)~(0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_RRX) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000001;
    cpu.CPSR() = 0x20000000; // C flag set
    uint32_t instr = 0xE1E02171; // MVN r2, r1, ROR #0 (==RRX)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], ~0xC0000000u);
}

TEST_F(ARMDataProcessingTest, MVN_ImmediateRotated) {
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE3E024FF; // MVN r2, #0xFF000000
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0xFF000000u);
}

