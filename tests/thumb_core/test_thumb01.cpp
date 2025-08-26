// test_thumb01.cpp - Modern Thumb CPU test fixture for Format 1: Move shifted register
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
        // Use KS_MODE_THUMB + KS_MODE_V8 to ensure we get 16-bit Thumb instructions
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for Thumb mode";
        }
        
        // Try to set syntax to prefer 16-bit Thumb instructions
        if (ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL) != KS_ERR_OK) {
            // This might not be necessary, but let's try
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

// ARM Thumb Format 1: Move shifted register
// Encoding: 000[op][offset5][Rs][Rd]
// Instructions: LSL, LSR, ASR

TEST_F(ThumbCPUTest, LSL_Basic) {
    // Test case: LSL R0, R0, #2 (shift 0b1 left by 2 positions)
    setup_registers({{0, 0b1}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("lsls r0, r0, #2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], static_cast<unsigned int>(0b100));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u); // Thumb instructions are 2 bytes
}

TEST_F(ThumbCPUTest, LSL_CarryOut) {
    // Test case: LSL with carry out (shift 0xC0000000 left by 1)
    setup_registers({{1, 0xC0000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("lsls r1, r1, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0x80000000u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));  // Result is negative
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // Carry out from bit 31
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSL_ZeroResult) {
    // Test case: LSL resulting in zero
    setup_registers({{2, 0x80000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("lsls r2, r2, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));  // Not negative
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSL_ShiftByZero) {
    // Test case: Shift by 0 (special case - no operation, carry unaffected)
    setup_registers({{3, 0xABCD}});
    cpu.CPSR() |= CPU::FLAG_C; // Pre-set carry flag
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("movs r3, r3", cpu.R()[15])); // LSL #0 is MOV in UAL
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0xABCDu);      // Value unchanged
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry flag not affected
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSL_MaxShift) {
    // Test case: Maximum shift amount (31)
    setup_registers({{4, 0b11}});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for LSL #31 since Keystone may not support it
    write_thumb16(cpu.R()[15], 0x07E4); // LSL R4, #31 (based on format01 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0x80000000u);  // 0b11 << 31, bit 1 -> bit 32 (carry), bit 0 -> bit 31
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));   // Result is negative (bit 31 set)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Bit 0 of original (1) shifted out
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_Basic) {
    // Test case: LSR R0, R0, #2 (logical shift right)
    setup_registers({{0, 0b1100}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("lsrs r0, r0, #2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], static_cast<unsigned int>(0b11));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_CarryOut) {
    // Test case: LSR with carry out
    setup_registers({{1, 0b101}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("lsrs r1, r1, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], static_cast<unsigned int>(0b10));
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from bit 0
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_ZeroResult) {
    // Test case: LSR resulting in zero
    setup_registers({{2, 0x1}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("lsrs r2, r2, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from LSB
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_ShiftByZero) {
    // Test case: Shift by 0 (special case, treated as LSR #32)
    setup_registers({{3, 0x80000000}});
    cpu.CPSR() &= ~CPU::FLAG_C; // Pre-clear carry flag
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for LSR #0 (treated as LSR #32)
    write_thumb16(cpu.R()[15], 0x081B); // LSR R3, #0 -> LSR R3, #32 (from format01 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0u);           // LSR #32 results in 0
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Bit 31 was 1
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, LSR_MaxShift) {
    // Test case: Maximum explicit shift amount (31)
    setup_registers({{4, 0xFFFFFFFF}});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for LSR #31
    write_thumb16(cpu.R()[15], 0x0FE4); // LSR R4, #31 (from format01 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 1u);           // 0xFFFFFFFF >> 31 = 1
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Bit 30 was 1
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_Basic) {
    // Test case: ASR (arithmetic shift right) - positive number
    setup_registers({{0, 0x80}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("asrs r0, r0, #2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x20u);        // 0x80 >> 2 = 0x20
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_NegativeNumber) {
    // Test case: ASR with negative number (sign extension)
    setup_registers({{1, 0x80000000}});  // Most negative 32-bit number
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("asrs r1, r1, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0xC0000000u);  // Sign bit extended
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));   // Still negative
    EXPECT_FALSE(getFlag(CPU::FLAG_C));  // No carry from bit 0
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_CarryOut) {
    // Test case: ASR with carry out from negative number
    setup_registers({{2, 0x80000001}});  // Negative number with LSB set
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("asrs r2, r2, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0xC0000000u);  // Sign extended with carry
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));   // Negative result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from LSB
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_ZeroResult) {
    // Test case: ASR resulting in zero
    setup_registers({{2, 0x1}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("asrs r2, r2, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));   // Zero flag set
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));   // Carry out from LSB
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_ShiftByZero) {
    // Test case: Shift by 0 (special case, treated as ASR #32)
    setup_registers({{3, 0x80000000}});
    cpu.CPSR() &= ~CPU::FLAG_C; // Pre-clear carry flag
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for ASR #0 (treated as ASR #32)
    write_thumb16(cpu.R()[15], 0x101B); // ASR R3, #0 -> ASR R3, #32 (from format01 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0xFFFFFFFFu); // ASR #32 of negative = all 1s (sign extended)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));  // Negative result
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // Bit 31 was 1
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest, ASR_MaxShift) {
    // Test case: Maximum shift amount (31)
    setup_registers({{4, 0xFFFFFFFF}});
    cpu.R()[15] = 0x00000000;
    
    // Use hardcoded instruction for ASR #31
    write_thumb16(cpu.R()[15], 0x17E4); // ASR R4, #31 (from format01 original)
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0xFFFFFFFFu); // Sign-extended (all 1s remain)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N)); // Negative result
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // Bit 0 was 1
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// Test different source and destination registers
TEST_F(ThumbCPUTest, LSL_DifferentRegisters) {
    // Test case: LSL with different source and destination (Rd != Rs)
    setup_registers({{3, 0x5}, {4, 0}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("lsls r4, r3, #1", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0x5u);         // Source unchanged
    EXPECT_EQ(cpu.R()[4], 0xAu);         // Destination = 0x5 << 1
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}
