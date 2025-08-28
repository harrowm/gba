// test_arm_data_processing.cpp
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

extern "C" {
#include <keystone/keystone.h>
}

class ARMDataProcessingTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;
    ks_engine* ks; // Keystone handle

    ARMDataProcessingTest() : memory(true), cpu(memory, interrupts), arm_cpu(cpu) {}

    void SetUp() override {
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        cpu.CPSR() = 0x10; // User mode, no flags set
        
        if (ks) ks_close(ks);
        if (ks_open(KS_ARCH_ARM, KS_MODE_ARM, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for ARM mode";
        }
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }

    // Helper: assemble ARM instruction and write to memory
    bool assemble_and_write(const std::string& asm_code, uint32_t addr, std::vector<uint8_t>* out_bytes = nullptr) {
        unsigned char* encode = nullptr;
        size_t size, count;
        int err = ks_asm(ks, asm_code.c_str(), addr, &encode, &size, &count);
        if ((ks_err)err != KS_ERR_OK) {
            fprintf(stderr, "Keystone error: %s\n", ks_strerror((ks_err)err));
            return false;
        }
        for (size_t i = 0; i < size; ++i)
            memory.write8(addr + i, encode[i]);
        if (out_bytes) out_bytes->assign(encode, encode + size);
        ks_free(encode);
        return true;
    }
};

// AND: Rd = Rn & Operand2
TEST_F(ARMDataProcessingTest, AND_Basic) {
    cpu.R()[0] = 0xF0F0F0F0; // Rn
    cpu.R()[1] = 0x0F0F0F0F; // Rm
    cpu.R()[15] = 0x00000000;
    // AND r2, r0, r1 (Rd=2, Rn=0, Operand2=R1)
    assemble_and_write("and r2, r0, r1", cpu.R()[15]); // AND r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// AND with all bits set
TEST_F(ARMDataProcessingTest, AND_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF; // Rn
    cpu.R()[1] = 0x12345678; // Rm
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1", cpu.R()[15]); // AND r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

// AND with zero
TEST_F(ARMDataProcessingTest, AND_Zero) {
    cpu.R()[0] = 0x0; // Rn
    cpu.R()[1] = 0xFFFFFFFF; // Rm
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1", cpu.R()[15]); // AND r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

// AND with S bit set (flags)
TEST_F(ARMDataProcessingTest, ANDS_SetsFlags) {
    cpu.R()[0] = 0x80000000; // Rn
    cpu.R()[1] = 0xFFFFFFFF; // Rm
    cpu.R()[15] = 0x00000000;
    assemble_and_write("ands r2, r0, r1", cpu.R()[15]); // ANDS r2, r0, r1 (S=1)
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
    assemble_and_write("ands r2, r0, r1", cpu.R()[15]); // ANDS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

// AND with immediate operand
TEST_F(ARMDataProcessingTest, AND_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, #0xF", cpu.R()[15]); // AND r2, r0, #0xF
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
}

// AND with shifted operand (LSL #4)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1, lsl #5", cpu.R()[15]); // AND r2, r0, r1, LSL #5 (shift=5)
    // 0x0000000F << 5 = 0x000001E0, 0xFFFF00FF & 0x1E0 = 0x000000E0
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x000000E0u);
}

// AND with shifted operand (LSR #4)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1, lsr #3", cpu.R()[15]); // AND r2, r0, r1, LSR #3 (shift=3)
    // 0xF0000000 >> 3 = 0x1E000000, 0x0F0F0F0F & 0x1E000000 = 0x0E000000
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0E000000u);
}

// AND with shifted operand (ASR #8)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1, asr #3", cpu.R()[15]); // AND r2, r0, r1, ASR #3 (shift=3)
    // 0x80000000 >> 3 (arithmetic) = 0xF0000000, 0xFFFFFFFF & 0xF0000000 = 0xF0000000
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0000000u);
}

// AND with shifted operand (ROR #4)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1, ror #2", cpu.R()[15]); // AND r2, r0, r1, ROR #2 (shift=2)
    // 0x0000000F ror 2 = 0xC0000003, 0xFF00FF00 & 0xC0000003 = 0xC0000000
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);
}

// ANDS with carry out from shifter (LSR #1, S=1)
TEST_F(ARMDataProcessingTest, ANDS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    assemble_and_write("ands r2, r0, r1, lsr #3", cpu.R()[15]); // ANDS r2, r0, r1, LSR #3 (shift=3, S=1)
    // 0x3 >> 3 = 0x0, carry out is bit 2 (should be 0)
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
    assemble_and_write("and r2, r0, r1", cpu.R()[15]); // AND r2, r0, r1 (S=0)
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
    assemble_and_write("ands pc, pc, #1", cpu.R()[15]); // ANDS pc, pc, #1 (Rd=15, S=1)
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
    assemble_and_write("andne r2, r0, r1", cpu.R()[15]); // ANDNE r2, r0, r1 (cond=0001, NE)
    arm_cpu.execute(1);
    // Should not execute, r2 unchanged
    EXPECT_EQ(cpu.R()[2], 0u);
}

// AND with edge values
TEST_F(ARMDataProcessingTest, AND_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1", cpu.R()[15]); // AND r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1", cpu.R()[15]); // AND r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

// AND a register with itself
TEST_F(ARMDataProcessingTest, AND_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r0", cpu.R()[15]); // AND r2, r0, r0
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

// AND with register-shifted register (LSL by register)
TEST_F(ARMDataProcessingTest, AND_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4; // shift amount in r3
    cpu.R()[15] = 0x00000000;
    assemble_and_write("and r2, r0, r1, lsl r3", cpu.R()[15]); // AND r2, r0, r1, LSL r3
    // 0x0000000F << 4 = 0x000000F0, 0xFFFF00FF & 0xF0 = 0x000000F0
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x000000F0u);
}

// AND with RRX (rotate right with extend)
TEST_F(ARMDataProcessingTest, AND_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("and r2, r0, r1, rrx", cpu.R()[15]); // AND r2, r0, r1, RRX
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);
}

// AND with rotated immediate (e.g., #0xFF000000)
TEST_F(ARMDataProcessingTest, AND_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    // #0xFF000000 is encoded as 0xFF rotated right by 8 (imm=0xFF, rot=4)
    assemble_and_write("and r2, r0, #0xFF000000", cpu.R()[15]); // AND r2, r0, #0xFF000000
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF000000u);
}

// ===================== EOR Tests =====================
// EOR: Rd = Rn ^ Operand2
TEST_F(ARMDataProcessingTest, EOR_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1", cpu.R()[15]); // EOR r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, EOR_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1", cpu.R()[15]); // EOR r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xEDCBA987u);
}

TEST_F(ARMDataProcessingTest, EOR_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1", cpu.R()[15]); // EOR r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, EORS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eors r2, r0, r1", cpu.R()[15]); // EORS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x7FFFFFFFu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, EORS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eors r2, r0, r1", cpu.R()[15]); // EORS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

TEST_F(ARMDataProcessingTest, EOR_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, #0xF", cpu.R()[15]); // EOR r2, r0, #0xF
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0FFu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1, lsl #5", cpu.R()[15]); // EOR r2, r0, r1, LSL #5
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xffff011fu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1, lsr #3", cpu.R()[15]); // EOR r2, r0, r1, LSR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x110F0F0Fu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1, asr #3", cpu.R()[15]); // EOR r2, r0, r1, ASR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0FFFFFFFu);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1, ror #2", cpu.R()[15]); // EOR r2, r0, r1, ROR #2
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x3F00FF03u);
}

TEST_F(ARMDataProcessingTest, EORS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    assemble_and_write("eors r2, r0, r1, lsr #3", cpu.R()[15]); // EORS r2, r0, r1, LSR #3 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, EOR_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("eor r2, r0, r1", cpu.R()[15]); // EOR r2, r0, r1 (S=0)
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
    assemble_and_write("eors r15, r15, #1", cpu.R()[15]); // EORS pc, pc, #1 (Rd=15, S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, EOR_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("eorne r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, EOR_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, EOR_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu ^ 0x000000F0u);
}

TEST_F(ARMDataProcessingTest, EOR_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("eor r2, r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu ^ 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, EOR_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("eor r2, r0, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00FFFFFFu);
}

// ===================== SUB Tests =====================
// SUB: Rd = Rn - Operand2
TEST_F(ARMDataProcessingTest, SUB_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1", cpu.R()[15]); // SUB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFu);
}

TEST_F(ARMDataProcessingTest, SUB_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1", cpu.R()[15]); // SUB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xEDCBA987u);
}

TEST_F(ARMDataProcessingTest, SUB_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1", cpu.R()[15]); // SUB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
}

TEST_F(ARMDataProcessingTest, SUBS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("subs r2, r0, r1", cpu.R()[15]); // SUBS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000001u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, SUBS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x1;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("subs r2, r0, r1", cpu.R()[15]); // SUBS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, SUB_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, #0xF", cpu.R()[15]); // SUB r2, r0, #0xF
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0E1u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1, lsl #5", cpu.R()[15]); // SUB r2, r0, r1, LSL #5
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - 0x1E0u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1, lsr #3", cpu.R()[15]); // SUB r2, r0, r1, LSR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu - 0x1E000000u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1, asr #3", cpu.R()[15]); // SUB r2, r0, r1, ASR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xF0000000u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1, ror #2", cpu.R()[15]); // SUB r2, r0, r1, ROR #2
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u - 0xC0000003u);
}

TEST_F(ARMDataProcessingTest, SUBS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    assemble_and_write("subs r2, r0, r1, lsr #3", cpu.R()[15]); // SUBS r2, r0, r1, LSR #3 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C flag should be set
}

TEST_F(ARMDataProcessingTest, SUBS_CarryOutFromShifter_CarrySet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x4; // binary 0100
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    assemble_and_write("subs r2, r0, r1, lsr #3", cpu.R()[15]); // SUBS r2, r0, r1, LSR #3 (S=1)
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
    assemble_and_write("sub r2, r0, r1", cpu.R()[15]); // SUB r2, r0, r1 (S=0)
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
    assemble_and_write("subs r15, r15, #1", cpu.R()[15]); // SUBS pc, pc, #1 (Rd=15, S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, SUB_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("subne r2, r0, r1", cpu.R()[15]); // SUBNE r2, r0, r1 (cond=0001, NE)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, SUB_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1", cpu.R()[15]); // SUB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, SUB_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r0", cpu.R()[15]); // SUB r2, r0, r0
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, r1, lsl r3", cpu.R()[15]); // SUB r2, r0, r1, LSL r3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - 0x000000F0u);
}

TEST_F(ARMDataProcessingTest, SUB_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sub r2, r0, r1, rrx", cpu.R()[15]); // SUB r2, r0, r1, ROR #0 (==RRX)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, SUB_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sub r2, r0, #0xFF000000", cpu.R()[15]); // SUB r2, r0, #0xFF000000
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFF000000u);
}

// ===================== RSB Tests =====================
// RSB: Rd = Operand2 - Rn
TEST_F(ARMDataProcessingTest, RSB_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1", cpu.R()[15]); // RSB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u - 0x10u);
}

TEST_F(ARMDataProcessingTest, RSB_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1", cpu.R()[15]); // RSB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, RSB_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1", cpu.R()[15]); // RSB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x0u);
}

TEST_F(ARMDataProcessingTest, RSBS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsbs r2, r0, r1", cpu.R()[15]); // RSBS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x80000000u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, RSBS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x1;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsbs r2, r0, r1", cpu.R()[15]); // RSBS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, RSB_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, #0xF", cpu.R()[15]); // RSB r2, r0, #0xF
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFu - 0xF0F0F0F0u);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1, lsl #5", cpu.R()[15]); // RSB r2, r0, r1, LSL #5
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 5) - 0xFFFF00FFu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1, lsr #3", cpu.R()[15]); // RSB r2, r0, r1, LSR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0xF0000000 >> 3) - 0x0F0F0F0Fu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1, asr #3", cpu.R()[15]); // RSB r2, r0, r1, ASR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((int32_t)0x80000000 >> 3) - 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1, ror #2", cpu.R()[15]); // RSB r2, r0, r1, ROR #2
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((0x0000000F >> 2) | (0x0000000F << 30)) - 0xFF00FF00u);
}

TEST_F(ARMDataProcessingTest, RSBS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    assemble_and_write("rsbs r2, r0, r1, lsr #3", cpu.R()[15]); // RSBS r2, r0, r1, LSR #3 (S=1)
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
    assemble_and_write("rsb r2, r0, r1", cpu.R()[15]); // RSB r2, r0, r1 (S=0)
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
    assemble_and_write("rsbs r15, r15, #1", cpu.R()[15]); // RSBS pc, pc, #1 (Rd=15, S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, RSB_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("rsbne r2, r0, r1", cpu.R()[15]); // RSBNE r2, r0, r1 (cond=0001, NE)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, RSB_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1", cpu.R()[15]); // RSB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x7FFFFFFFu - 0x80000000u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1", cpu.R()[15]); // RSB r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, RSB_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r0", cpu.R()[15]); // RSB r2, r0, r0
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0x12345678u);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, r1, lsl r3", cpu.R()[15]); // RSB r2, r0, r1, LSL r3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 4) - 0xFFFF00FFu);
}

TEST_F(ARMDataProcessingTest, RSB_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsb r2, r0, r1, rrx", cpu.R()[15]); // RSB r2, r0, r1, ROR #0 (==RRX)
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xC0000000u - 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, RSB_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsb r2, r0, #0xFF000000", cpu.R()[15]); // RSB r2, r0, #0xFF000000
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF000000u - 0xFFFFFFFFu);
}

// ===================== ADD Tests =====================
// ADD: Rd = Rn + Operand2
TEST_F(ARMDataProcessingTest, ADD_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1", cpu.R()[15]); // ADD r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x11u);
}

TEST_F(ARMDataProcessingTest, ADD_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1", cpu.R()[15]); // ADD r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345677u);
}

TEST_F(ARMDataProcessingTest, ADD_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1", cpu.R()[15]); // ADD r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ADDS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("adds r2, r0, r1", cpu.R()[15]); // ADDS r2, r0, r1 (S=1)
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
    assemble_and_write("adds r2, r0, r1", cpu.R()[15]); // ADDS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, ADD_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, #0xF", cpu.R()[15]); // ADD r2, r0, #0xF
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0FFu);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1, lsl #5", cpu.R()[15]); // ADD r2, r0, r1, LSL #5
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + 0x1E0u);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1, lsr #3", cpu.R()[15]); // ADD r2, r0, r1, LSR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu + 0x1E000000u);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1, asr #3", cpu.R()[15]); // ADD r2, r0, r1, ASR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + ((int32_t)0x80000000 >> 3));
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1, ror #2", cpu.R()[15]); // ADD r2, r0, r1, ROR #2
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u + ((0x0000000F >> 2) | (0x0000000F << 30)));
}

TEST_F(ARMDataProcessingTest, ADDS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // clear all flags
    assemble_and_write("adds r2, r0, r1, lsr #3", cpu.R()[15]); // ADDS r2, r0, r1, LSR #3 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + (0x3u >> 3));
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, ADD_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("add r2, r0, r1", cpu.R()[15]); // ADD r2, r0, r1 (S=0)
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
    assemble_and_write("adds r15, r15, #1", cpu.R()[15]); // ADDS pc, pc, #1 (Rd=15, S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, ADD_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("addne r2, r0, r1", cpu.R()[15]); // ADDNE r2, r0, r1 (cond=0001, NE)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, ADD_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1", cpu.R()[15]); // ADD r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1", cpu.R()[15]); // ADD r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFEu);
}

TEST_F(ARMDataProcessingTest, ADD_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r0", cpu.R()[15]); // ADD r2, r0, r0
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u + 0x12345678u);
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, r1, lsl r3", cpu.R()[15]); // ADD r2, r0, r1, LSL r3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + (0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, ADD_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("add r2, r0, r1, rrx", cpu.R()[15]); // ADD r2, r0, r1, ROR #0 (==RRX)
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, ADD_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("add r2, r0, #0xFF000000", cpu.R()[15]); // ADD r2, r0, #0xFF000000
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
    assemble_and_write("adc r1, r0, r2", cpu.R()[15]); // ADC r1, r0, r2
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 0x11u);
}

TEST_F(ARMDataProcessingTest, ADC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1", cpu.R()[15]); // ADC r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0x12345678u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x0; // C flag clear
    assemble_and_write("adc r2, r0, r1", cpu.R()[15]); // ADC r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ADCS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adcs r2, r0, r1", cpu.R()[15]); // ADCS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z false
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C set (carry out)
    EXPECT_TRUE(cpu.CPSR() & (1u << 28)); // V set (overflow)
}

TEST_F(ARMDataProcessingTest, ADCS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adcs r2, r0, r1", cpu.R()[15]); // ADCS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, ADC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, #0xF", cpu.R()[15]); // ADC r2, r0, #0xF
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u + 0xFu + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1, lsl #5", cpu.R()[15]); // ADC r2, r0, r1, LSL #5
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + 0x1E0u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1, lsr #3", cpu.R()[15]); // ADC r2, r0, r1, LSR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu + 0x1E000000u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1, asr #3", cpu.R()[15]); // ADC r2, r0, r1, ASR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + ((int32_t)0x80000000 >> 3) + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1, ror #2", cpu.R()[15]); // ADC r2, r0, r1, ROR #2
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u + ((0x0000000F >> 2) | (0x0000000F << 30)) + 1u);
}

TEST_F(ARMDataProcessingTest, ADCS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adcs r2, r0, r1, lsr #3", cpu.R()[15]); // ADCS r2, r0, r1, LSR #3 (S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + (0x3u >> 3) + 1u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, ADC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("adc r2, r0, r1", cpu.R()[15]); // ADC r2, r0, r1 (S=0)
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
    assemble_and_write("adcs r15, r15, #1", cpu.R()[15]); // ADCS pc, pc, #1 (Rd=15, S=1)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, ADC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("adcne r2, r0, r1", cpu.R()[15]); // ADCNE r2, r0, r1 (cond=0001, NE)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, ADC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1", cpu.R()[15]); // ADC r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u + 0x7FFFFFFFu + 1u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("adc r2, r0, r1", cpu.R()[15]); // ADC r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xFFFFFFFFu + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r0", cpu.R()[15]); // ADC r2, r0, r0
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u + 0x12345678u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1, lsl r3", cpu.R()[15]); // ADC r2, r0, r1, LSL r3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu + (0x0000000F << 4) + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, r1, rrx", cpu.R()[15]); // ADC r2, r0, r1, ROR #0 (==RRX)
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xC0000000u + 1u);
}

TEST_F(ARMDataProcessingTest, ADC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("adc r2, r0, #0xFF000000", cpu.R()[15]); // ADC r2, r0, #0xFF000000
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu + 0xFF000000u + 1u);
}

// ===================== SBC Tests =====================
// SBC: Rd = Rn - Operand2 - (1 - C)
TEST_F(ARMDataProcessingTest, SBC_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[2] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r1, r0, r2", cpu.R()[15]); // SBC r1, r0, r2
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 0xFu); // 0x10 - 0x1 - (1 - 1) = 0xF;
}

TEST_F(ARMDataProcessingTest, SBC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1", cpu.R()[15]); // SBC r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x12345678u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x0; // C flag clear
    assemble_and_write("sbc r2, r0, r1", cpu.R()[15]); // SBC r2, r0, r1
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u - 0xFFFFFFFFu - 1u);
}

TEST_F(ARMDataProcessingTest, SBCS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbcs r2, r0, r1", cpu.R()[15]); // SBCS r2, r0, r1 (S=1)
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
    assemble_and_write("sbcs r2, r0, r1", cpu.R()[15]); // SBCS r2, r0, r1 (S=1)
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, SBC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, #0xF", cpu.R()[15]); // SBC r2, r0, #0xF
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u - 0xFu - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1, lsl #5", cpu.R()[15]); // SBC r2, r0, r1, LSL #5
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - 0x1E0u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1, lsr #3", cpu.R()[15]); // SBC r2, r0, r1, LSR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu - 0x1E000000u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1, asr #3", cpu.R()[15]); // SBC r2, r0, r1, ASR #3
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - ((int32_t)0x80000000 >> 3) - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u - ((0x0000000F >> 2) | (0x0000000F << 30)) - 0u);
}

TEST_F(ARMDataProcessingTest, SBCS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbcs r2, r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - (0x3u >> 3) - 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, SBC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("sbc r2, r0, r1", cpu.R()[15]);
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
    assemble_and_write("sbcs pc, pc, #1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, SBC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("sbcne r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, SBC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u - 0x7FFFFFFFu - 0u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("sbc r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0x12345678u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu - (0x0000000F << 4) - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xC0000000u - 0u);
}

TEST_F(ARMDataProcessingTest, SBC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("sbc r2, r0, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFF000000u - 0u);
}

// ===================== RSC Tests =====================
// RSC: Rd = Operand2 - Rn - (1 - C)
TEST_F(ARMDataProcessingTest, RSC_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[2] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r1, r0, r2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 0x1u - 0x10u - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x0; // C flag clear
    assemble_and_write("rsc r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0x0u - 1u);
}

TEST_F(ARMDataProcessingTest, RSCS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rscs r2, r0, r1", cpu.R()[15]);
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
    assemble_and_write("rscs r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1u - 0x1u - 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, RSC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, #0xF", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFu - 0xF0F0F0F0u - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 5) - 0xFFFF00FFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0xF0000000 >> 3) - 0x0F0F0F0Fu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((int32_t)0x80000000 >> 3) - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ((0x0000000F >> 2) | (0x0000000F << 30)) - 0xFF00FF00u - 0u);
}

TEST_F(ARMDataProcessingTest, RSCS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rscs r2, r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x3u >> 3) - 0xFFFFFFFFu - 0u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, RSCS_CarryNotFromShifter) {
    cpu.R()[0] = 0x0;           // operand1
    cpu.R()[2] = 0x80000000;    // operand2
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000;    // C flag set (carry_in = 1)
    assemble_and_write("rscs r2, r1, r2, lsr #2", cpu.R()[15]);
    arm_cpu.execute(1);
    // The shifter's carry-out will be 0 (since bit 1 of 0x80000000 is 0), but the C flag after RSCS is set if no borrow occurs.
    // Here, shifted.value = 0x80000000 >> 2 = 0x20000000, carry_out = 0
    // result = 0x20000000 - 0x0 - 0 = 0x20000000 (no borrow, so C should be set)
    EXPECT_EQ(cpu.R()[2], 0x20000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C flag set (no borrow), not from shifter
}

TEST_F(ARMDataProcessingTest, RSC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("rsc r2, r0, r1", cpu.R()[15]);
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
    assemble_and_write("rscs pc, pc, #1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, RSC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("rscne r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, RSC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x7FFFFFFFu - 0x80000000u - 0u);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("rsc r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u - 0x12345678u - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (0x0000000F << 4) - 0xFFFF00FFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xC0000000u - 0xFFFFFFFFu - 0u);
}

TEST_F(ARMDataProcessingTest, RSC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("rsc r2, r0, #0xFF000000", cpu.R()[15]);
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
    assemble_and_write("tst r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set (result is 0)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, #0xF", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set (result is negative)
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TSTS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    // C flag should be 0 (bit 2 of r1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, TST_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xE0000000; // N, Z, and C set - makes NE condition false
    assemble_and_write("tstne r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    // Should not execute, flags unchanged
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N should remain set
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z should remain set  
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C should remain set
}

TEST_F(ARMDataProcessingTest, TST_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("tst r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TST_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("tst r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
}

TEST_F(ARMDataProcessingTest, TST_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("tst r0, #0xFF000000", cpu.R()[15]);
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
    assemble_and_write("teq r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear (result is 0xFFFFFFFF)
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set (result is negative)
}

TEST_F(ARMDataProcessingTest, TEQ_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set (result is 0)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, TEQ_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
}

TEST_F(ARMDataProcessingTest, TEQ_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, #0xF", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear (result is positive)
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    // C flag should be 0 (bit 2 of r1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, TEQ_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("teqne r0, r1", cpu.R()[15]);
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
    assemble_and_write("teq r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("teq r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, TEQ_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, TEQ_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("teq r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

// Extra RRX test to ensure RRX works correctly
TEST_F(ARMDataProcessingTest, MOVS_RRX) {
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("movs r2, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);        // RRX result
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));      // C flag should be bit 0 of original value (here, 1)
}

TEST_F(ARMDataProcessingTest, TEQ_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("teq r0, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    // TEQ performs XOR: 0xFFFFFFFF XOR 0xFF000000 = 0x00FFFFFF
    // Result has MSB = 0, so N should be 0 (false)
    // Result is non-zero, so Z should be 0 (false)
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear (result non-zero)
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear (MSB = 0)
}

// ===================== CMP Tests =====================
// CMP: updates flags as if SUB, result not written
TEST_F(ARMDataProcessingTest, CMP_Basic) {
    cpu.R()[0] = 0x10;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r1", cpu.R()[15]);
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
    assemble_and_write("cmp r0, r1", cpu.R()[15]);
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
    assemble_and_write("cmp r0, r1", cpu.R()[15]);
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
    assemble_and_write("cmp r0, #0xF", cpu.R()[15]);
    arm_cpu.execute(1);
    // 0xF0F0F0F0 - 0xF
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMPS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    // C flag should be 1 (no borrow)
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, CMP_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("cmpne r0, r1", cpu.R()[15]);
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
    assemble_and_write("cmp r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("cmp r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, CMP_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("cmp r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMP_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmp r0, #0xFF000000", cpu.R()[15]);
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
    assemble_and_write("cmn r0, r1", cpu.R()[15]);
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
    assemble_and_write("cmn r0, r1", cpu.R()[15]);
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
    assemble_and_write("cmn r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    // 0 + 0 = 0, Z set
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
}

TEST_F(ARMDataProcessingTest, CMN_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, #0xF", cpu.R()[15]);
    arm_cpu.execute(1);
    // 0xF0F0F0F0 + 0xF
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMNS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    // C flag should be cleared 
    EXPECT_FALSE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, CMN_FlagsUnchangedWhenConditionNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xE0000000; // N, Z, and C set - makes NE condition false
    assemble_and_write("cmnne r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    // Should not execute, flags unchanged
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N should remain set
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z should remain set
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C should remain set
}

TEST_F(ARMDataProcessingTest, CMN_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("cmn r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("cmn r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, CMN_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("cmn r0, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

// ===================== ORR Tests =====================
// ORR: Rd = Rn | Operand2
TEST_F(ARMDataProcessingTest, ORR_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORRS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orrs r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, ORRS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orrs r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, ORR_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, #0xF", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0FFu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF01FFu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1F0F0F0Fu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF03u);
}

TEST_F(ARMDataProcessingTest, ORRS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("orrs r2, r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, ORR_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("orr r2, r0, r1", cpu.R()[15]);
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
    assemble_and_write("orrs pc, pc, #1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, ORR_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("orrne r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, ORR_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, ORR_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu | (0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, ORR_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("orr r2, r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu | 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, ORR_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("orr r2, r0, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

// ===================== MOV Tests =====================
// MOV: Rd = Operand2
TEST_F(ARMDataProcessingTest, MOV_Basic) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x12345678;
    assemble_and_write("mov r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

TEST_F(ARMDataProcessingTest, MOV_AllBitsSet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xFFFFFFFF;
    assemble_and_write("mov r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, MOV_Zero) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    assemble_and_write("mov r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, MOVS_SetsFlags) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    assemble_and_write("movs r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, MOVS_ResultZeroSetsZ) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    assemble_and_write("movs r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, MOV_ImmediateOperand) {
    cpu.R()[15] = 0x00000000;
    assemble_and_write("mov r2, #0xFF", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFu);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_LSL) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    assemble_and_write("mov r2, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1E0u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_LSR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xF0000000;
    assemble_and_write("mov r2, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x1E000000u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_ASR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    assemble_and_write("mov r2, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0000000u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_ROR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    assemble_and_write("mov r2, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xC0000003u);
}

TEST_F(ARMDataProcessingTest, MOVS_CarryOutFromShifter) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x3;
    cpu.CPSR() = 0;
    assemble_and_write("movs r2, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, MOV_FlagsUnchangedWhenS0) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("mov r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, MOVS_Rd15_S1_UserMode) {
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    assemble_and_write("movs pc, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, MOV_ConditionCodeNotMet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x1;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("movne r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, MOV_EdgeValues) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    assemble_and_write("mov r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    cpu.R()[1] = 0xFFFFFFFF;
    assemble_and_write("mov r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, MOV_Self) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[2] = 0x12345678;
    assemble_and_write("mov r2, r2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedRegister_LSL_Reg) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    assemble_and_write("mov r2, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0u);
}

TEST_F(ARMDataProcessingTest, MOV_ShiftedOperand_RRX) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000001;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("mov r2, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);
}

TEST_F(ARMDataProcessingTest, MOV_ImmediateRotated) {
    cpu.R()[15] = 0x00000000;
    assemble_and_write("mov r2, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF000000u);
}

// ===================== BIC Tests =====================
// BIC: Rd = Rn & ~Operand2
TEST_F(ARMDataProcessingTest, BIC_Basic) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[1] = 0x0F0F0F0F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u & ~0x0F0F0F0Fu);
}

TEST_F(ARMDataProcessingTest, BIC_AllBitsSet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, BIC_Zero) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
}

TEST_F(ARMDataProcessingTest, BICS_SetsFlags) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bics r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, BICS_ResultZeroSetsZ) {
    cpu.R()[0] = 0x0;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bics r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, BIC_ImmediateOperand) {
    cpu.R()[0] = 0xF0F0F0F0;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, #0xF", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xF0F0F0F0u & ~0xFu);
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_LSL) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu & ~(0x0000000F << 5));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_LSR) {
    cpu.R()[0] = 0x0F0F0F0F;
    cpu.R()[1] = 0xF0000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0F0F0F0Fu & ~(0xF0000000 >> 3));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_ASR) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000000;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~(0x80000000 >> 3 | 0xE0000000));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_ROR) {
    cpu.R()[0] = 0xFF00FF00;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFF00FF00u & ~(0x0000000F >> 2 | 0xC0000000));
}

TEST_F(ARMDataProcessingTest, BICS_CarryOutFromShifter) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x3;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0;
    assemble_and_write("bics r2, r0, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~(0x3 >> 3));
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, BIC_FlagsUnchangedWhenS0) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x0;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("bic r2, r0, r1", cpu.R()[15]);
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
    assemble_and_write("bics pc, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, BIC_ConditionCodeNotMet) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x1;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("bicne r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, BIC_EdgeValues) {
    cpu.R()[0] = 0x80000000;
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x80000000u & ~0x7FFFFFFFu);
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, BIC_Self) {
    cpu.R()[0] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x12345678u & ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedRegister_LSL_Reg) {
    cpu.R()[0] = 0xFFFF00FF;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFF00FFu & ~(0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, BIC_ShiftedOperand_RRX) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x80000001;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("bic r2, r0, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0xC0000000u);
}

TEST_F(ARMDataProcessingTest, BIC_ImmediateRotated) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[15] = 0x00000000;
    assemble_and_write("bic r2, r0, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu & ~0xFF000000u);
}

// ===================== MVN Tests =====================
// MVN: Rd = ~Operand2
TEST_F(ARMDataProcessingTest, MVN_Basic) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x12345678;
    assemble_and_write("mvn r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, MVN_AllBitsSet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xFFFFFFFF;
    assemble_and_write("mvn r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
}

TEST_F(ARMDataProcessingTest, MVN_Zero) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    assemble_and_write("mvn r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
}

TEST_F(ARMDataProcessingTest, MVNS_SetsFlags) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x7FFFFFFF;
    assemble_and_write("mvns r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x7FFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
}

TEST_F(ARMDataProcessingTest, MVNS_ResultZeroSetsZ) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xFFFFFFFF;
    assemble_and_write("mvns r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N clear
}

TEST_F(ARMDataProcessingTest, MVN_ImmediateOperand) {
    cpu.R()[15] = 0x00000000;
    assemble_and_write("mvn r2, #0xFF", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0xFFu);
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_LSL) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    assemble_and_write("mvn r2, r1, lsl #5", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)~(0x0000000F << 5));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_LSR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0xF0000000;
    assemble_and_write("mvn r2, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~(0xF0000000 >> 3));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_ASR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    assemble_and_write("mvn r2, r1, asr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~(0x80000000 >> 3 | 0xE0000000));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_ROR) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    assemble_and_write("mvn r2, r1, ror #2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~(0x0000000F >> 2 | 0xC0000000));
}

TEST_F(ARMDataProcessingTest, MVNS_CarryOutFromShifter) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x3;
    cpu.CPSR() = 0;
    assemble_and_write("mvns r2, r1, lsr #3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)~(0x3 >> 3));
    EXPECT_FALSE(cpu.CPSR() & (1u << 29)); // C flag should be 0
}

TEST_F(ARMDataProcessingTest, MVN_FlagsUnchangedWhenS0) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0;
    cpu.CPSR() = 0xA0000000; // N and C set
    assemble_and_write("mvn r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31));
    EXPECT_TRUE(cpu.CPSR() & (1u << 29));
}

TEST_F(ARMDataProcessingTest, MVNS_Rd15_S1_UserMode) {
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0x10; // User mode
    assemble_and_write("mvns pc, r0", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR(), 0x10u);
}

TEST_F(ARMDataProcessingTest, MVN_ConditionCodeNotMet) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x1;
    cpu.CPSR() = 0x40000000; // Z flag set
    assemble_and_write("mvnne r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0u);
}

TEST_F(ARMDataProcessingTest, MVN_EdgeValues) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000000;
    assemble_and_write("mvn r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x80000000u);
    cpu.R()[1] = 0xFFFFFFFF;
    assemble_and_write("mvn r2, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000000u);
}

TEST_F(ARMDataProcessingTest, MVN_Self) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[2] = 0x12345678;
    assemble_and_write("mvn r2, r2", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0x12345678u);
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedRegister_LSL_Reg) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x0000000F;
    cpu.R()[3] = 4;
    assemble_and_write("mvn r2, r1, lsl r3", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)~(0x0000000F << 4));
}

TEST_F(ARMDataProcessingTest, MVN_ShiftedOperand_RRX) {
    cpu.R()[15] = 0x00000000;
    cpu.R()[1] = 0x80000001;
    cpu.CPSR() = 0x20000000; // C flag set
    assemble_and_write("mvn r2, r1, rrx", cpu.R()[15]);
    arm_cpu.execute(1);
    // RRX: 0x80000001 -> 0xC0000000 (with C=1)
    EXPECT_EQ(cpu.R()[2], ~0xC0000000u);
}

TEST_F(ARMDataProcessingTest, MVN_ImmediateRotated) {
    cpu.R()[15] = 0x00000000;
    assemble_and_write("mvn r2, #0xFF000000", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], ~0xFF000000u);
}

