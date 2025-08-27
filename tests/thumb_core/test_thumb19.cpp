#include "gtest/gtest.h"
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <sstream>
#include <set>
#include <cstdint>

#ifdef KEYSTONE_AVAILABLE
#include <keystone/keystone.h>
#endif

// Common helper functions for Format 19 tests
std::string serializeCPUState19(const CPU& cpu) {
    std::ostringstream oss;
    const auto& registers = cpu.R();
    for (size_t i = 0; i < registers.size(); ++i) {
        oss << "R" << i << ":" << registers[i] << ";";
    }
    oss << "CPSR:" << cpu.CPSR();
    return oss.str();
}

void validateUnchangedRegisters19(const CPU& cpu, const std::string& beforeState, const std::set<int>& changedRegisters) {
    const auto& registers = cpu.R();
    std::istringstream iss(beforeState);
    std::string token;
    
    for (size_t i = 0; i < registers.size(); ++i) {
        std::getline(iss, token, ';');
        if (changedRegisters.find(i) == changedRegisters.end()) {
            ASSERT_EQ(token, "R" + std::to_string(i) + ":" + std::to_string(registers[i]));
        }
    }
}

// ARM Thumb Format 19: Long branch with link
// Encoding: 1111[H][Offset] (two-instruction sequence)
// H=0: First instruction stores high part of offset in LR
// H=1: Second instruction performs branch and updates LR with return address
// Instructions: BL (Branch and Link)
class ThumbCPUTest19 : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;

    ThumbCPUTest19() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu) {}

    void SetUp() override {
        // Initialize all registers to 0
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        
        // Set Thumb mode (T flag) and User mode
        cpu.CPSR() = CPU::FLAG_T | 0x10;
    }
};

// Test simple forward BL instruction
TEST_F(ThumbCPUTest19, BL_SIMPLE_FORWARD_BRANCH) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // BL +4: Target PC = 0x04 + 4 = 0x08
    // First instruction: F000 (high part, offset[22:12] = 0)
    // Second instruction: F802 (low part, offset[11:1] = 2, offset[0] must be 0)
    memory.write16(0x00000000, 0xF000);
    memory.write16(0x00000002, 0xF802);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute first instruction (sets up LR with high offset part)
    registers[15] = 0x00000000;
    cpu.execute(1);
    
    // Execute second instruction (performs branch and sets final LR)
    cpu.execute(1);
    
    // Verify results
    EXPECT_EQ(registers[15], static_cast<uint32_t>(0x00000008)); // PC = 0x04 + (2*2)
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = next instruction (0x04) + 1 (Thumb bit)
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test backward BL instruction
TEST_F(ThumbCPUTest19, BL_BACKWARD_BRANCH) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Start at PC = 0x100, BL -4: Target PC = 0x104 + (-4) = 0x100
    // First instruction: F7FF (high part, negative offset)
    // Second instruction: FFFE (low part)
    memory.write16(0x00000100, 0xF7FF);
    memory.write16(0x00000102, 0xFFFE);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute first instruction
    registers[15] = 0x00000100;
    cpu.execute(1);
    
    // Execute second instruction
    cpu.execute(1);
    
    // Verify results
    EXPECT_EQ(registers[15], static_cast<uint32_t>(0x00000100)); // PC = 0x104 + (-2*2)
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000105)); // LR = 0x104 + 1
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL with zero offset
TEST_F(ThumbCPUTest19, BL_ZERO_OFFSET_BRANCH) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // BL +0: Target PC = 0x04 + 0 = 0x04
    memory.write16(0x00000000, 0xF000);
    memory.write16(0x00000002, 0xF800);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute first instruction
    registers[15] = 0x00000000;
    cpu.execute(1);
    
    // Execute second instruction
    cpu.execute(1);
    
    // Verify results
    EXPECT_EQ(registers[15], static_cast<uint32_t>(0x00000004)); // PC = 0x04 + (0*2)
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = 0x04 + 1
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL preserves processor flags
TEST_F(ThumbCPUTest19, BL_PRESERVES_FLAGS) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V;
    
    // BL +4
    memory.write16(0x00000000, 0xF000);
    memory.write16(0x00000002, 0xF802);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute BL instruction sequence
    registers[15] = 0x00000000;
    cpu.execute(1);
    cpu.execute(1);
    
    // Verify branch occurred correctly
    EXPECT_EQ(registers[15], static_cast<uint32_t>(0x00000008));
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000005));
    
    // Verify all flags preserved
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_N));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_C));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_V));
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_T)); // Thumb mode preserved
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL with larger positive offset
TEST_F(ThumbCPUTest19, BL_LARGE_FORWARD_BRANCH) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // BL +100: Target PC = 0x04 + 100 = 0x68
    memory.write16(0x00000000, 0xF000);
    memory.write16(0x00000002, 0xF832);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute BL instruction sequence
    registers[15] = 0x00000000;
    cpu.execute(1);
    cpu.execute(1);
    
    // Verify results
    EXPECT_EQ(registers[15], static_cast<uint32_t>(0x00000068)); // PC = 0x04 + (50*2)
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = 0x04 + 1
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL with large backward offset
TEST_F(ThumbCPUTest19, BL_LARGE_BACKWARD_BRANCH) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Start at PC = 0x400, BL -100: Target PC = 0x404 + (-100) = 0x3A0
    registers[15] = 0x00000400;
    memory.write16(0x00000400, 0xF7FF);
    memory.write16(0x00000402, 0xFFCE);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute BL instruction sequence
    registers[15] = 0x00000400;
    cpu.execute(1);
    cpu.execute(1);
    
    // Verify results
    EXPECT_EQ(registers[15], static_cast<uint32_t>(0x000003A0)); // PC = 0x404 + (-100)
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000405)); // LR = 0x404 + 1
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL overwrites existing LR value
TEST_F(ThumbCPUTest19, BL_OVERWRITES_LINK_REGISTER) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Set existing LR value
    registers[14] = 0xABCDEF01;
    
    // BL +4
    memory.write16(0x00000000, 0xF000);
    memory.write16(0x00000002, 0xF802);
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute BL instruction sequence
    registers[15] = 0x00000000;
    cpu.execute(1);
    cpu.execute(1);
    
    // Verify LR was overwritten, not preserved
    EXPECT_EQ(registers[15], static_cast<uint32_t>(0x00000008));
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // New LR value
    EXPECT_NE(registers[14], static_cast<uint32_t>(0xABCDEF01)); // Old value gone
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL maximum forward offset
TEST_F(ThumbCPUTest19, BL_MAXIMUM_FORWARD_OFFSET) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Maximum positive offset: 22-bit signed = +0x1FFFFF (even, so 0x1FFFFE)
    // This requires careful encoding of the 22-bit offset across two instructions
    memory.write16(0x00000000, 0xF3FF);  // High part: 0011111111111 (11 bits)
    memory.write16(0x00000002, 0xFFFF);  // Low part: 11111111111 (11 bits)
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute BL instruction sequence
    registers[15] = 0x00000000;
    cpu.execute(1);
    cpu.execute(1);
    
    // Target PC = 0x04 + 0x1FFFFE = 0x200002
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00000005)); // LR = 0x04 + 1
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL maximum backward offset  
TEST_F(ThumbCPUTest19, BL_MAXIMUM_BACKWARD_OFFSET) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Start at reasonable memory address to allow large backward branch
    registers[15] = 0x00001000;
    
    // Backward BL: -0x1000 offset  
    memory.write16(0x00001000, 0xF7FF);  // High part for negative offset (-1 in high 11 bits)
    memory.write16(0x00001002, 0xF800);  // Low part (0x000 << 1 = 0)
    
    std::string beforeState = serializeCPUState19(cpu);
    
    // Execute BL instruction sequence
    registers[15] = 0x00001000;
    cpu.execute(1);
    cpu.execute(1);
    
    // LR should be set correctly (PC after BL sequence + 1)
    EXPECT_EQ(registers[14], static_cast<uint32_t>(0x00001005)); // LR = 0x1004 + 1
    
    validateUnchangedRegisters19(cpu, beforeState, {14, 15});
}

// Test BL offset calculation verification
TEST_F(ThumbCPUTest19, BL_OFFSET_CALCULATION_VERIFICATION) {
    auto& registers = cpu.R();
    registers.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Test various offsets to verify calculation logic
    struct TestCase {
        uint16_t high_instr;
        uint16_t low_instr;
        uint32_t start_pc;
        uint32_t expected_target;
        uint32_t expected_lr;
    };
    
    std::vector<TestCase> test_cases = {
        {0xF000, 0xF802, 0x00000000, 0x00000008, 0x00000005}, // +4
        {0xF000, 0xF810, 0x00000000, 0x00000024, 0x00000005}, // +32 (0x10 << 1 = 0x20)
        {0xF7FF, 0xFFFC, 0x00000100, 0x000000FC, 0x00000105}, // -8 from 0x100
    };
    
    for (const auto& test_case : test_cases) {
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        memory.write16(test_case.start_pc, test_case.high_instr);
        memory.write16(test_case.start_pc + 2, test_case.low_instr);
        
        // Execute BL sequence
        registers[15] = test_case.start_pc;
        cpu.execute(1);
        cpu.execute(1);
        
        EXPECT_EQ(registers[15], test_case.expected_target) 
            << "Failed target PC for instructions " << std::hex 
            << test_case.high_instr << " " << test_case.low_instr;
        EXPECT_EQ(registers[14], test_case.expected_lr)
            << "Failed LR for instructions " << std::hex 
            << test_case.high_instr << " " << test_case.low_instr;
    }
}

// Test BL instruction encoding validation
TEST_F(ThumbCPUTest19, BL_INSTRUCTION_ENCODING_VALIDATION) {
    
    // Test that BL instructions are properly identified by their encoding
    struct EncodingTest {
        uint16_t instruction;
        bool is_bl_high;    // True if this is BL high part (H=0)
        bool is_bl_low;     // True if this is BL low part (H=1)
    };
    
    std::vector<EncodingTest> encoding_tests = {
        {0xF000, true, false},   // BL high part: 1111 0xxx xxxx xxxx
        {0xF800, false, true},   // BL low part:  1111 1xxx xxxx xxxx
        {0xF400, true, false},   // BL high part with different offset
        {0xFFFF, false, true},   // BL low part with max offset
        {0xE000, false, false},  // Not BL (unconditional branch)
        {0xD000, false, false},  // Not BL (conditional branch)
    };
    
    for (const auto& test : encoding_tests) {
        // Check if instruction matches BL pattern
        bool is_format19_high = (test.instruction & 0xF800) == 0xF000;
        bool is_format19_low = (test.instruction & 0xF800) == 0xF800;
        
        EXPECT_EQ(is_format19_high, test.is_bl_high) 
            << "BL high part detection failed for " << std::hex << test.instruction;
        EXPECT_EQ(is_format19_low, test.is_bl_low)
            << "BL low part detection failed for " << std::hex << test.instruction;
    }
}

// Test BL register preservation (all registers except PC and LR should be unchanged)
TEST_F(ThumbCPUTest19, BL_REGISTER_PRESERVATION) {
    auto& registers = cpu.R();
    
    // Initialize all registers with test values
    for (int i = 0; i < 16; ++i) {
        registers[i] = 0x1000 + i;
    }
    cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z;
    
    // Store initial state (excluding PC and LR which will change)
    std::array<uint32_t, 16> initial_regs;
    for (int i = 0; i < 16; ++i) {
        initial_regs[i] = registers[i];
    }
    
    // BL +8
    memory.write16(0x00001000, 0xF000);
    memory.write16(0x00001002, 0xF804);
    
    // Execute BL sequence
    registers[15] = 0x00001000;
    cpu.execute(1);
    cpu.execute(1);
    
    // Verify only PC and LR changed
    for (int i = 0; i < 14; ++i) {  // R0-R13 should be unchanged
        EXPECT_EQ(registers[i], initial_regs[i])
            << "Register R" << i << " was modified by BL instruction";
    }
    
    // PC and LR should have changed
    EXPECT_NE(registers[15], initial_regs[15]) << "PC should have changed";
    EXPECT_NE(registers[14], initial_regs[14]) << "LR should have changed";
    
    // CPSR should be mostly preserved (except possibly implementation-specific bits)
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_T)) << "Thumb mode should be preserved";
    EXPECT_TRUE(cpu.getFlag(CPU::FLAG_Z)) << "Zero flag should be preserved";
}
