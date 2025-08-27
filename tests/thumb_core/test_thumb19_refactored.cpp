// test_thumb19_refactored.cpp - Refactored version using common base class
#include "thumb_test_base.h"

// ARM Thumb Format 19: Long branch with link
// Encoding: 1111[H][Offset] (two-instruction sequence)
// H=0: First instruction stores high part of offset in LR
// H=1: Second instruction performs branch and updates LR with return address
// Instructions: BL (Branch and Link)
class ThumbCPUTest19 : public ThumbCPUTestBase {
    // No additional setup needed - everything is handled by the base class!
};

// Test simple forward BL instruction
TEST_F(ThumbCPUTest19, BL_SIMPLE_FORWARD_BRANCH) {
    auto& regs = registers();
    regs.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // BL +4: Target PC = 0x04 + 4 = 0x08
    writeInstruction(0x00000000, 0xF000);  // First instruction (high part)
    writeInstruction(0x00000002, 0xF802);  // Second instruction (low part)
    
    std::string beforeState = serializeCPUState();
    
    // Execute BL instruction sequence
    regs[15] = 0x00000000;
    execute(2);  // Execute both instructions
    
    // Verify results
    EXPECT_EQ(regs[15], static_cast<uint32_t>(0x00000008)); // PC = 0x04 + (2*2)
    EXPECT_EQ(regs[14], static_cast<uint32_t>(0x00000005)); // LR = next instruction + 1 (Thumb bit)
    
    validateUnchangedRegisters(beforeState, {14, 15});
}

// Test backward BL instruction
TEST_F(ThumbCPUTest19, BL_BACKWARD_BRANCH) {
    auto& regs = registers();
    regs.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Start at PC = 0x100, BL -4: Target PC = 0x104 + (-4) = 0x100
    writeInstruction(0x00000100, 0xF7FF);  // First instruction (negative offset)
    writeInstruction(0x00000102, 0xFFFE);  // Second instruction
    
    std::string beforeState = serializeCPUState();
    
    // Execute BL instruction sequence
    regs[15] = 0x00000100;
    execute(2);
    
    // Verify results
    EXPECT_EQ(regs[15], static_cast<uint32_t>(0x00000100)); // PC = 0x104 + (-2*2)
    EXPECT_EQ(regs[14], static_cast<uint32_t>(0x00000105)); // LR = 0x104 + 1
    
    validateUnchangedRegisters(beforeState, {14, 15});
}

// Test BL preserves processor flags
TEST_F(ThumbCPUTest19, BL_PRESERVES_FLAGS) {
    auto& regs = registers();
    regs.fill(0);
    setFlags(CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V);
    
    // BL +4
    writeInstruction(0x00000000, 0xF000);
    writeInstruction(0x00000002, 0xF802);
    
    std::string beforeState = serializeCPUState();
    
    // Execute BL instruction sequence
    regs[15] = 0x00000000;
    execute(2);
    
    // Verify branch occurred correctly
    EXPECT_EQ(regs[15], static_cast<uint32_t>(0x00000008));
    EXPECT_EQ(regs[14], static_cast<uint32_t>(0x00000005));
    
    // Verify all flags preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));
    EXPECT_TRUE(getFlag(CPU::FLAG_T)); // Thumb mode preserved
    
    validateUnchangedRegisters(beforeState, {14, 15});
}

// Test BL overwrites existing LR value
TEST_F(ThumbCPUTest19, BL_OVERWRITES_LINK_REGISTER) {
    auto& regs = registers();
    regs.fill(0);
    cpu.CPSR() = CPU::FLAG_T;
    
    // Set existing LR value
    regs[14] = 0xABCDEF01;
    
    // BL +4
    writeInstruction(0x00000000, 0xF000);
    writeInstruction(0x00000002, 0xF802);
    
    std::string beforeState = serializeCPUState();
    
    // Execute BL instruction sequence
    regs[15] = 0x00000000;
    execute(2);
    
    // Verify LR was overwritten, not preserved
    EXPECT_EQ(regs[15], static_cast<uint32_t>(0x00000008));
    EXPECT_EQ(regs[14], static_cast<uint32_t>(0x00000005)); // New LR value
    EXPECT_NE(regs[14], static_cast<uint32_t>(0xABCDEF01)); // Old value gone
    
    validateUnchangedRegisters(beforeState, {14, 15});
}

// Additional tests would follow the same pattern...
// Each test is now much cleaner and focuses on the test logic rather than setup
