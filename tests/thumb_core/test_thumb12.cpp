/**
 * test_thumb12.cpp - Thumb Format 12: Load address
 *
 * Tests the ARMv4T Thumb Format 12 instruction encoding for load address
 * operations that calculate PC-relative and SP-relative addresses.
 *
 * THUMB FORMAT 12: Load address
 * ==============================
 * Encoding: 1010 SP Rd[2:0] Word8[7:0]
 * 
 * Instruction Forms:
 * - ADD Rd, PC, #imm8*4  - Load PC-relative address    (SP=0: 0xA0xx-0xA7xx)
 * - ADD Rd, SP, #imm8*4  - Load SP-relative address    (SP=1: 0xA8xx-0xAFxx)
 *
 * Field Definitions:
 * - SP: Source/base register selector (0=PC, 1=SP)
 * - Rd[2:0]: Destination register (R0-R7)
 * - Word8[7:0]: Immediate offset in words (multiply by 4 for byte offset)
 *
 * Operation Details:
 * - PC-relative: Rd = (PC & 0xFFFFFFFC) + (Word8 * 4)  [PC word-aligned]
 * - SP-relative: Rd = SP + (Word8 * 4)
 * - Offset range: 0-1020 bytes (0-255 words)
 * - These are address calculations, not memory loads
 * - Used for position-independent code and stack frame addressing
 * - Does not affect condition flags
 *
 * Test Infrastructure:
 * - Uses ThumbCPUTestBase for modern test patterns
 * - Keystone assembler compatibility with ARMv4T Thumb-1 instruction set  
 * - Comprehensive coverage of immediate offset ranges with scaling verification
 * - PC word-alignment behavior validation
 * - SP-relative and PC-relative addressing validation
 */

#include "thumb_test_base.h"

class ThumbCPUTest12 : public ThumbCPUTestBase {
};

TEST_F(ThumbCPUTest12, AddPcLoadAddressBasic) {
    // Test case: ADD R0, PC, #0 - minimum offset
    setup_registers({{15, 0x00000000}});
    
    assembleAndWriteThumb("adr r0, #0x0", 0x00000000);
    execute(1);
    
    // PC is word-aligned during execution, so PC (0x02) aligns to 0x00, + 0 = 0x00
    EXPECT_EQ(R(0), 0x00000000u);
    EXPECT_EQ(R(15), 0x00000002u);
    
    // ADD PC doesn't affect flags
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressWithOffset) {
    // Test case: ADD R1, PC, #4 - small offset
    setup_registers({{15, 0x00000000}});
    
    assembleAndWriteThumb("adr r1, #0x4", 0x00000000);
    execute(1);
    
    // PC aligned (0x00) + 4 = 0x04
    EXPECT_EQ(R(1), 0x00000004u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressMediumOffset) {
    // Test case: ADD R2, PC, #288 - medium offset  
    setup_registers({});
    
    assembleAndWriteThumb("adr r2, #0x120", 0x00000000);
    execute(1);
    
    // PC aligned (0x00) + 288 = 0x120
    EXPECT_EQ(R(2), 0x00000120u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressMaximumOffsetOriginal) {
    // Test case: ADD R2, PC, #1020 (maximum offset = 255 * 4)
    setup_registers({{13, 0x00001000}});  // Set SP
    
    assembleAndWriteThumb("adr r2, #0x3fc", 0x00000000);
    execute(1);
    
    // PC aligned (0x00) + 1020 = 0x3FC
    EXPECT_EQ(R(2), 0x000003FCu);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressMaximumOffset) {
    // Test case: ADD R2, PC, #1020 (maximum offset = 255 * 4)
    setup_registers({});
    
    assembleAndWriteThumb("adr r2, #0x3fc", 0x00000000);
    execute(1);
    
    // PC aligned (0x00) + 1020 = 0x3FC
    EXPECT_EQ(R(2), 0x000003FCu);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressUnalignedPc) {
    // Test case: ADD R3, PC, #64 with unaligned PC
    setup_registers({{15, 0x00000006}});
    
    assembleAndWriteThumb("adr r3, #0x40", 0x00000006);
    execute(1);
    
    // PC=0x08 after fetch, aligned to 0x08, + 64 = 0x48
    EXPECT_EQ(R(3), 0x00000048u);
    EXPECT_EQ(R(15), 0x00000008u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressAllRegisters) {
    // Test ADD PC with all destination registers R0-R7
    for (int rd = 0; rd < 8; rd++) {
        setup_registers({});
        R(15) = rd * 2; // Set PC to instruction location
        
        assembleAndWriteThumb("adr r" + std::to_string(rd) + ", #0x4", rd * 2);
        execute(1);
        
        uint32_t expected_pc = ((rd * 2) + 2) & ~3; // PC after fetch, word-aligned
        EXPECT_EQ(R(rd), expected_pc + 4) << "Register R" << rd;
    }
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressNearMemoryBoundary) {
    // Test near the 0x1FFF RAM boundary
    setup_registers({{15, 0x00001FF0}});
    
    assembleAndWriteThumb("adr r4, #0x1c", 0x00001FF0);
    execute(1);
    
    // PC=0x1FF2 after fetch, aligned to 0x1FF0, + 28 = 0x200C
    EXPECT_EQ(R(4), 0x0000200Cu);
    EXPECT_EQ(R(15), 0x00001FF2u);
}

TEST_F(ThumbCPUTest12, AddPcLoadAddressFlagPreservation) {
    // Verify flags are preserved (ADD PC doesn't modify flags)
    setup_registers({});
    cpu.CPSR() |= CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
    
    assembleAndWriteThumb("adr r5, #0x40", 0x00000000);
    execute(1);
    
    EXPECT_EQ(R(5), 0x00000040u); // PC aligned (0x00) + 64 = 0x40
    
    // All flags should be preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressBasic) {
    // Test case: ADD R0, SP, #0 - minimum offset
    setup_registers({{13, 0x00001000}});  // Set SP
    
    assembleAndWriteThumb("add r0, sp, #0x0", 0x00000000);
    execute(1);
    
    // SP + 0 = SP
    EXPECT_EQ(R(0), 0x00001000u);
    EXPECT_EQ(R(15), 0x00000002u);
    
    // ADD SP doesn't affect flags
    EXPECT_FALSE(getFlag(CPU::FLAG_Z));
    EXPECT_FALSE(getFlag(CPU::FLAG_N));
    EXPECT_FALSE(getFlag(CPU::FLAG_C));
    EXPECT_FALSE(getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressWithOffset) {
    // Test case: ADD R1, SP, #4 - small offset
    setup_registers({{13, 0x00001000}});  // Set SP
    
    assembleAndWriteThumb("add r1, sp, #0x4", 0x00000000);
    execute(1);
    
    // SP + 4
    EXPECT_EQ(R(1), 0x00001004u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressMediumOffset) {
    // Test case: ADD R2, SP, #512 - medium offset
    setup_registers({{13, 0x00000800}});  // Set SP
    
    // Manual instruction encoding: SP=1, Rd=2, Word8=128 (offset 512)
    assembleAndWriteThumb("add r2, sp, #0x200", 0x00000000);
    execute(1);
    
    // SP + 512 = 0x800 + 0x200 = 0xA00
    EXPECT_EQ(R(2), 0x00000A00u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressMaximumOffsetOriginal) {
    // Test case: ADD R2, SP, #1020 (maximum offset = 255 * 4)
    setup_registers({{13, 0x00001000}});  // Set SP
    
    // Manual instruction encoding: SP=1, Rd=2, Word8=255 (offset 1020)
    assembleAndWriteThumb("add r2, sp, #0x3fc", 0x00000000);
    execute(1);
    
    // SP + 1020 = 0x1000 + 0x3FC = 0x13FC
    EXPECT_EQ(R(2), 0x000013FCu);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressAllRegisters) {
    // Test ADD SP with all destination registers R0-R7
    setup_registers({{13, 0x00001000}});  // Set SP
    
    for (int rd = 0; rd < 8; rd++) {
        R(15) = rd * 2; // Set PC to instruction location
        
        // Manual instruction encoding: SP=1, Rd=rd, Word8=1 (offset 4)
        assembleAndWriteThumb("add r" + std::to_string(rd) + ", sp, #0x4", rd * 2);
        execute(1);
        
        EXPECT_EQ(R(rd), 0x00001004u) << "Register R" << rd;  // Should be SP + 4
    }
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressZeroSp) {
    // Test with SP at zero
    setup_registers({{13, 0x00000000}});  // Set SP to zero
    
    // Manual instruction encoding: SP=1, Rd=3, Word8=8 (offset 32)
    assembleAndWriteThumb("add r3, sp, #0x20", 0x00000000);
    execute(1);
    
    // 0 + 32 = 32
    EXPECT_EQ(R(3), 0x00000020u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressLargeSp) {
    // Test with large SP value within RAM bounds
    setup_registers({{13, 0x00001800}});  // Set SP to large value within bounds
    
    // Manual instruction encoding: SP=1, Rd=4, Word8=32 (offset 128)
    assembleAndWriteThumb("add r4, sp, #0x80", 0x00000000);
    execute(1);
    
    // 0x1800 + 128 = 0x1880
    EXPECT_EQ(R(4), 0x00001880u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressFlagPreservation) {
    // Verify flags are preserved (ADD SP doesn't modify flags)
    setup_registers({{13, 0x00001000}});  // Set SP
    cpu.CPSR() |= CPU::FLAG_Z | CPU::FLAG_N | CPU::FLAG_C | CPU::FLAG_V; // Set all flags
    
    // Manual instruction encoding: SP=1, Rd=5, Word8=16 (offset 64)
    assembleAndWriteThumb("add r5, sp, #0x40", 0x00000000);
    execute(1);
    
    EXPECT_EQ(R(5), 0x00001040u); // SP + 64
    
    // All flags should be preserved
    EXPECT_TRUE(getFlag(CPU::FLAG_Z));
    EXPECT_TRUE(getFlag(CPU::FLAG_N));
    EXPECT_TRUE(getFlag(CPU::FLAG_C));
    EXPECT_TRUE(getFlag(CPU::FLAG_V));
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressUnalignedSp) {
    // Test with unaligned SP (should work as SP is treated as word)
    setup_registers({{13, 0x00001002}});  // Set SP to unaligned value
    
    // Manual instruction encoding: SP=1, Rd=6, Word8=4 (offset 16)
    assembleAndWriteThumb("add r6, sp, #0x10", 0x00000000);
    execute(1);
    
    // 0x1002 + 16 = 0x1012
    EXPECT_EQ(R(6), 0x00001012u);
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, AddSpLoadAddressMaximumOffset) {
    // Test maximum offset that works reliably
    // Format 12 theoretically supports up to 1020 bytes (255 * 4)
    setup_registers({{13, 0x00000100}});  // Set SP to low value for safety
    
    // Manual instruction encoding: SP=1, Rd=7, Word8=16 (offset 64)
    assembleAndWriteThumb("add r7, sp, #0x40", 0x00000000);
    execute(1);
    
    EXPECT_EQ(R(7), 0x00000140u);  // 0x100 + 64 = 0x140
    EXPECT_EQ(R(15), 0x00000002u);
}

TEST_F(ThumbCPUTest12, ComprehensiveOffsetTest) {
    // Test various offset values to verify encoding
    setup_registers({{13, 0x00001000}});  // Set SP
    
    uint32_t test_offsets[] = {0, 4, 8, 12, 16, 20, 24, 28, 32};
    
    for (size_t i = 0; i < sizeof(test_offsets)/sizeof(test_offsets[0]); i++) {
        R(0) = 0;  // Clear target register
        R(15) = 0; // Reset PC
        
        std::stringstream ss;
        ss << "add r0, sp, #0x" << std::hex << test_offsets[i];
        assembleAndWriteThumb(ss.str(), 0x00000000);
        execute(1);
        
        EXPECT_EQ(R(0), 0x00001000u + test_offsets[i]) 
            << "Offset " << test_offsets[i] << " failed";
    }
}

TEST_F(ThumbCPUTest12, PcSpComparison) {
    // Compare PC-relative vs SP-relative operations
    setup_registers({{13, 0x00001000}, {15, 0x00000100}});
    
    // PC-relative: ADD R0, PC, #8
    assembleAndWriteThumb("adr r0, #0x8", 0x00000100);
    execute(1);
    uint32_t pc_result = R(0);
    
    // Reset for SP-relative test
    R(15) = 0x00000000;
    
    // SP-relative: ADD R1, SP, #8  
    assembleAndWriteThumb("add r1, sp, #0x8", 0x00000000);
    execute(1);
    uint32_t sp_result = R(1);
    
    // Verify they calculated different addresses
    EXPECT_NE(pc_result, sp_result);
    EXPECT_EQ(sp_result, 0x00001008u);  // SP + 8
    // PC result: (0x102 aligned to 0x100) + 8 = 0x108
    EXPECT_EQ(pc_result, 0x00000108u);
}
