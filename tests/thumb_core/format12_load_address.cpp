#include "test_cpu_common.h"

// ARM Thumb Format 12: Load address
// Encoding: 1010[SP][Rd][Word8]
// Instructions: ADD Rd, PC, #imm ; ADD Rd, SP, #imm

TEST(CPU, ADD_PC_LOAD_ADDRESS) {
    std::string beforeState;

    // Test case 1: ADD R0, PC, #0 (minimal offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // When instruction executes, PC will be 0x00000002 (word-aligned to 0x00000000)
        gba.getCPU().getMemory().write16(0x00000000, 0xA000); // ADD R0, PC, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00000000)); // PC (0x02) aligned to 0x00 + 0 = 0x00
        // ADD PC doesn't affect flags
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: ADD R1, PC, #4 (small offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xA101); // ADD R1, PC, #4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x00000004)); // PC aligned (0x00) + 4 = 0x04
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: ADD R2, PC, #255 (maximum offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xA2FF); // ADD R2, PC, #1020 (255*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x000003FC)); // PC aligned (0x00) + 1020 = 0x3FC
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: ADD R3, PC, #128 (medium offset)  
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xA380); // ADD R3, PC, #512 (128*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x00000200)); // PC aligned (0x00) + 512 = 0x200
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 5: ADD R4, PC, #64 with unaligned PC
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Place instruction at unaligned address
        gba.getCPU().getMemory().write16(0x00000006, 0xA440); // ADD R4, PC, #256 (64*4)
        registers[15] = 0x00000006; // Set PC to unaligned address
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // PC=0x08 after fetch, aligned to 0x08, + 256 = 0x108
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x00000108));
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 6: Test all destination registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test each register R0-R7
        for (int rd = 0; rd < 8; rd++) {
            uint16_t instruction = 0xA000 | (rd << 8) | 0x01; // ADD Rd, PC, #4
            gba.getCPU().getMemory().write16(rd * 2, instruction);
            registers[15] = rd * 2; // Set PC to instruction location
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            uint32_t expected_pc = (rd * 2 + 2) & ~3; // PC after fetch, word-aligned
            ASSERT_EQ(registers[rd], expected_pc + 4); // Should be aligned PC + 4
            validateUnchangedRegisters(cpu, beforeState, {rd, 15});
        }
    }

    // Test case 7: ADD with address space boundary (ensure result stays within GBA limits)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Start near boundary to test address calculation
        registers[15] = 0x00001FF0; // Near the 0x1FFF limit
        gba.getCPU().getMemory().write16(0x00001FF0, 0xA507); // ADD R5, PC, #28 (7*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // PC=0x1FF2 after fetch, aligned to 0x1FF0, + 28 = 0x200C (should wrap or stay in bounds)
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x0000200C));
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 8: Verify flags are preserved
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
        
        gba.getCPU().getMemory().write16(0x00000000, 0xA610); // ADD R6, PC, #64 (16*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x00000040)); // PC aligned (0x00) + 64 = 0x40
        // All flags should be preserved (ADD PC doesn't modify flags)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }
}

TEST(CPU, ADD_SP_LOAD_ADDRESS) {
    std::string beforeState;

    // Test case 1: ADD R0, SP, #0 (minimal offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00002000; // Set SP to some value
        gba.getCPU().getMemory().write16(0x00000000, 0xA800); // ADD R0, SP, #0
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00002000)); // SP + 0 = SP
        // ADD SP doesn't affect flags  
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_FALSE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: ADD R1, SP, #4 (small offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Set SP
        gba.getCPU().getMemory().write16(0x00000000, 0xA901); // ADD R1, SP, #4
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x00001004)); // SP + 4
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: ADD R2, SP, #255 (maximum offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Set SP
        gba.getCPU().getMemory().write16(0x00000000, 0xAAFF); // ADD R2, SP, #1020 (255*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x000013FC)); // SP + 1020
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: ADD R3, SP, #128 (medium offset)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00000800; // Set SP
        gba.getCPU().getMemory().write16(0x00000000, 0xAB80); // ADD R3, SP, #512 (128*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x00000A00)); // SP + 512
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 5: Test all destination registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001000; // Set SP
        
        // Test each register R0-R7
        for (int rd = 0; rd < 8; rd++) {
            uint16_t instruction = 0xA800 | (rd << 8) | 0x01; // ADD Rd, SP, #4
            gba.getCPU().getMemory().write16(rd * 2, instruction);
            registers[15] = rd * 2; // Set PC to instruction location
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            ASSERT_EQ(registers[rd], static_cast<uint32_t>(0x00001004)); // Should be SP + 4
            validateUnchangedRegisters(cpu, beforeState, {rd, 15});
        }
    }

    // Test case 6: ADD with SP at zero
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00000000; // Set SP to zero
        gba.getCPU().getMemory().write16(0x00000000, 0xAC08); // ADD R4, SP, #32 (8*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x00000020)); // 0 + 32 = 32
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 7: ADD with large SP value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001800; // Set SP to large value within bounds
        gba.getCPU().getMemory().write16(0x00000000, 0xAD20); // ADD R5, SP, #128 (32*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x00001880)); // 0x1800 + 128 = 0x1880
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 8: Verify flags are preserved 
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
        
        registers[13] = 0x00001000; // Set SP
        gba.getCPU().getMemory().write16(0x00000000, 0xAE10); // ADD R6, SP, #64 (16*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x00001040)); // SP + 64
        // All flags should be preserved (ADD SP doesn't modify flags)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 9: ADD with unaligned SP (should work as SP is treated as word)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[13] = 0x00001002; // Set SP to unaligned value
        gba.getCPU().getMemory().write16(0x00000000, 0xAF04); // ADD R7, SP, #16 (4*4)
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x00001012)); // 0x1002 + 16 = 0x1012
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }
}
