// test_arm_other.cpp
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

class ARMOtherTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;

    ARMOtherTest() : cpu(memory, interrupts), arm_cpu(cpu) {}

    void SetUp() override {
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        cpu.CPSR() = 0x10; // User mode, no flags set
    }
};

// LDM/STM basic test
TEST_F(ARMOtherTest, LDM_STM_Basic) {
    // Store values in registers
    cpu.R()[0] = 0x11111111;
    cpu.R()[1] = 0x22222222;
    cpu.R()[2] = 0x33333333;
    cpu.R()[3] = 0x44444444;
    cpu.R()[4] = 0x100; // Base address
    // STMIA R4!, {R0-R3}
    uint32_t stm_instr = 0xE8A4000F; // STMIA R4!, {R0-R3}
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], stm_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x100), 0x11111111u);
    EXPECT_EQ(memory.read32(0x104), 0x22222222u);
    EXPECT_EQ(memory.read32(0x108), 0x33333333u);
    EXPECT_EQ(memory.read32(0x10C), 0x44444444u);
    EXPECT_EQ(cpu.R()[4], 0x110u); // Writeback
    // LDMIA R4!, {R0-R3}
    cpu.R()[4] = 0x100; // Reset base address
    cpu.R()[0] = cpu.R()[1] = cpu.R()[2] = cpu.R()[3] = 0;
    uint32_t ldm_instr = 0xE8B4000F; // LDMIA R4!, {R0-R3}
    cpu.R()[15] = 0x00000004;
    memory.write32(cpu.R()[15], ldm_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x11111111u);
    EXPECT_EQ(cpu.R()[1], 0x22222222u);
    EXPECT_EQ(cpu.R()[2], 0x33333333u);
    EXPECT_EQ(cpu.R()[3], 0x44444444u);
    EXPECT_EQ(cpu.R()[4], 0x110u); // Writeback
}

// Branch (B, BL) test
TEST_F(ARMOtherTest, Branch_B_BL) {
    cpu.R()[15] = 0x00000000;
    // B +8 (offset = 2)
    uint32_t b_instr = 0xEA000002; // B +8
    memory.write32(cpu.R()[15], b_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000010);
    // BL +12 (offset = 3)
    cpu.R()[15] = 0x00000010;
    uint32_t bl_instr = 0xEB000003; // BL +12
    memory.write32(cpu.R()[15], bl_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000024);
    EXPECT_EQ(cpu.R()[14], (uint32_t)0x00000014);
}

// SWP/SWPB test
TEST_F(ARMOtherTest, SWP_SWPB) {
    cpu.R()[1] = 0x200; // Address
    cpu.R()[2] = 0xDEADBEEF; // Value to store
    memory.write32(0x200, 0xCAFEBABE);
    // SWP R0, R2, [R1]
    uint32_t swp_instr = 0xE1010092; // SWP R0, R2, [R1]
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], swp_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xCAFEBABEu);
    EXPECT_EQ(memory.read32(0x200), 0xDEADBEEFu);
    // SWPB R3, R2, [R1]
    memory.write8(0x200, 0xAA);
    cpu.R()[2] = 0xBB;
    uint32_t swpb_instr = 0xE1413092; // SWPB R3, R2, [R1]
    cpu.R()[15] = 0x00000004;
    memory.write32(cpu.R()[15], swpb_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFF, 0xAAu);
    EXPECT_EQ(memory.read8(0x200), 0xBBu);
}

// Undefined and SWI test
TEST_F(ARMOtherTest, UndefinedAndSWI) {
    cpu.R()[15] = 0x00000000;
    // Undefined instruction (should branch to 0x04 and set mode)
    uint32_t undef_instr = 0xE1A000F0; // Undefined
    memory.write32(cpu.R()[15], undef_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x04u);
    EXPECT_EQ(cpu.CPSR() & 0x1F, 0x1Bu); // Mode = Undefined
    // SWI (should branch to 0x08 and set mode)
    cpu.R()[15] = 0x00000010;
    uint32_t swi_instr = 0xEF000011; // SWI #0x11
    memory.write32(cpu.R()[15], swi_instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x08u);
    EXPECT_EQ(cpu.CPSR() & 0x1F, 0x13u); // Mode = SVC
}
