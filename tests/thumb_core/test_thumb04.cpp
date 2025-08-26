#include <gtest/gtest.h>
#include <keystone/keystone.h>
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "arm_cpu.h"
#include "thumb_cpu.h"

// Test fixture for Thumb Format 4 ALU operations
class ThumbCPUTest4 : public ::testing::Test {
protected:
    GBA gba{true}; // Test mode
    CPU& cpu = gba.getCPU();
    Memory& memory = cpu.getMemory();
    ThumbCPU& thumb_cpu = cpu.getThumbCPU();
    ks_engine* ks = nullptr;

    void SetUp() override {
        // Reset CPU state
        cpu.R().fill(0);
        cpu.CPSR() = CPU::FLAG_T; // Set Thumb mode
        cpu.R()[15] = 0x00000000; // Reset PC

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
        
        if (size != 2 || count != 1) {
            fprintf(stderr, "Expected 2-byte Thumb instruction, got %zu bytes\n", size);
            if (encode) ks_free(encode);
            return false;
        }
        
        uint16_t instruction = encode[0] | (encode[1] << 8);
        memory.write16(addr, instruction);
        
        if (out_bytes) {
            out_bytes->assign(encode, encode + size);
        }
        
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

// ARM Thumb Format 4: ALU operations
// Encoding: 010000[Op][Rs][Rd]
// Instructions: AND, EOR, LSL, LSR, ASR, ADC, SBC, ROR, TST, NEG, CMP, CMN, ORR, MUL, BIC, MVN

// AND Tests
TEST_F(ThumbCPUTest4, AND_Basic) {
    // Test case: Basic AND operation
    setup_registers({{0, 0xFF00FF00}, {1, 0xF0F0F0F0}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4008); // AND R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xF000F000u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N)); // Result is negative
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // C is unaffected by AND
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, AND_ResultZero) {
    // Test case: AND resulting in zero
    setup_registers({{2, 0xAAAAAAAA}, {3, 0x55555555}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x401A); // AND R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// EOR (XOR) Tests  
TEST_F(ThumbCPUTest4, EOR_Basic) {
    // Test case: Basic XOR operation
    setup_registers({{0, 0xFF00FF00}, {1, 0xF0F0F0F0}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4048); // EOR R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x0FF00FF0u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, EOR_SelfZero) {
    // Test case: XOR with itself (should result in zero)
    setup_registers({{4, 0x12345678}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4064); // EOR R4, R4
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// LSL Tests
TEST_F(ThumbCPUTest4, LSL_Basic) {
    // Test case: Basic logical shift left (no carry out)
    setup_registers({{0, 0x00000001}, {1, 2}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4088); // LSL R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x00000004u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, LSL_CarryOut) {
    // Test case: LSL with carry out
    setup_registers({{2, 0x80000000}, {3, 1}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x409A); // LSL R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// LSR Tests
TEST_F(ThumbCPUTest4, LSR_Basic) {
    // Test case: Basic logical shift right (no carry out)
    setup_registers({{0, 0x00000010}, {1, 2}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x40C8); // LSR R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x00000004u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, LSR_CarryOut) {
    // Test case: LSR with carry out
    setup_registers({{2, 0x00000001}, {3, 1}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x40DA); // LSR R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// ASR Tests
TEST_F(ThumbCPUTest4, ASR_Basic) {
    // Test case: Basic arithmetic shift right (positive number, no carry out)
    setup_registers({{0, 0x00000010}, {1, 2}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4108); // ASR R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x00000004u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, ASR_Negative) {
    // Test case: ASR with negative number (sign extension)
    setup_registers({{2, 0x80000000}, {3, 4}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x411A); // ASR R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0xF8000000u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// TST Tests
TEST_F(ThumbCPUTest4, TST_NonZero) {
    // Test case: TST with non-zero result
    setup_registers({{0, 0xFF00FF00}, {1, 0xF0F0F0F0}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4208); // TST R0, R1
    thumb_cpu.execute(1);
    
    // TST doesn't modify the destination register, only flags
    EXPECT_EQ(cpu.R()[0], 0xFF00FF00u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, TST_Zero) {
    // Test case: TST with zero result
    setup_registers({{2, 0xAAAAAAAA}, {3, 0x55555555}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x421A); // TST R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0xAAAAAAAAu);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// NEG Tests
TEST_F(ThumbCPUTest4, NEG_Basic) {
    // Test case: Basic negation
    setup_registers({{0, 5}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4240); // NEG R0, R0
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFBu); // -5 in two's complement
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // Borrow occurred
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, NEG_Zero) {
    // Test case: Negation of zero
    setup_registers({{1, 0}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4249); // NEG R1, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // No borrow for 0-0
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// CMP Tests
TEST_F(ThumbCPUTest4, CMP_Equal) {
    // Test case: CMP with equal values
    setup_registers({{0, 10}, {1, 10}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4288); // CMP R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 10u); // CMP doesn't modify registers
    EXPECT_EQ(cpu.R()[1], 10u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, CMP_Less) {
    // Test case: CMP with first < second
    setup_registers({{2, 5}, {3, 10}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x429A); // CMP R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 5u);
    EXPECT_EQ(cpu.R()[3], 10u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // Borrow occurred
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// ORR Tests
TEST_F(ThumbCPUTest4, ORR_Basic) {
    // Test case: Basic OR operation
    setup_registers({{0, 0x00FF00FF}, {1, 0xFF0000FF}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4308); // ORR R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xFFFF00FFu);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// MUL Tests
TEST_F(ThumbCPUTest4, MUL_Basic) {
    // Test case: Basic multiplication
    setup_registers({{0, 6}, {1, 7}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4348); // MUL R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 42u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, MUL_Zero) {
    // Test case: Multiplication resulting in zero
    setup_registers({{2, 0}, {3, 999}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x435A); // MUL R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 0u);
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// BIC Tests
TEST_F(ThumbCPUTest4, BIC_Basic) {
    // Test case: Basic bit clear operation
    setup_registers({{0, 0xFFFFFFFF}, {1, 0xF0F0F0F0}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4388); // BIC R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x0F0F0F0Fu);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// MVN Tests
TEST_F(ThumbCPUTest4, MVN_Basic) {
    // Test case: Basic move NOT operation
    setup_registers({{0, 0xF0F0F0F0}, {1, 0}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x43C8); // MVN R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFFu);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// ADC Tests
TEST_F(ThumbCPUTest4, ADC_NoCarry) {
    // Test case: Add with carry, no previous carry
    setup_registers({{0, 5}, {1, 3}});
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() &= ~CPU::FLAG_C; // Clear carry flag
    
    write_thumb16(cpu.R()[15], 0x4148); // ADC R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 8u); // 5 + 3 + 0 (no carry)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, ADC_WithCarry) {
    // Test case: Add with carry, previous carry set
    setup_registers({{2, 5}, {3, 3}});
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() |= CPU::FLAG_C; // Set carry flag
    
    write_thumb16(cpu.R()[15], 0x415A); // ADC R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 9u); // 5 + 3 + 1 (carry)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// SBC Tests  
TEST_F(ThumbCPUTest4, SBC_NoBorrow) {
    // Test case: Subtract with carry, no borrow
    setup_registers({{0, 10}, {1, 3}});
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() |= CPU::FLAG_C; // Set carry flag (no borrow)
    
    write_thumb16(cpu.R()[15], 0x4188); // SBC R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 7u); // 10 - 3 - 0 (no borrow)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest4, SBC_WithBorrow) {
    // Test case: Subtract with carry, with borrow
    setup_registers({{2, 5}, {3, 3}});
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() &= ~CPU::FLAG_C; // Clear carry flag (borrow)
    
    write_thumb16(cpu.R()[15], 0x419A); // SBC R2, R3
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[2], 1u); // 5 - 3 - 1 (borrow)
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// ROR Tests
TEST_F(ThumbCPUTest4, ROR_Basic) {
    // Test case: Basic rotate right (carry flag set from rotated bit)
    setup_registers({{0, 0x80000001}, {1, 1}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x41C8); // ROR R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xC0000000u); // Bit 0 rotated to bit 31
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N)); // Result is negative
    EXPECT_TRUE(getFlag(CPU::FLAG_C)); // Bit rotated out to carry
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// CMN Tests
TEST_F(ThumbCPUTest4, CMN_Basic) {
    // Test case: Compare negative (CMN) - equivalent to ADD for flags
    setup_registers({{0, 5}, {1, 7}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x42C8); // CMN R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 5u); // CMN doesn't modify registers
    EXPECT_EQ(cpu.R()[1], 7u);
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // 5 + 7 = 12, not zero
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Result is positive
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // No carry
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}
