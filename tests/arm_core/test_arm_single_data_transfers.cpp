#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

extern "C" {
#include <keystone/keystone.h>
}

class ARMCPUSingleDataTransferTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;
    ks_engine* ks; // Keystone handle

    ARMCPUSingleDataTransferTest() : memory(true), cpu(memory, interrupts), arm_cpu(cpu) {}

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

// Example test for LDR (immediate, pre-indexed, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Pre_WB) {
    cpu.R()[1] = 0x1000; // base
    cpu.R()[15] = 0x00000000; // Set PC to start of RAM
    memory.write32(0x1004, 0xDEADBEEF);
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, #4]!", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, #4]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1], #4", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1], #4", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1, #4]!", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1, #4]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1], #4", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1], #4", cpu.R()[15]));

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
    ASSERT_TRUE(assemble_and_write("ldrb r2, [r1, #4]!", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldrb r2, [r1, #4]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldrb r2, [r1], #4", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldrb r2, [r1], #4", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("strb r2, [r1, #4]!", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("strb r2, [r1, #4]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("strb r2, [r1], #4", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("strb r2, [r1], #4", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, r3]!", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, r3]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1], r3", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1], r3", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1, r3]!", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1, r3]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1], r3", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("str r2, [r1], r3", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldrh r3, [r1, r3]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("strh r2, [r1, r3]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldrsb r3, [r1, r3]", cpu.R()[15]));
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
    ASSERT_TRUE(assemble_and_write("ldrsh r3, [r1, r3]", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -32768);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Negative (down) offset tests ---
// LDR (immediate, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Pre_WB_Down) {
    cpu.R()[1] = 0x1008; // base
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1004, 0xDEAD1234);
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, #-4]!", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xDEAD1234);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Pre_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xBEEF5678;
    cpu.R()[15] = 0x00000000;
    ASSERT_TRUE(assemble_and_write("str r2, [r1, #-4]!", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1004), (uint32_t)0xBEEF5678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Pre_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1004, 0xAB);
    ASSERT_TRUE(assemble_and_write("ldrb r2, [r1, #-4]!", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xAB);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Pre_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xCD;
    cpu.R()[15] = 0x00000000;
    ASSERT_TRUE(assemble_and_write("strb r2, [r1, #-4]!", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1004), (uint8_t)0xCD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (immediate, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Imm_Pre_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1004, 0xBEEF);
    uint32_t instr = 0xe1f130b4 ; // LDRH r3, [r1, #4]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (immediate, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Imm_Pre_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xABCD;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe1e120b4 ; // STRH r2, [r1, #4]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1004), (uint16_t)0xABCD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Negative (down) offset tests: pre-indexed, no writeback ---
// LDR (immediate, pre-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Pre_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1004, 0xCAFED00D);
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, #-4]", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xCAFED00D);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1008); // no writeback
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, pre-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Pre_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xBEEFCAFE;
    cpu.R()[15] = 0x00000000;
    ASSERT_TRUE(assemble_and_write("str r2, [r1, #-4]", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1004), (uint32_t)0xBEEFCAFE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1008);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, pre-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Pre_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1004, 0xEF);
    ASSERT_TRUE(assemble_and_write("ldrb r2, [r1, #-4]", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1008);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, pre-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Pre_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0x12;
    cpu.R()[15] = 0x00000000;
    ASSERT_TRUE(assemble_and_write("strb r2, [r1, #-4]", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1004), (uint8_t)0x12);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1008);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (immediate, pre-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Imm_Pre_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1004, 0x1234);
    ASSERT_TRUE(assemble_and_write("ldrh r3, [r1, #-4]", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0x1234);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1008);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (immediate, pre-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Imm_Pre_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0x5678;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe1c120b4 ; // STRH r2, [r1, #4]
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1004), (uint16_t)0x5678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1008);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Negative (down) offset tests: post-indexed, writeback ---
// LDR (immediate, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Post_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1008, 0xFACEB00C);
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1], #-4", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xFACEB00C);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Post_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xDEAD5678;
    cpu.R()[15] = 0x00000000;
    ASSERT_TRUE(assemble_and_write("str r2, [r1], #-4", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1008), (uint32_t)0xDEAD5678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Post_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1008, 0xA5);
    uint32_t instr = 0xE4D12004; // LDRB r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xA5);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Post_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0x5A;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE4C12004; // STRB r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1008), (uint8_t)0x5A);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (immediate, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Imm_Post_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1008, 0xBEEF);
    uint32_t instr = 0xe0d130b4 ; // LDRH r3, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (immediate, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Imm_Post_WB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xBEEF;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0c120b4 ; // STRH r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1008), (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Negative (down) offset tests: post-indexed, no writeback (always writeback in post-indexed, but for naming consistency) ---
// LDR (immediate, post-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Post_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1008, 0xCAFEBABE);
    uint32_t instr = 0xE4912004; // LDR r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, post-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Post_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xDEADC0DE;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE4812004; // STR r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1008), (uint32_t)0xDEADC0DE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, post-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Post_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1008, 0x7F);
    uint32_t instr = 0xE4D12004; // LDRB r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0x7F);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, post-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Post_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xA5;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE4C12004; // STRB r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1008), (uint8_t)0xA5);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (immediate, post-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Imm_Post_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1008, 0x1234);
    uint32_t instr = 0xe0d130b4 ; // LDRH r3, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0x1234);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (immediate, post-indexed, down offset, no writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Imm_Post_NoWB_Down) {
    cpu.R()[1] = 0x1008;
    cpu.R()[2] = 0xFACE;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe0c120b4 ; // STRH r2, [r1], #4
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1008), (uint16_t)0xFACE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Negative (down) offset tests: register offset ---
// LDR (register, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0xDEADCAFE);
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, -r3]!", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xDEADCAFE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (register, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[2] = 0xBEEF1234;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE7A12003; // STR r2, [r1, r3]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0xBEEF1234);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (register, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0xA5);
    uint32_t instr = 0xE7F12003; // LDRB r2, [r1, r3]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xA5);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (register, pre-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[2] = 0x5A;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE7E12003; // STRB r2, [r1, r3]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1000), (uint8_t)0x5A);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (register, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0xCAFEBABE);
    uint32_t instr = 0xE6912003; // LDR r2, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Phase 1: Edge cases for base and offset registers ---

// LDR (immediate, base register is PC)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_BaseIsPC) {
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000008, 0xDEADBEEF);
    ASSERT_TRUE(assemble_and_write("ldr r2, [pc, #8]", cpu.R()[15]));
    arm_cpu.execute(1);
    // PC is 8 ahead due to pipeline
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xDEADBEEF);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, base register is PC)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_BaseIsPC) {
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[15] = 0x00000000;
    ASSERT_TRUE(assemble_and_write("str r2, [pc, #8]", cpu.R()[15]));
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x00000008), (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (register, offset register is PC)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_OffsetIsPC) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x10;
    memory.write32(0x1010, 0xBEEFCAFE);
    ASSERT_TRUE(assemble_and_write("ldr r2, [r1, pc]", 0x10));
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xBEEFCAFE);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000014);
}

// STR (register, offset register is PC)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_OffsetIsPC) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0x12345678;
    cpu.R()[15] = 0x10;
    uint32_t instr = 0xE781200F; // STR r2, [r1, PC]
    memory.write32(0x10, instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1010), (uint32_t)0x12345678);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000014);
}

// LDR (immediate, base and dest overlap)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_BaseDestOverlap) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1004, 0xCAFED00D);
    uint32_t instr = 0xE5911004; // LDR r1, [r1, #4]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0xCAFED00D);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, base and src overlap)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_BaseSrcOverlap) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5811004; // STR r1, [r1, #4]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1004), (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (register, offset and dest overlap)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Reg_OffsetDestOverlap) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1010, 0xDEAD1234);
    uint32_t instr = 0xE7912202; // LDR r2, [r1, r2]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xDEAD1234);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (register, offset and src overlap)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_OffsetSrcOverlap) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE7812202; // STR r2, [r1, r2]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1010), (uint32_t)0x10);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Phase 2: Unaligned address handling ---

// LDR (immediate, unaligned address)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Unaligned) {
    cpu.R()[1] = 0x1001;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0xDEADBEEF);
    uint32_t instr = 0xE5912000; // LDR r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // ARM LDR from unaligned address: result is rotated
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xBE00DEAD); // implementation-defined, check for bswap for GBA
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, unaligned address)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Unaligned) {
    cpu.R()[1] = 0x1003;
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0xAABBCCDD); // Initial value
    uint32_t instr = 0xE5812000; // STR r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Only aligned portion is written, check 0x1000-0x1003
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0xBABBCCDD); // implementation-defined, will be partial for GBA
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (immediate, unaligned address)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Imm_Unaligned) {
    cpu.R()[1] = 0x1003;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1003, 0xBEEF);
    uint32_t instr = 0xE1D130B0; // LDRH r3, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // LDRH from unaligned address: result is unpredictable, but test for value
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (immediate, unaligned address)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Imm_Unaligned) {
    cpu.R()[1] = 0x1003;
    cpu.R()[2] = 0xABCD;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe1c120b0 ; // STRH r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // STRH to unaligned address: may be partial or unpredictable, but test for value
    EXPECT_EQ(memory.read16(0x1003), (uint16_t)0xABCD);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSH (immediate, unaligned address)
TEST_F(ARMCPUSingleDataTransferTest, LDRSH_Imm_Unaligned) {
    cpu.R()[1] = 0x1003;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1003, 0x8000);
    uint32_t instr = 0xE1D130F0; // LDRSH r3, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -32768);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, unaligned address)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_Unaligned) {
    cpu.R()[1] = 0x1003;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1003, 0x7F);
    uint32_t instr = 0xE5D12000; // LDRB r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0x7F);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, unaligned address)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_Unaligned) {
    cpu.R()[1] = 0x1002;
    cpu.R()[2] = 0xA5;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5C12000; // STRB r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1002), (uint8_t)0xA5);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Phase 3: Boundary and overflow conditions ---

// LDR (immediate, at end of RAM)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_EndOfRAM) {
    cpu.R()[1] = 0x1FFC;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1FFC, 0xDEADBEEF);
    uint32_t instr = 0xE5912000; // LDR r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0xDEADBEEF);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, at end of RAM)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_EndOfRAM) {
    cpu.R()[1] = 0x1FFC;
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5812000; // STR r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1FFC), (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, at last byte of RAM)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_EndOfRAM) {
    cpu.R()[1] = 0x1FFF;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1FFF, 0x7F);
    uint32_t instr = 0xE5D12000; // LDRB r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0x7F);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, at last byte of RAM)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_EndOfRAM) {
    cpu.R()[1] = 0x1FFF;
    cpu.R()[2] = 0xA5;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5C12000; // STRB r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1FFF), (uint8_t)0xA5);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (immediate, at last 2 bytes of RAM)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Imm_EndOfRAM) {
    cpu.R()[1] = 0x1FFE;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1FFE, 0xBEEF);
    uint32_t instr = 0xE1D130B0; // LDRH r3, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (immediate, at last 2 bytes of RAM)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Imm_EndOfRAM) {
    cpu.R()[1] = 0x1FFE;
    cpu.R()[2] = 0xABCD;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C120B0; // STRH r2, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1FFE), (uint16_t)0xABCD);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSH (immediate, at last 2 bytes of RAM)
TEST_F(ARMCPUSingleDataTransferTest, LDRSH_Imm_EndOfRAM) {
    cpu.R()[1] = 0x1FFE;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1FFE, 0x8000);
    uint32_t instr = 0xE1D130F0; // LDRSH r3, [r1]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -32768);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (immediate, large positive offset out of RAM)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Overflow) {
    cpu.R()[1] = 0x1FF0;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE591200C; // LDR r2, [r1, #12]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Should not crash, may return 0 or garbage
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, large positive offset out of RAM)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Overflow) {
    cpu.R()[1] = 0x1FF0;
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE581200C; // STR r2, [r1, #12]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // Should not crash, may not write
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Phase 4: Writeback with Rn == Rd ---

// STR (immediate, pre-indexed, writeback, Rn == Rd)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_Pre_WB_RnEqRd) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5A11004; // STR r1, [r1, #4]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // ARM allows STR with Rn == Rd, value stored is old Rn
    EXPECT_EQ(memory.read32(0x1004), (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDR (immediate, pre-indexed, writeback, Rn == Rd) -- unpredictable, but test for implementation
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_Pre_WB_RnEqRd) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1004, 0xCAFEBABE);
    uint32_t instr = 0xE5B11004; // LDR r1, [r1, #4]!
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    // ARM: result is unpredictable, but test for value
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1004);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (register, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STR_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xDEAD5678;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE6812003; // STR r2, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0xDEAD5678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (register, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0x7F);
    uint32_t instr = 0xE6D12003; // LDRB r2, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0x7F);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (register, post-indexed, down offset, writeback)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xA5;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE6C12003; // STRB r2, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1000), (uint8_t)0xA5);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (register, pre-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1000, 0xBEEF);
    uint32_t instr = 0xE1B132B3; // LDRH r3, [r1, r3]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (register, pre-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[2] = 0xFACE;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1A122B3; // STRH r2, [r1, r3]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1000), (uint16_t)0xFACE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (register, post-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1000, 0x1234);
    uint32_t instr = 0xE09130B3; // LDRH r3, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0x1234);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (register, post-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xBEEF;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xe08120b3 ; // STRH r2, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1000), (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSB (register, pre-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRSB_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0x80); // -128
    uint32_t instr = 0xe1b130d3 ; // LDRSB r3, [r1, r3]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -128);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSB (register, post-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRSB_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0x80); // -128
    uint32_t instr = 0xE09130D3; // LDRSB r3, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -128);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSH (register, pre-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRSH_Reg_Pre_WB_Down) {
    cpu.R()[1] = 0x1010;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1000, 0x8000); // -32768
    uint32_t instr = 0xE1B130F3; // LDRSH r3, [r1, r3]!
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -32768);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSH (register, post-indexed, down offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRSH_Reg_Post_WB_Down) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x10;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1000, 0x8000); // -32768
    uint32_t instr = 0xE09130F3; // LDRSH r3, [r1], r3
    instr &= ~(1 << 23); // Clear U bit (down)
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -32768);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x0FF0);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// --- Phase 5: Zero offset and LDRSB/LDRSH sign extension edge cases ---

// LDR (immediate, zero offset)
TEST_F(ARMCPUSingleDataTransferTest, LDR_Imm_ZeroOffset) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write32(0x1000, 0x12345678);
    uint32_t instr = 0xE5912000; // LDR r2, [r1, #0]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x12345678);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STR (immediate, zero offset)
TEST_F(ARMCPUSingleDataTransferTest, STR_Imm_ZeroOffset) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5812000; // STR r2, [r1, #0]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x1000), (uint32_t)0xCAFEBABE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRB (immediate, zero offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRB_Imm_ZeroOffset) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0xAB);
    uint32_t instr = 0xE5D12000; // LDRB r2, [r1, #0]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2] & 0xFF, (uint8_t)0xAB);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRB (immediate, zero offset)
TEST_F(ARMCPUSingleDataTransferTest, STRB_Imm_ZeroOffset) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xCD;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE5C12000; // STRB r2, [r1, #0]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read8(0x1000), (uint8_t)0xCD);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRH (immediate, zero offset)
TEST_F(ARMCPUSingleDataTransferTest, LDRH_Imm_ZeroOffset) {
    cpu.R()[1] = 0x1000;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1000, 0xBEEF);
    uint32_t instr = 0xE1D130B0; // LDRH r3, [r1, #0]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFFFF, (uint16_t)0xBEEF);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// STRH (immediate, zero offset)
TEST_F(ARMCPUSingleDataTransferTest, STRH_Imm_ZeroOffset) {
    cpu.R()[1] = 0x1000;
    cpu.R()[2] = 0xFACE;
    cpu.R()[15] = 0x00000000;
    uint32_t instr = 0xE1C120B0; // STRH r2, [r1, #0]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read16(0x1000), (uint16_t)0xFACE);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x1000);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSB (sign extension: positive value)
TEST_F(ARMCPUSingleDataTransferTest, LDRSB_SignExt_Positive) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x0;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0x7F); // 127
    uint32_t instr = 0xE19130D3; // LDRSB r3, [r1, r3]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], 127);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSB (sign extension: negative value)
TEST_F(ARMCPUSingleDataTransferTest, LDRSB_SignExt_Negative) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x0;
    cpu.R()[15] = 0x00000000;
    memory.write8(0x1000, 0x80); // -128
    uint32_t instr = 0xE19130D3; // LDRSB r3, [r1, r3]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -128);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSH (sign extension: positive value)
TEST_F(ARMCPUSingleDataTransferTest, LDRSH_SignExt_Positive) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x0;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1000, 0x7FFF); // 32767
    uint32_t instr = 0xE19130F3; // LDRSH r3, [r1, r3]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], 32767);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// LDRSH (sign extension: negative value)
TEST_F(ARMCPUSingleDataTransferTest, LDRSH_SignExt_Negative) {
    cpu.R()[1] = 0x1000;
    cpu.R()[3] = 0x0;
    cpu.R()[15] = 0x00000000;
    memory.write16(0x1000, 0x8000); // -32768
    uint32_t instr = 0xE19130F3; // LDRSH r3, [r1, r3]
    memory.write32(cpu.R()[15], instr);
    arm_cpu.execute(1);
    EXPECT_EQ((int32_t)cpu.R()[3], -32768);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
}

// // --- Phase 6: Illegal/unpredictable encodings ---

// // LDR (illegal: both H and S bits set)
// TEST_F(ARMCPUSingleDataTransferTest, LDR_Illegal_HS_BitsSet) {
//     cpu.R()[1] = 0x1000;
//     cpu.R()[2] = 0;
//     cpu.R()[15] = 0x00000000;
//     // Both H and S bits set (should be unpredictable/illegal)
//     uint32_t instr = 0xE49120F0; // LDR r2, [r1], #0, H=1, S=1
//     memory.write32(cpu.R()[15], instr);
//     testing::internal::CaptureStderr();
//     arm_cpu.execute(1);
//     std::string stderr_output = testing::internal::GetCapturedStderr();
//     // Should not crash, result is unpredictable, but test for no crash
//     EXPECT_NE(stderr_output.find("exec_arm_undefined"), std::string::npos)
//     << "Expected 'exec_arm_undefined' in stderr, but it was not found.\nStderr was:\n" << stderr_output;
//     EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
// }

// // STR (illegal: both H and S bits set)
// TEST_F(ARMCPUSingleDataTransferTest, STR_Illegal_HS_BitsSet) {
//     cpu.R()[1] = 0x1000;
//     cpu.R()[2] = 0x12345678;
//     cpu.R()[15] = 0x00000000;
//     uint32_t instr = 0xE1C120F0; // STR r2, [r1], #0, H=1, S=1
//     memory.write32(cpu.R()[15], instr);
//      testing::internal::CaptureStderr();
//     arm_cpu.execute(1);
//     std::string stderr_output = testing::internal::GetCapturedStderr();
//     // Should not crash, result is unpredictable, but test for no crash
//     EXPECT_NE(stderr_output.find("exec_arm_undefined"), std::string::npos)
//     << "Expected 'exec_arm_undefined' in stderr, but it was not found.\nStderr was:\n" << stderr_output;
//     EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
// }

// // LDR (illegal: reserved bits set)
// TEST_F(ARMCPUSingleDataTransferTest, LDR_Illegal_ReservedBits) {
//     cpu.R()[1] = 0x1000;
//     cpu.R()[2] = 0;
//     cpu.R()[15] = 0x00000000;
//     // Set reserved bits (bits 7-4 = 0b1111)
//     uint32_t instr = 0xE5912FF0; // LDR r2, [r1, #0xFF0]
//     memory.write32(cpu.R()[15], instr);
//     testing::internal::CaptureStderr();
//     arm_cpu.execute(1);
//     std::string stderr_output = testing::internal::GetCapturedStderr();
//     // Should not crash, result is unpredictable, but test for no crash
//     EXPECT_NE(stderr_output.find("exec_arm_undefined"), std::string::npos)
//     << "Expected 'exec_arm_undefined' in stderr, but it was not found.\nStderr was:\n" << stderr_output;
//     EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
// }

// // STR (illegal: reserved bits set)
// TEST_F(ARMCPUSingleDataTransferTest, STR_Illegal_ReservedBits) {
//     cpu.R()[1] = 0x1000;
//     cpu.R()[2] = 0xCAFEBABE;
//     cpu.R()[15] = 0x00000000;
//     uint32_t instr = 0xE5812FF0; // STR r2, [r1, #0xFF0]
//     memory.write32(cpu.R()[15], instr);
//     testing::internal::CaptureStderr();
//     arm_cpu.execute(1);
//     std::string stderr_output = testing::internal::GetCapturedStderr();
//     // Should not crash, result is unpredictable, but test for no crash
//     EXPECT_NE(stderr_output.find("exec_arm_undefined"), std::string::npos)
//     << "Expected 'exec_arm_undefined' in stderr, but it was not found.\nStderr was:\n" << stderr_output;
//     EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
// }

// // STRH (illegal: reserved bits set)
// TEST_F(ARMCPUSingleDataTransferTest, STRH_Illegal_ReservedBits) {
//     cpu.R()[1] = 0x1000;
//     cpu.R()[2] = 0xFACE;
//     cpu.R()[15] = 0x00000000;
//     uint32_t instr = 0xE1C12FF0; // STRH r2, [r1], #0, reserved bits set
//     memory.write32(cpu.R()[15], instr);
//     testing::internal::CaptureStderr();
//     arm_cpu.execute(1);
//     std::string stderr_output = testing::internal::GetCapturedStderr();
//     // Should not crash, result is unpredictable, but test for no crash
//     EXPECT_NE(stderr_output.find("exec_arm_undefined"), std::string::npos)
//     << "Expected 'exec_arm_undefined' in stderr, but it was not found.\nStderr was:\n" << stderr_output;
//     EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000004);
// }