// test_arm_data_processing_pc.cpp
// Tests for PC+8 pipeline offset in ARM data processing instructions
// 
// ARM7TDMI Pipeline: When R15 (PC) is used as an operand in data processing instructions,
// the value read is PC+8 (current instruction address + 8 bytes ahead)

#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

extern "C" {
#include <keystone/keystone.h>
}

class ARMDataProcessingPCTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;
    ks_engine* ks; // Keystone handle

    ARMDataProcessingPCTest() : memory(true), cpu(memory, interrupts), arm_cpu(cpu) {}

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

// ============================================================================
// ADD Tests (Already Fixed - these verify the existing implementation)
// ============================================================================

TEST_F(ARMDataProcessingPCTest, ADD_IMM_WithPC) {
    // Test: ADD R0, PC, #4
    // PC at instruction = 0x1000
    // Expected: R0 = (0x1000 + 8) + 4 = 0x100C
    cpu.R()[15] = 0x00001000;
    assemble_and_write("add r0, pc, #4", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x0000100Cu) << "ADD R0, PC, #4 should use PC+8";
    EXPECT_EQ(cpu.R()[15], 0x00001004u) << "PC should advance by 4";
}

TEST_F(ARMDataProcessingPCTest, ADD_REG_WithPC_AsRn) {
    // Test: ADD R0, PC, R1
    // PC at instruction = 0x2000, R1 = 0x100
    // Expected: R0 = (0x2000 + 8) + 0x100 = 0x2108
    cpu.R()[15] = 0x00002000;
    cpu.R()[1] = 0x00000100;
    assemble_and_write("add r0, pc, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x00002108u) << "ADD R0, PC, R1 should use PC+8 for Rn";
}

TEST_F(ARMDataProcessingPCTest, ADD_REG_WithPC_AsRm) {
    // Test: ADD R0, R1, PC
    // PC at instruction = 0x3000, R1 = 0x200
    // Expected: R0 = 0x200 + (0x3000 + 8) = 0x3208
    cpu.R()[15] = 0x00003000;
    cpu.R()[1] = 0x00000200;
    assemble_and_write("add r0, r1, pc", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x00003208u) << "ADD R0, R1, PC should use PC+8 for Rm";
}

// ============================================================================
// SUB Tests (First function to fix)
// ============================================================================

TEST_F(ARMDataProcessingPCTest, SUB_IMM_WithPC) {
    // Test: SUB R0, PC, #4
    // PC at instruction = 0x1000
    // Expected: R0 = (0x1000 + 8) - 4 = 0x1004
    cpu.R()[15] = 0x00001000;
    assemble_and_write("sub r0, pc, #4", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x00001004u) << "SUB R0, PC, #4 should use PC+8";
    EXPECT_EQ(cpu.R()[15], 0x00001004u) << "PC should advance by 4";
}

TEST_F(ARMDataProcessingPCTest, SUB_REG_WithPC_AsRn) {
    // Test: SUB R0, PC, R1
    // PC at instruction = 0x2000, R1 = 0x100
    // Expected: R0 = (0x2000 + 8) - 0x100 = 0x1F08
    cpu.R()[15] = 0x00002000;
    cpu.R()[1] = 0x00000100;
    assemble_and_write("sub r0, pc, r1", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x00001F08u) << "SUB R0, PC, R1 should use PC+8 for Rn";
}

TEST_F(ARMDataProcessingPCTest, SUB_REG_WithPC_AsRm) {
    // Test: SUB R0, R1, PC
    // PC at instruction = 0x3000, R1 = 0x4000
    // Expected: R0 = 0x4000 - (0x3000 + 8) = 0xFF8 (0x4000 - 0x3008)
    cpu.R()[15] = 0x00003000;
    cpu.R()[1] = 0x00004000;
    assemble_and_write("sub r0, r1, pc", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x00000FF8u) << "SUB R0, R1, PC should use PC+8 for Rm";
}
