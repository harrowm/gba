#include <gtest/gtest.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Include headers
extern "C" {
#include "timing.h"
#include "arm_timing.h"
}

#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"


class ArmCoreTest : public ::testing::Test {
protected:
    Memory memory; 
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;
    ThumbCPU thumb_cpu;

    ArmCoreTest() : memory(true), cpu(memory, interrupts), arm_cpu(cpu), thumb_cpu(cpu) {}

    void SetUp() override {
        for (int i = 0; i < 16; i++) {
            cpu.R()[i] = 0x1000 + i * 0x100;
        }
        cpu.CPSR() = 0x10; // User mode, no flags set
    }
};


TEST_F(ArmCoreTest, Multiply) {
    // Test MUL R0, R1, R2  (0xE0000291)
    cpu.R()[1] = 5;
    cpu.R()[2] = 7;
    cpu.R()[0] = 0; // Clear destination
    uint32_t mul_instruction = 0xE0000291; // MUL R0, R1, R2
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000000, mul_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)35) << "MUL R0, R1, R2 failed";

    // Test MLA R3, R4, R5, R6  
    cpu.R()[4] = 3;  // Rm = 3
    cpu.R()[5] = 4;  // Rs = 4  
    cpu.R()[6] = 10; // Rn = 10
    cpu.R()[3] = 0;  // Clear destination
    uint32_t mla_instruction = 0xE0236594; // MLA R3, R4, R5, R6
    cpu.R()[15] = 0x00000004;
    memory.write32(0x00000004, mla_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], (uint32_t)22) << "MLA R3, R4, R5, R6 failed";
}

TEST_F(ArmCoreTest, DataProcessing) {
    // Test ADD R0, R1, R2
    cpu.R()[1] = 100;
    cpu.R()[2] = 25;
    cpu.R()[0] = 0; // Clear destination
    uint32_t add_instruction = 0xE0810002; // ADD R0, R1, R2
    cpu.R()[15] = 0x00000008;
    memory.write32(0x00000008, add_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)125) << "ADD R0, R1, R2 failed";

    // Test SUB with flags
    uint32_t sub_instruction = 0xE0510002; // SUBS R0, R1, R2
    cpu.R()[15] = 0x0000000C;
    memory.write32(0x0000000C, sub_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)75) << "SUBS R0, R1, R2 failed";

    // Test with immediate: MOV R3, #42
    uint32_t mov_imm_instruction = 0xE3A0302A; // MOV R3, #42
    cpu.R()[15] = 0x00000010;
    memory.write32(0x00000010, mov_imm_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], (uint32_t)42) << "MOV R3, #42 failed";

    // Test ADD R0, R1, R2
    cpu.R()[1] = 100;
    cpu.R()[2] = 25;
    uint32_t add2_instruction = 0xE0810002;
    cpu.R()[15] = 0x00000010;
    memory.write32(0x00000010, add2_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)125) << "ADD R0, R1, R2 failed";

    // Test SUB R4, R1, R2 with flag setting
    uint32_t sub2_instruction = 0xE0514002; // SUBS R4, R1, R2
    memory.write32(0x00000014, sub2_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], (uint32_t)75) << "SUBS R4, R1, R2 failed";
    uint32_t expected_flags = CPU::FLAG_C; // No borrow so set C flag
    EXPECT_EQ(cpu.CPSR() & 0xF0000000, (uint32_t)expected_flags) << "SUBS R4, R1, R2 flag test failed";

    // Test with shifts: MOV R5, R1, LSL #2
    uint32_t mov_shift_instruction = 0xE1A05101;
    memory.write32(0x00000018, mov_shift_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[5], (uint32_t)400) << "MOV R5, R1, LSL #2 failed";
        
    // Test logical operations: ORR R6, R1, R2
    uint32_t orr_instruction = 0xE1816002;
    memory.write32(0x0000001C, orr_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[6], (uint32_t)125) << "ORR R6, R1, R2 failed";  
}


TEST_F(ArmCoreTest, ConditionalExecution) {
    // Set up flags for different conditions
    cpu.CPSR() |= 0x40000000; // Set Z flag

    // Test MOVEQ R0, #42 (0x03A0002A) - should execute (Z flag set)
    cpu.R()[0] = 0; // Clear destination
    uint32_t moveq_instruction = 0x03A0002A; // MOVEQ R0, #42
    cpu.R()[15] = 0x00000014;
    memory.write32(0x00000014, moveq_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)42) << "MOVEQ R0, #42 failed (Z flag set)";

    // Test MOVNE R1, #99 (0x13A01063) - should not execute (Z flag set)
    cpu.R()[1] = 0; // Clear destination
    uint32_t movne_instruction = 0x13A01063; // MOVNE R1, #99
    cpu.R()[15] = 0x00000018;
    memory.write32(0x00000018, movne_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], (uint32_t)0) << "MOVNE R1, #99 should not execute (Z flag set)";

    // Clear Z flag and test again
    cpu.CPSR() &= ~0x40000000; // Clear Z flag
    cpu.R()[15] = 0x0000001C;
    memory.write32(0x0000001C, movne_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], (uint32_t)99) << "MOVNE R1, #99 should execute (Z flag clear)";
}

TEST_F(ArmCoreTest, MemoryOperations) {
    uint32_t test_address = 0x00000020; // Use RAM address in test harness

    // Store test data
    cpu.R()[1] = 0x12345678;
    cpu.R()[2] = test_address;

    memory.write32(test_address, 0xDEADBEEF);
    EXPECT_EQ(memory.read32(test_address), (uint32_t)0xDEADBEEF) << "Direct memory write/read failed";

    // STR R1, [R2]
    uint32_t str_instruction = 0xE5821000; // STR R1, [R2]
    cpu.R()[15] = 0x00000010;
    // Write instruction at PC (R15) to match ARM fetch behavior
    memory.write32(cpu.R()[15], str_instruction);
    arm_cpu.execute(1);

    // Verify storage
    uint32_t stored_value = memory.read32(test_address);
    EXPECT_EQ(stored_value, (uint32_t)0x12345678) << "STR R1, [R2] failed";

    // Load it back
    cpu.R()[3] = 0; // Clear destination
    uint32_t ldr_instruction = 0xE5923000;
    cpu.R()[15] = 0x00000014;
    memory.write32(0x00000014, ldr_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], (uint32_t)0x12345678) << "LDR R3, [R2] failed";

    // Demonstrate pre-indexed addressing: STR R1, [R2, #4]!
    uint32_t str_pre_instruction = 0xE5A21004;
    cpu.R()[2] = 0x00000100; // Set up R2 for pre-indexed addressing test
    cpu.R()[15] = 0x00000018;
    memory.write32(0x00000018, str_pre_instruction);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x00000100) << "R2 not set up for pre-indexed addressing test";
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x00000104) << "R2 not incremented after pre-indexed addressing test";
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x12345678) << "STR R1, [R2, #4]! failed";

    // Demonstrate pre-indexed addressing: STR R1, [R2, R4]!
    uint32_t str_pre_reg_instruction = 0xE7A21004;
    cpu.R()[2] = 0x00000100; // Set up R2 for pre-indexed addressing test
    cpu.R()[15] = 0x00000018;
    cpu.R()[4] = 0x00000010; // Offset to add
    memory.write32(0x00000018, str_pre_reg_instruction);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x00000100) << "R2 not set up for pre-indexed addressing test";
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x00000110) << "R2 not incremented after pre-indexed reg addressing test";
    EXPECT_EQ(cpu.R()[1], (uint32_t)0x12345678) << "STR R1, [R2, #4]! failed";


    // Block transfer demonstration
    cpu.R()[0] = 0xAAAAAAAA;
    cpu.R()[1] = 0xBBBBBBBB;
    cpu.R()[4] = 0xCCCCCCCC;
    cpu.R()[5] = 0xDDDDDDDD;
    cpu.R()[2] = 0x00000100; // Set up R2 for block transfer test

    // STMIA R2!, {R0,R1,R4,R5}
    uint32_t stm_instruction = 0xE8A20033;
    cpu.R()[15] = 0x00000018;
    memory.write32(0x00000018, stm_instruction);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x00000100) << "R2 not set up for block transfer test";
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], (uint32_t)0x00000110) << "R2 not incremented after block transfer test";
    EXPECT_EQ(memory.read32(0x00000100), (uint32_t)0xAAAAAAAA) << "STMIA R2!, {R0,R1,R4,R5} failed R0";
    EXPECT_EQ(memory.read32(0x00000104), (uint32_t)0xBBBBBBBB) << "STMIA R2!, {R0,R1,R4,R5} failed R1";
    EXPECT_EQ(memory.read32(0x00000108), (uint32_t)0xCCCCCCCC) << "STMIA R2!, {R0,R1,R4,R5} failed R4";
    EXPECT_EQ(memory.read32(0x0000010C), (uint32_t)0xDDDDDDDD) << "STMIA R2!, {R0,R1,R4,R5} failed R5";
}


TEST_F(ArmCoreTest, BranchingAndControl) {
    // Set up a test scenario in RAM (0x0000 - 0x1FFF)
    cpu.R()[15] = 0x00000010; // Set PC to valid RAM
    cpu.R()[0] = 10; // Counter

    // CMP R0, #0 (should set Z=0, since R0 != 0)
    uint32_t cmp_instruction = 0xE3500000;
    memory.write32(cpu.R()[15], cmp_instruction);
    arm_cpu.execute(1);
    // Z flag is bit 30
    EXPECT_EQ((cpu.CPSR() >> 30) & 1, (uint32_t)0) << "CMP R0, #0 should clear Z flag when R0 != 0";

    // BNE +8 (should branch since Z==0)
    uint32_t bne_instruction = 0x1A000001;
    uint32_t pc_before = cpu.R()[15];
    memory.write32(cpu.R()[15], bne_instruction);
    arm_cpu.execute(1);
    // BNE offset is 1, so PC += 8 + (4 * 1) = 12
    EXPECT_EQ(cpu.R()[15], pc_before + 8 + 4) << "BNE did not branch correctly";

    // Function call simulation: BL subroutine (also in RAM)
    cpu.R()[15] = 0x00000020;
    cpu.R()[14] = 0; // Clear LR
    uint32_t bl_instruction = 0xEB000010; // BL +64
    uint32_t pc_bl_before = cpu.R()[15];
    memory.write32(cpu.R()[15], bl_instruction);
    arm_cpu.execute(1);
    // BL offset is 0x10, so PC should be pc_bl_before + 8 + (4 * 0x10) = pc_bl_before + 8 + 64 = pc_bl_before + 72
    EXPECT_EQ(cpu.R()[15], pc_bl_before + 8 + 64) << "BL did not branch to correct address";
    // LR should be set to pc_bl_before + 4
    EXPECT_EQ(cpu.R()[14], pc_bl_before + 4) << "BL did not set LR correctly";
}
    

TEST_F(ArmCoreTest, ExceptionHandling) {
    // Helper lambda to reset to user mode
    auto reset_to_user = [&]() {
        cpu.CPSR() = 0x10;
        cpu.setMode(CPU::USER); // Restore banked LR after setting CPSR
        cpu.R()[15] = 0x00000100;
        cpu.R()[14] = 0;
    };

    // --- Supervisor (SWI) Exception ---
    reset_to_user();
    uint32_t swi_instruction = 0xEF000042; // SWI #0x42
    memory.write32(cpu.R()[15], swi_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x08) << "SWI did not branch to correct vector";
    EXPECT_EQ(cpu.CPSR() & 0x1F, (uint32_t)0x13) << "SWI did not switch to Supervisor mode";
    EXPECT_EQ(cpu.bankedLR(CPU::SVC), (uint32_t)0x00000104) << "SWI did not set LR_svc correctly";
    EXPECT_TRUE((cpu.CPSR() & 0x80) != 0) << "SWI did not disable IRQ";

    // --- Undefined Instruction Exception ---
    reset_to_user();
    uint32_t undef_instruction = 0xE0400090;
    memory.write32(cpu.R()[15], undef_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x04) << "Undefined did not branch to correct vector";
    EXPECT_EQ(cpu.CPSR() & 0x1F, (uint32_t)0x1B) << "Undefined did not switch to Undefined mode";
    EXPECT_EQ(cpu.bankedLR(CPU::UND), (uint32_t)0x00000104) << "Undefined did not set LR_und correctly";

    // --- IRQ Exception (simulate by direct call) ---
    reset_to_user();
    arm_cpu.handleException(0x18, 0x12, true, false); // IRQ vector, IRQ mode
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x18) << "IRQ did not branch to correct vector";
    EXPECT_EQ(cpu.CPSR() & 0x1F, (uint32_t)0x12) << "IRQ did not switch to IRQ mode";
    EXPECT_EQ(cpu.bankedLR(CPU::IRQ), (uint32_t)0x00000104) << "IRQ did not set LR_irq correctly";
    EXPECT_TRUE((cpu.CPSR() & 0x80) != 0) << "IRQ did not disable IRQ";

    // --- Abort Exception (simulate by direct call) ---
    reset_to_user();
    arm_cpu.handleException(0x10, 0x17, true, false); // Prefetch abort vector, ABT mode
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x10) << "Abort did not branch to correct vector";
    EXPECT_EQ(cpu.CPSR() & 0x1F, (uint32_t)0x17) << "Abort did not switch to Abort mode";
    EXPECT_EQ(cpu.bankedLR(CPU::ABT), (uint32_t)0x00000104) << "Abort did not set LR_abt correctly";
    EXPECT_TRUE((cpu.CPSR() & 0x80) != 0) << "Abort did not disable IRQ";

    // --- FIQ Exception (simulate by direct call) ---
    reset_to_user();
    arm_cpu.handleException(0x1C, 0x11, true, true); // FIQ vector, FIQ mode
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x1C) << "FIQ did not branch to correct vector";
    EXPECT_EQ(cpu.CPSR() & 0x1F, (uint32_t)0x11) << "FIQ did not switch to FIQ mode";
    EXPECT_EQ(cpu.bankedLR(CPU::FIQ), (uint32_t)0x00000104) << "FIQ did not set LR_fiq correctly";
    EXPECT_TRUE((cpu.CPSR() & 0x80) != 0) << "FIQ did not disable IRQ";
    EXPECT_TRUE((cpu.CPSR() & 0x40) != 0) << "FIQ did not disable FIQ";

    // --- Check that user LR is preserved ---
    DEBUG_ERROR("Checking user LR preservation after exceptions");
    reset_to_user();
    cpu.R()[14] = 0xDEADBEEF;
    arm_cpu.handleException(0x08, 0x13, true, false); // SWI
    EXPECT_EQ(cpu.bankedLR(CPU::SVC), (uint32_t)0x00000104) << "SVC LR not set correctly after SWI";
    cpu.setMode(CPU::USER);
    EXPECT_EQ(cpu.R()[14], (uint32_t)0xDEADBEEF) << "User LR not preserved after exception";
}
    

TEST_F(ArmCoreTest, TimingAndPerformance) {
    // Use RAM region for all test code/data
    constexpr uint32_t test_pc = 0x00000000;
    // Clear all registers
    for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
    cpu.R()[15] = test_pc;
    cpu.CPSR() = 0x10;

    // Measure timing for different instruction types
    std::vector<uint32_t> test_instructions = {
        0xE1A00000, // MOV R0, R0 (NOP)
        0xE0811002, // ADD R1, R1, R2 (correct encoding)
        0xE0000291, // MUL R0, R1, R2
        0xE5912000, // LDR R2, [R1]
        0xE8BD000F, // LDMIA R13!, {R0-R3}
        0xEA000000  // B +0
    };
    std::vector<std::string> instruction_names = {
        "MOV (NOP)", "ADD", "MUL", "LDR", "LDMIA", "B"
    };
    for (size_t i = 0; i < test_instructions.size(); i++) {
        uint32_t cycles = arm_cpu.calculateInstructionCycles(test_instructions[i]);
        EXPECT_GE(cycles, (uint32_t)1) << instruction_names[i] << " should take at least 1 cycle";
    }

    // Performance benchmark: execute 1000 NOPs (MOV R0, R0)
    TimingState timing;
    timing_init(&timing);
    cpu.R()[15] = test_pc;
    memory.write32(test_pc, 0xE1A00000); // MOV R0, R0 (NOP)

    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) {
        arm_cpu.executeWithTiming(1, &timing);
        cpu.R()[15] = 0x00000000; // Reset PC to test instruction
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Check that timing.total_cycles is at least 1000 (1 per NOP)
    EXPECT_GE(timing.total_cycles, (uint64_t)1000) << "Should execute at least 1000 cycles for 1000 NOPs";
    // Optionally, print timing for manual review
    std::cout << "Timing: " << duration.count() << " us, cycles: " << timing.total_cycles << std::endl;
}


TEST_F(ArmCoreTest, ARMThumbInterworking) {
    // Start in ARM mode
    // Use RAM region for all test code/data
    constexpr uint32_t arm_pc = 0x00000000;
    constexpr uint32_t thumb_pc = 0x00000100; // must be halfword aligned

    // Clear all registers to zero for a clean test state
    for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
    cpu.R()[15] = arm_pc;
    cpu.CPSR() &= ~0x20; // Clear T bit (ARM mode)
    EXPECT_EQ((cpu.CPSR() >> 5) & 1, 0u) << "Should start in ARM mode (T bit clear)";

    // Write ARM ADD instruction to memory at PC and execute
    cpu.R()[1] = 10;
    cpu.R()[2] = 5;
    // Encoding for ADD R1, R1, R2: cond=1110, 00, I=0, opcode=0100, S=0, Rn=1, Rd=1, shifter=R2
    // 0xE0800002 is ADD R0, R0, R2
    // 0xE0811002 is ADD R1, R1, R2
    uint32_t arm_add = 0xE0811002;
    cpu.R()[15] = arm_pc;
    memory.write32(arm_pc, arm_add);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 15u) << "ARM ADD R1, R1, R2 failed";

    // Switch to Thumb mode (normally done by BX instruction)
    cpu.CPSR() |= 0x20; // Set T bit
    cpu.R()[15] = thumb_pc; // Thumb code location (must be halfword aligned)
    EXPECT_EQ((cpu.CPSR() >> 5) & 1, 1u) << "Should be in Thumb mode (T bit set)";

    // Write Thumb ADD instruction to memory at PC and execute
    uint16_t thumb_add = 0x1889;
    cpu.R()[1] = 20;
    cpu.R()[2] = 3;
    memory.write16(thumb_pc, thumb_add);
    thumb_cpu.execute(1);
    EXPECT_EQ(cpu.R()[1], 23u) << "Thumb ADD R1, R1, R2 failed";
}


TEST_F(ArmCoreTest, DataProcessingAndPSRTransfer) {
    // Clear all registers and flags
    for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
    cpu.CPSR() = 0x10; // User mode, no flags
    cpu.R()[15] = 0x00000000;

    // --- ADD (register, no flags) ---
    cpu.R()[1] = 0x7FFFFFFF;
    cpu.R()[2] = 1;
    uint32_t add_reg = 0xE0810002; // ADD R0, R1, R2
    memory.write32(0x00000000, add_reg);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x80000000u) << "ADD R0, R1, R2 failed (overflow to negative)";

    // --- ADD (immediate, set flags, overflow/carry) ---
    cpu.R()[1] = 0xFFFFFFFF;
    uint32_t add_imm_s = 0xE2910001; // ADDS R0, R1, #1
    memory.write32(0x00000004, add_imm_s);
    cpu.R()[15] = 0x00000004;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u) << "ADDS R0, R1, #1 failed (should wrap to 0)";
    EXPECT_TRUE(cpu.CPSR() & CPU::FLAG_Z) << "ADDS did not set Z flag";
    EXPECT_TRUE(cpu.CPSR() & CPU::FLAG_C) << "ADDS did not set C flag (carry out)";

    // --- SUB (register, set flags, negative result) ---
    cpu.R()[1] = 1;
    cpu.R()[2] = 2;
    uint32_t sub_reg_s = 0xE0510002; // SUBS R0, R1, R2
    memory.write32(0x00000008, sub_reg_s);
    cpu.R()[15] = 0x00000008;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFFu) << "SUBS R0, R1, R2 failed (should be -1)";
    EXPECT_TRUE(cpu.CPSR() & CPU::FLAG_N) << "SUBS did not set N flag (negative)";

    // --- AND (register, with zero) ---
    cpu.R()[1] = 0xF0F0F0F0;
    cpu.R()[2] = 0x0F0F0F0F;
    uint32_t and_reg = 0xE0110002; // AND R0, R1, R2
    memory.write32(0x0000000C, and_reg);
    cpu.R()[15] = 0x0000000C;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u) << "AND R0, R1, R2 failed (should be 0)";

    // --- ORR (immediate, set flags) ---
    cpu.R()[1] = 0x00000001;
    uint32_t orr_imm_s = 0xE3910002; // ORRS R0, R1, #2
    memory.write32(0x00000010, orr_imm_s);
    cpu.R()[15] = 0x00000010;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 3u) << "ORRS R0, R1, #2 failed (should be 3)";
    EXPECT_FALSE(cpu.CPSR() & CPU::FLAG_Z) << "ORRS set Z flag incorrectly";

    // --- EOR (register, edge case) ---
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[2] = 0xAAAAAAAA;
    uint32_t eor_reg = 0xE0210002; // EOR R0, R1, R2
    memory.write32(0x00000014, eor_reg);
    cpu.R()[15] = 0x00000014;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x55555555u) << "EOR R0, R1, R2 failed (should be 0x55555555)";

    // --- MOV (immediate, set flags, zero) ---
    uint32_t mov_imm_s = 0xE3B00000; // MOVS R0, #0
    memory.write32(0x00000018, mov_imm_s);
    cpu.R()[15] = 0x00000018;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u) << "MOVS R0, #0 failed";
    EXPECT_TRUE(cpu.CPSR() & CPU::FLAG_Z) << "MOVS did not set Z flag";

    // --- MVN (immediate, set flags) ---
    uint32_t mvn_imm_s = 0xE3F00001; // MVNS R0, #1
    memory.write32(0x0000001C, mvn_imm_s);
    cpu.R()[15] = 0x0000001C;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu) << "MVNS R0, #1 failed (should be ~1)";

    // --- CMP (register, negative result) ---
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    uint32_t cmp_reg = 0xE1510002; // CMP R1, R2
    memory.write32(0x00000020, cmp_reg);
    cpu.R()[15] = 0x00000020;
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & CPU::FLAG_N) << "CMP did not set N flag (should be negative)";

    // --- TST (register, zero result) ---
    cpu.R()[1] = 0x00000000;
    cpu.R()[2] = 0xFFFFFFFF;
    uint32_t tst_reg = 0xE1110002; // TST R1, R2
    memory.write32(0x00000024, tst_reg);
    cpu.R()[15] = 0x00000024;
    arm_cpu.execute(1);
    EXPECT_TRUE(cpu.CPSR() & CPU::FLAG_Z) << "TST did not set Z flag (should be zero)";

    // --- TEQ (register, nonzero result) ---
    cpu.R()[1] = 0xF0F0F0F0;
    cpu.R()[2] = 0x0F0F0F0F;
    uint32_t teq_reg = 0xE1310002; // TEQ R1, R2
    memory.write32(0x00000028, teq_reg);
    cpu.R()[15] = 0x00000028;
    arm_cpu.execute(1);
    EXPECT_FALSE(cpu.CPSR() & CPU::FLAG_Z) << "TEQ set Z flag incorrectly (should be nonzero)";

    // --- Shifted operand (LSL by register) ---
    cpu.R()[1] = 4;
    cpu.R()[2] = 4;
    uint32_t mov_lsl_reg = 0xE1A00112; // MOV R0, R2, LSL R1
    memory.write32(0x0000002C, mov_lsl_reg);
    cpu.R()[15] = 0x0000002C;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x40u) << "MOV R0, R2, LSL R1 failed (should be 0x40)";

    // --- PSR Transfer: MRS (read CPSR) ---
    // MRS R3, CPSR: 0xE10F3000
    uint32_t mrs_cpsr = 0xE10F3000;
    cpu.R()[3] = 0;
    memory.write32(0x00000030, mrs_cpsr);
    cpu.R()[15] = 0x00000030;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], cpu.CPSR()) << "MRS R3, CPSR failed";

    // --- PSR Transfer: MSR (write CPSR flags from immediate) ---
    // MSR CPSR_f, #0xF0000000: 0xE32F020F (immediate value 0xF, rotate_imm 4 = 2*2 )
    uint32_t msr_cpsr_f_imm = 0xE32F020F;
    memory.write32(0x00000034, msr_cpsr_f_imm);
    cpu.R()[15] = 0x00000034;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR() & 0xF0000000, 0xF0000000u) << "MSR CPSR_f, #0xF0000000 failed to set flags";

    // --- PSR Transfer: MSR (write CPSR from register) ---
    cpu.R()[4] = 0xA0000000;
    // MSR CPSR_f, R4: 0xE12FF004 (unchanged, correct encoding)
    uint32_t msr_cpsr_f_reg = 0xE12FF004;
    memory.write32(0x00000038, msr_cpsr_f_reg);
    cpu.R()[15] = 0x00000038;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.CPSR() & 0xF0000000, 0xA0000000u) << "MSR CPSR_f, R4 failed to set flags";

    // --- Edge: MOV with max shift (LSR #32) ---
    cpu.R()[2] = 0xFFFFFFFF;
    cpu.R()[0] = 0xDEADBEEF; // Set to known value
    // MOV R0, R2, LSR #32: 0xE1A00022 with bit 11-7 cleared (shift_imm = 0b00000 = 32, special case)
    uint32_t mov_lsr32 = 0xE1A00022;
    memory.write32(0x0000003C, mov_lsr32);
    cpu.R()[15] = 0x0000003C;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u) << "MOV R0, R2, LSR #32 failed (should be 0)";
}

TEST_F(ArmCoreTest, MultiplyInstructions) {
    // Clear all registers and flags
    for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
    cpu.CPSR() = 0x10; // User mode, no flags
    cpu.R()[15] = 0x00000000;

    // --- Basic MUL: MUL R0, R1, R2 ---
    cpu.R()[1] = 7;
    cpu.R()[2] = 6;
    uint32_t mul_inst = 0xE0000291; // MUL R0, R1, R2
    memory.write32(0x00000000, mul_inst);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 42u) << "MUL R0, R1, R2 failed";

    // --- MLA: MLA R3, R4, R5, R6 ---
    cpu.R()[4] = 3;
    cpu.R()[5] = 4;
    cpu.R()[6] = 10;
    cpu.R()[3] = 0;
    uint32_t mla_inst = 0xE0236594; // MLA R3, R4, R5, R6
    memory.write32(0x00000004, mla_inst);
    cpu.R()[15] = 0x00000004;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], 22u) << "MLA R3, R4, R5, R6 failed";

    // --- MUL with zero ---
    cpu.R()[1] = 0;
    cpu.R()[2] = 12345;
    cpu.R()[0] = 0xFFFFFFFF;
    memory.write32(0x00000008, mul_inst);
    cpu.R()[15] = 0x00000008;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u) << "MUL R0, R1=0, R2 failed (should be 0)";

    // --- MUL with negative numbers ---
    cpu.R()[1] = (uint32_t)-5;
    cpu.R()[2] = 3;
    memory.write32(0x0000000C, mul_inst);
    cpu.R()[15] = 0x0000000C;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)-15) << "MUL R0, R1=-5, R2=3 failed";

    // --- MLA with negative accumulator ---
    cpu.R()[4] = 2;
    cpu.R()[5] = 4;
    cpu.R()[6] = (uint32_t)-10;
    memory.write32(0x00000010, mla_inst);
    cpu.R()[15] = 0x00000010;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], (uint32_t)-2) << "MLA R3, R4=2, R5=4, R6=-10 failed";

    // --- MUL with max unsigned values ---
    cpu.R()[1] = 0xFFFFFFFF;
    cpu.R()[2] = 2;
    memory.write32(0x00000014, mul_inst);
    cpu.R()[15] = 0x00000014;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xFFFFFFFEu) << "MUL R0, R1=0xFFFFFFFF, R2=2 failed";

    // --- MLA with overflow ---
    cpu.R()[4] = 0x80000000;
    cpu.R()[5] = 2;
    cpu.R()[6] = 0x80000000;
    memory.write32(0x00000018, mla_inst);
    cpu.R()[15] = 0x00000018;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], 0x80000000u) << "MLA R3, overflow case failed (0x80000000*2+0x80000000==0x80000000)";

    // --- MULS: MUL with S bit set, check flags ---
    uint32_t muls_inst = 0xE0100291; // MULS R0, R1, R2
    cpu.R()[1] = 0x80000000;
    cpu.R()[2] = 2;
    cpu.R()[0] = 0;
    memory.write32(0x0000001C, muls_inst);
    cpu.R()[15] = 0x0000001C;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u) << "MULS R0, R1=0x80000000, R2=2 failed";
    EXPECT_FALSE(cpu.CPSR() & CPU::FLAG_N) << "MULS N flag should not be set (result is zero)";

    // --- MLAS: MLA with S bit set, check flags ---
    uint32_t mlas_inst = 0xE0336594; // MLAS R3, R4, R5, R6 (S bit set)
    cpu.R()[4] = 0xFFFFFFFF;
    cpu.R()[5] = 2;
    cpu.R()[6] = 1;
    memory.write32(0x00000020, mlas_inst);
    cpu.R()[15] = 0x00000020;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], 0xFFFFFFFFu) << "MLAS R3, R4=0xFFFFFFFF, R5=2, R6=1 failed";
    EXPECT_TRUE(cpu.CPSR() & CPU::FLAG_N) << "MLAS did not set N flag (should be negative)";

    // --- MUL with accumulator not used (MLA with Rn=0) ---
    cpu.R()[4] = 2;
    cpu.R()[5] = 3;
    cpu.R()[6] = 0;
    memory.write32(0x00000024, mla_inst);
    cpu.R()[15] = 0x00000024;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], 6u) << "MLA R3, R4=2, R5=3, R6=0 failed (should be 6)";

    // --- MUL with all zeros ---
    cpu.R()[1] = 0;
    cpu.R()[2] = 0;
    memory.write32(0x00000028, mul_inst);
    cpu.R()[15] = 0x00000028;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0u) << "MUL R0, R1=0, R2=0 failed (should be 0)";

    // --- MLA with all zeros ---
    cpu.R()[4] = 0;
    cpu.R()[5] = 0;
    cpu.R()[6] = 0;
    memory.write32(0x0000002C, mla_inst);
    cpu.R()[15] = 0x0000002C;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], 0u) << "MLA R3, R4=0, R5=0, R6=0 failed (should be 0)";

    // --- MUL with max RAM address ---
    cpu.R()[1] = 2;
    cpu.R()[2] = 3;
    cpu.R()[0] = 0;
    memory.write32(0x1FFC, mul_inst);
    cpu.R()[15] = 0x1FFC;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 6u) << "MUL R0, R1=2, R2=3 at max RAM failed";
}

TEST_F(ArmCoreTest, MultiplyLongInstructions) {
    // Clear all registers and flags
    for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
    cpu.CPSR() = 0x10; // User mode, no flags
    cpu.R()[15] = 0x00000000;

    // --- UMULL: Unsigned multiply long ---
    cpu.R()[2] = 0x12345678;
    cpu.R()[3] = 0x9ABCDEF0;
    cpu.R()[0] = 0;
    cpu.R()[1] = 0;
    uint32_t umull_inst = 0xE0810392; // UMULL R0, R1, R2, R3
    
    memory.write32(0x00000000, umull_inst);
    uint32_t src2_umull = cpu.R()[2];
    uint32_t src3_umull = cpu.R()[3];
    uint64_t expected_umull = (uint64_t)src2_umull * (uint64_t)src3_umull;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)expected_umull) << "UMULL low failed";
    EXPECT_EQ(cpu.R()[1], (uint32_t)(expected_umull >> 32)) << "UMULL high failed";

    // --- UMLAL: Unsigned multiply-accumulate long ---
    cpu.R()[2] = 0x1000;
    cpu.R()[3] = 0x2000;
    cpu.R()[0] = 0x1;
    cpu.R()[1] = 0x2;
    uint32_t umlal_inst = 0xE0A20392; // UMLAL R0, R1, R2, R3
    memory.write32(0x00000004, umlal_inst);
    cpu.R()[15] = 0x00000004;
    uint32_t src2_umlal = cpu.R()[2];
    uint32_t src3_umlal = cpu.R()[3];
    uint32_t acc_lo_umlal = cpu.R()[0];
    uint32_t acc_hi_umlal = cpu.R()[1];
    uint64_t acc = ((uint64_t)acc_hi_umlal << 32) | acc_lo_umlal;
    uint64_t expected_umlal = acc + (uint64_t)src2_umlal * (uint64_t)src3_umlal;
    arm_cpu.execute(1);
    EXPECT_EQ(((uint64_t)cpu.R()[1] << 32 | cpu.R()[0]), expected_umlal) << "UMLAL failed";

    // --- SMULL: Signed multiply long ---
    cpu.R()[2] = (uint32_t)-1234;
    cpu.R()[3] = (uint32_t)5678;
    cpu.R()[0] = 0;
    cpu.R()[1] = 0;
    uint32_t smull_inst = 0xE0C10392; // SMULL R0, R1, R2, R3
    memory.write32(0x00000008, smull_inst);
    cpu.R()[15] = 0x00000008;
    int32_t src2_smull = (int32_t)cpu.R()[2];
    int32_t src3_smull = (int32_t)cpu.R()[3];
    int64_t expected_smull = (int64_t)src2_smull * (int64_t)src3_smull;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)expected_smull) << "SMULL low failed";
    EXPECT_EQ(cpu.R()[1], (uint32_t)(expected_smull >> 32)) << "SMULL high failed";

    // --- SMLAL: Signed multiply-accumulate long ---
    cpu.R()[2] = (uint32_t)-100;
    cpu.R()[3] = (uint32_t)50;
    cpu.R()[0] = 0xFFFFFFFF;
    cpu.R()[1] = 0x7FFFFFFF;
    uint32_t smlal_inst = 0xE0E20392; // SMLAL R0, R1, R2, R3
    memory.write32(0x0000000C, smlal_inst);
    cpu.R()[15] = 0x0000000C;
    int32_t src2_smlal = (int32_t)cpu.R()[2];
    int32_t src3_smlal = (int32_t)cpu.R()[3];
    int64_t acc_smlal = ((int64_t)cpu.R()[1] << 32) | cpu.R()[0];
    int64_t expected_smlal = acc_smlal + (int64_t)src2_smlal * (int64_t)src3_smlal;
    arm_cpu.execute(1);
    EXPECT_EQ(((uint64_t)cpu.R()[1] << 32 | cpu.R()[0]), (uint64_t)expected_smlal) << "SMLAL failed";

    // --- UMULL with zero ---
    cpu.R()[2] = 0;
    cpu.R()[3] = 0xFFFFFFFF;
    cpu.R()[0] = 0xDEADBEEF;
    cpu.R()[1] = 0xCAFEBABE;
    uint32_t src2_umull0 = cpu.R()[2];
    uint32_t src3_umull0 = cpu.R()[3];
    uint64_t expected_umull0 = (uint64_t)src2_umull0 * (uint64_t)src3_umull0;
    memory.write32(0x00000010, umull_inst);
    cpu.R()[15] = 0x00000010;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)expected_umull0) << "UMULL with zero low failed";
    EXPECT_EQ(cpu.R()[1], (uint32_t)(expected_umull0 >> 32)) << "UMULL with zero high failed";

    // --- SMULL with negative numbers ---
    cpu.R()[2] = (uint32_t)-1;
    cpu.R()[3] = (uint32_t)-1;
    cpu.R()[0] = 0;
    cpu.R()[1] = 0;
    int32_t src2_smull_neg = (int32_t)cpu.R()[2];
    int32_t src3_smull_neg = (int32_t)cpu.R()[3];
    int64_t expected_neg = (int64_t)src2_smull_neg * (int64_t)src3_smull_neg;
    memory.write32(0x00000014, smull_inst);
    cpu.R()[15] = 0x00000014;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)expected_neg) << "SMULL negative low failed";
    EXPECT_EQ(cpu.R()[1], (uint32_t)(expected_neg >> 32)) << "SMULL negative high failed";

    // --- S bit: UMULLS, SMULLS, UMLALS, SMLALS (check flags) ---
    uint32_t umulls_inst = 0xE0910392; // UMULLS R0, R1, R2, R3 (S bit set)
    cpu.R()[2] = 0xFFFFFFFF;
    cpu.R()[3] = 2;
    cpu.R()[0] = 0;
    cpu.R()[1] = 0;
    uint32_t src2_umulls = cpu.R()[2];
    uint32_t src3_umulls = cpu.R()[3];
    uint64_t expected_umulls = (uint64_t)src2_umulls * (uint64_t)src3_umulls;
    memory.write32(0x00000018, umulls_inst);
    cpu.R()[15] = 0x00000018;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)expected_umulls) << "UMULLS low failed";
    EXPECT_EQ(cpu.R()[1], (uint32_t)(expected_umulls >> 32)) << "UMULLS high failed";
    // N flag should be set if high result MSB is 1
    EXPECT_EQ((cpu.CPSR() & CPU::FLAG_N) != 0, (cpu.R()[1] & 0x80000000) != 0) << "UMULLS N flag incorrect";
    // Z flag should be set if both high and low are zero
    EXPECT_EQ((cpu.CPSR() & CPU::FLAG_Z) != 0, expected_umulls == 0) << "UMULLS Z flag incorrect";

    // --- RAM boundary test ---
    cpu.R()[2] = 2;
    cpu.R()[3] = 3;
    cpu.R()[0] = 0;
    cpu.R()[1] = 0;
    uint32_t src2_umull_ram = cpu.R()[2];
    uint32_t src3_umull_ram = cpu.R()[3];
    uint64_t expected_umull_ram = (uint64_t)src2_umull_ram * (uint64_t)src3_umull_ram;
    memory.write32(0x1FFC, umull_inst);
    cpu.R()[15] = 0x1FFC;
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], (uint32_t)expected_umull_ram) << "UMULL at max RAM low failed";
    EXPECT_EQ(cpu.R()[1], (uint32_t)(expected_umull_ram >> 32)) << "UMULL at max RAM high failed";
}
