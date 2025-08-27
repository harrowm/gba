// test_thumb13.cpp - Modern Thumb CPU test fixture for Format 13: Stack operations
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <initializer_list>

extern "C" {
#include <keystone/keystone.h>
}

class ThumbCPUTest13 : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks; // Keystone handle

    ThumbCPUTest13() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu), ks(nullptr) {}

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
        
        // Force Thumb-1 mode with .thumb directive
        std::string full_assembly = ".thumb\n" + assembly;
        
        int err = ks_asm(ks, full_assembly.c_str(), address, &machine_code, &machine_size, &statement_count);
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

// ARM Thumb Format 13: Add/Subtract offset to Stack Pointer
// Encoding: 1011 0000 S [offset7]
// S=0: ADD SP, #imm (SP = SP + (offset7 * 4))
// S=1: SUB SP, #imm (SP = SP - (offset7 * 4))
// Offset range: 0-508 bytes (0-127 * 4)

TEST_F(ThumbCPUTest13, AddSpImmediateBasic) {
    // Test case: ADD SP, #0 - no change
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for ADD SP, #0: 0xB000
    memory.write16(0x00000000, 0xB000);
    thumb_cpu.execute(1);
    
    // SP should remain unchanged
    EXPECT_EQ(cpu.R()[13], 0x00001000u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateSmall) {
    // Test case: ADD SP, #4 - basic increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for ADD SP, #4: 0xB001
    memory.write16(0x00000000, 0xB001);
    thumb_cpu.execute(1);
    
    // SP should be incremented by 4
    EXPECT_EQ(cpu.R()[13], 0x00001004u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateMedium) {
    // Test case: ADD SP, #32 - medium increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=0 offset7=8 (32/4)
    memory.write16(0x00000000, 0xB008);  // ADD SP, #32
    thumb_cpu.execute(1);
    
    // SP should be incremented by 32
    EXPECT_EQ(cpu.R()[13], 0x00001020u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateLarge) {
    // Test case: ADD SP, #128 - large increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=0 offset7=32 (128/4)
    memory.write16(0x00000000, 0xB020);  // ADD SP, #128
    thumb_cpu.execute(1);
    
    // SP should be incremented by 128
    EXPECT_EQ(cpu.R()[13], 0x00001080u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, AddSpImmediateMaximum) {
    // Test case: ADD SP, #508 - maximum increment
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=0 offset7=127 (508/4)
    memory.write16(0x00000000, 0xB07F);  // ADD SP, #508
    thumb_cpu.execute(1);
    
    // SP should be incremented by 508
    EXPECT_EQ(cpu.R()[13], 0x000011FCu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateBasic) {
    // Test case: SUB SP, #0 - no change
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for SUB SP, #0: 0xB080
    memory.write16(0x00000000, 0xB080);
    thumb_cpu.execute(1);
    
    // SP should remain unchanged
    EXPECT_EQ(cpu.R()[13], 0x00001000u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateSmall) {
    // Test case: SUB SP, #4 - basic decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual encoding for SUB SP, #4: 0xB081
    memory.write16(0x00000000, 0xB081);
    thumb_cpu.execute(1);
    
    // SP should be decremented by 4
    EXPECT_EQ(cpu.R()[13], 0x00000FFCu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateMedium) {
    // Test case: SUB SP, #32 - medium decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=1 offset7=8 (32/4)
    memory.write16(0x00000000, 0xB088);  // SUB SP, #32
    thumb_cpu.execute(1);
    
    // SP should be decremented by 32
    EXPECT_EQ(cpu.R()[13], 0x00000FE0u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateLarge) {
    // Test case: SUB SP, #128 - large decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=1 offset7=32 (128/4)
    memory.write16(0x00000000, 0xB0A0);  // SUB SP, #128
    thumb_cpu.execute(1);
    
    // SP should be decremented by 128
    EXPECT_EQ(cpu.R()[13], 0x00000F80u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, SubSpImmediateMaximum) {
    // Test case: SUB SP, #508 - maximum decrement
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: 1011 0000 S=1 offset7=127 (508/4)
    memory.write16(0x00000000, 0xB0FF);  // SUB SP, #508
    thumb_cpu.execute(1);
    
    // SP should be decremented by 508
    EXPECT_EQ(cpu.R()[13], 0x00000E04u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, OffsetRangeValidation) {
    // Test various valid offsets that are multiples of 4
    uint32_t test_offsets[] = {0, 4, 8, 12, 16, 20, 32, 64, 128, 256, 508};
    
    for (uint32_t offset : test_offsets) {
        // Test ADD SP
        cpu.R().fill(0);
        cpu.R()[13] = 0x00001000;
        cpu.R()[15] = 0x00000000;
        
        // Manual instruction encoding for ADD: 1011 0000 0 [offset7]
        uint16_t offset7 = offset / 4;
        uint16_t add_opcode = 0xB000 | offset7;
        memory.write16(0x00000000, add_opcode);
        thumb_cpu.execute(1);
        
        uint32_t expected_sp = 0x00001000 + offset;
        EXPECT_EQ(cpu.R()[13], expected_sp) 
            << "ADD SP, #" << offset << " failed. Expected: 0x" 
            << std::hex << expected_sp << ", Got: 0x" << cpu.R()[13];
        
        // Test SUB SP (only if SP won't underflow too much)
        if (offset <= 0x1000) {
            cpu.R().fill(0);
            cpu.R()[13] = 0x00001000;
            cpu.R()[15] = 0x00000000;
            
            // Manual instruction encoding for SUB: 1011 0000 1 [offset7]
            uint16_t sub_opcode = 0xB080 | offset7;
            memory.write16(0x00000000, sub_opcode);
            thumb_cpu.execute(1);
            
            uint32_t expected_sp_sub = 0x00001000 - offset;
            EXPECT_EQ(cpu.R()[13], expected_sp_sub)
                << "SUB SP, #" << offset << " failed. Expected: 0x" 
                << std::hex << expected_sp_sub << ", Got: 0x" << cpu.R()[13];
        }
    }
}

TEST_F(ThumbCPUTest13, AddSubSequenceTest) {
    // Test ADD then SUB same amount - should return to original
    setup_registers({{13, 0x00001000}});  // Set SP
    uint32_t initial_sp = cpu.R()[13];
    
    // ADD SP, #32 - Manual encoding: 0xB008
    memory.write16(0x00000000, 0xB008);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], initial_sp + 32);
    
    // SUB SP, #32 - Manual encoding: 0xB088
    cpu.R()[15] = 0x00000000; // Reset PC
    memory.write16(0x00000000, 0xB088);
    thumb_cpu.execute(1);
    
    // Should be back to original value
    EXPECT_EQ(cpu.R()[13], initial_sp);
}

TEST_F(ThumbCPUTest13, MultipleAddOperations) {
    // Test multiple ADD operations accumulate correctly
    setup_registers({{13, 0x00001000}});  // Set SP
    uint32_t initial_sp = cpu.R()[13];
    
    // ADD SP, #16 three times - Manual encoding: 0xB004
    for (int i = 0; i < 3; i++) {
        cpu.R()[15] = 0x00000000; // Reset PC
        memory.write16(0x00000000, 0xB004);  // ADD SP, #16
        thumb_cpu.execute(1);
        EXPECT_EQ(cpu.R()[13], initial_sp + 16 * (i + 1));
    }
    
    // Final SP should be initial + 48
    EXPECT_EQ(cpu.R()[13], initial_sp + 48);
}

TEST_F(ThumbCPUTest13, MultipleSubOperations) {
    // Test multiple SUB operations accumulate correctly
    setup_registers({{13, 0x00001200}});  // Higher starting point for SUB
    uint32_t initial_sp = cpu.R()[13];
    
    // SUB SP, #16 three times - Manual encoding: 0xB084
    for (int i = 0; i < 3; i++) {
        cpu.R()[15] = 0x00000000; // Reset PC
        memory.write16(0x00000000, 0xB084);  // SUB SP, #16
        thumb_cpu.execute(1);
        EXPECT_EQ(cpu.R()[13], initial_sp - 16 * (i + 1));
    }
    
    // Final SP should be initial - 48
    EXPECT_EQ(cpu.R()[13], initial_sp - 48);
}

TEST_F(ThumbCPUTest13, MemoryBoundaryAddTest) {
    // Test SP near memory boundary with ADD
    setup_registers({{13, 0x00001F00}});  // Near end of test memory (0x1FFF)
    
    // Manual encoding for ADD SP, #4: 0xB001
    memory.write16(0x00000000, 0xB001);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x00001F04u);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, MemoryBoundarySubTest) {
    // Test SP near memory boundary with SUB
    setup_registers({{13, 0x00000100}});  // Near start of memory
    
    // Manual encoding for SUB SP, #4: 0xB081
    memory.write16(0x00000000, 0xB081);
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x000000FCu);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, SpOverflowTest) {
    // Test SP overflow (ADD maximum to high value)
    setup_registers({{13, 0xFFFFFF00}});  // High value that will overflow
    
    // Manual encoding for ADD SP, #508: 0xB07F
    memory.write16(0x00000000, 0xB07F);
    thumb_cpu.execute(1);
    
    // Should wrap around due to 32-bit arithmetic
    uint32_t expected = 0xFFFFFF00 + 508;
    EXPECT_EQ(cpu.R()[13], expected);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, SpUnderflowTest) {
    // Test SP underflow (SUB from low value)
    setup_registers({{13, 0x00000100}});  // Low value
    
    // Manual encoding for SUB SP, #508: 0xB0FF
    memory.write16(0x00000000, 0xB0FF);
    thumb_cpu.execute(1);
    
    // Should wrap around due to 32-bit arithmetic
    uint32_t expected = 0x00000100 - 508;
    EXPECT_EQ(cpu.R()[13], expected);
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, OtherRegistersUnaffected) {
    // Test that other registers are not affected by SP operations
    setup_registers({{0, 0xDEADBEEF}, {1, 0xCAFEBABE}, {13, 0x00001000}});
    
    // Manual encoding for ADD SP, #64: 0xB010
    memory.write16(0x00000000, 0xB010);
    thumb_cpu.execute(1);
    
    // Verify SP was modified correctly
    EXPECT_EQ(cpu.R()[13], 0x00001040u);
    // Verify other registers unchanged
    EXPECT_EQ(cpu.R()[0], 0xDEADBEEFu);
    EXPECT_EQ(cpu.R()[1], 0xCAFEBABEu);
    // Verify PC was incremented
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
}

TEST_F(ThumbCPUTest13, FlagsUnaffected) {
    // Test that CPSR flags are unaffected by SP operations
    setup_registers({{13, 0x00001000}});
    
    // Set various CPSR flags
    cpu.CPSR() |= CPU::FLAG_N | CPU::FLAG_Z | CPU::FLAG_C | CPU::FLAG_V;
    uint32_t original_cpsr = cpu.CPSR();
    
    // Manual encoding for ADD SP, #32: 0xB008
    memory.write16(0x00000000, 0xB008);
    thumb_cpu.execute(1);
    
    // CPSR should be unchanged
    EXPECT_EQ(cpu.CPSR(), original_cpsr);
    EXPECT_EQ(cpu.R()[13], 0x00001020u);
}

TEST_F(ThumbCPUTest13, InstructionEncodingValidation) {
    // Test comprehensive instruction encoding validation
    // Based on the original format13_stack_operations.cpp INSTRUCTION_ENCODING_VALIDATION test
    
    struct TestCase {
        uint16_t opcode;
        const char* description;
        uint32_t initial_sp;
        uint32_t expected_sp;
    };
    
    std::vector<TestCase> test_cases = {
        // ADD instructions
        {0xB000, "ADD SP, #0",   0x1000, 0x1000},
        {0xB001, "ADD SP, #4",   0x1000, 0x1004},
        {0xB002, "ADD SP, #8",   0x1000, 0x1008},
        {0xB004, "ADD SP, #16",  0x1000, 0x1010},
        {0xB008, "ADD SP, #32",  0x1000, 0x1020},
        {0xB010, "ADD SP, #64",  0x1000, 0x1040},
        {0xB020, "ADD SP, #128", 0x1000, 0x1080},
        {0xB040, "ADD SP, #256", 0x1000, 0x1100},
        {0xB07F, "ADD SP, #508", 0x1000, 0x11FC},
        
        // SUB instructions
        {0xB080, "SUB SP, #0",   0x1000, 0x1000},
        {0xB081, "SUB SP, #4",   0x1000, 0x0FFC},
        {0xB082, "SUB SP, #8",   0x1000, 0x0FF8},
        {0xB084, "SUB SP, #16",  0x1000, 0x0FF0},
        {0xB088, "SUB SP, #32",  0x1000, 0x0FE0},
        {0xB090, "SUB SP, #64",  0x1000, 0x0FC0},
        {0xB0A0, "SUB SP, #128", 0x1000, 0x0F80},
        {0xB0C0, "SUB SP, #256", 0x1000, 0x0F00},
        {0xB0FF, "SUB SP, #508", 0x1000, 0x0E04},
    };
    
    for (const auto& test : test_cases) {
        setup_registers({{13, test.initial_sp}});
        
        // Write the opcode directly to memory
        memory.write16(0x00000000, test.opcode);
        thumb_cpu.execute(1);
        
        EXPECT_EQ(cpu.R()[13], test.expected_sp) 
            << test.description << " failed. Expected: 0x" << std::hex << test.expected_sp
            << ", Got: 0x" << cpu.R()[13];
        EXPECT_EQ(cpu.R()[15], 0x00000002u) << test.description << " - PC should advance to 0x2";
    }
}
