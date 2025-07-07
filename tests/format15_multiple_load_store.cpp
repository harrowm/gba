#include "test_cpu_common.h"

// ARM Thumb Format 15: Multiple load/store
// Encoding: 1100 L Rn[2:0] RegisterList[7:0] 
// Instructions: STMIA (L=0), LDMIA (L=1)

TEST(Format15, STMIA_SINGLE_REGISTER) {
    std::string beforeState;

    // Test case 1: STMIA R0!, {R1}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00001000; // Base address
        registers[1] = 0xDEADBEEF; // Data to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC002); // STMIA R0!, {R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was stored at the correct address
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001000), static_cast<uint32_t>(0xDEADBEEF));
        // Check that R0 was incremented
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00001004));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: STMIA R2!, {R0}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x00001100; // Base address
        registers[0] = 0x12345678; // Data to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC201); // STMIA R2!, {R0}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was stored at the correct address
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001100), static_cast<uint32_t>(0x12345678));
        // Check that R2 was incremented
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x00001104));
        validateUnchangedRegisters(cpu, beforeState, {2, 15});
    }

    // Test case 3: STMIA R7!, {R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[7] = 0x00001200; // Base address and data
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC780); // STMIA R7!, {R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that original R7 value was stored
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001200), static_cast<uint32_t>(0x00001200));
        // Check that R7 was incremented
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x00001204));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }
}

TEST(Format15, STMIA_MULTIPLE_REGISTERS) {
    std::string beforeState;

    // Test case 1: STMIA R0!, {R0, R1}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00001000; // Base address
        registers[1] = 0x11111111; // Data to store
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC003); // STMIA R0!, {R0, R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was stored in order
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001000), static_cast<uint32_t>(0x00001000)); // R0 stored first
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001004), static_cast<uint32_t>(0x11111111)); // R1 stored second
        // Check that R0 was incremented by 8 (2 registers * 4 bytes)
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00001008));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: STMIA R3!, {R0, R2, R4, R6}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[3] = 0x00001300; // Base address
        registers[0] = 0xAAAAAAAA;
        registers[2] = 0xCCCCCCCC;
        registers[4] = 0xEEEEEEEE;
        registers[6] = 0x66666666;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC355); // STMIA R3!, {R0, R2, R4, R6}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was stored in register order (R0, R2, R4, R6)
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001300), static_cast<uint32_t>(0xAAAAAAAA)); // R0
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001304), static_cast<uint32_t>(0xCCCCCCCC)); // R2
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001308), static_cast<uint32_t>(0xEEEEEEEE)); // R4
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x0000130C), static_cast<uint32_t>(0x66666666)); // R6
        // Check that R3 was incremented by 16 (4 registers * 4 bytes)
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x00001310));
        validateUnchangedRegisters(cpu, beforeState, {3, 15});
    }

    // Test case 3: STMIA R1!, {R0-R7} (all registers)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        // Set all register values
        for (int i = 0; i < 8; i++) {
            registers[i] = 0x10101010 + (i * 0x11111111);
        }
        // Override R1 to be the base address
        registers[1] = 0x00001400; // Base address
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC1FF); // STMIA R1!, {R0-R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that all registers were stored
        for (int i = 0; i < 8; i++) {
            uint32_t expected_value;
            if (i == 1) {
                // R1 is the base register, so it stores the base address
                expected_value = 0x00001400;
            } else {
                expected_value = 0x10101010 + (i * 0x11111111);
            }
            ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001400 + (i * 4)), expected_value);
        }
        // Check that R1 was incremented by 32 (8 registers * 4 bytes)
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x00001420));
        validateUnchangedRegisters(cpu, beforeState, {1, 15});
    }
}

TEST(Format15, LDMIA_SINGLE_REGISTER) {
    std::string beforeState;

    // Test case 1: LDMIA R0!, {R1}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00001000; // Base address
        gba.getCPU().getMemory().write32(0x00001000, 0xDEADBEEF); // Data to load
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC802); // LDMIA R0!, {R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was loaded correctly
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0xDEADBEEF));
        // Check that R0 was incremented
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00001004));
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 15});
    }

    // Test case 2: LDMIA R2!, {R0}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x00001100; // Base address
        gba.getCPU().getMemory().write32(0x00001100, 0x12345678); // Data to load
        
        gba.getCPU().getMemory().write16(0x00000000, 0xCA01); // LDMIA R2!, {R0}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was loaded correctly
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        // Check that R2 was incremented
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x00001104));
        validateUnchangedRegisters(cpu, beforeState, {0, 2, 15});
    }

    // Test case 3: LDMIA R7!, {R7} (self-load)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[7] = 0x00001200; // Base address
        gba.getCPU().getMemory().write32(0x00001200, 0x87654321); // Data to load
        
        gba.getCPU().getMemory().write16(0x00000000, 0xCF80); // LDMIA R7!, {R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that R7 was loaded with the memory value (not the incremented base)
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x87654321));
        validateUnchangedRegisters(cpu, beforeState, {7, 15});
    }
}

TEST(Format15, LDMIA_MULTIPLE_REGISTERS) {
    std::string beforeState;

    // Test case 1: LDMIA R0!, {R0, R1}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00001000; // Base address
        gba.getCPU().getMemory().write32(0x00001000, 0x11111111); // Data for R0
        gba.getCPU().getMemory().write32(0x00001004, 0x22222222); // Data for R1
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC803); // LDMIA R0!, {R0, R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was loaded in order
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x11111111)); // R0 gets loaded value, not incremented base
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x22222222)); // R1 gets second value
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 15});
    }

    // Test case 2: LDMIA R3!, {R1, R3, R5, R7}
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[3] = 0x00001300; // Base address
        gba.getCPU().getMemory().write32(0x00001300, 0xAAAAAAAA); // R1
        gba.getCPU().getMemory().write32(0x00001304, 0xCCCCCCCC); // R3
        gba.getCPU().getMemory().write32(0x00001308, 0xEEEEEEEE); // R5
        gba.getCPU().getMemory().write32(0x0000130C, 0x77777777); // R7
        
        gba.getCPU().getMemory().write16(0x00000000, 0xCBAA); // LDMIA R3!, {R1, R3, R5, R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was loaded in register order
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0xAAAAAAAA));
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0xCCCCCCCC)); // R3 gets loaded value
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0xEEEEEEEE));
        ASSERT_EQ(registers[7], static_cast<uint32_t>(0x77777777));
        validateUnchangedRegisters(cpu, beforeState, {1, 3, 5, 7, 15});
    }

    // Test case 3: LDMIA R1!, {R0-R7} (all registers)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[1] = 0x00001400; // Base address
        
        // Set up memory with test patterns
        for (int i = 0; i < 8; i++) {
            uint32_t test_value = 0x80808080 + (i * 0x01010101);
            gba.getCPU().getMemory().write32(0x00001400 + (i * 4), test_value);
        }
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC9FF); // LDMIA R1!, {R0-R7}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that all registers were loaded
        for (int i = 0; i < 8; i++) {
            uint32_t expected_value = 0x80808080 + (i * 0x01010101);
            ASSERT_EQ(registers[i], expected_value);
        }
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 2, 3, 4, 5, 6, 7, 15});
    }
}

TEST(Format15, EMPTY_REGISTER_LIST) {
    std::string beforeState;

    // Test case 1: STMIA R0!, {} (empty register list)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[0] = 0x00001000; // Base address
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC000); // STMIA R0!, {}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Base register should not be modified for empty list
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00001000));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: LDMIA R2!, {} (empty register list)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[2] = 0x00001200; // Base address
        
        gba.getCPU().getMemory().write16(0x00000000, 0xCA00); // LDMIA R2!, {}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Base register should not be modified for empty list
        ASSERT_EQ(registers[2], static_cast<uint32_t>(0x00001200));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }
}

TEST(Format15, MEMORY_ALIGNMENT_AND_BOUNDS) {
    std::string beforeState;

    // Test case 1: Store at memory boundaries
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        // Test near end of test memory (0x1FFC is the last 4-byte aligned address)
        registers[0] = 0x00001FFC; // Near end of test memory
        registers[1] = 0xFEEDFACE;
        
        gba.getCPU().getMemory().write16(0x00000000, 0xC002); // STMIA R0!, {R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was stored correctly
        ASSERT_EQ(gba.getCPU().getMemory().read32(0x00001FFC), static_cast<uint32_t>(0xFEEDFACE));
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00002000)); // Incremented beyond memory
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: Load from memory boundaries  
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        registers[3] = 0x00001FF8; // Near end, room for 2 words
        gba.getCPU().getMemory().write32(0x00001FF8, 0x12345678);
        gba.getCPU().getMemory().write32(0x00001FFC, 0x9ABCDEF0);
        
        gba.getCPU().getMemory().write16(0x00000000, 0xCB03); // LDMIA R3!, {R0, R1}
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Check that data was loaded correctly
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x12345678));
        ASSERT_EQ(registers[1], static_cast<uint32_t>(0x9ABCDEF0));
        ASSERT_EQ(registers[3], static_cast<uint32_t>(0x00002000)); // Incremented beyond memory
        validateUnchangedRegisters(cpu, beforeState, {0, 1, 3, 15});
    }
}

TEST(Format15, INSTRUCTION_ENCODING_VALIDATION) {
    std::string beforeState;

    // Test case 1: Verify specific instruction encodings
    struct TestCase {
        uint16_t opcode;
        std::string description;
        uint8_t expected_rn;
        uint8_t expected_reglist;
        bool is_load;
    };

    std::vector<TestCase> test_cases = {
        {0xC000, "STMIA R0!, {}", 0, 0x00, false},
        {0xC001, "STMIA R0!, {R0}", 0, 0x01, false},
        {0xC780, "STMIA R7!, {R7}", 7, 0x80, false},
        {0xC32A, "STMIA R3!, {R1,R3,R5}", 3, 0x2A, false},
        {0xC801, "LDMIA R0!, {R0}", 0, 0x01, true},
        {0xCFFF, "LDMIA R7!, {R0-R7}", 7, 0xFF, true},
        {0xCAAA, "LDMIA R2!, {R1,R3,R5,R7}", 2, 0xAA, true},
        {0xC955, "LDMIA R1!, {R0,R2,R4,R6}", 1, 0x55, true},
    };

    for (const auto& test : test_cases) {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        cpu.CPSR() = CPU::FLAG_T;
        registers.fill(0);
        
        // Set up base register
        registers[test.expected_rn] = 0x00001000;
        
        // Set up register values for store operations
        if (!test.is_load) {
            for (int i = 0; i < 8; i++) {
                if (test.expected_reglist & (1 << i)) {
                    // Don't overwrite the base register if it's in the register list
                    if (i != test.expected_rn) {
                        registers[i] = 0x10000000 + (i * 0x1000000);
                    }
                }
            }
        } else {
            // Set up memory for load operations
            uint32_t addr = 0x00001000;
            for (int i = 0; i < 8; i++) {
                if (test.expected_reglist & (1 << i)) {
                    gba.getCPU().getMemory().write32(addr, 0x20000000 + (i * 0x1000000));
                    addr += 4;
                }
            }
        }
        
        gba.getCPU().getMemory().write16(0x00000000, test.opcode);
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the instruction was decoded correctly by checking effects
        uint8_t reg_count = 0;
        for (int i = 0; i < 8; i++) {
            if (test.expected_reglist & (1 << i)) {
                reg_count++;
            }
        }
        
        // Base register should be incremented by 4 * register count (if not empty)
        uint32_t expected_base = 0x00001000 + (reg_count * 4);
        if (test.is_load && (test.expected_reglist & (1 << test.expected_rn))) {
            // For loads, if base register is in the list, it gets the loaded value
            uint32_t expected_loaded_value = 0x20000000 + (test.expected_rn * 0x1000000);
            ASSERT_EQ(registers[test.expected_rn], expected_loaded_value);
        } else {
            ASSERT_EQ(registers[test.expected_rn], expected_base);
        }
    }
}
