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

// Test helper functions
void setup_test_cpu(CPU& cpu) {
    // Initialize registers to known values
    for (int i = 0; i < 16; i++) {
        cpu.R()[i] = 0x1000 + i * 0x100;
    }
    cpu.CPSR() = 0x10; // User mode, no flags set
}

void test_arm_multiply() {
    
    Memory memory;
    InterruptController interrupts;
    CPU cpu(memory, interrupts);
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Test MUL R0, R1, R2  (0xE0000291)
    cpu.R()[1] = 5;
    cpu.R()[2] = 7;
    cpu.R()[0] = 0; // Clear destination
    
    uint32_t mul_instruction = 0xE0000291; // MUL R0, R1, R2
    cpu.R()[15] = 0x00000000;
    memory.write32(0x00000000, mul_instruction);
    arm_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 35) << "MUL R0, R1, R2 failed";
    
    // Test MLA R3, R4, R5, R6  
    cpu.R()[4] = 3;  // Rm = 3
    cpu.R()[5] = 4;  // Rs = 4  
    cpu.R()[6] = 10; // Rn = 10
    cpu.R()[3] = 0;  // Clear destination
    
    // MLA R3, R4, R5, R6: R3 = R4 * R5 + R6 = 3 * 4 + 10 = 22
    // Format: cond 0000001S Rd   Rn   Rs   1001 Rm
    //         1110 0000001 0011 0110 0101 1001 0100
    uint32_t mla_instruction = 0xE0236594; // MLA R3, R4, R5, R6
    cpu.R()[15] = 0x00000004;
    memory.write32(0x00000004, mla_instruction);
    arm_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[3], 22) << "MLA R3, R4, R5, R6 failed";
}

void test_arm_data_processing() {
    
    Memory memory;
    InterruptController interrupts;
    CPU cpu(memory, interrupts);
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Test ADD R0, R1, R2
    cpu.R()[1] = 100;
    cpu.R()[2] = 25;
    cpu.R()[0] = 0; // Clear destination
    
    uint32_t add_instruction = 0xE0810002; // ADD R0, R1, R2
    cpu.R()[15] = 0x00000008;
    memory.write32(0x00000008, add_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 125) << "ADD R0, R1, R2 failed";
    
    // Test SUB with flags
    uint32_t sub_instruction = 0xE0510002; // SUBS R0, R1, R2
    cpu.R()[15] = 0x0000000C;
    memory.write32(0x0000000C, sub_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 75) << "SUBS R0, R1, R2 failed";
    
    // Test with immediate: MOV R3, #42
    uint32_t mov_imm_instruction = 0xE3A0302A; // MOV R3, #42
    cpu.R()[15] = 0x00000010;
    memory.write32(0x00000010, mov_imm_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], 42) << "MOV R3, #42 failed";

    // Test ADD R0, R1, R2
    cpu.R()[1] = 100;
    cpu.R()[2] = 25;
    uint32_t add2_instruction = 0xE0810002;
    cpu.R()[15] = 0x00000010;
    memory.write32(0x00000010, add2_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 125) << "ADD R0, R1, R2 failed";

    // Test SUB R4, R1, R2 with flag setting
    uint32_t sub2_instruction = 0xE0514002  | (1 << 20); // Set S bit;
    memory.write32(0x00000014, sub2_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 75) << "SUB R4, R1, R2 failed";
    uint32_t expected_flags = CPU::FLAG_C; // No borrow so set C flag
    EXPECT_EQ(cpu.CPSR() & 0xF0000000, expected_flags) << "SUB R4, R1, R2 flag test failed";

    // Test with shifts: MOV R5, R1, LSL #2
    uint32_t mov_shift_instruction = 0xE1A05101;
    memory.write32(0x00000018, mov_shift_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[5], 400) << "MOV R5, R1, LSL #2 failed";
        
    // Test logical operations: ORR R6, R1, R2
    uint32_t orr_instruction = 0xE1816002;
    memory.write32(0x0000001C, orr_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[6], 125) << "ORR R6, R1, R2 failed";  
}

void test_arm_conditional_execution() {
    
    Memory memory;
    InterruptController interrupts;
    CPU cpu(memory, interrupts);
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Set up flags for different conditions
    cpu.CPSR() |= 0x40000000; // Set Z flag
    
    // Test MOVEQ R0, #42 (0x03A0002A) - should execute (Z flag set)
    cpu.R()[0] = 0; // Clear destination
    uint32_t moveq_instruction = 0x03A0002A; // MOVEQ R0, #42
    cpu.R()[15] = 0x00000014;
    memory.write32(0x00000014, moveq_instruction);
    arm_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[0], 42) << "MOVEQ R0, #42 failed (Z flag set)";
    
    // Test MOVNE R1, #99 (0x13A01063) - should not execute (Z flag set)
    cpu.R()[1] = 0; // Clear destination
    uint32_t movne_instruction = 0x13A01063; // MOVNE R1, #99
    cpu.R()[15] = 0x00000018;
    memory.write32(0x00000018, movne_instruction);
    arm_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 0) << "MOVNE R1, #99 should not execute (Z flag set)";
    
    // Clear Z flag and test again
    cpu.CPSR() &= ~0x40000000; // Clear Z flag
    cpu.R()[15] = 0x0000001C;
    memory.write32(0x0000001C, movne_instruction);
    arm_cpu.execute(1);
    
    EXPECT_EQ(cpu.R()[1], 99) << "MOVNE R1, #99 should execute (Z flag clear)";
}

void test_arm_timing_integration() {
    
    Memory memory;
    InterruptController interrupts;
    CPU cpu(memory, interrupts);
    ARMCPU arm_cpu(cpu);
    TimingState timing;
    
    timing_init(&timing);
    setup_test_cpu(cpu);
    
    // Test instruction cycle calculation
    uint32_t add_instruction = 0xE0810002; // ADD R1, R1, R2
    uint32_t cycles = arm_cpu.calculateInstructionCycles(add_instruction);
    EXPECT_GE(cycles, 1) << "Instruction should take at least 1 cycle";
    
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
    EXPECT_EQ(format, 0) << "Data processing format detection failed";
    // printf("✓ Data processing format detected correctly (format %d)\n", format);
    
    uint32_t ldr = 0xE5910000; // LDR R0, [R1]
    format = ARM_GET_FORMAT(ldr);
    EXPECT_EQ(format, 2) << "Single data transfer format detection failed";
    // printf("✓ Single data transfer format detected correctly (format %d)\n", format);
    
    uint32_t branch = 0xEA000000; // B +0
    format = ARM_GET_FORMAT(branch);
    EXPECT_EQ(format, 5) << "Branch format detection failed";
    // printf("✓ Branch format detected correctly (format %d)\n", format);
    
    uint32_t ldm = 0xE8900003; // LDMIA R0, {R0,R1}
    format = ARM_GET_FORMAT(ldm);
    EXPECT_EQ(format, 4) << "Block transfer format detection failed";
    // printf("✓ Block transfer format detected correctly (format %d)\n", format);
}

void test_arm_memory_operations() {
    Memory memory;
    InterruptController interrupts;
    CPU cpu(memory, interrupts);
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    uint32_t test_address = 0x00000020; // Use RAM address in test harness

    // Store test data
    cpu.R()[1] = 0x12345678;
    cpu.R()[2] = test_address;
    printf("Before STR: R1=0x%08X, R2=0x%08X, mem[0x%08X]=0x%08X\n", cpu.R()[1], cpu.R()[2], test_address, memory.read32(test_address));

    memory.write32(test_address, 0xDEADBEEF);
    EXPECT_EQ(memory.read32(test_address), 0xDEADBEEF) << "Direct memory write/read failed";

    // STR R1, [R2]
    uint32_t str_instruction = 0xE5821000; // STR R1, [R2]
    cpu.R()[15] = 0x00000010;
    // Write instruction at PC (R15) to match ARM fetch behavior
    memory.write32(cpu.R()[15], str_instruction);
    arm_cpu.execute(1);
    printf("After STR:  R1=0x%08X, R2=0x%08X, mem[0x%08X]=0x%08X\n", cpu.R()[1], cpu.R()[2], test_address, memory.read32(test_address));
    printf("After STR:  mem[0x%08X]=0x%08X\n", 0x1020, memory.read32(0x1020));

    // Verify storage
    uint32_t stored_value = memory.read32(test_address);
    EXPECT_EQ(stored_value, 0x12345678) << "STR R1, [R2] failed";

    
    // Load it back
    cpu.R()[3] = 0; // Clear destination
    uint32_t ldr_instruction = 0xE5923000;
    cpu.R()[15] = 0x00000014;
    memory.write32(0x00000014, ldr_instruction);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3], 0x12345678) << "LDR R3, [R2] failed";

    // Demonstrate pre-indexed addressing: STR R1, [R2, #4]!
    uint32_t str_pre_instruction = 0xE5A21004;
    cpu.R()[2] = 0x00000100; // Set up R2 for pre-indexed addressing test
    cpu.R()[15] = 0x00000018;
    memory.write32(0x00000018, str_pre_instruction);
    EXPECT_EQ(cpu.R()[2], 0x00000100) << "R2 not set up for pre-indexed addressing test";
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000104) << "R2 not incremented after pre-indexed addressing test";
    EXPECT_EQ(cpu.R()[1], 0x12345678) << "STR R1, [R2, #4]! failed";

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
    EXPECT_EQ(cpu.R()[2], 0x00000100) << "R2 not set up for block transfer test";
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0x00000110) << "R2 not incremented after block transfer test";
    EXPECT_EQ(memory.read32(0x00000100), 0xAAAAAAAA) << "STMIA R2!, {R0,R1,R4,R5} failed R0";
    EXPECT_EQ(memory.read32(0x00000104), 0xBBBBBBBB) << "STMIA R2!, {R0,R1,R4,R5} failed R1";
    EXPECT_EQ(memory.read32(0x00000108), 0xCCCCCCCC) << "STMIA R2!, {R0,R1,R4,R5} failed R4";
    EXPECT_EQ(memory.read32(0x0000010C), 0xDDDDDDDD) << "STMIA R2!, {R0,R1,R4,R5} failed R5";
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
    
    
//     void demonstrateBranchingAndControl() {
//         printf("\n=== ARM Branching and Control Demonstration ===\n");
        
//         // Set up a test scenario
//         cpu.R()[15] = 0x08001000; // Set PC
//         cpu.R()[0] = 10; // Counter
        
//         printf("Starting PC: 0x%08X\n", cpu.R()[15]);
//         printf("Counter (R0): %d\n", cpu.R()[0]);
        
//         // Simulate a simple loop structure
//         // CMP R0, #0
//         uint32_t cmp_instruction = 0xE3500000;
//         printf("\nExecuting: CMP R0, #0\n");
//         arm_cpu.decodeAndExecute(cmp_instruction);
//         printf("CPSR after CMP: 0x%08X\n", cpu.CPSR());
        
//         // BNE +8 (branch if not equal)
//         uint32_t bne_instruction = 0x1A000001;
//         printf("\nExecuting: BNE +8 (should branch since R0 != 0)\n");
//         printf("PC before: 0x%08X\n", cpu.R()[15]);
//         arm_cpu.decodeAndExecute(bne_instruction);
//         printf("PC after: 0x%08X\n", cpu.R()[15]);
        
//         // Function call simulation: BL subroutine
//         cpu.R()[15] = 0x08002000;
//         cpu.R()[14] = 0; // Clear LR
//         uint32_t bl_instruction = 0xEB000010; // BL +64
//         printf("\nExecuting: BL +64 (function call)\n");
//         printf("PC before: 0x%08X, LR before: 0x%08X\n", cpu.R()[15], cpu.R()[14]);
//         arm_cpu.decodeAndExecute(bl_instruction);
//         printf("PC after: 0x%08X, LR after: 0x%08X\n", cpu.R()[15], cpu.R()[14]);
//     }
    
//     void demonstrateExceptionHandling() {
//         printf("\n=== ARM Exception Handling Demonstration ===\n");
        
//         // Set up initial state
//         cpu.R()[15] = 0x08003000;
//         cpu.CPSR() = 0x10; // User mode
        
//         printf("Initial state:\n");
//         printf("  PC: 0x%08X\n", cpu.R()[15]);
//         printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
//         printf("  LR: 0x%08X\n", cpu.R()[14]);
        
//         // Software interrupt
//         uint32_t swi_instruction = 0xEF000042; // SWI #0x42
//         printf("\nExecuting: SWI #0x42\n");
//         arm_cpu.decodeAndExecute(swi_instruction);
        
//         printf("After SWI:\n");
//         printf("  PC: 0x%08X (should be 0x08)\n", cpu.R()[15]);
//         printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
//         printf("  LR_svc: 0x%08X\n", cpu.R()[14]);
//         printf("  IRQ disabled: %s\n", (cpu.CPSR() & 0x80) ? "Yes" : "No");
        
//         // Undefined instruction
//         cpu.R()[15] = 0x08004000;
//         cpu.CPSR() = 0x10; // Reset to User mode
        
//         printf("\nTesting undefined instruction:\n");
//         printf("Initial PC: 0x%08X\n", cpu.R()[15]);
        
//         uint32_t undef_instruction = 0xE7F000F0;
//         printf("Executing undefined instruction: 0x%08X\n", undef_instruction);
//         arm_cpu.decodeAndExecute(undef_instruction);
        
//         printf("After undefined instruction:\n");
//         printf("  PC: 0x%08X (should be 0x04)\n", cpu.R()[15]);
//         printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
//     }
    
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

TEST(ArmCore, DataProcessing) {
    test_arm_data_processing();
}

TEST(ArmCore, Multiply) {
    test_arm_multiply();
}

TEST(ArmCore, ConditionalExecution) {
    test_arm_conditional_execution();
}

TEST(ArmCore, TimingIntegration) {
    test_arm_timing_integration();
}

TEST(ArmCore, MemoryOperations) {
    test_arm_memory_operations();
};

// TEST(ArmCore, BranchingAndControl();
// TEST(ArmCore, ExceptionHandling();
// TEST(ArmCore, TimingAndPerformance();
// TEST(ArmCore, ARMThumbInterworking();


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
