#include <stdio.h>
#include <assert.h>
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
    printf("Testing ARM multiply instructions...\n");
    
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
    arm_cpu.decodeAndExecute(mul_instruction);
    
    assert(cpu.R()[0] == 35); // 5 * 7 = 35
    printf("✓ MUL instruction executed correctly\n");
    
    // Test MLA R3, R4, R5, R6  
    cpu.R()[4] = 3;  // Rm = 3
    cpu.R()[5] = 4;  // Rs = 4  
    cpu.R()[6] = 10; // Rn = 10
    cpu.R()[3] = 0;  // Clear destination
    
    // MLA R3, R4, R5, R6: R3 = R4 * R5 + R6 = 3 * 4 + 10 = 22
    // Format: cond 0000001S Rd   Rn   Rs   1001 Rm
    //         1110 0000001 0011 0110 0101 1001 0100
    uint32_t mla_instruction = 0xE0236594; // MLA R3, R4, R5, R6
    arm_cpu.decodeAndExecute(mla_instruction);
    
    assert(cpu.R()[3] == 22); // 3 * 4 + 10 = 22
    printf("✓ MLA instruction executed correctly\n");
}

void test_arm_data_processing() {
    printf("Testing ARM data processing instructions...\n");
    
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
    arm_cpu.decodeAndExecute(add_instruction);
    
    assert(cpu.R()[0] == 125); // 100 + 25 = 125
    printf("✓ ADD instruction executed correctly\n");
    
    // Test SUB with flags
    uint32_t sub_instruction = 0xE0510002; // SUBS R0, R1, R2
    arm_cpu.decodeAndExecute(sub_instruction);
    
    assert(cpu.R()[0] == 75); // 100 - 25 = 75
    printf("✓ SUB instruction executed correctly\n");
    
    // Test with immediate: MOV R3, #42
    uint32_t mov_imm_instruction = 0xE3A0302A; // MOV R3, #42
    arm_cpu.decodeAndExecute(mov_imm_instruction);
    
    assert(cpu.R()[3] == 42);
    printf("✓ MOV immediate instruction executed correctly\n");
}

void test_arm_conditional_execution() {
    printf("Testing ARM conditional execution...\n");
    
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
    arm_cpu.decodeAndExecute(moveq_instruction);
    
    assert(cpu.R()[0] == 42);
    printf("✓ MOVEQ instruction executed correctly (condition met)\n");
    
    // Test MOVNE R1, #99 (0x13A01063) - should not execute (Z flag set)
    cpu.R()[1] = 0; // Clear destination
    uint32_t movne_instruction = 0x13A01063; // MOVNE R1, #99
    arm_cpu.decodeAndExecute(movne_instruction);
    
    assert(cpu.R()[1] == 0); // Should remain unchanged
    printf("✓ MOVNE instruction skipped correctly (condition not met)\n");
    
    // Clear Z flag and test again
    cpu.CPSR() &= ~0x40000000; // Clear Z flag
    arm_cpu.decodeAndExecute(movne_instruction);
    
    assert(cpu.R()[1] == 99); // Should execute now
    printf("✓ MOVNE instruction executed correctly (condition met)\n");
}

void test_arm_timing_integration() {
    printf("Testing ARM timing integration...\n");
    
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
    assert(cycles >= 1); // Should take at least 1 cycle
    printf("✓ ARM instruction cycle calculation working: %d cycles\n", cycles);
    
    // Test different instruction types
    uint32_t mul_instruction = 0xE0000291; // MUL R0, R1, R2
    uint32_t mul_cycles = arm_cpu.calculateInstructionCycles(mul_instruction);
    printf("✓ MUL instruction cycles: %d\n", mul_cycles);
    
    uint32_t ldr_instruction = 0xE5910000; // LDR R0, [R1]
    uint32_t ldr_cycles = arm_cpu.calculateInstructionCycles(ldr_instruction);
    printf("✓ LDR instruction cycles: %d\n", ldr_cycles);
}

void test_arm_instruction_decoding() {
    printf("Testing ARM instruction format detection...\n");
    
    // Test different instruction formats using the format bits
    uint32_t data_proc = 0xE0810002; // ADD R0, R1, R2
    uint32_t format = ARM_GET_FORMAT(data_proc);
    assert(format == 0); // Data processing format 000
    printf("✓ Data processing format detected correctly (format %d)\n", format);
    
    uint32_t ldr = 0xE5910000; // LDR R0, [R1]
    format = ARM_GET_FORMAT(ldr);
    assert(format == 2); // Single data transfer format 010
    printf("✓ Single data transfer format detected correctly (format %d)\n", format);
    
    uint32_t branch = 0xEA000000; // B +0
    format = ARM_GET_FORMAT(branch);
    assert(format == 5); // Branch format 101
    printf("✓ Branch format detected correctly (format %d)\n", format);
    
    uint32_t ldm = 0xE8900003; // LDMIA R0, {R0,R1}
    format = ARM_GET_FORMAT(ldm);
    assert(format == 4); // Block transfer format 100
    printf("✓ Block transfer format detected correctly (format %d)\n", format);
}

int main() {
    printf("Running ARM CPU Advanced Tests (Simplified)\n");
    printf("==========================================\n\n");
    
    try {
        test_arm_instruction_decoding();
        printf("\n");
        
        test_arm_data_processing();
        printf("\n");
        
        test_arm_multiply();
        printf("\n");
        
        test_arm_conditional_execution();
        printf("\n");
        
        test_arm_timing_integration();
        printf("\n");
        
        printf("✅ All ARM CPU tests passed!\n");
        printf("\nFeatures tested:\n");
        printf("  • Instruction format detection\n");
        printf("  • Data processing operations\n");
        printf("  • Multiply operations\n");
        printf("  • Conditional execution\n");
        printf("  • Cycle timing calculation\n");
        
        return 0;
    } catch (const std::exception& e) {
        printf("❌ Test failed with exception: %s\n", e.what());
        return 1;
    } catch (...) {
        printf("❌ Test failed with unknown exception\n");
        return 1;
    }
}
