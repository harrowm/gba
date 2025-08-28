/**
 * test_thumb13.cpp - Thumb Format 13: Add/Subtract offset to Stack Pointer
 *
 * Tests the ARMv4T Thumb Format 13 instruction encoding for stack pointer
 * adjustment operations that modify the stack pointer by immediate values.
 *
 * THUMB FORMAT 13: Add/Subtract offset to Stack Pointer
 * =====================================================
 * Encoding: 1011 0000 S offset7[6:0]
 * 
 * Instruction Forms:
 * - ADD SP, #imm7*4  - Add immediate to stack pointer     (S=0: 0xB000-0xB07F)
 * - SUB SP, #imm7*4  - Subtract immediate from stack pointer (S=1: 0xB080-0xB0FF)
 *
 * Field Definitions:
 * - S: Operation selector (0=ADD, 1=SUB)
 * - offset7[6:0]: Immediate offset in words (multiply by 4 for byte offset)
 *
 * Operation Details:
 * - ADD: SP = SP + (offset7 * 4)
 * - SUB: SP = SP - (offset7 * 4)
 * - Offset range: 0-508 bytes (0-127 words)
 * - Used for stack frame allocation and deallocation
 * - Does not affect condition flags
 * - Stack pointer remains word-aligned
 *
 * Test Infrastructure:
 * - Uses ThumbCPUTestBase for modern test patterns
 * - Keystone assembler compatibility with ARMv4T Thumb-1 instruction set  
 * - Comprehensive coverage of immediate offset ranges with scaling verification
 * - Stack frame allocation/deallocation scenarios
 * - Boundary condition testing for maximum offsets
 */

#include <gtest/gtest.h>
#include <string>
#include <sstream>
#include <iomanip>
#include "thumb_test_base.h"

class ThumbCPUTest13 : public ThumbCPUTestBase {
};

// ARM Thumb Format 13: Add/Subtract offset to Stack Pointer
// Encoding: 1011 0000 S [offset7]
// S=0: ADD SP, #imm (SP = SP + (offset7 * 4))
// S=1: SUB SP, #imm (SP = SP - (offset7 * 4))
// Offset range: 0-508 bytes (0-127 * 4)

TEST_F(ThumbCPUTest13, AddSpImmediateBasic) {
    // Test case: ADD SP, #0 - no change
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for ADD SP, #0: 0xB000
    assembleAndWriteThumb("add sp, #0x0", 0x00000000);
    execute(1);
    
    // SP should remain unchanged
    EXPECT_EQ(R(13), 0x00001000u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateSmall) {
    // Test case: ADD SP, #4 - basic increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for ADD SP, #4: 0xB001
    assembleAndWriteThumb("add sp, #0x4", 0x00000000);
    execute(1);
    
    // SP should be incremented by 4
    EXPECT_EQ(R(13), 0x00001004u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateMedium) {
    // Test case: ADD SP, #32 - medium increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=0 offset7=8 (32/4)
    assembleAndWriteThumb("add sp, #0x20", 0x00000000);  // ADD SP, #32
    execute(1);
    
    // SP should be incremented by 32
    EXPECT_EQ(R(13), 0x00001020u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateLarge) {
    // Test case: ADD SP, #128 - large increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=0 offset7=32 (128/4)
    assembleAndWriteThumb("add sp, #0x80", 0x00000000);  // ADD SP, #128
    execute(1);
    
    // SP should be incremented by 128
    EXPECT_EQ(R(13), 0x00001080u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateMaximum) {
    // Test case: ADD SP, #508 - maximum increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=0 offset7=127 (508/4)
    assembleAndWriteThumb("add sp, #0x1FC", 0x00000000);  // ADD SP, #508
    execute(1);
    
    // SP should be incremented by 508
    EXPECT_EQ(R(13), 0x000011FCu);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateBasic) {
    // Test case: SUB SP, #0 - no change
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for SUB SP, #0: 0xB080
    assembleAndWriteThumb("sub sp, #0x0", 0x00000000);
    execute(1);
    
    // SP should remain unchanged
    EXPECT_EQ(R(13), 0x00001000u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateSmall) {
    // Test case: SUB SP, #4 - basic decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for SUB SP, #4: 0xB081
    assembleAndWriteThumb("sub sp, #0x4", 0x00000000);
    execute(1);
    
    // SP should be decremented by 4
    EXPECT_EQ(R(13), 0x00000FFCu);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateMedium) {
    // Test case: SUB SP, #32 - medium decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=1 offset7=8 (32/4)
    assembleAndWriteThumb("sub sp, #0x20", 0x00000000);  // SUB SP, #32
    execute(1);
    
    // SP should be decremented by 32
    EXPECT_EQ(R(13), 0x00000FE0u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateLarge) {
    // Test case: SUB SP, #128 - large decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=1 offset7=32 (128/4)
    assembleAndWriteThumb("sub sp, #0x80", 0x00000000);  // SUB SP, #128
    execute(1);
    
    // SP should be decremented by 128
    EXPECT_EQ(R(13), 0x00000F80u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateMaximum) {
    // Test case: SUB SP, #508 - maximum decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=1 offset7=127 (508/4)
    assembleAndWriteThumb("sub sp, #0x1FC", 0x00000000);  // SUB SP, #508
    execute(1);
    
    // SP should be decremented by 508
    EXPECT_EQ(R(13), 0x00000E04u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, OffsetRangeValidation) {
    // Test various valid offsets that are multiples of 4
    uint32_t test_offsets[] = {0, 4, 8, 12, 16, 20, 32, 64, 128, 256, 508};
    
    for (uint32_t offset : test_offsets) {
        // Test ADD SP
        cpu.R().fill(0);
        R(13) = 0x00001000;
        R(15) = 0x00000000;
        
        // Use assembleAndWriteThumb for ADD SP with hex format
        std::stringstream ss;
        ss << "add sp, #0x" << std::hex << offset;
        assembleAndWriteThumb(ss.str(), 0x00000000);
        execute(1);
        
        uint32_t expected_sp = 0x00001000 + offset;
        EXPECT_EQ(R(13), expected_sp) 
            << "ADD SP, #" << offset << " failed. Expected: 0x" 
            << std::hex << expected_sp << ", Got: 0x" << R(13);
        
        // Test SUB SP (only if SP won't underflow too much)
        if (offset <= 0x1000) {
            cpu.R().fill(0);
            R(13) = 0x00001000;
            R(15) = 0x00000000;
            
            // Use assembleAndWriteThumb for SUB SP with hex format
            std::stringstream ss;
            ss << "sub sp, #0x" << std::hex << offset;
            assembleAndWriteThumb(ss.str(), 0x00000000);
            execute(1);
            
            uint32_t expected_sp_sub = 0x00001000 - offset;
            EXPECT_EQ(R(13), expected_sp_sub)
                << "SUB SP, #" << offset << " failed. Expected: 0x" 
                << std::hex << expected_sp_sub << ", Got: 0x" << R(13);
        }
    }
}

TEST_F(ThumbCPUTest13, AddSubSequenceTest) {
    // Test ADD then SUB same amount - should return to original
    setup_registers({{13, 0x00001000}});  // Set SP
    uint32_t initial_sp = R(13);
    
    // ADD SP, #32 - Manual encoding: 0xB008
    assembleAndWriteThumb("add sp, #0x20", 0x00000000);
    execute(1);
    
    EXPECT_EQ(R(13), initial_sp + 32);
    
    // SUB SP, #32 - Manual encoding: 0xB088
    R(15) = 0x00000000; // Reset PC
    assembleAndWriteThumb("sub sp, #0x20", 0x00000000);
    execute(1);
    
    // Should be back to original value
    EXPECT_EQ(R(13), initial_sp);
}

TEST_F(ThumbCPUTest13, MultipleAddOperations) {
    // Test multiple ADD operations accumulate correctly
    setup_registers({{13, 0x00001000}});  // Set SP
    uint32_t initial_sp = R(13);
    
    // ADD SP, #16 three times - Manual encoding: 0xB004
    for (int i = 0; i < 3; i++) {
        R(15) = 0x00000000; // Reset PC
        assembleAndWriteThumb("add sp, #0x10", 0x00000000);  // ADD SP, #16
        execute(1);
        EXPECT_EQ(R(13), initial_sp + 16 * (i + 1));
    }
    
    // Final SP should be initial + 48
    EXPECT_EQ(R(13), initial_sp + 48);
}

TEST_F(ThumbCPUTest13, MultipleSubOperations) {
    // Test multiple SUB operations accumulate correctly
    setup_registers({{13, 0x00001200}});  // Higher starting point for SUB
    uint32_t initial_sp = R(13);
    
    // SUB SP, #16 three times - Manual encoding: 0xB084
    for (int i = 0; i < 3; i++) {
        R(15) = 0x00000000; // Reset PC
        assembleAndWriteThumb("sub sp, #0x10", 0x00000000);  // SUB SP, #16
        execute(1);
        EXPECT_EQ(R(13), initial_sp - 16 * (i + 1));
    }
    
    // Final SP should be initial - 48
    EXPECT_EQ(R(13), initial_sp - 48);
}

TEST_F(ThumbCPUTest13, MemoryBoundaryAddTest) {
    // Test SP near memory boundary with ADD
    setup_registers({{13, 0x00001F00}});  // Near end of test memory (0x1FFF)
    
    // Manual encoding for ADD SP, #4: 0xB001
    assembleAndWriteThumb("add sp, #0x4", 0x00000000);
    execute(1);
    
    EXPECT_EQ(R(13), 0x00001F04u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, MemoryBoundarySubTest) {
    // Test SP near memory boundary with SUB
    setup_registers({{13, 0x00000100}});  // Near start of memory
    
    // Manual encoding for SUB SP, #4: 0xB081
    assembleAndWriteThumb("sub sp, #0x4", 0x00000000);
    execute(1);
    
    EXPECT_EQ(R(13), 0x000000FCu);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, SpOverflowTest) {
    // Test SP overflow (ADD maximum to high value)
    setup_registers({{13, 0xFFFFFF00}});  // High value that will overflow
    
    // Manual encoding for ADD SP, #508: 0xB07F
    assembleAndWriteThumb("add sp, #0x1FC", 0x00000000);
    execute(1);
    
    // Should wrap around due to 32-bit arithmetic
    uint32_t expected = 0xFFFFFF00 + 508;
    EXPECT_EQ(R(13), expected);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, SpUnderflowTest) {
    // Test SP underflow (SUB from low value)
    setup_registers({{13, 0x00000100}});  // Low value
    
    // Manual encoding for SUB SP, #508: 0xB0FF
    assembleAndWriteThumb("sub sp, #0x1FC", 0x00000000);
    execute(1);
    
    // Should wrap around due to 32-bit arithmetic
    uint32_t expected = 0x00000100 - 508;
    EXPECT_EQ(R(13), expected);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, OtherRegistersUnaffected) {
    // Test that other registers are not affected by SP operations
    setup_registers({{0, 0xDEADBEEF}, {1, 0xCAFEBABE}, {13, 0x00001000}});
    
    // Manual encoding for ADD SP, #64: 0xB010
    assembleAndWriteThumb("add sp, #0x40", 0x00000000);
    execute(1);
    
    // Verify SP was modified correctly
    EXPECT_EQ(R(13), 0x00001040u);
    // Verify other registers unchanged
    EXPECT_EQ(R(0), 0xDEADBEEFu);
    EXPECT_EQ(R(1), 0xCAFEBABEu);
    // Verify PC was incremented
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest13, FlagsUnaffected) {
    // Test that CPSR flags are unaffected by SP operations
    setup_registers({{13, 0x00001000}});
    
    // Set various CPSR flags
    cpu.CPSR() |= CPU::FLAG_N | CPU::FLAG_Z | CPU::FLAG_C | CPU::FLAG_V;
    uint32_t original_cpsr = cpu.CPSR();
    
    // Manual encoding for ADD SP, #32: 0xB008
    assembleAndWriteThumb("add sp, #0x20", 0x00000000);
    execute(1);
    
    // CPSR should be unchanged
    EXPECT_EQ(cpu.CPSR(), original_cpsr);
    EXPECT_EQ(R(13), 0x00001020u);
}

TEST_F(ThumbCPUTest13, InstructionEncodingValidation) {
    // Test comprehensive instruction encoding validation
    // Based on the original format13_stack_operations.cpp INSTRUCTION_ENCODING_VALIDATION test
    
    struct TestCase {
        const char* instruction;
        const char* description;
        uint32_t initial_sp;
        uint32_t expected_sp;
    };
    
    std::vector<TestCase> test_cases = {
        // ADD instructions
        {"add sp, #0x0",   "ADD SP, #0",   0x1000, 0x1000},
        {"add sp, #0x4",   "ADD SP, #4",   0x1000, 0x1004},
        {"add sp, #0x8",   "ADD SP, #8",   0x1000, 0x1008},
        {"add sp, #0x10",  "ADD SP, #16",  0x1000, 0x1010},
        {"add sp, #0x20",  "ADD SP, #32",  0x1000, 0x1020},
        {"add sp, #0x40",  "ADD SP, #64",  0x1000, 0x1040},
        {"add sp, #0x80",  "ADD SP, #128", 0x1000, 0x1080},
        {"add sp, #0x100", "ADD SP, #256", 0x1000, 0x1100},
        {"add sp, #0x1FC", "ADD SP, #508", 0x1000, 0x11FC},
        
        // SUB instructions
        {"sub sp, #0x0",   "SUB SP, #0",   0x1000, 0x1000},
        {"sub sp, #0x4",   "SUB SP, #4",   0x1000, 0x0FFC},
        {"sub sp, #0x8",   "SUB SP, #8",   0x1000, 0x0FF8},
        {"sub sp, #0x10",  "SUB SP, #16",  0x1000, 0x0FF0},
        {"sub sp, #0x20",  "SUB SP, #32",  0x1000, 0x0FE0},
        {"sub sp, #0x40",  "SUB SP, #64",  0x1000, 0x0FC0},
        {"sub sp, #0x80",  "SUB SP, #128", 0x1000, 0x0F80},
        {"sub sp, #0x100", "SUB SP, #256", 0x1000, 0x0F00},
        {"sub sp, #0x1FC", "SUB SP, #508", 0x1000, 0x0E04},
    };
    
    for (const auto& test : test_cases) {
        setup_registers({{13, test.initial_sp}, {15, 0x00000000}});  // Reset SP and PC
        
        // Use assembleAndWriteThumb and properly reset PC for each test
        assembleAndWriteThumb(test.instruction, 0x00000000);
        execute(1);
        
        EXPECT_EQ(R(13), test.expected_sp) 
            << test.description << " failed. Expected: 0x" << std::hex << test.expected_sp
            << ", Got: 0x" << R(13);
        EXPECT_EQ(R(15), 0x00000002u) << test.description << " - PC should advance to 0x2";
    }
}
