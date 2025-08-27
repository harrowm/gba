// test_thumb18.cpp - Modern Thumb CPU test fixture for Format 18: Unconditional branch operations
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <initializer_list>

extern "C" {
#include <keystone/keystone.h>
}

// ThumbCPUTest18 fixture for Format 18: Unconditional branch operations
// ARM Thumb Format 18: Unconditional branch
// Encoding: 11100[Offset11]
// Instructions: B
class ThumbCPUTest18 : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks; // Keystone handle

    ThumbCPUTest18() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu), ks(nullptr) {}

    void SetUp() override {
        // Initialize all registers to 0
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        
        // Set Thumb mode (T flag) and User mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
        
        // Initialize Keystone for Thumb mode
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            ks = nullptr;
        }
    }
    
    void TearDown() override {
        if (ks) {
            ks_close(ks);
        }
    }
    
    // Helper to set up multiple registers at once
    void setup_registers(std::initializer_list<std::pair<int, uint32_t>> reg_values) {
        cpu.R().fill(0);
        for (const auto& pair : reg_values) {
            cpu.R()[pair.first] = pair.second;
        }
        // Always ensure Thumb mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
    }
    
    // Helper to assemble and write Thumb instruction using Keystone
    bool assemble_and_write_thumb(const std::string& assembly, uint32_t address) {
        if (!ks) return false;
        
        unsigned char* encode;
        size_t count;
        size_t stat_count;
        
        if (ks_asm(ks, assembly.c_str(), address, &encode, &count, &stat_count) == KS_ERR_OK) {
            if (count >= 2) { // Thumb instructions are 2 bytes
                uint16_t instruction = encode[0] | (encode[1] << 8);
                memory.write16(address, instruction);
                ks_free(encode);
                return true;
            }
            ks_free(encode);
        }
        return false;
    }
    
    // Helper to calculate expected PC after branch
    uint32_t calculate_branch_target(uint32_t current_pc, int16_t offset11) {
        // PC after instruction fetch and increment
        uint32_t pc_after_increment = current_pc + 4;
        
        // Sign extend 11-bit offset to 32-bit and multiply by 2
        int32_t signed_offset = ((offset11 << 21) >> 21); // Sign extend
        int32_t byte_offset = signed_offset * 2;
        
        return pc_after_increment + byte_offset;
    }
};

TEST_F(ThumbCPUTest18, B_SIMPLE_FORWARD_BRANCH) {
    // Test case: Simple forward branch
    setup_registers({{15, 0x00000000}});
    
    // Branch forward by 4 bytes (offset11 = +2)
    if (!assemble_and_write_thumb("b +4", 0x00000000)) {
        memory.write16(0x00000000, 0xE002); // B +4 (offset11 = 2, 2*2 = 4 bytes)
    }
    
    // Write some NOPs and target instruction
    memory.write16(0x00000002, 0x0000); // NOP (should be skipped)
    memory.write16(0x00000004, 0x0000); // Target instruction
    
    cpu.execute(1);
    
    // Expected: PC = 0x00 + 2 + (2 * 2) = 0x06 (current PC + 4 + offset*2)
    EXPECT_EQ(cpu.R()[15], 0x00000006u);
}

TEST_F(ThumbCPUTest18, B_BACKWARD_BRANCH) {
    // Test case: Backward branch
    setup_registers({{15, 0x00000010}});
    
    // Branch backward by 4 bytes (offset11 = -2)
    if (!assemble_and_write_thumb("b -4", 0x00000010)) {
        memory.write16(0x00000010, 0xE7FE); // B -4 (offset11 = -2, -2*2 = -4 bytes)
    }
    
    cpu.execute(1);
    
    // Expected: PC = 0x10 + 2 + (-2 * 2) = 0x0E (current PC + 4 - 4)
    EXPECT_EQ(cpu.R()[15], 0x0000000Eu);
}

TEST_F(ThumbCPUTest18, B_ZERO_OFFSET_BRANCH) {
    // Test case: Zero offset branch (self-loop)
    setup_registers({{15, 0x00000000}});
    
    if (!assemble_and_write_thumb("b +0", 0x00000000)) {
        memory.write16(0x00000000, 0xE000); // B +0 (offset11 = 0)
    }
    
    cpu.execute(1);
    
    // Expected: PC = 0x00 + 2 + (0 * 2) = 0x02
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest18, B_PRESERVES_FLAGS) {
    // Test case: Branch preserves all flags
    setup_registers({{15, 0x00000000}});
    
    // Set all condition flags
    cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
    
    if (!assemble_and_write_thumb("b +10", 0x00000000)) {
        memory.write16(0x00000000, 0xE005); // B +10 (offset11 = 5, 5*2 = 10 bytes)
    }
    
    cpu.execute(1);
    
    // Expected: PC = 0x00 + 2 + (5 * 2) = 0x0C
    EXPECT_EQ(cpu.R()[15], 0x0000000Cu);
    
    // Verify all flags are preserved
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_Z)) << "Zero flag should be preserved";
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_N)) << "Negative flag should be preserved";
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_C)) << "Carry flag should be preserved";
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_V)) << "Overflow flag should be preserved";
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_T)) << "Thumb flag should be preserved";
}

TEST_F(ThumbCPUTest18, B_LARGE_FORWARD_BRANCH) {
    // Test case: Large forward branch within memory bounds
    setup_registers({{15, 0x00000100}});
    
    // Branch forward by 500 bytes (offset11 = +250)
    if (!assemble_and_write_thumb("b +500", 0x00000100)) {
        memory.write16(0x00000100, 0xE0FA); // B +500 (offset11 = 250, 250*2 = 500 bytes)
    }
    
    cpu.execute(1);
    
    // Expected: PC = 0x100 + 2 + (250 * 2) = 0x2F6
    EXPECT_EQ(cpu.R()[15], 0x000002F6u);
}

TEST_F(ThumbCPUTest18, B_LARGE_BACKWARD_BRANCH) {
    // Test case: Large backward branch within memory bounds
    setup_registers({{15, 0x00000300}});
    
    // Branch backward by 200 bytes (offset11 = -100)
    // Note: -100 in 11-bit two's complement is 0x79C (0x800 - 100 = 0x79C)
    if (!assemble_and_write_thumb("b -200", 0x00000300)) {
        memory.write16(0x00000300, 0xE79C); // B -200 (offset11 = -100 in 11-bit, -100*2 = -200 bytes)
    }
    
    cpu.execute(1);
    
    // Expected: PC = 0x300 + 2 + (-100 * 2) = 0x23A
    EXPECT_EQ(cpu.R()[15], 0x0000023Au);
}

TEST_F(ThumbCPUTest18, B_MAXIMUM_FORWARD_OFFSET) {
    // Test case: Maximum positive offset (+2046 bytes)
    setup_registers({{15, 0x00001000}});
    
    // Maximum positive offset11 = +1023, byte offset = 1023 * 2 = 2046 bytes
    memory.write16(0x00001000, 0xE3FF); // B +2046 (offset11 = 0x3FF = 1023)
    
    cpu.execute(1);
    
    // Expected: PC = 0x1000 + 2 + (1023 * 2) = 0x1000 + 2 + 2046 = 0x1800
    EXPECT_EQ(cpu.R()[15], 0x00001800u);
}

TEST_F(ThumbCPUTest18, B_MAXIMUM_BACKWARD_OFFSET) {
    // Test case: Maximum negative offset (-2048 bytes)
    setup_registers({{15, 0x00002000}});
    
    // Maximum negative offset11 = -1024, byte offset = -1024 * 2 = -2048 bytes
    // -1024 in 11-bit two's complement is 0x400
    memory.write16(0x00002000, 0xE400); // B -2048 (offset11 = 0x400 = -1024 in 11-bit)
    
    cpu.execute(1);
    
    // Expected: PC = 0x2000 + 2 + (-1024 * 2) = 0x2002 - 2048 = 0x1802
    EXPECT_EQ(cpu.R()[15], 0x00001802u);
}

TEST_F(ThumbCPUTest18, B_OFFSET_CALCULATION_VERIFICATION) {
    // Test case: Verify offset calculation with various values
    struct OffsetTest {
        uint32_t start_pc;
        int16_t offset11;
        uint16_t instruction;
        uint32_t expected_pc;
        std::string description;
    };
    
    OffsetTest tests[] = {
        {0x00000000, 0, 0xE000, 0x00000002, "Zero offset"},
        {0x00000000, 1, 0xE001, 0x00000004, "Offset +1"},
        {0x00000000, -1, 0xE7FF, 0x00000000, "Offset -1"},
        {0x00000010, 8, 0xE008, 0x00000022, "Offset +8"},
        {0x00000010, -8, 0xE7F8, 0x00000002, "Offset -8"}
    };
    
    for (const auto& test : tests) {
        setup_registers({{15, test.start_pc}});
        memory.write16(test.start_pc, test.instruction);
        
        cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[15], test.expected_pc) 
            << "Failed for " << test.description 
            << " (offset11=" << test.offset11 << ")";
    }
}

TEST_F(ThumbCPUTest18, B_INSTRUCTION_ENCODING_VALIDATION) {
    // Test case: Validate instruction encoding structure
    // Format 18: 11100[Offset11] where Offset11 is an 11-bit signed offset
    
    struct EncodingTest {
        int16_t offset11;
        uint16_t expected_encoding;
        std::string description;
    };
    
    EncodingTest tests[] = {
        {0, 0xE000, "Zero encoding"},
        {1, 0xE001, "Positive 1"},
        {-1, 0xE7FF, "Negative 1 (11-bit two's complement)"},
        {512, 0xE200, "Mid-range positive"},
        {-512, 0xE600, "Mid-range negative"},
        {1023, 0xE3FF, "Maximum positive"},
        {-1024, 0xE400, "Maximum negative"}
    };
    
    for (const auto& test : tests) {
        // Verify encoding format: bits 15-11 should be 11100b (0x1C)
        uint16_t high_bits = (test.expected_encoding >> 11) & 0x1F;
        EXPECT_EQ(high_bits, 0x1Cu) 
            << "High bits should be 11100b for " << test.description;
        
        // Verify offset extraction: bits 10-0 should match offset11
        uint16_t extracted_offset = test.expected_encoding & 0x7FF;
        uint16_t expected_offset = static_cast<uint16_t>(test.offset11) & 0x7FF;
        EXPECT_EQ(extracted_offset, expected_offset)
            << "Offset extraction failed for " << test.description;
    }
}

TEST_F(ThumbCPUTest18, B_REGISTER_PRESERVATION) {
    // Test case: Ensure unconditional branch only affects PC, not other registers
    setup_registers({
        {0, 0x11111111}, {1, 0x22222222}, {2, 0x33333333}, {3, 0x44444444},
        {4, 0x55555555}, {5, 0x66666666}, {6, 0x77777777}, {7, 0x88888888},
        {8, 0x99999999}, {9, 0xAAAAAAAA}, {10, 0xBBBBBBBB}, {11, 0xCCCCCCCC},
        {12, 0xDDDDDDDD}, {13, 0xEEEEEEEE}, {14, 0xFFFFFFFF}, {15, 0x00000000}
    });
    
    memory.write16(0x00000000, 0xE010); // B +32 (offset11 = 16, 16*2 = 32)
    
    cpu.execute(1);
    
    // Verify PC changed correctly
    EXPECT_EQ(cpu.R()[15], 0x00000022u); // 0x00 + 2 + 32 = 0x22
    
    // Verify all other registers are unchanged
    EXPECT_EQ(cpu.R()[0], 0x11111111u);
    EXPECT_EQ(cpu.R()[1], 0x22222222u);
    EXPECT_EQ(cpu.R()[2], 0x33333333u);
    EXPECT_EQ(cpu.R()[3], 0x44444444u);
    EXPECT_EQ(cpu.R()[4], 0x55555555u);
    EXPECT_EQ(cpu.R()[5], 0x66666666u);
    EXPECT_EQ(cpu.R()[6], 0x77777777u);
    EXPECT_EQ(cpu.R()[7], 0x88888888u);
    EXPECT_EQ(cpu.R()[8], 0x99999999u);
    EXPECT_EQ(cpu.R()[9], 0xAAAAAAAAu);
    EXPECT_EQ(cpu.R()[10], 0xBBBBBBBBu);
    EXPECT_EQ(cpu.R()[11], 0xCCCCCCCCu);
    EXPECT_EQ(cpu.R()[12], 0xDDDDDDDDu);
    EXPECT_EQ(cpu.R()[13], 0xEEEEEEEEu);
    EXPECT_EQ(cpu.R()[14], 0xFFFFFFFFu);
}

TEST_F(ThumbCPUTest18, B_EDGE_CASES_AND_BOUNDARIES) {
    // Test case: Various edge cases and boundary conditions
    
    // Sub-test 1: Branch to even addresses (Thumb requirement)
    setup_registers({{15, 0x00000000}});
    memory.write16(0x00000000, 0xE002); // B +4
    cpu.execute(1);
    EXPECT_EQ(cpu.R()[15] & 1, 0u) << "Branch target should be even (Thumb mode)";
    
    // Sub-test 2: Multiple consecutive branches
    setup_registers({{15, 0x00000000}});
    memory.write16(0x00000000, 0xE001); // B +2 (to 0x04)
    memory.write16(0x00000004, 0xE001); // B +2 (to 0x08) 
    memory.write16(0x00000008, 0xE001); // B +2 (to 0x0C)
    
    cpu.execute(1); // First branch
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    
    cpu.execute(1); // Second branch  
    EXPECT_EQ(cpu.R()[15], 0x00000008u);
    
    cpu.execute(1); // Third branch
    EXPECT_EQ(cpu.R()[15], 0x0000000Cu);
    
    // Sub-test 3: Branch across memory boundaries
    setup_registers({{15, 0x0000FFF0}});
    memory.write16(0x0000FFF0, 0xE008); // B +16 (crosses 64KB boundary)
    cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x00010002u); // Should wrap correctly
}
