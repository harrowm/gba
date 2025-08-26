// test_thumb03.cpp - Modern Thumb CPU test fixture for Format 3: Move/compare/add/subtract immediate
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

// ARM Thumb Format 3: Move/compare/add/subtract immediate
// Encoding: 001[Op][Rd][Offset8]
// Instructions: MOV, CMP, ADD, SUB with 8-bit immediate

// MOV Immediate Tests
TEST_F(ThumbCPUTest, MOV_IMM_Basic) {
    // Test case: MOV R0, #1
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("movs r0, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 1u);
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // MOV doesn't affect C
    EXPECT_FALSE(getFlag(CPU::FLAG_V));      // MOV doesn't affect V
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_Max) {
    // Test case: MOV R1, #255 (maximum 8-bit immediate)
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for MOV R1, #255
    write_thumb16(cpu.R()[15], 0x21FF); // MOV R1, #255 (from format03 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 255u);
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_Zero) {
    // Test case: MOV R2, #0 (sets Z flag)
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("movs r2, #0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_42) {
    // Test MOV R3, #42
    setup_registers({});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for MOV R3, #42 (0x232A)
    write_thumb16(cpu.R()[15], 0x232A);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 42u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_127) {
    // Test MOV R4, #127
    setup_registers({});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for MOV R4, #127 (0x247F)
    write_thumb16(cpu.R()[15], 0x247F);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 127u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_255_R5) {
    // Test MOV R5, #0xFF (duplicate coverage for register R5)
    setup_registers({});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for MOV R5, #255 (0x25FF)
    write_thumb16(cpu.R()[15], 0x25FF);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[5], 255u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_Zero_R6) {
    // Test MOV R6, #0x00 (duplicate coverage for register R6)
    setup_registers({});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("movs r6, #0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[6], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_128) {
    // Test MOV R7, #0x80 (128)
    setup_registers({});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for MOV R7, #128 (0x2780)
    write_thumb16(cpu.R()[15], 0x2780);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[7], 128u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, MOV_IMM_FlagPreservation) {
    // Test MOV R7, #0x80 - test NCV flag preservation (only Z and N are affected)
    setup_registers({{7, 0}});
    cpu.R()[15] = 0x00000000;
    
    // Set N, C, V flags initially
    cpu.CPSR() |= CPU::FLAG_N;
    cpu.CPSR() |= CPU::FLAG_C;
    cpu.CPSR() |= CPU::FLAG_V;
    
    // Use hardcoded instruction for MOV R7, #128 (0x2780)
    write_thumb16(cpu.R()[15], 0x2780);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[7], 128u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Z flag updated by MOV result
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // N flag updated by MOV result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // C flag preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // V flag preserved
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// CMP Immediate Tests
TEST_F(ThumbCPUTest, CMP_IMM_Equal) {
    // Test case: CMP equal values (R0 = 5, compare with 5)
    setup_registers({{0, 5}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r0, #5", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 5u);               // Register unchanged
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Equal -> Z set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow -> C set
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, CMP_IMM_Less) {
    // Test case: CMP less than (R1 = 0, compare with 1)
    setup_registers({{1, 0}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r1, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0u);               // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // 0 - 1 = negative
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // Borrow occurred -> C clear
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, CMP_IMM_Greater) {
    // Test case: CMP greater than (R2 = 10, compare with 5)
    setup_registers({{2, 10}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r2, #5", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 10u);              // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));      // 10 - 5 = positive
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow -> C set
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, CMP_IMM_Overflow) {
    // Test case: CMP with signed overflow (most negative - positive)
    setup_registers({{3, 0x80000000}});     // Most negative 32-bit signed int
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for CMP R3, #255
    write_thumb16(cpu.R()[15], 0x2BFF); // CMP R3, #255 (from format03 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0x80000000u);      // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));      // Result is positive
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, CMP_IMM_MaxValue) {
    // Test case: CMP with maximum value (0xFFFFFFFF - 255)
    setup_registers({{4, 0xFFFFFFFF}});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for CMP R4, #255
    write_thumb16(cpu.R()[15], 0x2CFF); // CMP R4, #255 (from format03 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFFu);      // Register unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Large positive result (negative in 2's complement)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// ADD Immediate Tests
TEST_F(ThumbCPUTest, ADD_IMM_Simple) {
    // Test case: Simple addition (R0 = 5, ADD #3)
    setup_registers({{0, 5}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r0, #3", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 8u);               // 5 + 3 = 8
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_IMM_Negative) {
    // Test case: Addition resulting in negative (large + large)
    setup_registers({{1, 0x80000000}});     // Large negative
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r1, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0x80000001u);      // Still negative
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_IMM_Zero) {
    // Test case: Addition resulting in zero
    setup_registers({{2, static_cast<uint32_t>(-100)}});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for ADD R2, #100
    write_thumb16(cpu.R()[15], 0x3264); // ADD R2, #100 (calculated from encoding)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);               // -100 + 100 = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry from unsigned arithmetic
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_IMM_Overflow) {
    // Test case: Addition with signed overflow (positive + positive = negative)
    setup_registers({{3, 0x7FFFFFFF}});     // Maximum positive signed int
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r3, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0x80000000u);      // Overflowed to negative
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ADD_IMM_Carry) {
    // Test case: Addition with carry out
    setup_registers({{4, 0xFFFFFFFF}});     // Maximum unsigned value
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("adds r4, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0u);               // Wrapped to 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero result
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // Carry out
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// SUB Immediate Tests
TEST_F(ThumbCPUTest, SUB_IMM_Simple) {
    // Test case: Simple subtraction (R0 = 10, SUB #3)
    setup_registers({{0, 10}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r0, #3", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 7u);               // 10 - 3 = 7
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow -> C set
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_IMM_Zero) {
    // Test case: Subtraction resulting in zero
    setup_registers({{1, 100}});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for SUB R1, #100
    write_thumb16(cpu.R()[15], 0x3964); // SUB R1, #100 (calculated from encoding)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0u);               // 100 - 100 = 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));       // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_IMM_Negative) {
    // Test case: Subtraction resulting in negative (borrow)
    setup_registers({{2, 5}});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for SUB R2, #10
    write_thumb16(cpu.R()[15], 0x3A0A); // SUB R2, #10 (calculated from encoding)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], static_cast<uint32_t>(-5)); // 5 - 10 = -5
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative result
    EXPECT_FALSE(getFlag(CPU::FLAG_C));      // Borrow occurred -> C clear
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_IMM_Overflow) {
    // Test case: Subtraction with signed overflow (negative - positive = positive)
    setup_registers({{3, 0x80000000}});     // Most negative value
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r3, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0x7FFFFFFFu);      // Overflowed to maximum positive
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));      // Positive result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_TRUE(getFlag(CPU::FLAG_V));       // Signed overflow
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, SUB_IMM_NoBorrow) {
    // Test case: Large subtraction with no borrow
    setup_registers({{4, 0xFFFFFFFF}});     // Maximum value
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("subs r4, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFEu);      // Still large positive
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));       // Negative in 2's complement view
    EXPECT_TRUE(getFlag(CPU::FLAG_C));       // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}
