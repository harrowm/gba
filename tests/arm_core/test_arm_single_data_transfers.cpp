// test_arm_single_data_transfers.cpp
// Google Test suite for ARM Single Data Transfers
// Covers all handlers in arm_exec_single_data_transfers.cpp


#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

class ARMCPUSingleDataTransferTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;

    ARMCPUSingleDataTransferTest() : cpu(memory, interrupts), arm_cpu(cpu) {}

    void SetUp() override {
        // RAM is available from 0x00000000 to 0x00001FFF (8KB) by default in test mode
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        cpu.CPSR() = 0x10; // User mode, no flags set
    }
};

// Example test for LDR (immediate, pre-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Pre_WB) {
    cpu.R()[1] = 0x1000; // base
    cpu.R()[15] = 0x00000000; // Set PC to start of RAM
    memory.write32(0x1004, 0xDEADBEEF);
    uint32_t instr = 0xE5B12004; // LDR r2, [r1, #4]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xDEADBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004); // writeback
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004); // PC incremented by 4
}


// LDR (immediate, pre-indexed, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Pre_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1004, 0xCAFEBABE);
    uint32_t instr = 0xE5912004; // LDR r2, [r1, #4]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000); // no writeback
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (immediate, post-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Post_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0x12345678);
    uint32_t instr = 0xE4912004; // LDR r2, [r1], #4

    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x12345678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004); // writeback
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (immediate, post-indexed - hence alwayswriteback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Post_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0xAABBCCDD);
    uint32_t instr = 0xE4912004; // LDR r2, [r1], #4 
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xAABBCCDD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004); // writeback alwats occurs in post-indexed
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, pre-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Pre_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xDEADBEEF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5A12004; // STR r2, [r1, #4]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1004), (uint32_t)0xDEADBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, pre-indexed, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Pre_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5812004; // STR r2, [r1, #4]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1004), (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, post-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Post_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0x12345678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE4812004; // STR r2, [r1], #4!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0x12345678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, post-indexed, always writeback as post-indexed)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Post_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xAABBCCDD;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE4812004; // STR r2, [r1], #4

    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0xAABBCCDD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, pre-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Pre_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1004, 0xAB);
    uint32_t instr = 0xE5F12004; // LDRB r2, [r1, #4]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xAB);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, pre-indexed, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Pre_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1004, 0xCD);
    uint32_t instr = 0xE5D12004; // LDRB r2, [r1, #4]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xCD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, post-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Post_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0xEF);
    uint32_t instr = 0xE4D12004; // LDRB r2, [r1], #4!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, post-indexed, always writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Post_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0x12);
    uint32_t instr = 0xE4D12004; // LDRB r2, [r1], #4
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0x12);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, pre-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Pre_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xAB;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5E12004; // STRB r2, [r1, #4]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1004), (uint8_t)0xAB);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, pre-indexed, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Pre_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xCD;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5C12004; // STRB r2, [r1, #4]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1004), (uint8_t)0xCD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, post-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Post_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xEF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE4C12004; // STRB r2, [r1], #4!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1000), (uint8_t)0xEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, post-indexed, always writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Post_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0x12;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE4C12004; // STRB r2, [r1], #4
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1000), (uint8_t)0x12);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR/STR (register offset, pre/post, wb/nowb)
// LDR (register, pre-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_Pre_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1010, 0xBEEFCAFE);
    uint32_t instr = 0xE7B12003; // LDR r2, [r1, r3]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xBEEFCAFE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1010);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (register, pre-indexed, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_Pre_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1010, 0xCAFEBABE);
    uint32_t instr = 0xE7912003; // LDR r2, [r1, r3]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (register, post-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_Post_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0x12345678);
    uint32_t instr = 0xE6912003; // LDR r2, [r1], r3!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x12345678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1010);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (register, post-indexed, always writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_Post_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0xAABBCCDD);
    uint32_t instr = 0xE6912003; // LDR r2, [r1], r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xAABBCCDD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1010);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (register, pre-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_Pre_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xDEADBEEF;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE7A12003; // STR r2, [r1, r3]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1010), (uint32_t)0xDEADBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1010);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (register, pre-indexed, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_Pre_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE7812003; // STR r2, [r1, r3]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1010), (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (register, post-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_Post_WB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0x12345678;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE6812003; // STR r2, [r1], r3!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0x12345678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1010);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (register, post-indexed, always writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_Post_NoWB) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xAABBCCDD;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE6812003; // STR r2, [r1], r3
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0xAABBCCDD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1010);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (register offset)
// Instruction: 0xe19130b3
// LDRH r3, [r1, r3]
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Reg) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x2;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1002, 0xBEEF);
    memory.write32(cpu.R()[15], 0xe19130b3);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (register offset)
// Instruction: 0xE18120B3
// STRH r2, [r1, r3]
TEST_F(ARMCPUSingleDataTransferTest, STRH_Reg) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xABCD;
    cpu.R()[3] = 0x2;
    cpu.R()[15] = 0x00000000;
    memory.write32(cpu.R()[15], 0xE18120B3);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1002), (uint16_t)0xABCD);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSB (register offset)
// Instruction: 0xe19130d3 
// LDRSB r3, [r1, r3]
TEST_F(ARMCPUSingleDataTransferTest, LDRSB_Reg) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x2;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1002, 0x80); // -128
    memory.write32(cpu.R()[15], 0xe19130d3);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -128);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSH (register offset)
// Instruction: 0xe19130f3 
// LDRSH r3, [r1, r3]
TEST_F(ARMCPUSingleDataTransferTest, LDRSH_Reg) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x2;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1002, 0x8000); // -32768
    memory.write32(cpu.R()[15], 0xe19130f3);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -32768);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}
