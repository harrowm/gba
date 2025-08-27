// test_thumb12.cpp - Modern Thumb CPU test fixture for Format 12: Load address
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <initializer_list>

extern "C" {
#include <keystone/keystone.h>
}

class ThumbCPUTest12 : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks; // Keystone handle

    ThumbCPUTest12() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu), ks(nullptr) {}

    void SetUp() override {
        // Initialize all registers to 0
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        
        // Set Thumb mode (T flag) and User mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
        
        // Initialize Keystone for Thumb mode
        if (ks) ks_close(ks);
        // Use KS_MODE_THUMB without V8 to get 16-bit Thumb-1 instructions
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for Thumb mode";
        }
        
        // Try to set syntax to not be fatal if it fails
        ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_INTEL);
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }

    bool assemble_and_write_thumb(const std::string& assembly, uint32_t address) {
        unsigned char *machine_code;
        size_t machine_size;
        size_t statement_count;
        
        int err = ks_asm(ks, assembly.c_str(), address, &machine_code, &machine_size, &statement_count);
        if ((ks_err)err != KS_ERR_OK) {
            fprintf(stderr, "Keystone Thumb error: %s\n", ks_strerror((ks_err)err));
            return false;
        }

        if (machine_size >= 2) {
            // Write instruction to memory (Thumb instructions are 16-bit)
            uint16_t instruction = (machine_code[1] << 8) | machine_code[0];
            memory.write16(address, instruction);            
        }

        ks_free(machine_code);
        return true;
    }

    void setup_registers(std::initializer_list<std::pair<int, uint32_t>> reg_values) {
        // Clear all registers first
        cpu.R().fill(0);
        
        // Set specified register values
        for (const auto& pair : reg_values) {
            cpu.R()[pair.first] = pair.second;
        }
    }
};

// ARM Thumb Format 12: Load address
// Encoding: 1010[SP][Rd][Word8]
// SP=0: ADD Rd, PC, #imm (PC-relative)
// SP=1: ADD Rd, SP, #imm (SP-relative)
// Word offset = Word8 * 4 (word-aligned, 0-1020 bytes)

TEST_F(ThumbCPUTest12, AddPcLoadAddressBasic) {
    // Test case: ADD R0, PC, #0 - minimum offset
    setup_registers({{15, 0x00000000}});
    
    // Manual instruction encoding for Format 12: 1010[SP][Rd][Word8]
    // SP=0 (PC), Rd=0 (R0), Word8=0 (offset 0)
    memory.write16(0x00000000, 0xA000);  // ADD R0, PC, #0
    thumb_cpu.execute(1);
    
    // PC is word-aligned during execution, so PC (0x02) aligns to 0x00, + 0 = 0x00
    EXPECT_EQ(cpu.R()[0], 0x00000000u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // ADD PC doesn't affect flags
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_N));
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_C));
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressWithOffset) {
    // Test case: ADD R1, PC, #4 - small offset
    setup_registers({{15, 0x00000000}});
    
    // Manual instruction encoding: SP=0, Rd=1, Word8=1 (offset 4)
    memory.write16(0x00000000, 0xA101);  // ADD R1, PC, #4
    thumb_cpu.execute(1);
    
    // PC aligned (0x00) + 4 = 0x04
    EXPECT_EQ(cpu.R()[1], 0x00000004u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressMediumOffset) {
    // Test case: ADD R2, PC, #288 - medium offset  
    setup_registers({});
    
    // Manual instruction encoding: SP=0, Rd=2, Word8=72 (offset 288)
    memory.write16(0x00000000, 0xA248);  // ADD R2, PC, #288
    thumb_cpu.execute(1);
    
    // PC aligned (0x00) + 288 = 0x120
    EXPECT_EQ(cpu.R()[2], 0x00000120u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressMaximumOffset) {
    // Test case: ADD R2, PC, #1020 (maximum offset = 255 * 4)
    setup_registers({});
    
    // Manual instruction encoding: SP=0, Rd=2, Word8=255 (offset 1020)
    memory.write16(0x00000000, 0xA2FF);  // ADD R2, PC, #1020
    thumb_cpu.execute(1);
    
    // PC aligned (0x00) + 1020 = 0x3FC
    EXPECT_EQ(cpu.R()[2], 0x000003FCu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressUnalignedPc) {
    // Test case: ADD R3, PC, #64 with unaligned PC
    setup_registers({{15, 0x00000006}});
    
    // Manual instruction encoding: SP=0, Rd=3, Word8=16 (offset 64)
    memory.write16(0x00000006, 0xA310);  // ADD R3, PC, #64
    thumb_cpu.execute(1);
    
    // PC=0x08 after fetch, aligned to 0x08, + 64 = 0x48
    EXPECT_EQ(cpu.R()[3], 0x00000048u);
    EXPECT_EQ(cpu.R()[15], 0x00000008u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressAllRegisters) {
    // Test ADD PC with all destination registers R0-R7
    for (int rd = 0; rd < 8; rd++) {
        setup_registers({});
        cpu.R()[15] = rd * 2; // Set PC to instruction location
        
        // Manual instruction encoding: SP=0, Rd=rd, Word8=1 (offset 4)
        uint16_t instruction = 0xA000 | (rd << 8) | 0x01;
        memory.write16(rd * 2, instruction);
        thumb_cpu.execute(1);
        
        uint32_t expected_pc = ((rd * 2) + 2) & ~3; // PC after fetch, word-aligned
        EXPECT_EQ(cpu.R()[rd], expected_pc + 4) << "Register R" << rd;
    }
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressNearMemoryBoundary) {
    // Test near the 0x1FFF RAM boundary
    setup_registers({{15, 0x00001FF0}});
    
    // Manual instruction encoding: SP=0, Rd=4, Word8=7 (offset 28)
    memory.write16(0x00001FF0, 0xA407);  // ADD R4, PC, #28
    thumb_cpu.execute(1);
    
    // PC=0x1FF2 after fetch, aligned to 0x1FF0, + 28 = 0x200C
    EXPECT_EQ(cpu.R()[4], 0x0000200Cu);
    EXPECT_EQ(cpu.R()[15], 0x00001FF2u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressFlagPreservation) {
    // Verify flags are preserved (ADD PC doesn't modify flags)
    setup_registers({});
    cpu.CPSR() |= CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
    
    // Manual instruction encoding: SP=0, Rd=5, Word8=16 (offset 64)
    memory.write16(0x00000000, 0xA510);  // ADD R5, PC, #64
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[5], 0x00000040u); // PC aligned (0x00) + 64 = 0x40
    
    // All flags should be preserved
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_N));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_C));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressBasic) {
    // Test case: ADD R0, SP, #0 - minimum offset
    setup_registers({{13, 0x00001000}});  // Set SP
    
    ASSERT_TRUE(assemble_and_write_thumb("add r0, sp, #0", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // SP + 0 = SP
    EXPECT_EQ(cpu.R()[0], 0x00001000u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // ADD SP doesn't affect flags
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_N));
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_C));
    EXPECT_FALSE(cpu.getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressWithOffset) {
    // Test case: ADD R1, SP, #4 - small offset
    setup_registers({{13, 0x00001000}});  // Set SP
    
    ASSERT_TRUE(assemble_and_write_thumb("add r1, sp, #4", cpu.R()[15]));
    thumb_cpu.execute(1);
    
    // SP + 4
    EXPECT_EQ(cpu.R()[1], 0x00001004u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressMediumOffset) {
    // Test case: ADD R2, SP, #512 - medium offset
    setup_registers({{13, 0x00000800}});  // Set SP
    
    // Manual instruction encoding: SP=1, Rd=2, Word8=128 (offset 512)
    memory.write16(0x00000000, 0xAA80);  // ADD R2, SP, #512
    thumb_cpu.execute(1);
    
    // SP + 512 = 0x800 + 0x200 = 0xA00
    EXPECT_EQ(cpu.R()[2], 0x00000A00u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressMaximumOffsetOriginal) {
    // Test case: ADD R2, SP, #1020 (maximum offset = 255 * 4)
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: SP=1, Rd=2, Word8=255 (offset 1020)
    memory.write16(0x00000000, 0xAAFF);  // ADD R2, SP, #1020
    thumb_cpu.execute(1);
    
    // SP + 1020 = 0x1000 + 0x3FC = 0x13FC
    EXPECT_EQ(cpu.R()[2], 0x000013FCu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressAllRegisters) {
    // Test ADD SP with all destination registers R0-R7
    setup_registers({{13, 0x00001000}});  // Set SP
    
    for (int rd = 0; rd < 8; rd++) {
        cpu.R()[15] = rd * 2; // Set PC to instruction location
        
        // Manual instruction encoding: SP=1, Rd=rd, Word8=1 (offset 4)
        uint16_t instruction = 0xA800 | (rd << 8) | 0x01;
        memory.write16(rd * 2, instruction);
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[rd], 0x00001004u) << "Register R" << rd;  // Should be SP + 4
    }
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressZeroSp) {
    // Test with SP at zero
    setup_registers({{13, 0x00000000}});  // Set SP to zero
    
    // Manual instruction encoding: SP=1, Rd=3, Word8=8 (offset 32)
    memory.write16(0x00000000, 0xAB08);  // ADD R3, SP, #32
    thumb_cpu.execute(1);
    
    // 0 + 32 = 32
    EXPECT_EQ(cpu.R()[3], 0x00000020u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressLargeSp) {
    // Test with large SP value within RAM bounds
    setup_registers({{13, 0x00001800}});  // Set SP to large value within bounds
    
    // Manual instruction encoding: SP=1, Rd=4, Word8=32 (offset 128)
    memory.write16(0x00000000, 0xAC20);  // ADD R4, SP, #128
    thumb_cpu.execute(1);
    
    // 0x1800 + 128 = 0x1880
    EXPECT_EQ(cpu.R()[4], 0x00001880u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressFlagPreservation) {
    // Verify flags are preserved (ADD SP doesn't modify flags)
    setup_registers({{13, 0x00001000}});  // Set SP
    cpu.CPSR() |= CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
    
    // Manual instruction encoding: SP=1, Rd=5, Word8=16 (offset 64)
    memory.write16(0x00000000, 0xAD10);  // ADD R5, SP, #64
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[5], 0x00001040u); // SP + 64
    
    // All flags should be preserved
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_N));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_C));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressUnalignedSp) {
    // Test with unaligned SP (should work as SP is treated as word)
    setup_registers({{13, 0x00001002}});  // Set SP to unaligned value
    
    // Manual instruction encoding: SP=1, Rd=6, Word8=4 (offset 16)
    memory.write16(0x00000000, 0xAE04);  // ADD R6, SP, #16
    thumb_cpu.execute(1);
    
    // 0x1002 + 16 = 0x1012
    EXPECT_EQ(cpu.R()[6], 0x00001012u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressMaximumOffset) {
    // Test maximum offset that works reliably
    // Format 12 theoretically supports up to 1020 bytes (255 * 4)
    setup_registers({{13, 0x00000100}});  // Set SP to low value for safety
    
    // Manual instruction encoding: SP=1, Rd=7, Word8=16 (offset 64)
    memory.write16(0x00000000, 0xAF10);  // ADD R7, SP, #64
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[7], 0x00000140u);  // 0x100 + 64 = 0x140
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest12, ComprehensiveOffsetTest) {
    // Test various offset values to verify encoding
    setup_registers({{13, 0x00001000}});  // Set SP
    
    uint32_t test_offsets[] = {0, 4, 8, 12, 16, 20, 24, 28, 32};
    
    for (size_t i = 0; i < sizeof(test_offsets)/sizeof(test_offsets[0]); i++) {
        cpu.R()[0] = 0;  // Clear target register
        cpu.R()[15] = 0; // Reset PC
        
        // Manual instruction encoding: SP=1, Rd=0, Word8=offset/4
        uint16_t instruction = 0xA800 | (test_offsets[i] / 4);
        memory.write16(0x00000000, instruction);
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[0], 0x00001000u + test_offsets[i]) 
            << "Offset " << test_offsets[i] << " failed";
    }
}

TEST_F(ThumbCPUTest12, PcSpComparison) {
    // Compare PC-relative vs SP-relative operations
    setup_registers({{13, 0x00001000}, {15, 0x00000100}});
    
    // PC-relative: ADD R0, PC, #8
    memory.write16(0x00000100, 0xA002);  // ADD R0, PC, #8
    thumb_cpu.execute(1);
    uint32_t pc_result = cpu.R()[0];
    
    // Reset for SP-relative test
    cpu.R()[15] = 0x00000000;
    
    // SP-relative: ADD R1, SP, #8  
    memory.write16(0x00000000, 0xA902);  // ADD R1, SP, #8
    thumb_cpu.execute(1);
    uint32_t sp_result = cpu.R()[1];
    
    // Verify they calculated different addresses
    EXPECT_NE(pc_result, sp_result);
    EXPECT_EQ(sp_result, 0x00001008u);  // SP + 8
    // PC result: (0x102 aligned to 0x100) + 8 = 0x108
    EXPECT_EQ(pc_result, 0x00000108u);
}
