// test_thumb14.cpp - Modern Thumb CPU test fixture for Format 14: Load/store multiple operations
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <initializer_list>

extern "C" {
#include <keystone/keystone.h>
}

class ThumbCPUTest14 : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ThumbCPU thumb_cpu;
    ks_engine* ks; // Keystone handle

    ThumbCPUTest14() : memory(true), cpu(memory, interrupts), thumb_cpu(cpu), ks(nullptr) {}

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
        
        std::string full_assembly = ".syntax unified\n.thumb\n" + assembly;
        if (ks_asm(ks, full_assembly.c_str(), address, &machine_code, &machine_size, &statement_count) == KS_ERR_OK) {
            // Write the machine code to memory
            for (size_t i = 0; i < machine_size; i += 2) {
                uint16_t instruction = machine_code[i] | (machine_code[i+1] << 8);
                memory.write16(address + i, instruction);
            }
            ks_free(machine_code);
            return true;
        }
        return false;
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

// ARM Thumb Format 14: Load/store multiple operations (PUSH/POP)
// Encoding: 1011 [L]1[R]0 [register_list]
// L=0: PUSH, L=1: POP
// R=0: No LR/PC, R=1: Include LR (PUSH) or PC (POP)

TEST_F(ThumbCPUTest14, PUSH_SINGLE_REGISTER) {
    // Test case 1: PUSH {R0}
    setup_registers({{0, 0x12345678}, {13, 0x1000}});  // R0 data, SP
    
    if (!assemble_and_write_thumb("push {r0}", 0x00000000)) {
        // Keystone failed, use manual encoding
        memory.write16(0x00000000, 0xB401); // PUSH {R0}
    }
    
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1000u - 4u); // SP decremented by 4
    EXPECT_EQ(memory.read32(0x1000 - 4), 0x12345678u); // R0 pushed to stack
    EXPECT_EQ(cpu.R()[15], 0x00000002u); // PC incremented
    
    // Test case 2: PUSH {R7}
    setup_registers({{7, 0xDEADBEEF}, {13, 0x1800}});
    
    if (!assemble_and_write_thumb("push {r7}", 0x00000002)) {
        memory.write16(0x00000002, 0xB480); // PUSH {R7}
    }
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1800u - 4u);
    EXPECT_EQ(memory.read32(0x1800 - 4), 0xDEADBEEFu);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    
    // Test case 3: PUSH {R4}
    setup_registers({{4, 0xCAFEBABE}, {13, 0x1C00}});
    
    if (!assemble_and_write_thumb("push {r4}", 0x00000004)) {
        memory.write16(0x00000004, 0xB410); // PUSH {R4}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1C00u - 4u);
    EXPECT_EQ(memory.read32(0x1C00 - 4), 0xCAFEBABEu);
    EXPECT_EQ(cpu.R()[15], 0x00000006u);
}

TEST_F(ThumbCPUTest14, PUSH_MULTIPLE_REGISTERS) {
    // Test case 1: PUSH {R0, R1}
    setup_registers({{0, 0x11111111}, {1, 0x22222222}, {13, 0x1000}});
    
    if (!assemble_and_write_thumb("push {r0, r1}", 0x00000000)) {
        memory.write16(0x00000000, 0xB403); // PUSH {R0, R1}
    }
    
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1000u - 8u); // SP decremented by 8
    EXPECT_EQ(memory.read32(0x1000 - 8), 0x11111111u); // R0 (lower address)
    EXPECT_EQ(memory.read32(0x1000 - 4), 0x22222222u); // R1 (higher address)
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // Test case 2: PUSH {R4, R5, R6, R7}
    setup_registers({{4, 0x44444444}, {5, 0x55555555}, {6, 0x66666666}, {7, 0x77777777}, {13, 0x1800}});
    
    if (!assemble_and_write_thumb("push {r4, r5, r6, r7}", 0x00000002)) {
        memory.write16(0x00000002, 0xB4F0); // PUSH {R4, R5, R6, R7}
    }
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1800u - 16u); // SP decremented by 16
    EXPECT_EQ(memory.read32(0x1800 - 16), 0x44444444u); // R4
    EXPECT_EQ(memory.read32(0x1800 - 12), 0x55555555u); // R5
    EXPECT_EQ(memory.read32(0x1800 - 8), 0x66666666u);  // R6
    EXPECT_EQ(memory.read32(0x1800 - 4), 0x77777777u);  // R7
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    
    // Test case 3: PUSH {R0-R7} (all low registers)
    setup_registers({{13, 0x1C00}});
    for (int i = 0; i < 8; i++) {
        cpu.R()[i] = 0x10000000 + i;
    }
    
    if (!assemble_and_write_thumb("push {r0, r1, r2, r3, r4, r5, r6, r7}", 0x00000004)) {
        memory.write16(0x00000004, 0xB4FF); // PUSH {R0-R7}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1C00u - 32u); // SP decremented by 32 (8*4)
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(memory.read32(0x1C00 - 32 + (i * 4)), 0x10000000u + i);
    }
    EXPECT_EQ(cpu.R()[15], 0x00000006u);
}

TEST_F(ThumbCPUTest14, PUSH_WITH_LR) {
    // Test case 1: PUSH {R0, LR}
    setup_registers({{0, 0xAAAAAAAA}, {14, 0xBBBBBBBB}, {13, 0x1400}});
    
    if (!assemble_and_write_thumb("push {r0, lr}", 0x00000000)) {
        memory.write16(0x00000000, 0xB501); // PUSH {R0, LR}
    }
    
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1400u - 8u); // SP decremented by 8
    EXPECT_EQ(memory.read32(0x1400 - 8), 0xAAAAAAAAu); // R0
    EXPECT_EQ(memory.read32(0x1400 - 4), 0xBBBBBBBBu); // LR
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // Test case 2: PUSH {LR} only
    setup_registers({{14, 0x12345678}, {13, 0x1600}});
    
    if (!assemble_and_write_thumb("push {lr}", 0x00000002)) {
        memory.write16(0x00000002, 0xB500); // PUSH {LR}
    }
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1600u - 4u); // SP decremented by 4
    EXPECT_EQ(memory.read32(0x1600 - 4), 0x12345678u); // LR
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    
    // Test case 3: PUSH {R0-R7, LR}
    setup_registers({{14, 0xFEDCBA98}, {13, 0x1F00}});
    for (int i = 0; i < 8; i++) {
        cpu.R()[i] = 0x20000000 + i;
    }
    
    if (!assemble_and_write_thumb("push {r0, r1, r2, r3, r4, r5, r6, r7, lr}", 0x00000004)) {
        memory.write16(0x00000004, 0xB5FF); // PUSH {R0-R7, LR}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1F00u - 36u); // SP decremented by 36 (9*4)
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(memory.read32(0x1F00 - 36 + (i * 4)), 0x20000000u + i);
    }
    EXPECT_EQ(memory.read32(0x1F00 - 4), 0xFEDCBA98u); // LR at the end
    EXPECT_EQ(cpu.R()[15], 0x00000006u);
}

TEST_F(ThumbCPUTest14, POP_SINGLE_REGISTER) {
    // Test case 1: POP {R0}
    setup_registers({{13, 0x1000 - 4}});  // SP pointing to stack data
    memory.write32(0x1000 - 4, 0x87654321); // Data on stack
    
    if (!assemble_and_write_thumb("pop {r0}", 0x00000000)) {
        memory.write16(0x00000000, 0xBC01); // POP {R0}
    }
    
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x87654321u); // R0 loaded from stack
    EXPECT_EQ(cpu.R()[13], 0x1000u); // SP incremented by 4
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // Test case 2: POP {R3}
    setup_registers({{13, 0x1400 - 4}});
    memory.write32(0x1400 - 4, 0xDEADBEEF);
    
    if (!assemble_and_write_thumb("pop {r3}", 0x00000002)) {
        memory.write16(0x00000002, 0xBC08); // POP {R3}
    }
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 0xDEADBEEFu);
    EXPECT_EQ(cpu.R()[13], 0x1400u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    
    // Test case 3: POP {R7}
    setup_registers({{13, 0x1800 - 4}});
    memory.write32(0x1800 - 4, 0xCAFEBABE);
    
    if (!assemble_and_write_thumb("pop {r7}", 0x00000004)) {
        memory.write16(0x00000004, 0xBC80); // POP {R7}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[7], 0xCAFEBABEu);
    EXPECT_EQ(cpu.R()[13], 0x1800u);
    EXPECT_EQ(cpu.R()[15], 0x00000006u);
}

TEST_F(ThumbCPUTest14, POP_MULTIPLE_REGISTERS) {
    // Test case 1: POP {R0, R1}
    setup_registers({{13, 0x1000 - 8}});  // SP pointing to stack data
    memory.write32(0x1000 - 8, 0x11111111); // R0 data
    memory.write32(0x1000 - 4, 0x22222222); // R1 data
    
    if (!assemble_and_write_thumb("pop {r0, r1}", 0x00000000)) {
        memory.write16(0x00000000, 0xBC03); // POP {R0, R1}
    }
    
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x11111111u); // R0 loaded
    EXPECT_EQ(cpu.R()[1], 0x22222222u); // R1 loaded
    EXPECT_EQ(cpu.R()[13], 0x1000u); // SP incremented by 8
    EXPECT_EQ(cpu.R()[15], 0x00000002u);
    
    // Test case 2: POP {R4, R5, R6, R7}
    setup_registers({{13, 0x1400 - 16}});
    memory.write32(0x1400 - 16, 0x44444444); // R4 data
    memory.write32(0x1400 - 12, 0x55555555); // R5 data
    memory.write32(0x1400 - 8, 0x66666666);  // R6 data
    memory.write32(0x1400 - 4, 0x77777777);  // R7 data
    
    if (!assemble_and_write_thumb("pop {r4, r5, r6, r7}", 0x00000002)) {
        memory.write16(0x00000002, 0xBCF0); // POP {R4, R5, R6, R7}
    }
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[4], 0x44444444u);
    EXPECT_EQ(cpu.R()[5], 0x55555555u);
    EXPECT_EQ(cpu.R()[6], 0x66666666u);
    EXPECT_EQ(cpu.R()[7], 0x77777777u);
    EXPECT_EQ(cpu.R()[13], 0x1400u);
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    
    // Test case 3: POP {R0-R7} (all low registers)
    setup_registers({{13, 0x1800 - 32}});
    for (int i = 0; i < 8; i++) {
        memory.write32(0x1800 - 32 + (i * 4), 0x30000000u + i);
    }
    
    if (!assemble_and_write_thumb("pop {r0, r1, r2, r3, r4, r5, r6, r7}", 0x00000004)) {
        memory.write16(0x00000004, 0xBCFF); // POP {R0-R7}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(cpu.R()[i], 0x30000000u + i);
    }
    EXPECT_EQ(cpu.R()[13], 0x1800u);
    EXPECT_EQ(cpu.R()[15], 0x00000006u);
}

TEST_F(ThumbCPUTest14, POP_WITH_PC) {
    // Test case 1: POP {R0, PC}
    setup_registers({{13, 0x1000 - 8}});
    memory.write32(0x1000 - 8, 0xAAAAAAAA); // R0 data
    memory.write32(0x1000 - 4, 0x00000100); // PC data
    
    if (!assemble_and_write_thumb("pop {r0, pc}", 0x00000000)) {
        memory.write16(0x00000000, 0xBD01); // POP {R0, PC}
    }
    
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xAAAAAAAAu); // R0 loaded
    EXPECT_EQ(cpu.R()[15], 0x00000100u); // PC loaded from stack
    EXPECT_EQ(cpu.R()[13], 0x1000u); // SP incremented by 8
    
    // Test case 2: POP {PC} only
    setup_registers({{13, 0x1400 - 4}});
    memory.write32(0x1400 - 4, 0x00000200); // PC data
    
    if (!assemble_and_write_thumb("pop {pc}", 0x00000002)) {
        memory.write16(0x00000002, 0xBD00); // POP {PC}
    }
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[15], 0x00000200u); // PC loaded from stack
    EXPECT_EQ(cpu.R()[13], 0x1400u); // SP incremented by 4
    
    // Test case 3: POP {R0-R7, PC}
    setup_registers({{13, 0x1800 - 36}}); // 8 registers + PC
    for (int i = 0; i < 8; i++) {
        memory.write32(0x1800 - 36 + (i * 4), 0x40000000u + i);
    }
    memory.write32(0x1800 - 4, 0x00000300); // PC data
    
    if (!assemble_and_write_thumb("pop {r0, r1, r2, r3, r4, r5, r6, r7, pc}", 0x00000004)) {
        memory.write16(0x00000004, 0xBDFF); // POP {R0-R7, PC}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(cpu.R()[i], 0x40000000u + i);
    }
    EXPECT_EQ(cpu.R()[15], 0x00000300u); // PC loaded
    EXPECT_EQ(cpu.R()[13], 0x1800u); // SP incremented by 36
}

TEST_F(ThumbCPUTest14, PUSH_POP_ROUNDTRIP) {
    // Test case 1: PUSH then POP same registers
    setup_registers({{0, 0x11111111}, {1, 0x22222222}, {2, 0x33333333}, {13, 0x1500}});
    
    // PUSH {R0, R1, R2}
    if (!assemble_and_write_thumb("push {r0, r1, r2}", 0x00000000)) {
        memory.write16(0x00000000, 0xB407); // PUSH {R0, R1, R2}
    }
    
    thumb_cpu.execute(1);
    
    // Verify stack state
    EXPECT_EQ(cpu.R()[13], 0x1500u - 12u); // SP decremented
    EXPECT_EQ(memory.read32(0x1500 - 12), 0x11111111u); // R0
    EXPECT_EQ(memory.read32(0x1500 - 8), 0x22222222u);  // R1
    EXPECT_EQ(memory.read32(0x1500 - 4), 0x33333333u);  // R2
    
    // Clear registers
    cpu.R()[0] = 0;
    cpu.R()[1] = 0;
    cpu.R()[2] = 0;
    
    // POP {R0, R1, R2}
    if (!assemble_and_write_thumb("pop {r0, r1, r2}", 0x00000002)) {
        memory.write16(0x00000002, 0xBC07); // POP {R0, R1, R2}
    }
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    // Verify restoration
    EXPECT_EQ(cpu.R()[0], 0x11111111u);
    EXPECT_EQ(cpu.R()[1], 0x22222222u);
    EXPECT_EQ(cpu.R()[2], 0x33333333u);
    EXPECT_EQ(cpu.R()[13], 0x1500u); // SP restored
    
    // Test case 2: PUSH with LR, POP with PC
    setup_registers({{0, 0xABCDEF01}, {14, 0x00000100}, {13, 0x1600}});
    
    // PUSH {R0, LR}
    if (!assemble_and_write_thumb("push {r0, lr}", 0x00000004)) {
        memory.write16(0x00000004, 0xB501); // PUSH {R0, LR}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1600u - 8u);
    
    // Clear registers
    cpu.R()[0] = 0;
    
    // POP {R0, PC} - this should restore R0 and jump to LR value
    if (!assemble_and_write_thumb("pop {r0, pc}", 0x00000006)) {
        memory.write16(0x00000006, 0xBD01); // POP {R0, PC}
    }
    
    cpu.R()[15] = 0x00000006;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0xABCDEF01u); // R0 restored
    EXPECT_EQ(cpu.R()[15], 0x00000100u); // PC = original LR
    EXPECT_EQ(cpu.R()[13], 0x1600u); // SP restored
}

TEST_F(ThumbCPUTest14, EDGE_CASES) {
    // Test case 1: Empty register list PUSH
    setup_registers({{13, 0x1000}});
    
    // Manual encoding for empty list (may not be valid assembly)
    memory.write16(0x00000000, 0xB400); // PUSH {} (empty list)
    
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1000u); // SP unchanged (no registers to push)
    EXPECT_EQ(cpu.R()[15], 0x00000002u); // Only PC should change
    
    // Test case 2: Empty register list POP
    setup_registers({{13, 0x1000}});
    
    memory.write16(0x00000002, 0xBC00); // POP {} (empty list)
    
    cpu.R()[15] = 0x00000002;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1000u); // SP unchanged
    EXPECT_EQ(cpu.R()[15], 0x00000004u);
    
    // Test case 3: PUSH/POP at memory boundaries (respecting 0x1FFF limit)
    setup_registers({{0, 0x12345678}, {13, 0x1FFC}}); // Near top of memory
    
    if (!assemble_and_write_thumb("push {r0}", 0x00000004)) {
        memory.write16(0x00000004, 0xB401); // PUSH {R0}
    }
    
    cpu.R()[15] = 0x00000004;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[13], 0x1FFCu - 4u);
    EXPECT_EQ(memory.read32(0x1FFC - 4), 0x12345678u);
    
    // POP it back
    cpu.R()[0] = 0;
    if (!assemble_and_write_thumb("pop {r0}", 0x00000006)) {
        memory.write16(0x00000006, 0xBC01); // POP {R0}
    }
    
    cpu.R()[15] = 0x00000006;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x12345678u);
    EXPECT_EQ(cpu.R()[13], 0x1FFCu);
    
    // Test case 4: Zero and maximum values
    setup_registers({{0, 0x00000000}, {1, 0x00000001}, {7, 0xFFFFFFFF}, {13, 0x1000}});
    
    if (!assemble_and_write_thumb("push {r0, r1}", 0x00000008)) {
        memory.write16(0x00000008, 0xB403); // PUSH {R0, R1}
    }
    
    cpu.R()[15] = 0x00000008;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(memory.read32(0x1000 - 8), 0x00000000u); // Zero preserved
    EXPECT_EQ(memory.read32(0x1000 - 4), 0x00000001u);
    
    // Test maximum value with R7
    cpu.R()[13] = 0x1200;
    if (!assemble_and_write_thumb("push {r7}", 0x0000000A)) {
        memory.write16(0x0000000A, 0xB480); // PUSH {R7}
    }
    
    cpu.R()[15] = 0x0000000A;
    thumb_cpu.execute(1);
    
    EXPECT_EQ(memory.read32(0x1200 - 4), 0xFFFFFFFFu);
    
    // Pop back to verify
    cpu.R()[0] = 0xFF;
    cpu.R()[1] = 0xFF;
    cpu.R()[7] = 0;
    
    if (!assemble_and_write_thumb("pop {r0, r1}", 0x0000000C)) {
        memory.write16(0x0000000C, 0xBC03); // POP {R0, R1}
    }
    
    cpu.R()[15] = 0x0000000C;
    cpu.R()[13] = 0x1000 - 8; // Reset SP to where we pushed R0, R1
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 0x00000000u); // Zero correctly popped
    EXPECT_EQ(cpu.R()[1], 0x00000001u);
    
    if (!assemble_and_write_thumb("pop {r7}", 0x0000000E)) {
        memory.write16(0x0000000E, 0xBC80); // POP {R7}
    }
    
    cpu.R()[15] = 0x0000000E;
    cpu.R()[13] = 0x1200 - 4; // Reset SP to where we pushed R7
    thumb_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[7], 0xFFFFFFFFu);
}
