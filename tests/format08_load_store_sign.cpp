#include "test_cpu_common.h"

// ARM Thumb Format 8: Load/store sign-extended byte/halfword
// Encoding: 0101[H][S][1][Ro][Rb][Rd]  
// Instructions: STRH, LDSB, LDRH, LDSH
// H=0,S=0: STRH (Store Halfword) - maps to 0x52
// H=0,S=1: LDSB (Load Sign-extended Byte) - maps to 0x56
// H=1,S=0: LDRH (Load Halfword) - maps to 0x5A
// H=1,S=1: LDSH (Load Sign-extended Halfword) - maps to 0x5E

TEST(Format8, STRH_HALFWORD_REGISTER_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: STRH R0, [R1, R2] - basic halfword store
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000006; // Offset
        registers[0] = 0x12345678; // Value to store (only lower 16 bits should be stored)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5288); // STRH R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify only the halfword was stored
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001006);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0x5678));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: STRH with different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001100; // Base address
        registers[4] = 0x0000000A; // Offset
        registers[5] = 0xFFFFABCD; // Value to store (lower 16 bits)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x531D); // STRH R5, [R3, R4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify only the halfword was stored
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x0000110A);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0xABCD));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: STRH with different halfword values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000010; // Base offset
        
        uint16_t test_halfwords[] = {0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF};
        
        for (size_t i = 0; i < sizeof(test_halfwords)/sizeof(test_halfwords[0]); i++) {
            registers[0] = 0x12340000 | test_halfwords[i]; // Set halfword value
            registers[2] = 0x00000010 + (i * 2); // Different offset for each (halfword aligned)
            
            // STRH R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 4, 0x5288); // STRH R0, [R1, R2]
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify halfword was stored correctly
            uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001010 + (i * 2));
            ASSERT_EQ(stored_value, test_halfwords[i]) << "Halfword value 0x" << std::hex << test_halfwords[i];
            validateUnchangedRegisters(cpu, beforeState, {15});
        }
    }
}

TEST(Format8, LDSB_SIGN_EXTENDED_BYTE_BASIC) {
    std::string beforeState;

    // Test case 1: LDSB R0, [R1, R2] - positive byte (no sign extension)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000003; // Offset
        registers[0] = 0xDEADBEEF; // Should be overwritten
        
        // Pre-store a positive byte value in memory
        gba.getCPU().getMemory().write8(0x00001003, 0x7F); // Positive byte
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5688); // LDSB R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the byte was loaded and zero-extended (positive)
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x0000007F));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDSB with negative byte (sign extension)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001200; // Base address
        registers[4] = 0x00000007; // Offset
        registers[5] = 0x12345678; // Should be overwritten
        
        // Pre-store a negative byte value in memory
        gba.getCPU().getMemory().write8(0x00001207, 0x80); // Negative byte (MSB set)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x571D); // LDSB R5, [R3, R4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the byte was loaded and sign-extended (negative)
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0xFFFFFF80));
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 3: LDSB with various byte values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000010; // Base offset
        
        struct TestCase {
            uint8_t byte_value;
            uint32_t expected_result;
        };
        
        TestCase test_cases[] = {
            {0x00, 0x00000000}, // Zero
            {0x01, 0x00000001}, // Small positive
            {0x7F, 0x0000007F}, // Maximum positive (127)
            {0x80, 0xFFFFFF80}, // Minimum negative (-128)
            {0xFF, 0xFFFFFFFF}, // -1
            {0xFE, 0xFFFFFFFE}, // -2
            {0x81, 0xFFFFFF81}  // -127
        };
        
        for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
            // Pre-store byte in memory
            gba.getCPU().getMemory().write8(0x00001010 + i, test_cases[i].byte_value);
            
            registers[0] = 0xDEADBEEF; // Reset destination
            registers[2] = 0x00000010 + i; // Adjust offset
            
            // LDSB R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 4, 0x5688); // LDSB R0, [R1, R2]
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify byte was loaded and sign-extended correctly
            ASSERT_EQ(registers[0], test_cases[i].expected_result) 
                << "Byte 0x" << std::hex << (int)test_cases[i].byte_value 
                << " expected 0x" << test_cases[i].expected_result 
                << " got 0x" << registers[0];
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }
}

TEST(Format8, LDRH_HALFWORD_REGISTER_OFFSET_BASIC) {
    std::string beforeState;

    // Test case 1: LDRH R0, [R1, R2] - basic halfword load
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000008; // Offset
        registers[0] = 0xDEADBEEF; // Should be overwritten
        
        // Pre-store a halfword value in memory
        gba.getCPU().getMemory().write16(0x00001008, 0xABCD);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5A88); // LDRH R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded and zero-extended
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x0000ABCD));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDRH with different registers
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[4] = 0x00001300; // Base address
        registers[5] = 0x0000000C; // Offset
        registers[6] = 0xFFFFFFFF; // Should be overwritten
        
        // Pre-store a halfword value in memory
        gba.getCPU().getMemory().write16(0x0000130C, 0x1234);
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5B66); // LDRH R6, [R4, R5]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded and zero-extended
        ASSERT_EQ(registers[6], static_cast<uint32_t>(0x00001234));
        validateUnchangedRegisters(cpu, beforeState, {6, 15});
    }

    // Test case 3: LDRH with different halfword values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000020; // Base offset
        
        uint16_t test_halfwords[] = {0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF};
        
        for (size_t i = 0; i < sizeof(test_halfwords)/sizeof(test_halfwords[0]); i++) {
            // Pre-store halfword in memory
            gba.getCPU().getMemory().write16(0x00001020 + (i * 2), test_halfwords[i]);
            
            registers[0] = 0xDEADBEEF; // Reset destination
            registers[2] = 0x00000020 + (i * 2); // Adjust offset (halfword aligned)
            
            // LDRH R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 4, 0x5A88); // LDRH R0, [R1, R2]
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify halfword was loaded and zero-extended correctly
            ASSERT_EQ(registers[0], static_cast<uint32_t>(test_halfwords[i])) 
                << "Halfword 0x" << std::hex << test_halfwords[i];
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }
}

TEST(Format8, LDSH_SIGN_EXTENDED_HALFWORD_BASIC) {
    std::string beforeState;

    // Test case 1: LDSH R0, [R1, R2] - positive halfword (no sign extension)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x0000000E; // Offset
        registers[0] = 0xDEADBEEF; // Should be overwritten
        
        // Pre-store a positive halfword value in memory
        gba.getCPU().getMemory().write16(0x0000100E, 0x7FFF); // Positive halfword
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5E88); // LDSH R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded without sign extension (positive)
        ASSERT_EQ(registers[0], static_cast<uint32_t>(0x00007FFF));
        validateUnchangedRegisters(cpu, beforeState, {0, 15});
    }

    // Test case 2: LDSH with negative halfword (sign extension)
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[3] = 0x00001400; // Base address
        registers[4] = 0x00000012; // Offset
        registers[5] = 0x12345678; // Should be overwritten
        
        // Pre-store a negative halfword value in memory
        gba.getCPU().getMemory().write16(0x00001412, 0x8000); // Negative halfword (MSB set)
        
        gba.getCPU().getMemory().write16(0x00000000, 0x5F1D); // LDSH R5, [R3, R4]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify the halfword was loaded and sign-extended (negative)
        ASSERT_EQ(registers[5], static_cast<uint32_t>(0xFFFF8000));
        validateUnchangedRegisters(cpu, beforeState, {5, 15});
    }

    // Test case 3: LDSH with various halfword values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000030; // Base offset
        
        struct TestCase {
            uint16_t halfword_value;
            uint32_t expected_result;
        };
        
        TestCase test_cases[] = {
            {0x0000, 0x00000000}, // Zero
            {0x0001, 0x00000001}, // Small positive
            {0x7FFF, 0x00007FFF}, // Maximum positive (32767)
            {0x8000, 0xFFFF8000}, // Minimum negative (-32768)
            {0xFFFF, 0xFFFFFFFF}, // -1
            {0xFFFE, 0xFFFFFFFE}, // -2
            {0x8001, 0xFFFF8001}  // -32767
        };
        
        for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
            // Pre-store halfword in memory
            gba.getCPU().getMemory().write16(0x00001030 + (i * 2), test_cases[i].halfword_value);
            
            registers[0] = 0xDEADBEEF; // Reset destination
            registers[2] = 0x00000030 + (i * 2); // Adjust offset (halfword aligned)
            
            // LDSH R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 4, 0x5E88); // LDSH R0, [R1, R2]
            registers[15] = i * 4; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Verify halfword was loaded and sign-extended correctly
            ASSERT_EQ(registers[0], test_cases[i].expected_result) 
                << "Halfword 0x" << std::hex << test_cases[i].halfword_value 
                << " expected 0x" << test_cases[i].expected_result 
                << " got 0x" << registers[0];
            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }
}

TEST(Format8, STRH_LDRH_ROUNDTRIP_TESTS) {
    std::string beforeState;

    // Test case 1: Store and load back halfword values
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000040; // Offset
        
        uint16_t test_halfwords[] = {0x0000, 0x1234, 0x7FFF, 0x8000, 0xFFFF, 0xABCD};
        
        for (size_t i = 0; i < sizeof(test_halfwords)/sizeof(test_halfwords[0]); i++) {
            registers[0] = 0x55550000 | test_halfwords[i]; // Set halfword value
            registers[2] = 0x00000040 + (i * 2); // Different offset
            
            // Store halfword: STRH R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 8, 0x5288); // STRH R0, [R1, R2]
            registers[15] = i * 8; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Load back: LDRH R3, [R1, R2]
            registers[3] = 0xDEADBEEF;
            gba.getCPU().getMemory().write16(i * 8 + 2, 0x5A8B); // LDRH R3, [R1, R2]
            cpu.execute(1);
            
            // Verify halfword round-trip (zero-extended)
            ASSERT_EQ(registers[3], static_cast<uint32_t>(test_halfwords[i])) 
                << "Halfword 0x" << std::hex << test_halfwords[i];
            validateUnchangedRegisters(cpu, beforeState, {3, 15});
        }
    }

    // Test case 2: Store halfword, load as sign-extended
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000050; // Offset
        
        struct TestCase {
            uint16_t store_value;
            uint32_t expected_ldrh;
            uint32_t expected_ldsh;
        };
        
        TestCase test_cases[] = {
            {0x7FFF, 0x00007FFF, 0x00007FFF}, // Positive: same result
            {0x8000, 0x00008000, 0xFFFF8000}, // Negative: different results
            {0xFFFF, 0x0000FFFF, 0xFFFFFFFF}  // -1: different results
        };
        
        for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
            registers[0] = 0x12340000 | test_cases[i].store_value;
            registers[2] = 0x00000050 + (i * 4); // 4-byte spacing for comparison
            
            // Store: STRH R0, [R1, R2]
            gba.getCPU().getMemory().write16(i * 12, 0x5288); // STRH R0, [R1, R2]
            registers[15] = i * 12; // Reset PC
            beforeState = serializeCPUState(cpu);
            cpu.execute(1);
            
            // Load as unsigned: LDRH R3, [R1, R2]
            registers[3] = 0xDEADBEEF;
            gba.getCPU().getMemory().write16(i * 12 + 2, 0x5A8B); // LDRH R3, [R1, R2]
            cpu.execute(1);
            ASSERT_EQ(registers[3], test_cases[i].expected_ldrh) 
                << "LDRH for 0x" << std::hex << test_cases[i].store_value;
            
            // Load as signed: LDSH R4, [R1, R2]
            registers[4] = 0xDEADBEEF;
            gba.getCPU().getMemory().write16(i * 12 + 4, 0x5E8C); // LDSH R4, [R1, R2]
            cpu.execute(1);
            ASSERT_EQ(registers[4], test_cases[i].expected_ldsh) 
                << "LDSH for 0x" << std::hex << test_cases[i].store_value;
            
            validateUnchangedRegisters(cpu, beforeState, {3, 4, 15});
        }
    }
}

TEST(Format8, EDGE_CASES_AND_BOUNDARY_CONDITIONS) {
    std::string beforeState;

    // Test case 1: Memory alignment and boundary conditions
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        // Test near end of memory
        registers[1] = 0x00001FF0; // Base near end
        registers[2] = 0x0000000E; // Offset
        registers[0] = 0x12345678; // Test value
        
        // STRH R0, [R1, R2] - Address will be 0x1FFE
        gba.getCPU().getMemory().write16(0x00000000, 0x5288); // STRH R0, [R1, R2]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify halfword stored correctly at boundary
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001FFE);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0x5678));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 2: Same register combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00000400; // Base address (also offset when doubled)
        registers[0] = 0xAAAABBBB; // Test value
        
        // STRH R0, [R1, R1] - Address will be 0x400 + 0x400 = 0x800
        gba.getCPU().getMemory().write16(0x00000000, 0x5248); // STRH R0, [R1, R1]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify halfword stored at calculated address
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00000800);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0xBBBB));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 3: Maximum register combinations
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[7] = 0x00001000; // Base (max register)
        registers[6] = 0x00000200; // Offset (max-1 register)
        registers[5] = 0xDDDDEEEE; // Value (max-2 register)
        
        // STRH R5, [R7, R6] - Use maximum register numbers
        gba.getCPU().getMemory().write16(0x00000000, 0x53BD); // STRH R5, [R7, R6]
        beforeState = serializeCPUState(cpu);
        cpu.execute(1);
        
        // Verify halfword stored correctly
        uint16_t stored_value = gba.getCPU().getMemory().read16(0x00001200);
        ASSERT_EQ(stored_value, static_cast<uint16_t>(0xEEEE));
        validateUnchangedRegisters(cpu, beforeState, {15});
    }

    // Test case 4: Sign extension edge cases
    {
        GBA gba(true); // Test mode
        auto& cpu = gba.getCPU();
        auto& registers = cpu.R();
        registers.fill(0);
        cpu.CPSR() = CPU::FLAG_T;
        
        registers[1] = 0x00001000; // Base address
        registers[2] = 0x00000060; // Offset
        
        // Test critical sign extension boundaries
        struct TestCase {
            uint8_t byte_value;
            uint16_t halfword_value;
            uint32_t expected_ldsb;
            uint32_t expected_ldsh;
        };
        
        TestCase test_cases[] = {
            {0x7F, 0x7FFF, 0x0000007F, 0x00007FFF}, // Maximum positive
            {0x80, 0x8000, 0xFFFFFF80, 0xFFFF8000}, // Minimum negative
            {0xFF, 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFF}  // All bits set
        };
        
        for (size_t i = 0; i < sizeof(test_cases)/sizeof(test_cases[0]); i++) {
            uint32_t base_addr = 0x00001060 + (i * 4);
            
            // Store test values
            gba.getCPU().getMemory().write8(base_addr, test_cases[i].byte_value);
            gba.getCPU().getMemory().write16(base_addr + 2, test_cases[i].halfword_value);
            
            // Test LDSB
            registers[0] = 0xDEADBEEF;
            registers[2] = 0x00000060 + (i * 4);
            gba.getCPU().getMemory().write16(i * 16, 0x5688); // LDSB R0, [R1, R2]
            registers[15] = i * 16;
            beforeState = serializeCPUState(cpu);  // Capture state before each execution
            cpu.execute(1);
            ASSERT_EQ(registers[0], test_cases[i].expected_ldsb) 
                << "LDSB for byte 0x" << std::hex << (int)test_cases[i].byte_value;

            // Test LDSH
            registers[0] = 0xDEADBEEF;
            registers[2] = 0x00000060 + (i * 4) + 2;
            gba.getCPU().getMemory().write16(i * 16 + 2, 0x5E88); // LDSH R0, [R1, R2]
            beforeState = serializeCPUState(cpu);  // Capture state before each execution
            cpu.execute(1);
            ASSERT_EQ(registers[0], test_cases[i].expected_ldsh) 
                << "LDSH for halfword 0x" << std::hex << test_cases[i].halfword_value;

            validateUnchangedRegisters(cpu, beforeState, {0, 15});
        }
    }
}
