#include "test_cpu_common.h"

// ARM Thumb Format 14: Push/Pop registers
// Encoding: 1011 [L]1[R]0 [register_list]
// L=0: PUSH (store), L=1: POP (load)
// R=0: No LR/PC, R=1: Include LR (PUSH) or PC (POP)
// Instructions: PUSH, POP

TEST(Format14, PUSH_SINGLE_REGISTER) {
    std::string beforeState;

    // Test case 1: PUSH {R0}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Initialize registers and stack pointer
        registers[0] = 0x12345678;
        registers[13] = 0x1000; // SP at 0x1000
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB401); // PUSH {R0}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that R0 was pushed to stack
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1000 - 4)); // SP decremented by 4
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1000 - 4), static_cast<uint32_t>(0x12345678)); // R0 value on stack
        validateUnchangedRegisters(cpu, beforeState, {13, 15}); // Only SP and PC should change
    }

    // Test case 2: PUSH {R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0xDEADBEEF;
        registers[13] = 0x1800; // SP at 0x1800
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB480); // PUSH {R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1800 - 4));
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1800 - 4), static_cast<uint32_t>(0xDEADBEEF));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 3: PUSH {R4}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0xCAFEBABE;
        registers[13] = 0x1C00;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB410); // PUSH {R4}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1C00 - 4));
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1C00 - 4), static_cast<uint32_t>(0xCAFEBABE));
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }
}

TEST(Format14, PUSH_MULTIPLE_REGISTERS) {
    std::string beforeState;

    // Test case 1: PUSH {R0, R1}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x11111111;
        registers[1] = 0x22222222;
        registers[13] = 0x1000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB403); // PUSH {R0, R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Registers are pushed in order: R0 first, then R1
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1000 - 8)); // SP decremented by 8
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1000 - 8), static_cast<uint32_t>(0x11111111)); // R0 pushed first
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1000 - 4), static_cast<uint32_t>(0x22222222)); // R1 pushed second
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 2: PUSH {R4, R5, R6, R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x44444444;
        registers[5] = 0x55555555;
        registers[6] = 0x66666666;
        registers[7] = 0x77777777;
        registers[13] = 0x1800;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB4F0); // PUSH {R4, R5, R6, R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1800 - 16)); // SP decremented by 16
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1800 - 16), static_cast<uint32_t>(0x44444444)); // R4
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1800 - 12), static_cast<uint32_t>(0x55555555)); // R5
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1800 - 8), static_cast<uint32_t>(0x66666666)); // R6
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1800 - 4), static_cast<uint32_t>(0x77777777)); // R7
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 3: PUSH all registers {R0-R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        for (int i = 0; i < 8; i++) {
            registers[i] = 0x10000000 + i; // Unique values
        }
        registers[13] = 0x1C00;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB4FF); // PUSH {R0-R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1C00 - 32)); // SP decremented by 32
        for (int i = 0; i < 8; i++) {
            ASSERT_EQ(gba.getCPU().getMemory().read32(0x1C00 - 32 + (i * 4)), static_cast<uint32_t>(0x10000000 + i));
        }
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }
}

TEST(Format14, PUSH_WITH_LR) {
    std::string beforeState;

    // Test case 1: PUSH {R0, LR}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0xAAAAAAAA;
        registers[14] = 0xBBBBBBBB; // LR
        registers[13] = 0x1400;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB501); // PUSH {R0, LR}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1400 - 8)); // SP decremented by 8
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1400 - 8), static_cast<uint32_t>(0xAAAAAAAA)); // R0 pushed first
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1400 - 4), static_cast<uint32_t>(0xBBBBBBBB)); // LR pushed second
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 2: PUSH {LR} only
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[14] = 0x12345678; // LR
        registers[13] = 0x1600;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB500); // PUSH {LR}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1600 - 4)); // SP decremented by 4
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1600 - 4), static_cast<uint32_t>(0x12345678)); // LR pushed
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 3: PUSH {R0-R7, LR}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        for (int i = 0; i < 8; i++) {
            registers[i] = 0x20000000 + i;
        }
        registers[14] = 0xFEDCBA98; // LR
        registers[13] = 0x1F00;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB5FF); // PUSH {R0-R7, LR}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1F00 - 36)); // SP decremented by 36 (8 regs + LR)
        for (int i = 0; i < 8; i++) {
            ASSERT_EQ(gba.getCPU().getMemory().read32(0x1F00 - 36 + (i * 4)), static_cast<uint32_t>(0x20000000 + i));
        }
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1F00 - 4), static_cast<uint32_t>(0xFEDCBA98)); // LR at top
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }
}

TEST(Format14, POP_SINGLE_REGISTER) {
    std::string beforeState;

    // Test case 1: POP {R0}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1000 - 4; // SP pointing to data
        gba.getCPU().getMemory().write32(0x1000 - 4, 0x87654321); // Data on stack
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBC01); // POP {R0}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x87654321)); // R0 loaded from stack
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1000)); // SP incremented by 4
        validateUnchangedRegisters(cpu, beforeState, {0, 13, 15});
    }

    // Test case 2: POP {R3}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1400 - 4;
        gba.getCPU().getMemory().write32(0x1400 - 4, 0xDEADBEEF);
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBC08); // POP {R3}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0xDEADBEEF));
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1400));
        validateUnchangedRegisters(cpu, beforeState, {3, 13, 15});
    }

    // Test case 3: POP {R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1800 - 4;
        gba.getCPU().getMemory().write32(0x1800 - 4, 0xCAFEBABE);
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBC80); // POP {R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0xCAFEBABE));
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1800));
        validateUnchangedRegisters(cpu, beforeState, {7, 13, 15});
    }
}

TEST(Format14, POP_MULTIPLE_REGISTERS) {
    std::string beforeState;

    // Test case 1: POP {R0, R1}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1000 - 8; // SP pointing to data for 2 registers
        gba.getCPU().getMemory().write32(0x1000 - 8, 0x11111111); // R0 data
        gba.getCPU().getMemory().write32(0x1000 - 4, 0x22222222); // R1 data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBC03); // POP {R0, R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x11111111)); // R0 popped first
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x22222222)); // R1 popped second
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1000)); // SP incremented by 8
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 13, 15});
    }

    // Test case 2: POP {R4, R5, R6, R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1400 - 16;
        gba.getCPU().getMemory().write32(0x1400 - 16, 0x44444444); // R4
        gba.getCPU().getMemory().write32(0x1400 - 12, 0x55555555); // R5
        gba.getCPU().getMemory().write32(0x1400 - 8, 0x66666666); // R6
        gba.getCPU().getMemory().write32(0x1400 - 4, 0x77777777); // R7
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBCF0); // POP {R4, R5, R6, R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x44444444));
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x55555555));
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x66666666));
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x77777777));
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1400));
        validateUnchangedRegisters(cpu, beforeState, {4, 5, 6, 7, 13, 15});
    }

    // Test case 3: POP all registers {R0-R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1800 - 32;
        for (int i = 0; i < 8; i++) {
            gba.getCPU().getMemory().write32(0x1800 - 32 + (i * 4), 0x30000000 + i);
        }
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBCFF); // POP {R0-R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        for (int i = 0; i < 8; i++) {
            ASSERT_EQ(registers[i], static_cast<uint32_t>(0x30000000 + i));
        }
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1800));
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 2, 3, 4, 5, 6, 7, 13, 15});
    }
}

TEST(Format14, POP_WITH_PC) {
    std::string beforeState;

    // Test case 1: POP {R0, PC}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1000 - 8;
        gba.getCPU().getMemory().write32(0x1000 - 8, 0xAAAAAAAA); // R0 data
        gba.getCPU().getMemory().write32(0x1000 - 4, 0x00000100); // PC data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBD01); // POP {R0, PC}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xAAAAAAAA)); // R0 loaded
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000100)); // PC loaded from stack
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1000)); // SP incremented by 8
        validateUnchangedRegisters(cpu, beforeState, {0, 13, 15});
    }

    // Test case 2: POP {PC} only
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1400 - 4;
        gba.getCPU().getMemory().write32(0x1400 - 4, 0x00000200); // PC data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBD00); // POP {PC}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000200)); // PC loaded from stack
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1400)); // SP incremented by 4
        validateUnchangedRegisters(cpu, beforeState, {13, 15});
    }

    // Test case 3: POP {R0-R7, PC}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1800 - 36; // 8 registers + PC
        for (int i = 0; i < 8; i++) {
            gba.getCPU().getMemory().write32(0x1800 - 36 + (i * 4), 0x40000000 + i);
        }
        gba.getCPU().getMemory().write32(0x1800 - 4, 0x00000300); // PC data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBDFF); // POP {R0-R7, PC}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        for (int i = 0; i < 8; i++) {
            ASSERT_EQ(registers[i], static_cast<uint32_t>(0x40000000 + i));
        }
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000300)); // PC loaded
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1800)); // SP incremented by 36
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 2, 3, 4, 5, 6, 7, 13, 15});
    }
}

TEST(Format14, PUSH_POP_ROUNDTRIP) {
    std::string beforeState;

    // Test case 1: PUSH then POP same registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Set up initial values
        registers[0] = 0x11111111;
        registers[1] = 0x22222222;
        registers[2] = 0x33333333;
        registers[13] = 0x1500; // SP
        
        // PUSH {R0, R1, R2}
        gba.getCPU().getMemory().write16(0x00000000, 0xB407); // PUSH {R0, R1, R2}
        cpu.execute(1);
        
        // Verify stack state
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1500 - 12)); // SP decremented
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1500 - 12), static_cast<uint32_t>(0x11111111)); // R0
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1500 - 8), static_cast<uint32_t>(0x22222222)); // R1
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1500 - 4), static_cast<uint32_t>(0x33333333)); // R2
        
        // Clear registers
        registers[0] = 0;
        registers[1] = 0;
        registers[2] = 0;
        
        // POP {R0, R1, R2}
        gba.getCPU().getMemory().write16(0x00000002, 0xBC07); // POP {R0, R1, R2}
        registers[15] = 0x00000002; // Set PC to next instruction
        cpu.execute(1);
        
        // Verify restoration
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x11111111));
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x22222222));
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x33333333));
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1500)); // SP restored
    }

    // Test case 2: PUSH with LR, POP with PC
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0xABCDEF01;
        registers[14] = 0x00000100; // LR (return address)
        registers[13] = 0x1600;
        
        // PUSH {R0, LR}
        gba.getCPU().getMemory().write16(0x00000000, 0xB501); // PUSH {R0, LR}
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1600 - 8));
        
        // Clear registers
        registers[0] = 0;
        
        // POP {R0, PC} - this should restore R0 and jump to LR value
        gba.getCPU().getMemory().write16(0x00000002, 0xBD01); // POP {R0, PC}
        registers[15] = 0x00000002;
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xABCDEF01)); // R0 restored
        ASSERT_EQ(registers[15], static_cast<uint32_t>(0x00000100)); // PC = original LR
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1600)); // SP restored
    }
}

TEST(Format14, EDGE_CASES) {
    std::string beforeState;

    // Test case 1: Empty register list PUSH (should only affect SP if no registers)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB400); // PUSH {} (empty list)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1000)); // SP unchanged (no registers to push)
        validateUnchangedRegisters(cpu, beforeState, {15}); // Only PC should change
    }

    // Test case 2: Empty register list POP
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x1000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xBC00); // POP {} (empty list)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1000)); // SP unchanged
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: PUSH/POP at memory boundaries
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test near upper memory boundary
        registers[0] = 0x12345678;
        registers[13] = 0x1FFC; // Near top of memory (0x1FFF)
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB401); // PUSH {R0}
        cpu.execute(1);
        
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1FFC - 4));
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1FFC - 4), static_cast<uint32_t>(0x12345678));
        
        // POP it back
        registers[0] = 0;
        gba.getCPU().getMemory().write16(0x00000002, 0xBC01); // POP {R0}
        registers[15] = 0x00000002;
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        ASSERT_EQ(registers[13], static_cast<uint32_t>(0x1FFC));
    }

    // Test case 4: Zero values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[0] = 0x00000000; // Zero value
        registers[1] = 0x00000001; // Non-zero for comparison
        registers[13] = 0x1000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB403); // PUSH {R0, R1}
        cpu.execute(1);
        
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1000 - 8), static_cast<uint32_t>(0x00000000)); // Zero preserved
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1000 - 4), static_cast<uint32_t>(0x00000001));
        
        // Clear and pop back
        registers[0] = 0xFF;
        registers[1] = 0xFF;
        gba.getCPU().getMemory().write16(0x00000002, 0xBC03); // POP {R0, R1}
        registers[15] = 0x00000002;
        cpu.execute(1);
        
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00000000)); // Zero correctly popped
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x00000001));
    }

    // Test case 5: Maximum values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0xFFFFFFFF; // Maximum 32-bit value
        registers[13] = 0x1000;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xB480); // PUSH {R7}
        cpu.execute(1);
        
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x1000 - 4), static_cast<uint32_t>(0xFFFFFFFF));
        
        registers[7] = 0;
        gba.getCPU().getMemory().write16(0x00000002, 0xBC80); // POP {R7}
        registers[15] = 0x00000002;
        cpu.execute(1);
        
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0xFFFFFFFF));
    }
}
