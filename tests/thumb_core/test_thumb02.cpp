// test_thumb02.cpp - Modern Thumb CPU test fixture for Format 2: Add/subtract
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <initializer_list>

extern "C" {
#include <keystone/keystone.h>
}

class ThumbCPUTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks; // Keystone handle

    ThumbCPUTest() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu), ks(nullptr) {}

    void SetUp() override {
        // Initialize all registers to 0
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        
        // Set Thumb mode (T flag) and User mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
        
        // Initialize Keystone for Thumb mode
        if (ks) ks_close(ks);
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for Thumb mode";
        }
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }

    // Helper: assemble Thumb instruction and write to memory
    bool assemble_and_write_thumb(const std::string& asm_code, uint32_t addr, std::vector<uint8_t>* out_bytes = nullptr) {
        unsigned char* encode = nullptr;
        size_t size, count;
        int err = ks_asm(ks, asm_code.c_str(), addr, &encode, &size, &count);
        if ((ks_err)err != KS_ERR_OK) {
            fprintf(stderr, "Keystone Thumb error: %s\n", ks_strerror((ks_err)err));
            return false;
        }
        for (size_t i = 0; i < size; ++i)
            memory.write8(addr + i, encode[i]);
        if (out_bytes) out_bytes->assign(encode, encode + size);
        ks_free(encode);
        return true;
    }

    // Helper: set up specific register values
    void setup_registers(std::initializer_list<std::pair<int, uint32_t>> reg_values) {
        for (const auto& pair : reg_values) {
            cpu.R()[pair.first] = pair.second;
        }
    }

    // Helper: write Thumb-1 instruction directly to memory (16-bit)
    void write_thumb16(uint32_t addr, uint16_t instruction) {
        memory.write16(addr, instruction);
    }

    // Helper: check flag state
    bool getFlag(uint32_t flag) const {
        return cpu.CPSR() & flag;
    }
};

// ARM Thumb Format 2: Add/subtract
// Encoding: 00011[I][Op][Rn/Offset3][Rs][Rd]
// Instructions: ADD/SUB register, ADD/SUB immediate

// ADD Register Tests
TEST_F(ThumbCPUTest, ADD_REG_Simple) {
    // Test case: Simple addition (ADD R0, R1, R2)
    setup_registers({{1, 5}, {2, 3}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 8u);               // 5 + 3 = 8
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_REG_ZeroResult) {
    // Test case: Addition resulting in zero
    setup_registers({{0, 10}, {3, static_cast<uint32_t>(-10)}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r1, r0, r3", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0u);               // 10 + (-10) = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry out from unsigned addition
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_REG_NegativeResult) {
    // Test case: Addition resulting in negative
    setup_registers({{1, static_cast<uint32_t>(-5)}, {2, static_cast<uint32_t>(-3)}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], static_cast<uint32_t>(-8)); // (-5) + (-3) = -8
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry out from unsigned addition
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_REG_CarryOut) {
    // Test case: Addition with carry out (unsigned overflow)
    setup_registers({{1, 0xFFFFFFFF}, {2, 1}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0u);               // 0xFFFFFFFF + 1 = 0 (wrap)
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry out
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_REG_SignedOverflow) {
    // Test case: Addition with signed overflow (positive + positive = negative)
    setup_registers({{1, 0x7FFFFFFF}, {2, 1}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x80000000u);      // Overflow to negative
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_REG_CarryAndOverflow) {
    // Test case: Addition with both carry and overflow (negative + negative = positive)
    setup_registers({{1, 0x80000000}, {2, 0x80000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0u);               // Two large negatives wrap to 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry out
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_REG_MaxValues) {
    // Test case: Addition with maximum values
    setup_registers({{1, 0xFFFFFFFF}, {2, 0xFFFFFFFF}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu);      // 0xFFFFFFFF + 0xFFFFFFFF = 0xFFFFFFFE (+ carry)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry out
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_REG_SameRegister) {
    // Test case: Addition with same register (Rd = Rs case)
    setup_registers({{1, 15}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r1, r1, r1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 30u);              // 15 + 15 = 30
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// SUB Register Tests
TEST_F(ThumbCPUTest, SUB_REG_Simple) {
    // Test case: Simple subtraction (SUB R0, R1, R2)
    setup_registers({{1, 8}, {2, 3}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 5u);               // 8 - 3 = 5
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow (C=1 means no borrow)
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_REG_ZeroResult) {
    // Test case: Subtraction resulting in zero
    setup_registers({{1, 5}, {2, 5}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0u);               // 5 - 5 = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_REG_NegativeWithBorrow) {
    // Test case: Subtraction resulting in negative (borrow)
    setup_registers({{1, 3}, {2, 8}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], static_cast<uint32_t>(-5)); // 3 - 8 = -5
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // Borrow occurred (C=0)
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_REG_PositiveResult) {
    // Test case: Subtraction with no borrow (positive result)
    setup_registers({{1, 0xFFFFFFFF}, {2, 1}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu);      // Large positive result
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative in 2's complement
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_REG_SignedOverflow) {
    // Test case: Subtraction with signed overflow (negative - positive = positive)
    setup_registers({{1, 0x80000000}, {2, 1}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x7FFFFFFFu);      // Overflow to large positive
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_REG_BorrowNoOverflow) {
    // Test case: Subtraction with borrow and no overflow
    setup_registers({{1, 5}, {2, 10}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], static_cast<uint32_t>(-5)); // 5 - 10 = -5
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // Borrow occurred
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_REG_MaxValues) {
    // Test case: Subtraction with maximum values
    setup_registers({{1, 0xFFFFFFFF}, {2, 0xFFFFFFFF}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0u);               // 0xFFFFFFFF - 0xFFFFFFFF = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_REG_SameRegister) {
    // Test case: Subtraction with same register (Rd = Rs case)
    setup_registers({{1, 20}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r1, r1, r1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0u);               // 20 - 20 = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// ADD/SUB Immediate Tests
TEST_F(ThumbCPUTest, ADD_IMM) {
    // Test case: ADD immediate (ADD R0, R1, #2)
    setup_registers({{1, 5}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, r1, #2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 7u);               // 5 + 2 = 7
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_IMM) {
    // Test case: SUB immediate (SUB R0, R1, #2)
    setup_registers({{1, 8}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, r1, #2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 6u);               // 8 - 2 = 6
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}
