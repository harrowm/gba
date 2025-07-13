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

    ArmCoreTest() : cpu(memory, interrupts), arm_cpu(cpu) {}

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
    uint32_t sub2_instruction = 0xE0514002  | (1 << 20); // Set S bit;
    memory.write32(0x00000014, sub2_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], (uint32_t)75) << "SUB R4, R1, R2 failed";
    uint32_t expected_flags = CPU::FLAG_C; // No borrow so set C flag
    EXPECT_EQ(cpu.CPSR() & 0xF0000000, (uint32_t)expected_flags) << "SUB R4, R1, R2 flag test failed";

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

void test_arm_timing_integration() {
    
    Memory memory;
    InterruptController interrupts;
    CPU cpu(memory, interrupts);
    ARMCPU arm_cpu(cpu);
    TimingState timing;
    
    timing_init(&timing);
    
    // Test instruction cycle calculation
    uint32_t add_instruction = 0xE0810002; // ADD R1, R1, R2
    uint32_t cycles = arm_cpu.calculateInstructionCycles(add_instruction);
    EXPECT_GE(cycles, (uint32_t)1) << "Instruction should take at least 1 cycle";
    
    // Test different instruction types
    uint32_t mul_instruction = 0xE0000291; // MUL R0, R1, R2
    uint32_t mul_cycles = arm_cpu.calculateInstructionCycles(mul_instruction);
    printf("✓ MUL instruction cycles: %d\n", mul_cycles);
    
    uint32_t ldr_instruction = 0xE5910000; // LDR R0, [R1]
    uint32_t ldr_cycles = arm_cpu.calculateInstructionCycles(ldr_instruction);
    printf("✓ LDR instruction cycles: %d\n", ldr_cycles);
}

void test_arm_instruction_decoding() {
    // Test different instruction formats using the format bits
    uint32_t data_proc = 0xE0810002; // ADD R0, R1, R2
    uint32_t format = ARM_GET_FORMAT(data_proc);
    EXPECT_EQ(format, (uint32_t)0) << "Data processing format detection failed";
    // printf("✓ Data processing format detected correctly (format %d)\n", format);
    
    uint32_t ldr = 0xE5910000; // LDR R0, [R1]
    format = ARM_GET_FORMAT(ldr);
    EXPECT_EQ(format, (uint32_t)2) << "Single data transfer format detection failed";
    // printf("✓ Single data transfer format detected correctly (format %d)\n", format);
    
    uint32_t branch = 0xEA000000; // B +0
    format = ARM_GET_FORMAT(branch);
    EXPECT_EQ(format, (uint32_t)5) << "Branch format detection failed";
    // printf("✓ Branch format detected correctly (format %d)\n", format);
    
    uint32_t ldm = 0xE8900003; // LDMIA R0, {R0,R1}
    format = ARM_GET_FORMAT(ldm);
    EXPECT_EQ(format, (uint32_t)4) << "Block transfer format detection failed";
    // printf("✓ Block transfer format detected correctly (format %d)\n", format);
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


/// ----------
// #include <stdio.h>
// #include <stdint.h>
// #include <vector>
// #include <chrono>

// Include headers
// extern "C" {
// #include "timing.h"
// #include "arm_timing.h"
// }

// #include "cpu.h"
// #include "arm_cpu.h"
// #include "thumb_cpu.h"
// #include "debug.h"

// class ARMDemonstration {
// private:
//     Memory memory;
//     InterruptController interrupts;
//     CPU cpu;
//     ARMCPU arm_cpu;
//     ThumbCPU thumb_cpu;
//     TimingState timing;
    
// public:
//     ARMDemonstration() : cpu(memory, interrupts), arm_cpu(cpu), thumb_cpu(cpu) {
//         timing_init(&timing);
//         setupInitialState();
//     }
    
//     void setupInitialState() {
//         // Initialize CPU to a known state
//         for (int i = 0; i < 16; i++) {
//             cpu.R()[i] = 0;
//         }
//         cpu.R()[13] = 0x03008000; // Stack pointer (IWRAM)
//         cpu.R()[15] = 0x08000000; // Program counter (ROM)
//         cpu.CPSR() = 0x1F; // System mode, all interrupts enabled
        
//         printf("CPU initialized:\n");
//         printf("  SP (R13): 0x%08X\n", cpu.R()[13]);
//         printf("  PC (R15): 0x%08X\n", cpu.R()[15]);
//         printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
//     }
    
//     std::string getModeString(uint32_t mode) {
//         switch (mode) {
//             case 0x10: return "User";
//             case 0x11: return "FIQ";
//             case 0x12: return "IRQ";
//             case 0x13: return "Supervisor";
//             case 0x17: return "Abort";
//             case 0x1B: return "Undefined";
//             case 0x1F: return "System";
//             default: return "Unknown";
//         }
//     }
    
    


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
    uint32_t undef_instruction = 0xE7F000F0;
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
    reset_to_user();
    cpu.R()[14] = 0xDEADBEEF;
    arm_cpu.handleException(0x08, 0x13, true, false); // SWI
    EXPECT_EQ(cpu.bankedLR(CPU::SVC), (uint32_t)0x00000104) << "SVC LR not set correctly after SWI";
    cpu.setMode(CPU::USER);
    EXPECT_EQ(cpu.R()[14], (uint32_t)0xDEADBEEF) << "User LR not preserved after exception";
}
    
//     void demonstrateTimingAndPerformance() {
//         printf("\n=== ARM Timing and Performance Demonstration ===\n");
        
//         // Set up test program
//         cpu.R()[15] = 0x08000000;
        
//         // Measure timing for different instruction types
//         std::vector<uint32_t> test_instructions = {
//             0xE1A00000, // MOV R0, R0 (NOP)
//             0xE0810002, // ADD R1, R1, R2
//             0xE0000291, // MUL R0, R1, R2
//             0xE5912000, // LDR R2, [R1]
//             0xE8BD000F, // LDMIA R13!, {R0-R3}
//             0xEA000000  // B +0
//         };
        
//         std::vector<std::string> instruction_names = {
//             "MOV (NOP)",
//             "ADD",
//             "MUL",
//             "LDR",
//             "LDMIA",
//             "B"
//         };
        
//         printf("Instruction cycle timing:\n");
//         for (size_t i = 0; i < test_instructions.size(); i++) {
//             uint32_t cycles = arm_cpu.calculateInstructionCycles(test_instructions[i]);
//             printf("  %-10s: %d cycles\n", instruction_names[i].c_str(), cycles);
//         }
        
//         // Performance benchmark
//         printf("\nPerformance benchmark (1000 instructions):\n");
        
//         auto start_time = std::chrono::high_resolution_clock::now();
        
//         // Execute 1000 simple instructions with timing
//         for (int i = 0; i < 1000; i++) {
//             arm_cpu.executeWithTiming(1, &timing);
//         }
        
//         auto end_time = std::chrono::high_resolution_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
//         printf("  Time taken: %lld microseconds\n", duration.count());
//         printf("  Instructions per second: %.2f million\n", 
//                (1000.0 / duration.count()) * 1000000 / 1000000);
//         printf("  System clock cycles: %llu\n", timing.total_cycles);
//         printf("  Emulated time: %.3f ms\n", 
//                (double)timing.total_cycles / GBA_CLOCK_FREQUENCY * 1000);
//     }
    
//     void demonstrateARMThumbInterworking() {
//         printf("\n=== ARM/Thumb Interworking Demonstration ===\n");
        
//         // Start in ARM mode
//         cpu.R()[15] = 0x08000000;
//         cpu.CPSR() &= ~0x20; // Clear T bit (ARM mode)
        
//         printf("Starting in ARM mode\n");
//         printf("CPSR: 0x%08X (T bit: %d)\n", cpu.CPSR(), (cpu.CPSR() >> 5) & 1);
        
//         // Execute ARM instruction
//         uint32_t arm_add = 0xE0810002; // ADD R1, R1, R2
//         cpu.R()[1] = 10;
//         cpu.R()[2] = 5;
        
//         printf("\nExecuting ARM ADD R1, R1, R2\n");
//         printf("Before: R1=%d, R2=%d\n", cpu.R()[1], cpu.R()[2]);
//         arm_cpu.decodeAndExecute(arm_add);
//         printf("After: R1=%d\n", cpu.R()[1]);
        
//         // Switch to Thumb mode (normally done by BX instruction)
//         printf("\nSwitching to Thumb mode...\n");
//         cpu.CPSR() |= 0x20; // Set T bit
//         cpu.R()[15] = 0x08001000; // Thumb code location (must be halfword aligned)
        
//         printf("CPSR: 0x%08X (T bit: %d)\n", cpu.CPSR(), (cpu.CPSR() >> 5) & 1);
        
//         // Execute Thumb instruction
//         uint16_t thumb_add = 0x1889; // ADD R1, R1, R2 (Thumb)
//         cpu.R()[1] = 20;
//         cpu.R()[2] = 3;
        
//         printf("\nExecuting Thumb ADD R1, R1, R2\n");
//         printf("Before: R1=%d, R2=%d\n", cpu.R()[1], cpu.R()[2]);
        
//         // Write Thumb instruction to memory and execute
//         cpu.getMemory().write16(cpu.R()[15], thumb_add);
//         thumb_cpu.execute(1);
        
//         printf("After: R1=%d\n", cpu.R()[1]);
        
//         printf("\nARM/Thumb interworking complete!\n");
//     }
// };

// int main() {
//     printf("ARM7TDMI Advanced Features Demonstration\n");
//     printf("========================================\n");
    
//     ARMDemonstration demo;
    
//     demo.demonstrateDataProcessing();
//     demo.demonstrateMemoryOperations();
//     demo.demonstrateBranchingAndControl();
//     demo.demonstrateExceptionHandling();
//     demo.demonstrateTimingAndPerformance();
//     demo.demonstrateARMThumbInterworking();
    
  
// /// ----------

TEST(ArmCore, InstructionDecoding) {
    test_arm_instruction_decoding();
}

