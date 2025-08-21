// test_arm_multiply.cpp
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

extern "C" {
#include <keystone/keystone.h>
}

class ARMMultiplyTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;
    ks_engine* ks; // Keystone handle

    ARMMultiplyTest() : memory(true), cpu(memory, interrupts), arm_cpu(cpu) {}

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

// MUL: Rd = Rm * Rs
TEST_F(ARMMultiplyTest, MUL_Basic) {
    cpu.R()[0] = 3; // Rm
    cpu.R()[1] = 4; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0000091; // MUL r0, r1, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 12u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    // The Z flag should not be set for a nonzero result (MUL does not set flags unless S bit is set)
    EXPECT_FALSE(cpu.CPSR() & (1u << 30));
}

// --------- Stage 1: All S (flag-setting) variants for N/Z flags ---------
TEST_F(ARMMultiplyTest, MULS_ResultZeroSetsZ) {
    cpu.R()[0] = 0; // Rm
    cpu.R()[1] = 0; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0100091; // MULS r0, r1, r0 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

TEST_F(ARMMultiplyTest, MLAS_NegativeResultSetsN) {
    cpu.R()[0] = 0xFFFFFFFF; // Rm
    cpu.R()[1] = 2; // Rs
    cpu.R()[2] = 0; // Rn
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0302091; // MLAS r0, r1, r0, r2 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag set
}

TEST_F(ARMMultiplyTest, UMULLS_ResultZeroSetsZ) {
    cpu.R()[2] = 0; // Rm
    cpu.R()[3] = 0; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0954392; // UMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0u);
    EXPECT_EQ(cpu.R()[5], 0u);
}

// --------- Additional edge/robustness cases for full ARM spec coverage ---------
// 1. All operands the same register (MUL r0, r0, r0)
TEST_F(ARMMultiplyTest, MUL_AllOperandsSame) {
    cpu.R()[0] = 7;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0000090; // MUL r0, r0, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 49u);
}

// 2. MUL/MLA with Rd == Rs
TEST_F(ARMMultiplyTest, MUL_RdEqualsRs) {
    cpu.R()[0] = 5; // Rm
    cpu.R()[1] = 3; // Rs (also Rd)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0010091; // MUL r1, r1, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 15u);
}

TEST_F(ARMMultiplyTest, MLA_RdEqualsRs) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs (also Rd)
    cpu.R()[2] = 4; // Rn
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0212091; // MLA r1, r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 10u);
}

// 3. Forbidden destination as PC (Rd == 15)
TEST_F(ARMMultiplyTest, MUL_RdIsPC) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE000F091; // MUL r15, r1, r0
    memory.write32(cpu.R()[15], instr);
    // Should not crash, result is unpredictable, but PC should increment
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// 4. S variant with result exactly 0x80000000 (N flag set, Z clear)
TEST_F(ARMMultiplyTest, MULS_ResultIs0x80000000) {
    cpu.R()[0] = 0x80000000; // Rm
    cpu.R()[1] = 1; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0100091; // MULS r0, r1, r0 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x80000000u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N set
}

// 5. C/V flags are not affected by S variants
TEST_F(ARMMultiplyTest, MULS_CVFlagsUnaffected) {
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 2;
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = (1u << 29) | (1u << 28); // Set C and V
    uint32_t instr = 0xe0100091 ; // MULS r0, r1, r0 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // C and V should remain set
    EXPECT_TRUE(cpu.CPSR() & (1u << 29)); // C
    EXPECT_TRUE(cpu.CPSR() & (1u << 28)); // V
}

// 6. Rs = 0 (should always result in zero)
TEST_F(ARMMultiplyTest, MUL_RsZero) {
    cpu.R()[0] = 0xDEADBEEF; // Rm
    cpu.R()[1] = 0; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0000091; // MUL r0, r1, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u);
}

// 7. Rm = 0, Rs = 0, acc != 0 for MLA/UMLAL/SMLAL
TEST_F(ARMMultiplyTest, MLA_ZeroOperandsNonzeroAcc) {
    cpu.R()[0] = 0; // Rm
    cpu.R()[1] = 0; // Rs
    cpu.R()[2] = 0x12345678; // Rn
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0202091; // MLA r0, r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x12345678u);
}

TEST_F(ARMMultiplyTest, UMLAL_ZeroOperandsNonzeroAcc) {
    cpu.R()[2] = 0; // Rm
    cpu.R()[3] = 0; // Rs
    cpu.R()[4] = 0x12345678; // acc lo (RdLo)
    cpu.R()[5] = 0x9ABCDEF0; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0A54392; // UMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0x12345678u);
    EXPECT_EQ(cpu.R()[5], 0x9ABCDEF0u);
}

TEST_F(ARMMultiplyTest, SMLAL_ZeroOperandsNonzeroAcc) {
    cpu.R()[2] = 0; // Rm
    cpu.R()[3] = 0; // Rs
    cpu.R()[4] = 0x12345678; // acc lo (RdLo)
    cpu.R()[5] = 0x9ABCDEF0; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0E54392; // SMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0x12345678u);
    EXPECT_EQ(cpu.R()[5], 0x9ABCDEF0u);
}

// 8. SMLAL negative accumulator overflow
TEST_F(ARMMultiplyTest, SMLAL_NegativeAccumulatorOverflow) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm (-1)
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0xFFFFFFFF; // acc lo (RdLo)
    cpu.R()[5] = 0xFFFFFFFF; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0E54392; // SMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // result = (int64_t)-1 * 2 + 0xFFFFFFFFFFFFFFFF = 0xFFFFFFFFFFFFFFFD
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFDu);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFFu);
}

// --------- Stage 4: Carry/overflow and N/Z flag checks (S variants) ---------
TEST_F(ARMMultiplyTest, UMULLS_NegativeResultSetsN) {
    cpu.R()[2] = 0x80000000; // Rm
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0954392; // UMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0x80000000 * 2 = 0x100000000
    EXPECT_EQ(cpu.R()[4], 0u);
    EXPECT_EQ(cpu.R()[5], 1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear (high bit not set)
}

TEST_F(ARMMultiplyTest, SMLALS_ZeroResultSetsZ) {
    cpu.R()[2] = 0; // Rm
    cpu.R()[3] = 0; // Rs
    cpu.R()[4] = 0; // acc lo (RdLo)
    cpu.R()[5] = 0; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0F54392; // SMLALS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0u);
    EXPECT_EQ(cpu.R()[5], 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
}

// --------- Stage 5: PC and forbidden register usage ---------
TEST_F(ARMMultiplyTest, MLA_UsesPCAsOperand) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs
    cpu.R()[2] = 5; // Rn
    cpu.R()[15] = 0x00000008; // PC
    uint32_t instr = 0xE0202F91; // MLA r0, r1, r0, r15 (Rn=15)
    memory.write32(cpu.R()[15], instr);
    // Should not crash, result is unpredictable, but test for no crash and PC increment
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x0000000C);
}

TEST_F(ARMMultiplyTest, SMLAL_UsesPCAsOperand) {
    cpu.R()[2] = 0x7FFFFFFF; // Rm
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 1; // acc lo (RdLo)
    cpu.R()[5] = 1; // acc hi (RdHi)
    cpu.R()[15] = 0x00000008; // PC
    uint32_t instr = 0xE0E54F92; // SMLAL r4, r5, r2, r15 (Rn=15)
    memory.write32(cpu.R()[15], instr);
    // Should not crash, result is unpredictable, but test for no crash and PC increment
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x0000000C);
}

// --------- Stage 6: Multiple accumulations ---------
TEST_F(ARMMultiplyTest, SMLAL_MultipleAccumulate) {
    cpu.R()[2] = 2; // Rm
    cpu.R()[3] = 3; // Rs
    cpu.R()[4] = 1; // acc lo (RdLo)
    cpu.R()[5] = 1; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0E54392; // SMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // First: (2*3)+((1LL<<32)|1) = 6+4294967297 = 4294967303
    EXPECT_EQ(cpu.R()[4], 0x00000007u);
    EXPECT_EQ(cpu.R()[5], 1u);
    // Now accumulate again
    cpu.R()[2] = 1;
    cpu.R()[3] = 1;
    cpu.R()[4] = cpu.R()[4];
    cpu.R()[5] = cpu.R()[5];
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Second: (1*1)+4294967303 = 4294967304
    EXPECT_EQ(cpu.R()[4], 0x00000008u);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// --------- Stage 7: Additional edge/robustness cases ---------
TEST_F(ARMMultiplyTest, MULS_NegativeZero) {
    cpu.R()[0] = 0x80000000; // Rm
    cpu.R()[1] = 0; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0100091; // MULS r0, r1, r0 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

TEST_F(ARMMultiplyTest, UMULLS_MaxUnsignedSetsN) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 0xFFFFFFFF; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0954392; // UMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE00000001
    EXPECT_EQ(cpu.R()[4], 0x00000001u);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFEu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag set
}

TEST_F(ARMMultiplyTest, SMULLS_NegativeZero) {
    cpu.R()[2] = 0x80000000; // Rm (signed)
    cpu.R()[3] = 0; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0D54392; // SMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0u);
    EXPECT_EQ(cpu.R()[5], 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}



// --------- Stage 2: Register overlap for all multiply types ---------
TEST_F(ARMMultiplyTest, MUL_Overlap_Rd_Rm) {
    cpu.R()[0] = 3; // Rm (also Rd)
    cpu.R()[1] = 4; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0000091; // MUL r0, r1, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 12u);
}

TEST_F(ARMMultiplyTest, MLA_Overlap_Rd_Rn) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs
    cpu.R()[2] = 5; // Rn (also Rd)
    cpu.R()[15] = 0x00000000;
    // Save the original Rn value for the expected result
    uint32_t orig_rn = cpu.R()[2];
    uint32_t instr = 0xe0222091 ; // MLA r2, r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // The result should be (Rm * Rs) + original Rn
    EXPECT_EQ(cpu.R()[2], (uint32_t)(cpu.R()[0] * cpu.R()[1] + orig_rn));
}

TEST_F(ARMMultiplyTest, UMULL_TripleOverlap) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm (also RdLo)
    cpu.R()[3] = 2; // Rs (also RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0832392; // UMULL r2, r3, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[3], 1u);
}

TEST_F(ARMMultiplyTest, SMULL_TripleOverlap) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm (also RdLo)
    cpu.R()[3] = 2; // Rs (also RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0C32392; // SMULL r2, r3, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[3], 0xFFFFFFFFu);
}

// --------- Stage 3: Signed/unsigned edge cases ---------
TEST_F(ARMMultiplyTest, MUL_NegativeTimesNegative) {
    cpu.R()[0] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[1] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0000091; // MUL r0, r1, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 1u);
}

TEST_F(ARMMultiplyTest, UMULL_UnsignedOverflow) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 0xFFFFFFFE; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0854392; // UMULL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0xFFFFFFFF * 0xFFFFFFFE = 0xFFFFFFFD00000002
    EXPECT_EQ(cpu.R()[4], 0x00000002u);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFDu);
}

TEST_F(ARMMultiplyTest, SMULL_SignedOverflow) {
    cpu.R()[2] = 0x80000000; // -2147483648 (signed)
    cpu.R()[3] = 2; // 2
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0C54392; // SMULL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // (-2147483648) * 2 = -4294967296 = 0xFFFFFFFF00000000
    EXPECT_EQ(cpu.R()[4], 0x00000000u);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFFu);
}

TEST_F(ARMMultiplyTest, MUL_SetsFlags) {
    cpu.R()[2] = 0xFFFFFFFF;
    cpu.R()[3] = 2;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0120293; // MULS r2, r3, r2 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFEu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// MLA: Rd = (Rm * Rs) + Rn
TEST_F(ARMMultiplyTest, MLA_Basic) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs
    cpu.R()[2] = 5; // Rn
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0202091 ; // MLA r0, r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)(2 * 3 + 5));
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// UMULL: RdLo, RdHi = Rm * Rs (unsigned)
TEST_F(ARMMultiplyTest, UMULL_Basic) {
    cpu.R()[0] = 0xFFFFFFFF; // Rm
    cpu.R()[1] = 2; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0810190 ; // UMULL r0, r1, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[1], 1u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// UMLAL: RdLo, RdHi = (Rm * Rs) + acc (unsigned)
TEST_F(ARMMultiplyTest, UMLAL_Basic) {
    cpu.R()[4] = 0xFFFFFFFF; // Rm
    cpu.R()[5] = 2; // Rs
    cpu.R()[0] = 1; // acc lo (RdLo)
    cpu.R()[1] = 1; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0A10594; // UMLAL r0, r1, r4, r5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFFu); // RdLo
    EXPECT_EQ(cpu.R()[1], 2u);         // RdHi
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// SMULL: RdLo, RdHi = Rm * Rs (signed)
TEST_F(ARMMultiplyTest, SMULL_Basic) {
    cpu.R()[0] = 0x7FFFFFFF; // Rm
    cpu.R()[1] = 2; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0c10190; // SMULL r0, r1, r0, r1
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[1], 0x0u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// SMLAL: RdLo, RdHi = (Rm * Rs) + acc (signed)
TEST_F(ARMMultiplyTest, SMLAL_Basic) {
    cpu.R()[2] = 0x7FFFFFFF; // Rm (distinct)
    cpu.R()[3] = 2;          // Rs (distinct)
    cpu.R()[4] = 1;          // acc lo (RdLo)
    cpu.R()[5] = 1;          // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0E54392; // SMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // result = (int64_t)0x7FFFFFFF * 2 + ((1LL << 32) | 1) = 4294967294 + 4294967297 = 8589934591
    // RdLo = 0xFFFFFFFF, RdHi = 1
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFFu);
    EXPECT_EQ(cpu.R()[5], 1u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
}

// Add more edge and flag tests as needed

// --------- Stage 1: All flag-setting (S) variants ---------
TEST_F(ARMMultiplyTest, MULS_SetsNZFlags) {
    cpu.R()[0] = 0x80000000; // Rm and Rd
    cpu.R()[1] = 2; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0100091; // MULS r0, r1, r0 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    printf("[TEST DEBUG] After execute: cpu.R()[0]=0x%08X (%u)\n", cpu.R()[0], cpu.R()[0]);
    EXPECT_EQ(cpu.R()[0], 0u); // 0x80000000 * 2 = 0 (overflow, but unsigned)
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

TEST_F(ARMMultiplyTest, MLA_SetsFlags) {
    cpu.R()[0] = 0xFFFFFFFF; // Rm
    cpu.R()[1] = 2; // Rs
    cpu.R()[2] = 1; // Rn
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0302091 ; // MLAS r0, r1, r0, r2 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu + 1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag set
}

TEST_F(ARMMultiplyTest, UMULL_SetsFlags) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 0xFFFFFFFF; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0954392 ; // UMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0x00000001u);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFEu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag set
}

TEST_F(ARMMultiplyTest, UMLAL_SetsFlags) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0xFFFFFFFF; // acc lo (RdLo)
    cpu.R()[5] = 0xFFFFFFFF; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0b54392 ; // UMLALS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFDu);
    EXPECT_EQ(cpu.R()[5], 1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

TEST_F(ARMMultiplyTest, SMULL_SetsFlags) {
    cpu.R()[2] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0d54392 ; // SMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFFu);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag set
}

TEST_F(ARMMultiplyTest, SMLAL_SetsFlags) {
    cpu.R()[2] = 0x7FFFFFFF; // Rm
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 1; // acc lo (RdLo)
    cpu.R()[5] = 1; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0f54392 ; // SMLALS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFFu);
    EXPECT_EQ(cpu.R()[5], 1u);
    EXPECT_FALSE(cpu.CPSR() & (1u << 30)); // Z flag clear
    EXPECT_FALSE(cpu.CPSR() & (1u << 31)); // N flag clear
}

// --------- Stage 2: Register overlap cases ---------
// Overlap RdLo == Rm
TEST_F(ARMMultiplyTest, UMULL_Overlap_RdLo_Rm) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm (also RdLo)
    cpu.R()[3] = 2; // Rs
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0852392; // UMULL r2, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// Overlap RdHi == Rs
TEST_F(ARMMultiplyTest, UMULL_Overlap_RdHi_Rs) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[5] = 2; // Rs (also RdHi)
    cpu.R()[4] = 0; // RdLo
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0854592; // UMULL r4, r5, r2, r5
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// Overlap RdLo == Rs
TEST_F(ARMMultiplyTest, UMULL_Overlap_RdLo_Rs) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[4] = 2; // Rs (also RdLo)
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0854492 ; // UMULL r4, r5, r2, r4
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// Overlap RdHi == Rm
TEST_F(ARMMultiplyTest, UMULL_Overlap_RdHi_Rm) {
    cpu.R()[5] = 0xFFFFFFFF; // Rm (also RdHi)
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0854395; // UMULL r4, r5, r5, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// --------- Stage 3: Signed/unsigned edge cases ---------
TEST_F(ARMMultiplyTest, SMULL_NegativeTimesNegative) {
    cpu.R()[2] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[3] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0c54392; // SMULL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // (-1) * (-1) = 1
    EXPECT_EQ(cpu.R()[4], 1u);
    EXPECT_EQ(cpu.R()[5], 0u);
}

TEST_F(ARMMultiplyTest, SMULL_NegativeTimesPositive) {
    cpu.R()[2] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[3] = 2; // 2
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0c54392 ; // SMULL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // (-1) * 2 = -2
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFFu);
}

TEST_F(ARMMultiplyTest, UMULL_HighLowBits) {
    cpu.R()[2] = 0x80000000; // High bit set
    cpu.R()[3] = 0x00000002; // Low bit set
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0854392; // UMULL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0x80000000 * 2 = 0x100000000
    EXPECT_EQ(cpu.R()[4], 0u);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// --------- Stage 4: Carry/overflow and N/Z flag checks ---------
TEST_F(ARMMultiplyTest, UMULLS_ZeroResultSetsZ) {
    cpu.R()[2] = 0; // Rm
    cpu.R()[3] = 0; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0954392 ; // UMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0u);
    EXPECT_EQ(cpu.R()[5], 0u);
    EXPECT_TRUE(cpu.CPSR() & (1u << 30)); // Z flag set
}

TEST_F(ARMMultiplyTest, SMULLS_NegativeResultSetsN) {
    cpu.R()[2] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[3] = 2; // 2
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0d54392; // SMULLS r4, r5, r2, r3 (S=1)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFFu);
    EXPECT_TRUE(cpu.CPSR() & (1u << 31)); // N flag set
}

// Note: ARM multiply does not set carry/overflow flags, but N/Z are set for S variants.

// --------- Stage 5: PC and forbidden register usage ---------
// ARM spec: Using R15 (PC) as a destination or operand is unpredictable, but should not crash.
TEST_F(ARMMultiplyTest, MUL_UsesPCAsOperand) {
    cpu.R()[0] = 3; // Rm
    cpu.R()[1] = 4; // Rs
    cpu.R()[15] = 0x00000008; // PC
    uint32_t instr = 0xE0000F91; // MUL r0, r1, r15 (Rm=15)
    memory.write32(cpu.R()[15], instr);
    // Should not crash, result is unpredictable, but test for no crash and PC increment
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x0000000C);
}

TEST_F(ARMMultiplyTest, UMULL_UsesPCAsOperand) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 0xFFFFFFFF; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000008; // PC
    uint32_t instr = 0xE0853F92; // UMULL r4, r5, r3, r15 (Rs=15)
    memory.write32(cpu.R()[15], instr);
    // Should not crash, result is unpredictable, but test for no crash and PC increment
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x0000000C);
}

// --------- Stage 6: Multiple accumulations ---------
TEST_F(ARMMultiplyTest, MLA_MultipleAccumulate) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs
    cpu.R()[2] = 5; // Rn
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0202091 ; // MLA r0, r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // First: 2*3+5=11
    EXPECT_EQ(cpu.R()[0], 11u);
    // Now accumulate again
    cpu.R()[2] = cpu.R()[0];
    cpu.R()[0] = 2;
    cpu.R()[1] = 3;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Second: 2*3+11=17
    EXPECT_EQ(cpu.R()[0], 17u);
}

TEST_F(ARMMultiplyTest, UMLAL_MultipleAccumulate) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0xFFFFFFFF; // acc lo (RdLo)
    cpu.R()[5] = 0xFFFFFFFF; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0a54392 ; // UMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // First: (0xFFFFFFFF * 2) + 0xFFFFFFFFFFFFFFFF = 0x1FFFFFFFFFD
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFDu);
    EXPECT_EQ(cpu.R()[5], 1u);
    // Now accumulate again
    cpu.R()[4] = cpu.R()[4];
    cpu.R()[5] = cpu.R()[5];
    cpu.R()[2] = 1;
    cpu.R()[3] = 1;
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Second: (1*1)+0x1FFFFFFFFFD = 0x1FFFFFFFFFE
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// MUL: Edge cases
TEST_F(ARMMultiplyTest, MUL_Zero) {
    cpu.R()[0] = 0; // Rm
    cpu.R()[1] = 123456; // Rs
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0000091; // MUL r0, r1, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u);
}

TEST_F(ARMMultiplyTest, MUL_Negative) {
    cpu.R()[0] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[1] = 2;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0000091; // MUL r0, r1, r0
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu);
}

// MLA: Edge cases
TEST_F(ARMMultiplyTest, MLA_ZeroAcc) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs
    cpu.R()[2] = 0; // Rn
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0202091; // MLA r0, r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 6u);
}

TEST_F(ARMMultiplyTest, MLA_NegativeAcc) {
    cpu.R()[0] = 2; // Rm
    cpu.R()[1] = 3; // Rs
    cpu.R()[2] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0202091 ; // MLA r0, r1, r0, r2
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 5u);
}

// UMULL: Edge cases
TEST_F(ARMMultiplyTest, UMULL_MaxUnsigned) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 0xFFFFFFFF; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0854392; // UMULL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // 0xFFFFFFFF * 0xFFFFFFFF = 0xFFFFFFFE00000001
    EXPECT_EQ(cpu.R()[4], 0x00000001u);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFEu);
}

// UMLAL: Edge cases
TEST_F(ARMMultiplyTest, UMLAL_Accumulates) {
    cpu.R()[2] = 0xFFFFFFFF; // Rm
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0xFFFFFFFF; // acc lo (RdLo)
    cpu.R()[5] = 0xFFFFFFFF; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0A54392; // UMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // result = (0xFFFFFFFF * 2) + 0xFFFFFFFFFFFFFFFF = 0x1FFFFFFFFFD
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFDu);
    EXPECT_EQ(cpu.R()[5], 1u);
}

// SMULL: Edge cases
TEST_F(ARMMultiplyTest, SMULL_Negative) {
    cpu.R()[2] = 0xFFFFFFFF; // -1 (signed)
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0; // RdLo
    cpu.R()[5] = 0; // RdHi
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0C54392; // SMULL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // result = (int64_t)-1 * 2 = -2
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);
    EXPECT_EQ(cpu.R()[5], 0xFFFFFFFFu);
}

// SMLAL: Edge cases
TEST_F(ARMMultiplyTest, SMLAL_NegativeAcc) {
    cpu.R()[2] = 0x7FFFFFFF; // Rm
    cpu.R()[3] = 2; // Rs
    cpu.R()[4] = 0xFFFFFFFF; // acc lo (RdLo)
    cpu.R()[5] = 0xFFFFFFFF; // acc hi (RdHi)
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE0E54392; // SMLAL r4, r5, r2, r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // result = (int64_t)0x7FFFFFFF * 2 + 0xFFFFFFFFFFFFFFFF = 0x100000000FFFFFFFD
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFDu);
    EXPECT_EQ(cpu.R()[5], 0u);
}
