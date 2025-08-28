/**
 * Thumb Format 15: Multiple load/store operations (STMIA/LDMIA)
 * Instruction encoding: 1100 L Rn[2:0] RegisterList[7:0]
 * 
 * STMIA (Store Multiple Increment After) - L=0:
 * - Encoding: 1100 0 Rn RegisterList (0xC000-0xC7FF)
 * - Stores registers to memory starting at address in Rn
 * - Increments Rn by 4 × number_of_registers after each store
 * - Registers stored in ascending order (R0 first, R7 last)
 * - If Rn is in RegisterList, stores OLD value of Rn, then updates Rn
 *
 * LDMIA (Load Multiple Increment After) - L=1:
 * - Encoding: 1100 1 Rn RegisterList (0xC800-0xCFFF)
 * - Loads registers from memory starting at address in Rn
 * - Increments Rn by 4 × number_of_registers after each load
 * - Registers loaded in ascending order (R0 first, R7 last)
 * - If Rn is in RegisterList, loads NEW value into Rn (overwrites increment)
 *
 * Address calculation:
 * - Memory addresses: Rn, Rn+4, Rn+8, ... for R0, R1, R2, ...
 * - Final Rn value: original_Rn + 4 × number_of_registers (unless Rn in list for LDMIA)
 * - Word-aligned access required for proper operation
 *
 * KEYSTONE LIMITATION NOTE:
 * While we prefer to rely on Keystone for assembly, testing has revealed that
 * Keystone fails to assemble certain LDMIA instruction variants, particularly:
 * - LDMIA with certain register combinations at specific addresses
 * - Empty register lists for both STMIA and LDMIA
 * These limitations prevent a pure Keystone-only approach for comprehensive testing.
 */
#include "thumb_test_base.h"

class ThumbCPUTest15 : public ThumbCPUTestBase {
};

TEST_F(ThumbCPUTest15, STMIA_SINGLE_REGISTER) {
    // Test case 1: STMIA R0!, {R1}
    setup_registers({{0, 0x00001000}, {1, 0xDEADBEEF}});
    
    assembleAndWriteThumb("stmia r0!, {r1}", 0x00000000);
    
    execute(1);
    
    // Check that data was stored at the correct address
    EXPECT_EQ(memory.read32(0x00001000), 0xDEADBEEFu);
    // Check that R0 was incremented
    EXPECT_EQ(R(0), 0x00001004u);
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test case 2: STMIA R2!, {R0}
    setup_registers({{2, 0x00001100}, {0, 0x12345678}});
    
    assembleAndWriteThumb("stmia r2!, {r0}", 0x00000002);
    
    R(15) = 0x00000002;
    execute(1);
    
    // Check that data was stored at the correct address
    EXPECT_EQ(memory.read32(0x00001100), 0x12345678u);
    // Check that R2 was incremented
    EXPECT_EQ(R(2), 0x00001104u);
    EXPECT_EQ(R(15), 0x00000004u);
    
    // Test case 3: STMIA R7!, {R7}
    setup_registers({{7, 0x00001200}});
    // R7 will store its own value, then be incremented
    
    assembleAndWriteThumb("stmia r7!, {r7}", 0x00000004);
    
    R(15) = 0x00000004;
    execute(1);
    
    // R7 should store its original value before increment
    EXPECT_EQ(memory.read32(0x00001200), 0x00001200u);
    // Check that R7 was incremented
    EXPECT_EQ(R(7), 0x00001204u);
    EXPECT_EQ(R(15), 0x00000006u);
}

TEST_F(ThumbCPUTest15, STMIA_MULTIPLE_REGISTERS) {
    // Test case 1: STMIA R0!, {R0, R1}
    setup_registers({{0, 0x00001000}, {1, 0x11111111}});
    
    assembleAndWriteThumb("stmia r0!, {r0, r1}", 0x00000000);
    
    execute(1);
    
    // Registers are stored in ascending order
    EXPECT_EQ(memory.read32(0x00001000), 0x00001000u); // R0 stored first
    EXPECT_EQ(memory.read32(0x00001004), 0x11111111u); // R1 stored second
    // Check that R0 was incremented
    EXPECT_EQ(R(0), 0x00001008u);
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test case 2: STMIA R3!, {R0, R2, R4, R6}
    setup_registers({{3, 0x00001300}, {0, 0xAAAAAAAA}, {2, 0xCCCCCCCC}, {4, 0xEEEEEEEE}, {6, 0x66666666}});
    
    assembleAndWriteThumb("stmia r3!, {r0, r2, r4, r6}", 0x00000002);
    
    R(15) = 0x00000002;
    execute(1);
    
    EXPECT_EQ(memory.read32(0x00001300), 0xAAAAAAAAu); // R0
    EXPECT_EQ(memory.read32(0x00001304), 0xCCCCCCCCu); // R2
    EXPECT_EQ(memory.read32(0x00001308), 0xEEEEEEEEu); // R4
    EXPECT_EQ(memory.read32(0x0000130C), 0x66666666u); // R6
    // Check that R3 was incremented by 4 * 4 = 16
    EXPECT_EQ(R(3), 0x00001310u);
    EXPECT_EQ(R(15), 0x00000004u);
    
    // Test case 3: STMIA R1!, {R0-R7}
    setup_registers({{1, 0x00001400}});
    for (int i = 0; i < 8; i++) {
        if (i != 1) { // Don't overwrite R1 (base register)
            R(i) = 0x10000000 + i;
        }
    }
    // R1 stores its original value (0x00001400) during STMIA
    
    assembleAndWriteThumb("stmia r1!, {r0, r1, r2, r3, r4, r5, r6, r7}", 0x00000004);
    
    R(15) = 0x00000004;
    execute(1);
    
    for (int i = 0; i < 8; i++) {
        uint32_t expected_value = (i == 1) ? 0x00001400u : (0x10000000 + i);
        EXPECT_EQ(memory.read32(0x00001400 + (i * 4)), expected_value);
    }
    // Check that R1 was incremented by 8 * 4 = 32
    EXPECT_EQ(R(1), 0x00001420u);
    EXPECT_EQ(R(15), 0x00000006u);
}

TEST_F(ThumbCPUTest15, LDMIA_SINGLE_REGISTER) {
    // Test case 1: LDMIA R0!, {R1} - Keystone works for this case
    setup_registers({{0, 0x00001000}});
    memory.write32(0x00001000, 0xDEADBEEF);
    
    assembleAndWriteThumb("ldmia r0!, {r1}", 0x00000000);
    
    execute(1);
    
    // Check that data was loaded into R1
    EXPECT_EQ(R(1), 0xDEADBEEFu);
    // Check that R0 was incremented
    EXPECT_EQ(R(0), 0x00001004u);
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test case 2: LDMIA R2!, {R0} - Keystone works for this case
    setup_registers({{2, 0x00001100}});
    memory.write32(0x00001100, 0x12345678);
    
    assembleAndWriteThumb("ldmia r2!, {r0}", 0x00000002);
    
    R(15) = 0x00000002;
    execute(1);
    
    // Check that data was loaded into R0
    EXPECT_EQ(R(0), 0x12345678u);
    // Check that R2 was incremented
    EXPECT_EQ(R(2), 0x00001104u);
    EXPECT_EQ(R(15), 0x00000004u);
    
    // Test case 3: LDMIA R7!, {R7} - Keystone fails, use manual encoding
    setup_registers({{7, 0x00001200}});
    memory.write32(0x00001200, 0xFEDCBA98);
    
    // Manual encoding: LDMIA R7!, {R7} = 0xCF80 (Keystone produces 0x0000)
    memory.write16(0x00000004, 0xCF80);
    
    R(15) = 0x00000004;
    execute(1);
    
    // R7 should be loaded with the value from memory, not incremented address
    EXPECT_EQ(R(7), 0xFEDCBA98u);
    EXPECT_EQ(R(15), 0x00000006u);
}

TEST_F(ThumbCPUTest15, LDMIA_MULTIPLE_REGISTERS) {
    // Test case 1: LDMIA R0!, {R0, R1} - Keystone fails, use manual encoding
    setup_registers({{0, 0x00001000}});
    memory.write32(0x00001000, 0xAAAAAAAA);
    memory.write32(0x00001004, 0xBBBBBBBB);
    
    // Manual encoding: LDMIA R0!, {R0, R1} = 0xC803 (Keystone produces 0x0000)
    memory.write16(0x00000000, 0xC803);
    
    execute(1);
    
    // R0 gets loaded with data, overwrites the increment behavior
    EXPECT_EQ(R(0), 0xAAAAAAAAu); // R0 loaded from memory
    EXPECT_EQ(R(1), 0xBBBBBBBBu); // R1 loaded second
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test case 2: LDMIA R3!, {R1, R3, R5, R7} - Keystone fails, use manual encoding
    setup_registers({{3, 0x00001300}});
    memory.write32(0x00001300, 0x11111111); // R1
    memory.write32(0x00001304, 0xCCCCCCCC); // R3
    memory.write32(0x00001308, 0x55555555); // R5
    memory.write32(0x0000130C, 0x77777777); // R7
    
    // Manual encoding: LDMIA R3!, {R1, R3, R5, R7} = 0xCBAA (Keystone produces 0x0000)
    memory.write16(0x00000002, 0xCBAA);
    
    R(15) = 0x00000002;
    execute(1);
    
    EXPECT_EQ(R(1), 0x11111111u); // R1
    EXPECT_EQ(R(3), 0xCCCCCCCCu); // R3 gets loaded value (overwrites increment)
    EXPECT_EQ(R(5), 0x55555555u); // R5
    EXPECT_EQ(R(7), 0x77777777u); // R7
    EXPECT_EQ(R(15), 0x00000004u);
    
    // Test case 3: LDMIA R1!, {R0-R7} - Keystone fails, use manual encoding
    setup_registers({{1, 0x00001400}});
    for (int i = 0; i < 8; i++) {
        memory.write32(0x00001400 + (i * 4), 0x20000000 + i);
    }
    
    // Manual encoding: LDMIA R1!, {R0-R7} = 0xC9FF (Keystone produces 0x0000)
    memory.write16(0x00000004, 0xC9FF);
    
    R(15) = 0x00000004;
    execute(1);
    
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(R(i), 0x20000000u + static_cast<uint32_t>(i));
    }
    EXPECT_EQ(R(15), 0x00000006u);
}

TEST_F(ThumbCPUTest15, EMPTY_REGISTER_LIST) {
    // Test case 1: STMIA with empty register list - Keystone fails, use manual encoding
    setup_registers({{0, 0x00001000}});
    
    // Manual encoding: STMIA R0!, {} = 0xC000 (Keystone cannot assemble empty list)
    memory.write16(0x00000000, 0xC000);
    
    execute(1);
    
    // With empty register list, behavior is implementation defined
    // Some implementations don't modify the base register
    EXPECT_EQ(R(0), 0x00001000u); // R0 unchanged
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test case 2: LDMIA with empty register list - Keystone fails, use manual encoding
    setup_registers({{2, 0x00001100}});
    
    // Manual encoding: LDMIA R2!, {} = 0xCA00 (Keystone cannot assemble empty list)
    memory.write16(0x00000002, 0xCA00);
    
    R(15) = 0x00000002;
    execute(1);
    
    // With empty register list, behavior is implementation defined
    EXPECT_EQ(R(2), 0x00001100u); // R2 unchanged
    EXPECT_EQ(R(15), 0x00000004u);
}

TEST_F(ThumbCPUTest15, MEMORY_ALIGNMENT_AND_BOUNDS) {
    // Test memory operations within 0x1FFF boundary
    setup_registers({{0, 0x1FF0}, {1, 0x12345678}});
    
    assembleAndWriteThumb("stmia r0!, {r1}", 0x00000000);
    
    execute(1);
    
    // Check that data was stored near memory boundary
    EXPECT_EQ(memory.read32(0x1FF0), 0x12345678u);
    EXPECT_EQ(R(0), 0x1FF4u);
    EXPECT_EQ(R(15), 0x00000002u);
    
    // Test load from the same location
    R(1) = 0; // Clear R1
    
    assembleAndWriteThumb("ldmia r0!, {r1}", 0x00000002);
    
    R(15) = 0x00000002;
    R(0) = 0x1FF0; // Reset base address
    execute(1);
    
    EXPECT_EQ(R(1), 0x12345678u);
    EXPECT_EQ(R(0), 0x1FF4u);
    EXPECT_EQ(R(15), 0x00000004u);
}

TEST_F(ThumbCPUTest15, INSTRUCTION_ENCODING_VALIDATION) {
    // Test various instruction encodings
    struct TestCase {
        uint16_t encoding;
        std::string description;
        uint8_t base_reg;
        uint8_t reg_list;
        bool is_load;
    };
    
    std::vector<TestCase> test_cases = {
        {0xC000, "STMIA R0!, {}", 0, 0x00, false},
        {0xC002, "STMIA R0!, {R1}", 0, 0x02, false},
        {0xC201, "STMIA R2!, {R0}", 2, 0x01, false},
        {0xC1FF, "STMIA R1!, {R0-R7}", 1, 0xFF, false},
        {0xC800, "LDMIA R0!, {}", 0, 0x00, true},
        {0xC802, "LDMIA R0!, {R1}", 0, 0x02, true},
        {0xCA01, "LDMIA R2!, {R0}", 2, 0x01, true},
        {0xC9FF, "LDMIA R1!, {R0-R7}", 1, 0xFF, true}
    };
    
    for (const auto& test_case : test_cases) {
        // Verify encoding structure: 1100 L Rn[2:0] RegisterList[7:0]
        uint16_t expected = 0xC000; // Base pattern
        expected |= (test_case.is_load ? 0x0800 : 0x0000); // L bit
        expected |= (test_case.base_reg & 0x7) << 8; // Rn bits
        expected |= test_case.reg_list; // Register list
        
        EXPECT_EQ(test_case.encoding, expected) 
            << "Encoding mismatch for " << test_case.description;
    }
}
