#include <gtest/gtest.h>
#include "memory.h"
#include "cpu.h"
#include "thumb_cpu.h"
#include <keystone/keystone.h>

class ThumbCPUTest14 : public ::testing::Test {
protected:
    Memory* memory;
    CPU* cpu;
    ThumbCPU* thumb_cpu;
    ks_engine* ks;

    void SetUp() override {
        memory = new Memory(true);
        cpu = new CPU(*memory, nullptr);
        thumb_cpu = new ThumbCPU(*cpu);
        
        // Initialize Keystone for Thumb mode
        if (ks_open(KS_ARCH_ARM, KS_MODE_THUMB, &ks) != KS_ERR_OK) {
            throw std::runtime_error("Failed to initialize Keystone");
        }
        
        // Set syntax to GAS (GNU Assembler)
        if (ks_option(ks, KS_OPT_SYNTAX, KS_OPT_SYNTAX_GAS) != KS_ERR_OK) {
            throw std::runtime_error("Failed to set Keystone syntax");
        }
    }

    void TearDown() override {
        ks_close(ks);
        delete thumb_cpu;
        delete cpu;
        delete memory;
    }

    void assemble_and_load(const std::string& assembly, uint32_t address) {
        std::string code = ".thumb\n" + assembly;
        
        unsigned char* encode;
        size_t encode_size;
        size_t statement_count;
        
        if (ks_asm(ks, code.c_str(), address, &encode, &encode_size, &statement_count) == KS_ERR_OK) {
            for (size_t i = 0; i < encode_size; i += 2) {
                if (i + 1 < encode_size) {
                    uint16_t instruction = encode[i] | (encode[i+1] << 8);
                    memory->write16(address + i, instruction);
                }
            }
            ks_free(encode);
        } else {
            throw std::runtime_error("Failed to assemble: " + assembly);
        }
    }
};

TEST_F(ThumbCPUTest14, PUSH_SINGLE_REGISTER) {
    // Test case 1: PUSH {R0}
    {
        cpu->set_register(0, 0x12345678);
        cpu->set_register(13, 0x1000); // SP
        cpu->set_cpsr(cpu->get_cpsr() | 0x20); // Thumb mode
        
        try {
            assemble_and_load("push {r0}", 0x0000);
        } catch (...) {
            // Keystone failed, use manual encoding
            memory->write16(0x0000, 0xB401); // PUSH {R0}
        }
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1000 - 4); // SP decremented by 4
        EXPECT_EQ(memory->read32(0x1000 - 4), 0x12345678); // R0 pushed to stack
    }

    // Test case 2: PUSH {R1}
    {
        cpu->set_register(1, 0x87654321);
        cpu->set_register(13, 0x1200);
        
        try {
            assemble_and_load("push {r1}", 0x0002);
        } catch (...) {
            memory->write16(0x0002, 0xB402); // PUSH {R1}
        }
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1200 - 4);
        EXPECT_EQ(memory->read32(0x1200 - 4), 0x87654321);
    }

    // Test case 3: PUSH {R7}
    {
        cpu->set_register(7, 0xABCDEF01);
        cpu->set_register(13, 0x1400);
        
        try {
            assemble_and_load("push {r7}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xB480); // PUSH {R7}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1400 - 4);
        EXPECT_EQ(memory->read32(0x1400 - 4), 0xABCDEF01);
    }
}

TEST_F(ThumbCPUTest14, PUSH_MULTIPLE_REGISTERS) {
    // Test case 1: PUSH {R0, R1}
    {
        cpu->set_register(0, 0x11111111);
        cpu->set_register(1, 0x22222222);
        cpu->set_register(13, 0x1000);
        cpu->set_cpsr(cpu->get_cpsr() | 0x20);
        
        try {
            assemble_and_load("push {r0, r1}", 0x0000);
        } catch (...) {
            memory->write16(0x0000, 0xB403); // PUSH {R0, R1}
        }
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1000 - 8); // SP decremented by 8
        EXPECT_EQ(memory->read32(0x1000 - 8), 0x11111111); // R0 (lower register first)
        EXPECT_EQ(memory->read32(0x1000 - 4), 0x22222222); // R1
    }

    // Test case 2: PUSH {R0, R1, R2}
    {
        cpu->set_register(0, 0x33333333);
        cpu->set_register(1, 0x44444444);
        cpu->set_register(2, 0x55555555);
        cpu->set_register(13, 0x1200);
        
        try {
            assemble_and_load("push {r0, r1, r2}", 0x0002);
        } catch (...) {
            memory->write16(0x0002, 0xB407); // PUSH {R0, R1, R2}
        }
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1200 - 12); // SP decremented by 12
        EXPECT_EQ(memory->read32(0x1200 - 12), 0x33333333); // R0
        EXPECT_EQ(memory->read32(0x1200 - 8), 0x44444444);  // R1
        EXPECT_EQ(memory->read32(0x1200 - 4), 0x55555555);  // R2
    }

    // Test case 3: PUSH {R0-R7} (all low registers)
    {
        for (int i = 0; i < 8; i++) {
            cpu->set_register(i, 0x10000000 + i);
        }
        cpu->set_register(13, 0x1400);
        
        try {
            assemble_and_load("push {r0, r1, r2, r3, r4, r5, r6, r7}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xB4FF); // PUSH {R0-R7}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1400 - 32); // SP decremented by 32 (8*4)
        for (int i = 0; i < 8; i++) {
            EXPECT_EQ(memory->read32(0x1400 - 32 + (i * 4)), 0x10000000 + i);
        }
    }
}

TEST_F(ThumbCPUTest14, PUSH_WITH_LR) {
    // Test case 1: PUSH {R0, LR}
    {
        cpu->set_register(0, 0xAAAABBBB);
        cpu->set_register(14, 0x00001000); // LR
        cpu->set_register(13, 0x1500); // SP
        cpu->set_cpsr(cpu->get_cpsr() | 0x20);
        
        try {
            assemble_and_load("push {r0, lr}", 0x0000);
        } catch (...) {
            memory->write16(0x0000, 0xB501); // PUSH {R0, LR}
        }
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1500 - 8); // SP decremented by 8
        EXPECT_EQ(memory->read32(0x1500 - 8), 0xAAAABBBB); // R0
        EXPECT_EQ(memory->read32(0x1500 - 4), 0x00001000); // LR
    }

    // Test case 2: PUSH {LR} only
    {
        cpu->set_register(14, 0x12345678);
        cpu->set_register(13, 0x1600);
        
        try {
            assemble_and_load("push {lr}", 0x0002);
        } catch (...) {
            memory->write16(0x0002, 0xB500); // PUSH {LR}
        }
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1600 - 4); // SP decremented by 4
        EXPECT_EQ(memory->read32(0x1600 - 4), 0x12345678); // LR
    }

    // Test case 3: PUSH {R0-R7, LR}
    {
        for (int i = 0; i < 8; i++) {
            cpu->set_register(i, 0x20000000 + i);
        }
        cpu->set_register(14, 0x87654321);
        cpu->set_register(13, 0x1700);
        
        try {
            assemble_and_load("push {r0, r1, r2, r3, r4, r5, r6, r7, lr}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xB5FF); // PUSH {R0-R7, LR}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1700 - 36); // SP decremented by 36 (9*4)
        for (int i = 0; i < 8; i++) {
            EXPECT_EQ(memory->read32(0x1700 - 36 + (i * 4)), 0x20000000 + i);
        }
        EXPECT_EQ(memory->read32(0x1700 - 4), 0x87654321); // LR at the end
    }
}

TEST_F(ThumbCPUTest14, POP_SINGLE_REGISTER) {
    // Test case 1: POP {R0}
    {
        cpu->set_register(13, 0x1000 - 4); // SP pointing to stack data
        memory->write32(0x1000 - 4, 0x12345678); // Data on stack
        cpu->set_cpsr(cpu->get_cpsr() | 0x20);
        
        try {
            assemble_and_load("pop {r0}", 0x0000);
        } catch (...) {
            memory->write16(0x0000, 0xBC01); // POP {R0}
        }
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(0), 0x12345678); // R0 loaded from stack
        EXPECT_EQ(cpu->get_register(13), 0x1000); // SP incremented by 4
    }

    // Test case 2: POP {R3}
    {
        cpu->set_register(13, 0x1200 - 4);
        memory->write32(0x1200 - 4, 0x87654321);
        
        try {
            assemble_and_load("pop {r3}", 0x0002);
        } catch (...) {
            memory->write16(0x0002, 0xBC08); // POP {R3}
        }
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(3), 0x87654321);
        EXPECT_EQ(cpu->get_register(13), 0x1200);
    }

    // Test case 3: POP {R7}
    {
        cpu->set_register(13, 0x1400 - 4);
        memory->write32(0x1400 - 4, 0xABCDEF01);
        
        try {
            assemble_and_load("pop {r7}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xBC80); // POP {R7}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(7), 0xABCDEF01);
        EXPECT_EQ(cpu->get_register(13), 0x1400);
    }
}

TEST_F(ThumbCPUTest14, POP_MULTIPLE_REGISTERS) {
    // Test case 1: POP {R0, R1}
    {
        cpu->set_register(13, 0x1000 - 8); // SP pointing to stack data
        memory->write32(0x1000 - 8, 0x11111111); // R0 data
        memory->write32(0x1000 - 4, 0x22222222); // R1 data
        cpu->set_cpsr(cpu->get_cpsr() | 0x20);
        
        try {
            assemble_and_load("pop {r0, r1}", 0x0000);
        } catch (...) {
            memory->write16(0x0000, 0xBC03); // POP {R0, R1}
        }
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(0), 0x11111111); // R0 loaded
        EXPECT_EQ(cpu->get_register(1), 0x22222222); // R1 loaded
        EXPECT_EQ(cpu->get_register(13), 0x1000); // SP incremented by 8
    }

    // Test case 2: POP {R1, R2, R4}
    {
        cpu->set_register(13, 0x1200 - 12);
        memory->write32(0x1200 - 12, 0x33333333); // R1 data
        memory->write32(0x1200 - 8, 0x44444444);  // R2 data
        memory->write32(0x1200 - 4, 0x55555555);  // R4 data
        
        try {
            assemble_and_load("pop {r1, r2, r4}", 0x0002);
        } catch (...) {
            memory->write16(0x0002, 0xBC16); // POP {R1, R2, R4}
        }
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(1), 0x33333333);
        EXPECT_EQ(cpu->get_register(2), 0x44444444);
        EXPECT_EQ(cpu->get_register(4), 0x55555555);
        EXPECT_EQ(cpu->get_register(13), 0x1200);
    }

    // Test case 3: POP {R0-R7} (all low registers)
    {
        cpu->set_register(13, 0x1400 - 32);
        for (int i = 0; i < 8; i++) {
            memory->write32(0x1400 - 32 + (i * 4), 0x60000000 + i);
        }
        
        try {
            assemble_and_load("pop {r0, r1, r2, r3, r4, r5, r6, r7}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xBCFF); // POP {R0-R7}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        for (int i = 0; i < 8; i++) {
            EXPECT_EQ(cpu->get_register(i), 0x60000000 + i);
        }
        EXPECT_EQ(cpu->get_register(13), 0x1400);
    }
}

TEST_F(ThumbCPUTest14, POP_WITH_PC) {
    // Test case 1: POP {R0, PC}
    {
        cpu->set_register(13, 0x1000 - 8);
        memory->write32(0x1000 - 8, 0xAAAAAAAA); // R0 data
        memory->write32(0x1000 - 4, 0x00000100); // PC data
        cpu->set_cpsr(cpu->get_cpsr() | 0x20);
        
        try {
            assemble_and_load("pop {r0, pc}", 0x0000);
        } catch (...) {
            memory->write16(0x0000, 0xBD01); // POP {R0, PC}
        }
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(0), 0xAAAAAAAA); // R0 loaded
        EXPECT_EQ(cpu->get_register(15), 0x00000100); // PC loaded from stack
        EXPECT_EQ(cpu->get_register(13), 0x1000); // SP incremented by 8
    }

    // Test case 2: POP {PC} only
    {
        cpu->set_register(13, 0x1400 - 4);
        memory->write32(0x1400 - 4, 0x00000200); // PC data
        
        try {
            assemble_and_load("pop {pc}", 0x0002);
        } catch (...) {
            memory->write16(0x0002, 0xBD00); // POP {PC}
        }
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(15), 0x00000200); // PC loaded from stack
        EXPECT_EQ(cpu->get_register(13), 0x1400); // SP incremented by 4
    }

    // Test case 3: POP {R0-R7, PC}
    {
        cpu->set_register(13, 0x1800 - 36); // 8 registers + PC
        for (int i = 0; i < 8; i++) {
            memory->write32(0x1800 - 36 + (i * 4), 0x40000000 + i);
        }
        memory->write32(0x1800 - 4, 0x00000300); // PC data
        
        try {
            assemble_and_load("pop {r0, r1, r2, r3, r4, r5, r6, r7, pc}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xBDFF); // POP {R0-R7, PC}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        for (int i = 0; i < 8; i++) {
            EXPECT_EQ(cpu->get_register(i), 0x40000000 + i);
        }
        EXPECT_EQ(cpu->get_register(15), 0x00000300); // PC loaded
        EXPECT_EQ(cpu->get_register(13), 0x1800); // SP incremented by 36
    }
}

TEST_F(ThumbCPUTest14, PUSH_POP_ROUNDTRIP) {
    // Test case 1: PUSH then POP same registers
    {
        // Set up initial values
        cpu->set_register(0, 0x11111111);
        cpu->set_register(1, 0x22222222);
        cpu->set_register(2, 0x33333333);
        cpu->set_register(13, 0x1500); // SP
        cpu->set_cpsr(cpu->get_cpsr() | 0x20);
        
        // PUSH {R0, R1, R2}
        try {
            assemble_and_load("push {r0, r1, r2}", 0x0000);
        } catch (...) {
            memory->write16(0x0000, 0xB407); // PUSH {R0, R1, R2}
        }
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        // Verify stack state
        EXPECT_EQ(cpu->get_register(13), 0x1500 - 12); // SP decremented
        EXPECT_EQ(memory->read32(0x1500 - 12), 0x11111111); // R0
        EXPECT_EQ(memory->read32(0x1500 - 8), 0x22222222);  // R1
        EXPECT_EQ(memory->read32(0x1500 - 4), 0x33333333);  // R2
        
        // Clear registers
        cpu->set_register(0, 0);
        cpu->set_register(1, 0);
        cpu->set_register(2, 0);
        
        // POP {R0, R1, R2}
        try {
            assemble_and_load("pop {r0, r1, r2}", 0x0002);
        } catch (...) {
            memory->write16(0x0002, 0xBC07); // POP {R0, R1, R2}
        }
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        // Verify restoration
        EXPECT_EQ(cpu->get_register(0), 0x11111111);
        EXPECT_EQ(cpu->get_register(1), 0x22222222);
        EXPECT_EQ(cpu->get_register(2), 0x33333333);
        EXPECT_EQ(cpu->get_register(13), 0x1500); // SP restored
    }

    // Test case 2: PUSH with LR, POP with PC
    {
        cpu->set_register(0, 0xABCDEF01);
        cpu->set_register(14, 0x00000100); // LR (return address)
        cpu->set_register(13, 0x1600);
        
        // PUSH {R0, LR}
        try {
            assemble_and_load("push {r0, lr}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xB501); // PUSH {R0, LR}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1600 - 8);
        
        // Clear registers
        cpu->set_register(0, 0);
        
        // POP {R0, PC} - this should restore R0 and jump to LR value
        try {
            assemble_and_load("pop {r0, pc}", 0x0006);
        } catch (...) {
            memory->write16(0x0006, 0xBD01); // POP {R0, PC}
        }
        
        cpu->set_register(15, 0x0006);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(0), 0xABCDEF01); // R0 restored
        EXPECT_EQ(cpu->get_register(15), 0x00000100); // PC = original LR
        EXPECT_EQ(cpu->get_register(13), 0x1600); // SP restored
    }
}

TEST_F(ThumbCPUTest14, EDGE_CASES) {
    // Test case 1: Empty register list PUSH
    {
        cpu->set_register(13, 0x1000);
        cpu->set_cpsr(cpu->get_cpsr() | 0x20);
        
        // Manual encoding for empty list (may not be valid assembly)
        memory->write16(0x0000, 0xB400); // PUSH {} (empty list)
        
        cpu->set_register(15, 0x0000);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1000); // SP unchanged (no registers to push)
    }

    // Test case 2: Empty register list POP
    {
        cpu->set_register(13, 0x1000);
        
        memory->write16(0x0002, 0xBC00); // POP {} (empty list)
        
        cpu->set_register(15, 0x0002);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1000); // SP unchanged
    }

    // Test case 3: PUSH/POP at memory boundaries (respecting 0x1FFF limit)
    {
        // Test near upper memory boundary
        cpu->set_register(0, 0x12345678);
        cpu->set_register(13, 0x1FFC); // Near top of memory (0x1FFF)
        
        try {
            assemble_and_load("push {r0}", 0x0004);
        } catch (...) {
            memory->write16(0x0004, 0xB401); // PUSH {R0}
        }
        
        cpu->set_register(15, 0x0004);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(13), 0x1FFC - 4);
        EXPECT_EQ(memory->read32(0x1FFC - 4), 0x12345678);
        
        // POP it back
        cpu->set_register(0, 0);
        try {
            assemble_and_load("pop {r0}", 0x0006);
        } catch (...) {
            memory->write16(0x0006, 0xBC01); // POP {R0}
        }
        
        cpu->set_register(15, 0x0006);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(0), 0x12345678);
        EXPECT_EQ(cpu->get_register(13), 0x1FFC);
    }

    // Test case 4: Zero values
    {
        cpu->set_register(0, 0x00000000); // Zero value
        cpu->set_register(1, 0x00000001); // Non-zero for comparison
        cpu->set_register(13, 0x1000);
        
        try {
            assemble_and_load("push {r0, r1}", 0x0008);
        } catch (...) {
            memory->write16(0x0008, 0xB403); // PUSH {R0, R1}
        }
        
        cpu->set_register(15, 0x0008);
        thumb_cpu->execute();
        
        EXPECT_EQ(memory->read32(0x1000 - 8), 0x00000000); // Zero preserved
        EXPECT_EQ(memory->read32(0x1000 - 4), 0x00000001);
        
        // Clear and pop back
        cpu->set_register(0, 0xFF);
        cpu->set_register(1, 0xFF);
        try {
            assemble_and_load("pop {r0, r1}", 0x000A);
        } catch (...) {
            memory->write16(0x000A, 0xBC03); // POP {R0, R1}
        }
        
        cpu->set_register(15, 0x000A);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(0), 0x00000000); // Zero correctly popped
        EXPECT_EQ(cpu->get_register(1), 0x00000001);
    }

    // Test case 5: Maximum values
    {
        cpu->set_register(7, 0xFFFFFFFF); // Maximum 32-bit value
        cpu->set_register(13, 0x1000);
        
        try {
            assemble_and_load("push {r7}", 0x000C);
        } catch (...) {
            memory->write16(0x000C, 0xB480); // PUSH {R7}
        }
        
        cpu->set_register(15, 0x000C);
        thumb_cpu->execute();
        
        EXPECT_EQ(memory->read32(0x1000 - 4), 0xFFFFFFFF);
        
        cpu->set_register(7, 0);
        try {
            assemble_and_load("pop {r7}", 0x000E);
        } catch (...) {
            memory->write16(0x000E, 0xBC80); // POP {R7}
        }
        
        cpu->set_register(15, 0x000E);
        thumb_cpu->execute();
        
        EXPECT_EQ(cpu->get_register(7), 0xFFFFFFFF);
    }
}
