#include "test_cpu_common.h"

// ARM Thumb Format 6: PC-relative load
// Encoding: 01001[Rd][Word8]
// Instructions: LDR Rd, [PC, #imm]

TEST(CPU, LDR) {
    std::string beforeState;

    // Test case 1: Simple PC-relative load
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup test data in memory
        // LDR R0, [PC, #4] will access address ((0x0 + 4) & ~3) + 4 = 0x4 + 4 = 0x8
        gba.getCPU().getMemory().write32(0x00000008, 0x12345678);
        gba.getCPU().getMemory().write16(0x00000000, 0x4801); // LDR R0, [PC, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678)); // Should load the test data
        // LDR doesn't affect CPSR flags
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load zero value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup zero data in memory
        gba.getCPU().getMemory().write32(0x00000008, 0x00000000);
        gba.getCPU().getMemory().write16(0x00000000, 0x4902); // LDR R1, [PC, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x00000000)); // Should load zero
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 3: Load maximum 32-bit value
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup maximum value in memory
        gba.getCPU().getMemory().write32(0x00000010, 0xFFFFFFFF);  // PC+4+12 = 0x4+12 = 0x10
        gba.getCPU().getMemory().write16(0x00000000, 0x4A03); // LDR R2, [PC, #12]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xFFFFFFFF)); // Should load max value
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 4: Load with minimum offset (0)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test loading with offset 0: place instruction at 0x100, data at 0x104
        // Using correct ARM formula: ((0x100 + 4) & ~3) + 0 = 0x104 + 0 = 0x104
        gba.getCPU().getMemory().write32(0x00000104, 0xABCDEF01); // Data at 0x104
        gba.getCPU().getMemory().write16(0x00000100, 0x4B00); // LDR R3, [PC, #0]
        registers[15] = 0x00000100; // Set PC to 0x100
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // Using ARM formula: PC base = (0x100 + 4) & ~3 = 0x104, load from 0x104 + 0 = 0x104
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0xABCDEF01)); 
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 5: Load with medium offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data at offset 16 bytes from PC
        // LDR R4, [PC, #16] will access address ((0x0 + 4) & ~3) + 16 = 0x4 + 16 = 0x14
        gba.getCPU().getMemory().write32(0x00000014, 0x87654321);
        gba.getCPU().getMemory().write16(0x00000000, 0x4C04); // LDR R4, [PC, #16]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[4], static_cast<uint32_t>(0x87654321)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {4, 15});
    }

    // Test case 6: Load with large offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data at offset 32 bytes from PC
        // LDR R5, [PC, #32] will access address ((0x0 + 4) & ~3) + 32 = 0x4 + 32 = 0x24
        gba.getCPU().getMemory().write32(0x00000024, 0x13579BDF);
        gba.getCPU().getMemory().write16(0x00000000, 0x4D08); // LDR R5, [PC, #32]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0x13579BDF)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 7: Load with very large offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data at offset 64 bytes from PC
        // LDR R6, [PC, #64] will access address ((0x0 + 4) & ~3) + 64 = 0x4 + 64 = 0x44
        gba.getCPU().getMemory().write32(0x00000044, 0xFEDCBA98);
        gba.getCPU().getMemory().write16(0x00000000, 0x4E10); // LDR R6, [PC, #64]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0xFEDCBA98)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 8: Load to different registers with same offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup data and load into R7
        // LDR R7, [PC, #8] will access address ((0x0 + 4) & ~3) + 8 = 0x4 + 8 = 0xC
        gba.getCPU().getMemory().write32(0x0000000C, 0x24681357);
        gba.getCPU().getMemory().write16(0x00000000, 0x4F02); // LDR R7, [PC, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x24681357)); // Should load the data
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }

    // Test case 9: Load signed negative value (test sign extension not applied)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup negative value (should be loaded as-is, no sign extension for 32-bit loads)
        // LDR R0, [PC, #4] will access address ((0x0 + 4) & ~3) + 4 = 0x4 + 4 = 0x8
        gba.getCPU().getMemory().write32(0x00000008, 0x80000001);
        gba.getCPU().getMemory().write16(0x00000000, 0x4801); // LDR R0, [PC, #4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x80000001)); // Should load exactly as stored
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 10: Load with boundary pattern
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup alternating bit pattern
        // LDR R2, [PC, #12] will access address ((0x0 + 4) & ~3) + 12 = 0x4 + 12 = 0x10
        gba.getCPU().getMemory().write32(0x00000010, 0xAAAA5555);
        gba.getCPU().getMemory().write16(0x00000000, 0x4A03); // LDR R2, [PC, #12]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xAAAA5555)); // Should load the pattern
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 11: Load preserves existing flags
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
        
        // Setup test data
        // LDR R1, [PC, #8] will access address ((0x0 + 4) & ~3) + 8 = 0x4 + 8 = 0xC
        gba.getCPU().getMemory().write32(0x0000000C, 0x11223344);
        gba.getCPU().getMemory().write16(0x00000000, 0x4902); // LDR R1, [PC, #8]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x11223344)); // Should load the data
        // Flags should be preserved (LDR doesn't modify flags)
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_Z));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_N));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_C));
        ASSERT_TRUE(cpu.getFlag(CPU::FLAG_V));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }

    // Test case 12: Load with PC alignment (PC is word-aligned in calculation)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Place instruction at word boundary + 2 to test PC alignment
        // When instruction executes, PC=0x00000004, PC&~3 = 0x00000004
        gba.getCPU().getMemory().write32(0x00000008, 0x55667788);
        gba.getCPU().getMemory().write16(0x00000002, 0x4801); // LDR R0, [PC, #4] at address 0x02
        
        // Set PC to the instruction location
        registers[15] = 0x00000002;
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x55667788)); // Should load from aligned PC + offset
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 13: Maximum offset (1020 bytes = 255 words)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test maximum offset: 1020 bytes from PC
        // PC=0x002 when executing, aligned PC = (0x002 + 4) & ~3 = 0x004, so load from 0x004 + 1020 = 0x400
        gba.getCPU().getMemory().write32(0x00000400, 0xDEADBEEF);
        gba.getCPU().getMemory().write16(0x00000000, 0x4FFF); // LDR R7, [PC, #1020]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0xDEADBEEF));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }

    // Test case 14: All registers with same offset
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        // Test each register R0-R7 with offset 20 bytes
        uint32_t test_values[8] = {
            0x11111111, 0x22222222, 0x33333333, 0x44444444,
            0x55555555, 0x66666666, 0x77777777, 0x88888888
        };
        
        for (int rd = 0; rd < 8; rd++) {
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            // Setup unique test data for each register
            // LDR Rd, [PC, #20] will access address ((0x0 + 4) & ~3) + 20 = 0x4 + 20 = 0x18
            gba.getCPU().getMemory().write32(0x00000018, test_values[rd]); // 20 bytes offset
            
            // Encode: LDR Rd, [PC, #20] - offset 20 bytes = 5 words
            uint16_t opcode = 0x4800 | (rd << 8) | 0x05;
            gba.getCPU().getMemory().write16(0x00000000, opcode);
            
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            ASSERT_EQ(registers[rd], test_values[rd]) << "Failed for R" << rd;
            validateUnchangedRegisters(cpu, beforeState, {static_cast<int>(rd), 15});
        }
    }

    // Test case 15: Load from memory near upper boundary
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Place instruction near end of memory space and load from valid location
        // Memory ends at 0x1FFF, so place instruction at 0x1800 and load from earlier
        registers[15] = 0x00001800; // Set PC to near end of memory
        gba.getCPU().getMemory().write32(0x00001824, 0xCAFEBABE); // Load target
        gba.getCPU().getMemory().write16(0x00001800, 0x4808); // LDR R0, [PC, #32]
        
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // PC=0x1802 when executing, aligned to 0x1804, load from 0x1804+32=0x1824
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xCAFEBABE));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 16: Zero offset edge case
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test loading with zero offset from various PC positions
        // Using correct ARM formula: ((0x100 + 4) & ~3) + 0 = 0x104 + 0 = 0x104
        registers[15] = 0x00000100; // Set PC to 0x100
        gba.getCPU().getMemory().write32(0x00000104, 0x12344800); // Data at 0x104
        gba.getCPU().getMemory().write16(0x00000100, 0x4800); // LDR R0, [PC, #0]
        
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // Using ARM formula: PC base = (0x100 + 4) & ~3 = 0x104, load from 0x104 + 0 = 0x104
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12344800));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 17: PC alignment with odd addresses
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test PC alignment when instruction is at odd word boundary
        registers[15] = 0x00000202; // PC at address ending in 02
        gba.getCPU().getMemory().write32(0x00000208, 0xA5A5A5A5); // Target data
        gba.getCPU().getMemory().write16(0x00000202, 0x4801); // LDR R0, [PC, #4]
        
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // PC=0x204 when executing, aligned to 0x204, load from 0x204+4=0x208
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xA5A5A5A5));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 18: Boundary offsets pattern
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        // Test various boundary offsets: powers of 2 and other significant values
        struct {
            uint16_t opcode;
            uint32_t offset;
            uint32_t expected_data;
        } test_cases[] = {
            {0x4800, 0,   0x10101010}, // 0 bytes
            {0x4801, 4,   0x20202020}, // 4 bytes
            {0x4802, 8,   0x30303030}, // 8 bytes
            {0x4804, 16,  0x40404040}, // 16 bytes
            {0x4808, 32,  0x50505050}, // 32 bytes
            {0x4810, 64,  0x60606060}, // 64 bytes
            {0x4820, 128, 0x70707070}, // 128 bytes
            {0x4840, 256, 0x80808080}, // 256 bytes
            {0x4880, 512, 0x90909090}, // 512 bytes
        };
        
        for (auto& test : test_cases) {
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            // Setup test data at the calculated offset from PC=0x4
            uint32_t pc_start = 0x00000000;
            uint32_t aligned_pc = (pc_start + 4) & ~3; // PC after fetch = 0x4, aligned to 0x4
            uint32_t target_address = aligned_pc + test.offset;
            
            gba.getCPU().getMemory().write32(target_address, test.expected_data);
            gba.getCPU().getMemory().write16(pc_start, test.opcode);
            
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            ASSERT_EQ(registers[0], test.expected_data) << "Failed for offset " << test.offset;
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }

    // Test case 19: Multiple consecutive loads
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Setup multiple instructions and data
        // First instruction at 0x0: ((0x0 + 4) & ~3) + 16 = 0x4 + 16 = 0x14
        // Second instruction at 0x2: ((0x2 + 4) & ~3) + 16 = (0x6 & ~3) + 16 = 0x4 + 16 = 0x14
        // Third instruction at 0x4: ((0x4 + 4) & ~3) + 16 = (0x8 & ~3) + 16 = 0x8 + 16 = 0x18
        gba.getCPU().getMemory().write32(0x00000014, 0xAAAABBBB); // Data 1 and 2
        gba.getCPU().getMemory().write32(0x00000018, 0xEEEEFFFF); // Data 3
        
        gba.getCPU().getMemory().write16(0x00000000, 0x4804); // LDR R0, [PC, #16] at PC=0x000
        gba.getCPU().getMemory().write16(0x00000002, 0x4904); // LDR R1, [PC, #16] at PC=0x002
        gba.getCPU().getMemory().write16(0x00000004, 0x4A04); // LDR R2, [PC, #16] at PC=0x004
        
        // Execute first instruction: PC=0x002, load from 0x14
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xAAAABBBB));
        
        // Execute second instruction: PC=0x004, load from 0x14
        cpu.execute(1);
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0xAAAABBBB));
        
        // Execute third instruction: PC=0x006, load from 0x18
        cpu.execute(1);
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0xEEEEFFFF));
    }

    // Test case 20: Load with alternating bit patterns
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        // Test various bit patterns to verify no data corruption
        uint32_t patterns[] = {
            0x00000000, 0xFFFFFFFF, 0xAAAAAAAA, 0x55555555,
            0xF0F0F0F0, 0x0F0F0F0F, 0xFF00FF00, 0x00FF00FF
        };
        
        for (size_t i = 0; i < sizeof(patterns)/sizeof(patterns[0]); i++) {
            registers.fill(0);
            cpu.CPSR() = CPU::FLAG_T;
            
            // LDR R0, [PC, #8] will access address ((0x0 + 4) & ~3) + 8 = 0x4 + 8 = 0xC
            gba.getCPU().getMemory().write32(0x0000000C, patterns[i]);
            gba.getCPU().getMemory().write16(0x00000000, 0x4802); // LDR R0, [PC, #8]
            
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            ASSERT_EQ(registers[0], patterns[i]) << "Failed for pattern 0x" << std::hex << patterns[i];
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }

    // Test case 21: Edge case - load from instruction location
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Place instruction and data to test PC+4 calculation
        // Using correct ARM formula: ((0x200 + 4) & ~3) + 0 = 0x204 + 0 = 0x204
        registers[15] = 0x00000200; // Set PC to 0x200
        gba.getCPU().getMemory().write32(0x00000204, 0xABCD4800); // Data at 0x204
        gba.getCPU().getMemory().write16(0x00000200, 0x4800); // LDR R0, [PC, #0]
        
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        // Using ARM formula: PC base = (0x200 + 4) & ~3 = 0x204, load from 0x204 + 0 = 0x204
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0xABCD4800));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 22: Verify all flag preservation with different initial flag states
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        
        // Test different combinations of flags
        uint32_t flag_combinations[] = {
            CPU::FLAG_T,                                           // Only Thumb
            CPU::FLAG_T | CPU::FLAG_Z,                            // Thumb + Zero
            CPU::FLAG_T | CPU::FLAG_N,                            // Thumb + Negative
            CPU::FLAG_T | CPU::FLAG_C,                            // Thumb + Carry
            CPU::FLAG_T | CPU::FLAG_V,                            // Thumb + Overflow
            CPU::FLAG_T | CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V // All flags
        };
        
        for (auto flags : flag_combinations) {
            registers.fill(0);
            cpu.CPSR() = flags;
            
            // LDR R0, [PC, #12] will access address ((0x0 + 4) & ~3) + 12 = 0x4 + 12 = 0x10
            gba.getCPU().getMemory().write32(0x00000010, 0x12345678);
            gba.getCPU().getMemory().write16(0x00000000, 0x4803); // LDR R0, [PC, #12]
            
            uint32_t initial_cpsr = cpu.CPSR();
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify data loaded correctly
            ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
            
            // Verify all flags preserved exactly
            ASSERT_EQ(cpu.CPSR(), initial_cpsr) << "Flags changed for initial CPSR 0x" << std::hex << initial_cpsr;
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }
}
