#include <gtest/gtest.h>
#include <keystone/keystone.h>
#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "arm_cpu.h"
#include "thumb_cpu.h"

// Test fixture for Thumb Format 5 Hi register operations/braTEST_F(ThumbCPUTest5, MOV_LowToHigh) {
    // Test case: MOV R8, R0 (low to high register)
    setup_registers({{0, 0x12345678}, {8, 0x00000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("mov r8, r0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[8], 0x12345678u); // R8 gets R0's value
    EXPECT_EQ(cpu.R()[0], 0x12345678u); // R0 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}
class ThumbCPUTest5 : public ::testing::Test {
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

// ARM Thumb Format 5: Hi register operations/branch exchange
// Encoding: 010001[Op][H1][H2][Rs/Hs][Rd/Hd]
// Instructions: ADD Rd, Rs; CMP Rd, Rs; MOV Rd, Rs; BX Rs

// ADD Hi Register Tests
TEST_F(ThumbCPUTest5, ADD_LowPlusHigh) {
    // Test case: ADD R0, R8 (low + high register)
    setup_registers({{0, 0x12345678}, {8, 0x87654321}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("add r0, r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x99999999u); // 0x12345678 + 0x87654321
    EXPECT_EQ(cpu.R()[8], 0x87654321u); // R8 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_HighPlusLow) {
    // Test case: ADD R8, R0 (high + low register)
    setup_registers({{8, 0x11111111}, {0, 0x22222222}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("add r8, r0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[8], 0x33333333u); // 0x11111111 + 0x22222222
    EXPECT_EQ(cpu.R()[0], 0x22222222u); // R0 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_HighPlusHigh) {
    // Test case: ADD R8, R9 (high + high register)
    setup_registers({{8, 0xAAAAAAAA}, {9, 0x55555555}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("add r8, r9", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[8], 0xFFFFFFFFu); // 0xAAAAAAAA + 0x55555555
    EXPECT_EQ(cpu.R()[9], 0x55555555u); // R9 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_WithPC) {
    // Test case: ADD R0, PC (PC is R15, high register)
    setup_registers({{0, 0x00000100}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("add r0, pc", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // PC+4 alignment in Thumb mode (PC is read as current PC + 4)
    EXPECT_EQ(cpu.R()[0], 0x00000104u); // 0x100 + (0x0 + 4)
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, ADD_ZeroValues) {
    // Test case: ADD with zero values
    setup_registers({{0, 0x00000000}, {8, 0x00000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("add r0, r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x00000000u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// CMP Hi Register Tests
TEST_F(ThumbCPUTest5, CMP_Equal) {
    // Test case: CMP R0, R8 (equal values)
    setup_registers({{0, 0x12345678}, {8, 0x12345678}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r0, r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));  // Equal values set Z
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Result is zero (positive)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V)); // No overflow
    EXPECT_EQ(cpu.R()[0], 0x12345678u); // R0 unchanged
    EXPECT_EQ(cpu.R()[8], 0x12345678u); // R8 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_FirstGreater) {
    // Test case: CMP R8, R0 (first greater than second)
    setup_registers({{8, 0x80000000}, {0, 0x12345678}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r8, r0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Not equal
    EXPECT_FALSE(getFlag(CPU::FLAG_N)); // Result positive (unsigned comparison)
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // Overflow detected by CPU implementation
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_FirstLess) {
    // Test case: CMP R0, R8 (first less than second)
    setup_registers({{0, 0x12345678}, {8, 0x80000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r0, r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Not equal
    EXPECT_TRUE(getFlag(CPU::FLAG_N));  // Result negative (borrow occurred)
    EXPECT_FALSE(getFlag(CPU::FLAG_C)); // Borrow occurred
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // Overflow detected by CPU implementation
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_WithPC) {
    // Test case: CMP R0, PC
    setup_registers({{0, 0x00000004}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r0, pc", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // PC+4 alignment: CMP 0x4, (0x0 + 4) = CMP 0x4, 0x4
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));  // Equal
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));  // No borrow
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

// MOV Hi Register Tests
TEST_F(ThumbCPUTest5, MOV_LowToHigh) {
    // Test case: MOV R8, R0 (low to high register)
    setup_registers({{0, 0x12345678}, {8, 0xDEADBEEF}});
    cpu.R()[15] = 0x00000000;
    
    write_thumb16(cpu.R()[15], 0x4680); // MOV R8, R0
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[8], 0x12345678u); // R8 gets R0's value
    EXPECT_EQ(cpu.R()[0], 0x12345678u); // R0 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_HighToLow) {
    // Test case: MOV R0, R8 (high to low register)
    setup_registers({{8, 0x87654321}, {0, 0x11111111}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("mov r0, r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x87654321u); // R0 gets R8's value
    EXPECT_EQ(cpu.R()[8], 0x87654321u); // R8 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_HighToHigh) {
    // Test case: MOV R8, R9 (high to high register)
    setup_registers({{9, 0xCAFEBABE}, {8, 0x00000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("mov r8, r9", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[8], 0xCAFEBABEu); // R8 gets R9's value
    EXPECT_EQ(cpu.R()[9], 0xCAFEBABEu); // R9 unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_PCToRegister) {
    // Test case: MOV R0, PC
    setup_registers({{0, 0x11111111}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("mov r0, pc", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // PC+4 alignment: R0 gets PC+4
    EXPECT_EQ(cpu.R()[0], 0x00000004u); // PC (0x0) + 4
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_ToPC) {
    // Test case: MOV PC, R0 (branch to address in R0)
    setup_registers({{0, 0x00000200}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("mov pc, r0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[15], 0x00000200u); // PC set to R0's value
    EXPECT_TRUE(getFlag(CPU::FLAG_T));   // Still in Thumb mode
}

// BX Branch Exchange Tests
TEST_F(ThumbCPUTest5, BX_ToARM) {
    // Test case: BX R0 (branch to ARM mode - bit 0 clear)
    setup_registers({{0, 0x00000200}}); // ARM address (even)
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assemble_and_write_thumb("bx r0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[15], 0x00000200u); // PC set to target address
    EXPECT_FALSE(getFlag(CPU::FLAG_T));  // Switched to ARM mode (T flag clear)
}

TEST_F(ThumbCPUTest5, BX_ToThumb) {
    // Test case: BX R1 (branch to Thumb mode - bit 0 set)
    setup_registers({{1, 0x00000301}}); // Thumb address (odd)
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = 0; // Start in ARM mode (T flag clear)
    
    // Use ARM encoding for BX since we're starting in ARM mode
    memory.write32(cpu.R()[15], 0xE12FFF11); // BX R1 (ARM encoding)
    cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[15], 0x00000300u); // PC set to target with bit 0 cleared
    EXPECT_TRUE(getFlag(CPU::FLAG_T));   // Switched to Thumb mode (T flag set)
}

TEST_F(ThumbCPUTest5, BX_HighRegister) {
    // Test case: BX R8 (branch with high register)
    setup_registers({{8, 0x00000400}}); // ARM address
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assemble_and_write_thumb("bx r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[15], 0x00000400u); // PC set to R8's value
    EXPECT_FALSE(getFlag(CPU::FLAG_T));  // Switched to ARM mode
}

TEST_F(ThumbCPUTest5, BX_ThumbToThumb) {
    // Test case: BX with Thumb address while in Thumb mode
    setup_registers({{2, 0x00000501}}); // Thumb address (odd)
    cpu.R()[15] = 0x00000000;
    cpu.CPSR() = CPU::FLAG_T; // Start in Thumb mode
    
    ASSERT_TRUE(assemble_and_write_thumb("bx r2", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[15], 0x00000500u); // PC set with bit 0 cleared
    EXPECT_TRUE(getFlag(CPU::FLAG_T));   // Stay in Thumb mode
}

// Edge Cases and Boundary Conditions
TEST_F(ThumbCPUTest5, ADD_Overflow) {
    // Test case: ADD causing 32-bit overflow
    setup_registers({{0, 0xFFFFFFFF}, {8, 0x00000001}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("add r0, r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x00000000u); // Wraps to 0
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, CMP_Overflow) {
    // Test case: CMP with signed overflow
    setup_registers({{0, 0x7FFFFFFF}, {8, 0x80000000}}); // Max positive - max negative
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("cmp r0, r8", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_FALSE(getFlag(CPU::FLAG_Z)); // Not equal
    EXPECT_TRUE(getFlag(CPU::FLAG_V));  // Signed overflow occurred
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest5, MOV_LR) {
    // Test case: MOV involving LR (R14)
    setup_registers({{14, 0xDEADBEEF}, {0, 0x00000000}});
    cpu.R()[15] = 0x00000000;
    
    ASSERT_TRUE(assemble_and_write_thumb("mov r0, lr", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xDEADBEEFu); // R0 gets LR's value
    EXPECT_EQ(cpu.R()[14], 0xDEADBEEFu); // LR unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}
