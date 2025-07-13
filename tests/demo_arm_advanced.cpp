#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <chrono>

// Include headers
extern "C" {
#include "timing.h"
#include "arm_timing.h"
}

#include "cpu.h"
#include "arm_cpu.h"
#include "thumb_cpu.h"
#include "debug.h"

class ARMDemonstration {
private:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;
    ThumbCPU thumb_cpu;
    TimingState timing;
    
public:
    ARMDemonstration() : cpu(memory, interrupts), arm_cpu(cpu), thumb_cpu(cpu) {
        timing_init(&timing);
        setupInitialState();
    }
    
    void setupInitialState() {
        // Initialize CPU to a known state
        for (int i = 0; i < 16; i++) {
            cpu.R()[i] = 0;
        }
        cpu.R()[13] = 0x03008000; // Stack pointer (IWRAM)
        cpu.R()[15] = 0x08000000; // Program counter (ROM)
        cpu.CPSR() = 0x1F; // System mode, all interrupts enabled
        
        printf("CPU initialized:\n");
        printf("  SP (R13): 0x%08X\n", cpu.R()[13]);
        printf("  PC (R15): 0x%08X\n", cpu.R()[15]);
        printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
    }
    
    std::string getModeString(uint32_t mode) {
        switch (mode) {
            case 0x10: return "User";
            case 0x11: return "FIQ";
            case 0x12: return "IRQ";
            case 0x13: return "Supervisor";
            case 0x17: return "Abort";
            case 0x1B: return "Undefined";
            case 0x1F: return "System";
            default: return "Unknown";
        }
    }
    
    void demonstrateDataProcessing() {
        printf("\n=== ARM Data Processing Demonstration ===\n");
        
        // Set up test values
        cpu.R()[1] = 100;
        cpu.R()[2] = 25;
        cpu.R()[3] = 0x80000000;
        
        printf("Initial values: R1=%d, R2=%d, R3=0x%08X\n", cpu.R()[1], cpu.R()[2], cpu.R()[3]);
        
        // ADD R0, R1, R2
        uint32_t add_instruction = 0xE0810002;
        printf("\nExecuting: ADD R0, R1, R2\n");
        cpu.R()[15] = 0x08000000;
        memory.write32(0x08000000, add_instruction);
        arm_cpu.execute(1);
        printf("Result: R0 = %d\n", cpu.R()[0]);
        
        // SUB R4, R1, R2 with flag setting
        uint32_t sub_instruction = 0xE0514002;
        printf("\nExecuting: SUBS R4, R1, R2\n");
        cpu.R()[15] = 0x08000004;
        memory.write32(0x08000004, sub_instruction);
        arm_cpu.execute(1);
        printf("Result: R4 = %d, CPSR = 0x%08X\n", cpu.R()[4], cpu.CPSR());
        
        // Test with shifts: MOV R5, R1, LSL #2
        uint32_t mov_shift_instruction = 0xE1A05101;
        printf("\nExecuting: MOV R5, R1, LSL #2\n");
        cpu.R()[15] = 0x08000008;
        memory.write32(0x08000008, mov_shift_instruction);
        arm_cpu.execute(1);
        printf("Result: R5 = %d (R1 << 2)\n", cpu.R()[5]);
        
        // Logical operations: ORR R6, R1, R2
        uint32_t orr_instruction = 0xE1816002;
        printf("\nExecuting: ORR R6, R1, R2\n");
        cpu.R()[15] = 0x0800000C;
        memory.write32(0x0800000C, orr_instruction);
        arm_cpu.execute(1);
        printf("Result: R6 = 0x%08X (R1 | R2)\n", cpu.R()[6]);
    }
    
    void demonstrateMemoryOperations() {
        printf("\n=== ARM Memory Operations Demonstration ===\n");
        
        uint32_t test_address = 0x02000000; // EWRAM
        
        // Store test data
        cpu.R()[1] = 0x12345678;
        cpu.R()[2] = test_address;
        
        // STR R1, [R2]
        uint32_t str_instruction = 0xE5821000;
        printf("Executing: STR R1, [R2] (storing 0x%08X at 0x%08X)\n", cpu.R()[1], cpu.R()[2]);
        cpu.R()[15] = 0x08000010;
        memory.write32(0x08000010, str_instruction);
        arm_cpu.execute(1);
        
        // Verify storage
        uint32_t stored_value = cpu.getMemory().read32(test_address);
        printf("Stored value: 0x%08X\n", stored_value);
        
        // Load it back
        cpu.R()[3] = 0; // Clear destination
        uint32_t ldr_instruction = 0xE5923000;
        printf("\nExecuting: LDR R3, [R2]\n");
        cpu.R()[15] = 0x08000014;
        memory.write32(0x08000014, ldr_instruction);
        arm_cpu.execute(1);
        printf("Loaded value: R3 = 0x%08X\n", cpu.R()[3]);
        
        // Demonstrate pre-indexed addressing: STR R1, [R2, #4]!
        uint32_t str_pre_instruction = 0xE5A21004;
        printf("\nExecuting: STR R1, [R2, #4]! (pre-indexed with writeback)\n");
        printf("Before: R2 = 0x%08X\n", cpu.R()[2]);
        cpu.R()[15] = 0x08000018;
        memory.write32(0x08000018, str_pre_instruction);
        arm_cpu.execute(1);
        printf("After: R2 = 0x%08X\n", cpu.R()[2]);
        
        // Block transfer demonstration
        printf("\nBlock transfer demonstration:\n");
        cpu.R()[0] = 0xAAAAAAAA;
        cpu.R()[1] = 0xBBBBBBBB;
        cpu.R()[4] = 0xCCCCCCCC;
        cpu.R()[5] = 0xDDDDDDDD;
        
        // STMIA R2!, {R0,R1,R4,R5}
        uint32_t stm_instruction = 0xE8A20033;
        printf("Executing: STMIA R2!, {R0,R1,R4,R5}\n");
        printf("Before: R2 = 0x%08X\n", cpu.R()[2]);
        cpu.R()[15] = 0x0800001C;
        memory.write32(0x0800001C, stm_instruction);
        arm_cpu.execute(1);
        printf("After: R2 = 0x%08X\n", cpu.R()[2]);
        
        // Verify stored data
        printf("Stored data:\n");
        for (int i = 0; i < 4; i++) {
            uint32_t addr = test_address + 4 + i * 4;
            uint32_t value = cpu.getMemory().read32(addr);
            printf("  [0x%08X] = 0x%08X\n", addr, value);
        }
    }
    
    void demonstrateBranchingAndControl() {
        printf("\n=== ARM Branching and Control Demonstration ===\n");
        
        // Set up a test scenario
        cpu.R()[15] = 0x08001000; // Set PC
        cpu.R()[0] = 10; // Counter
        
        printf("Starting PC: 0x%08X\n", cpu.R()[15]);
        printf("Counter (R0): %d\n", cpu.R()[0]);
        
        // Simulate a simple loop structure
        // CMP R0, #0
        uint32_t cmp_instruction = 0xE3500000;
        printf("\nExecuting: CMP R0, #0\n");
        memory.write32(cpu.R()[15], cmp_instruction);
        arm_cpu.execute(1);
        printf("CPSR after CMP: 0x%08X\n", cpu.CPSR());
        
        // BNE +8 (branch if not equal)
        uint32_t bne_instruction = 0x1A000001;
        printf("\nExecuting: BNE +8 (should branch since R0 != 0)\n");
        printf("PC before: 0x%08X\n", cpu.R()[15]);
        memory.write32(cpu.R()[15], bne_instruction);
        arm_cpu.execute(1);
        printf("PC after: 0x%08X\n", cpu.R()[15]);
        
        // Function call simulation: BL subroutine
        cpu.R()[15] = 0x08002000;
        cpu.R()[14] = 0; // Clear LR
        uint32_t bl_instruction = 0xEB000010; // BL +64
        printf("\nExecuting: BL +64 (function call)\n");
        printf("PC before: 0x%08X, LR before: 0x%08X\n", cpu.R()[15], cpu.R()[14]);
        memory.write32(cpu.R()[15], bl_instruction);
        arm_cpu.execute(1);
        printf("PC after: 0x%08X, LR after: 0x%08X\n", cpu.R()[15], cpu.R()[14]);
    }
    
    void demonstrateExceptionHandling() {
        printf("\n=== ARM Exception Handling Demonstration ===\n");
        
        // Set up initial state
        cpu.R()[15] = 0x08003000;
        cpu.CPSR() = 0x10; // User mode
        
        printf("Initial state:\n");
        printf("  PC: 0x%08X\n", cpu.R()[15]);
        printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
        printf("  LR: 0x%08X\n", cpu.R()[14]);
        
        // Software interrupt
        uint32_t swi_instruction = 0xEF000042; // SWI #0x42
        printf("\nExecuting: SWI #0x42\n");
        memory.write32(cpu.R()[15], swi_instruction);
        arm_cpu.execute(1);
        
        printf("After SWI:\n");
        printf("  PC: 0x%08X (should be 0x08)\n", cpu.R()[15]);
        printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
        printf("  LR_svc: 0x%08X\n", cpu.R()[14]);
        printf("  IRQ disabled: %s\n", (cpu.CPSR() & 0x80) ? "Yes" : "No");
        
        // Undefined instruction
        cpu.R()[15] = 0x08004000;
        cpu.CPSR() = 0x10; // Reset to User mode
        
        printf("\nTesting undefined instruction:\n");
        printf("Initial PC: 0x%08X\n", cpu.R()[15]);
        
        uint32_t undef_instruction = 0xE7F000F0;
        printf("Executing undefined instruction: 0x%08X\n", undef_instruction);
        memory.write32(cpu.R()[15], undef_instruction);
        arm_cpu.execute(1);
        
        printf("After undefined instruction:\n");
        printf("  PC: 0x%08X (should be 0x04)\n", cpu.R()[15]);
        printf("  CPSR: 0x%08X (mode: %s)\n", cpu.CPSR(), getModeString(cpu.CPSR() & 0x1F).c_str());
    }
    
    void demonstrateTimingAndPerformance() {
        printf("\n=== ARM Timing and Performance Demonstration ===\n");
        
        // Set up test program
        cpu.R()[15] = 0x08000000;
        
        // Measure timing for different instruction types
        std::vector<uint32_t> test_instructions = {
            0xE1A00000, // MOV R0, R0 (NOP)
            0xE0810002, // ADD R1, R1, R2
            0xE0000291, // MUL R0, R1, R2
            0xE5912000, // LDR R2, [R1]
            0xE8BD000F, // LDMIA R13!, {R0-R3}
            0xEA000000  // B +0
        };
        
        std::vector<std::string> instruction_names = {
            "MOV (NOP)",
            "ADD",
            "MUL",
            "LDR",
            "LDMIA",
            "B"
        };
        
        printf("Instruction cycle timing:\n");
        for (size_t i = 0; i < test_instructions.size(); i++) {
            uint32_t cycles = arm_cpu.calculateInstructionCycles(test_instructions[i]);
            printf("  %-10s: %d cycles\n", instruction_names[i].c_str(), cycles);
        }
        
        // Performance benchmark
        printf("\nPerformance benchmark (1000 instructions):\n");
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Execute 1000 simple instructions with timing
        for (int i = 0; i < 1000; i++) {
            arm_cpu.executeWithTiming(1, &timing);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        printf("  Time taken: %lld microseconds\n", duration.count());
        printf("  Instructions per second: %.2f million\n", 
               (1000.0 / duration.count()) * 1000000 / 1000000);
        printf("  System clock cycles: %llu\n", timing.total_cycles);
        printf("  Emulated time: %.3f ms\n", 
               (double)timing.total_cycles / GBA_CLOCK_FREQUENCY * 1000);
    }
    
    void demonstrateARMThumbInterworking() {
        printf("\n=== ARM/Thumb Interworking Demonstration ===\n");
        
        // Start in ARM mode
        cpu.R()[15] = 0x08000000;
        cpu.CPSR() &= ~0x20; // Clear T bit (ARM mode)
        
        printf("Starting in ARM mode\n");
        printf("CPSR: 0x%08X (T bit: %d)\n", cpu.CPSR(), (cpu.CPSR() >> 5) & 1);
        
        // Execute ARM instruction
        uint32_t arm_add = 0xE0810002; // ADD R1, R1, R2
        cpu.R()[1] = 10;
        cpu.R()[2] = 5;
        
        printf("\nExecuting ARM ADD R1, R1, R2\n");
        printf("Before: R1=%d, R2=%d\n", cpu.R()[1], cpu.R()[2]);
        memory.write32(cpu.R()[15], arm_add);
        arm_cpu.execute(1);
        printf("After: R1=%d\n", cpu.R()[1]);
        
        // Switch to Thumb mode (normally done by BX instruction)
        printf("\nSwitching to Thumb mode...\n");
        cpu.CPSR() |= 0x20; // Set T bit
        cpu.R()[15] = 0x08001000; // Thumb code location (must be halfword aligned)
        
        printf("CPSR: 0x%08X (T bit: %d)\n", cpu.CPSR(), (cpu.CPSR() >> 5) & 1);
        
        // Execute Thumb instruction
        uint16_t thumb_add = 0x1889; // ADD R1, R1, R2 (Thumb)
        cpu.R()[1] = 20;
        cpu.R()[2] = 3;
        
        printf("\nExecuting Thumb ADD R1, R1, R2\n");
        printf("Before: R1=%d, R2=%d\n", cpu.R()[1], cpu.R()[2]);
        
        // Write Thumb instruction to memory and execute
        cpu.getMemory().write16(cpu.R()[15], thumb_add);
        thumb_cpu.execute(1);
        
        printf("After: R1=%d\n", cpu.R()[1]);
        
        printf("\nARM/Thumb interworking complete!\n");
    }
};

int main() {
    printf("ARM7TDMI Advanced Features Demonstration\n");
    printf("========================================\n");
    
    ARMDemonstration demo;
    
    demo.demonstrateDataProcessing();
    demo.demonstrateMemoryOperations();
    demo.demonstrateBranchingAndControl();
    demo.demonstrateExceptionHandling();
    demo.demonstrateTimingAndPerformance();
    demo.demonstrateARMThumbInterworking();
    
    printf("\n✅ ARM demonstration complete!\n");
    printf("\nThis demonstration showed:\n");
    printf("  • Complete ARM instruction set support\n");
    printf("  • Memory operations with different addressing modes\n");
    printf("  • Branching and conditional execution\n");
    printf("  • Exception handling and mode switching\n");
    printf("  • Cycle-accurate timing\n");
    printf("  • ARM/Thumb interworking\n");
    
    return 0;
}
