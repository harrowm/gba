// test_arm_multiply.cpp
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

class ARMMultiplyTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;

    ARMMultiplyTest() : cpu(memory, interrupts), arm_cpu(cpu) {}

    void SetUp() override {
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        cpu.CPSR() = 0x10; // User mode, no flags set
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
