#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

// Include headers
extern "C" {
#include "timing.h"
#include "arm_timing.h"
}

#include "cpu.h"
#include "arm_cpu.h"
#include "debug.h"

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
    
    CPU cpu;
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
    
    // Test MLA R3, R4, R5, R6  (0xE0234594)
    cpu.R()[4] = 3;
    cpu.R()[5] = 4;
    cpu.R()[6] = 10;
    cpu.R()[3] = 0; // Clear destination
    
    uint32_t mla_instruction = 0xE0234594; // MLA R3, R4, R5, R6
    arm_cpu.decodeAndExecute(mla_instruction);
    
    assert(cpu.R()[3] == 22); // 3 * 4 + 10 = 22
    printf("✓ MLA instruction executed correctly\n");
}

void test_arm_branch() {
    printf("Testing ARM branch instructions...\n");
    
    CPU cpu;
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Test B +8  (0xEA000001)
    cpu.R()[15] = 0x8000; // Set PC
    uint32_t branch_instruction = 0xEA000001; // B +8 (branch forward by 2 words)
    
    arm_cpu.decodeAndExecute(branch_instruction);
    
    assert(cpu.R()[15] == 0x8010); // PC should be 0x8000 + 8 + 8 = 0x8010
    printf("✓ B instruction executed correctly\n");
    
    // Test BL -4  (0xEBFFFFFF)
    cpu.R()[15] = 0x8000; // Reset PC
    cpu.R()[14] = 0; // Clear LR
    uint32_t bl_instruction = 0xEBFFFFFF; // BL -4
    
    arm_cpu.decodeAndExecute(bl_instruction);
    
    assert(cpu.R()[14] == 0x8004); // LR should be PC + 4
    assert(cpu.R()[15] == 0x8004); // PC should be 0x8000 + (-4) + 8 = 0x8004
    printf("✓ BL instruction executed correctly\n");
}

void test_arm_memory_transfer() {
    printf("Testing ARM memory transfer instructions...\n");
    
    CPU cpu;
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Test STR R1, [R2] (0xE5821000)
    uint32_t test_address = 0x02000000; // EWRAM
    cpu.R()[1] = 0x12345678;
    cpu.R()[2] = test_address;
    
    uint32_t str_instruction = 0xE5821000; // STR R1, [R2]
    arm_cpu.decodeAndExecute(str_instruction);
    
    uint32_t stored_value = cpu.getMemory().read32(test_address);
    assert(stored_value == 0x12345678);
    printf("✓ STR instruction executed correctly\n");
    
    // Test LDR R3, [R2] (0xE5923000)
    cpu.R()[3] = 0; // Clear destination
    uint32_t ldr_instruction = 0xE5923000; // LDR R3, [R2]
    arm_cpu.decodeAndExecute(ldr_instruction);
    
    assert(cpu.R()[3] == 0x12345678);
    printf("✓ LDR instruction executed correctly\n");
    
    // Test STRB R1, [R2, #4] (0xE5C21004)
    cpu.R()[1] = 0xAB;
    uint32_t strb_instruction = 0xE5C21004; // STRB R1, [R2, #4]
    arm_cpu.decodeAndExecute(strb_instruction);
    
    uint8_t stored_byte = cpu.getMemory().read8(test_address + 4);
    assert(stored_byte == 0xAB);
    printf("✓ STRB instruction executed correctly\n");
    
    // Test LDRB R4, [R2, #4] (0xE5D24004)
    cpu.R()[4] = 0; // Clear destination
    uint32_t ldrb_instruction = 0xE5D24004; // LDRB R4, [R2, #4]
    arm_cpu.decodeAndExecute(ldrb_instruction);
    
    assert(cpu.R()[4] == 0xAB);
    printf("✓ LDRB instruction executed correctly\n");
}

void test_arm_block_transfer() {
    printf("Testing ARM block transfer instructions...\n");
    
    CPU cpu;
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    uint32_t test_address = 0x02000100; // EWRAM
    cpu.R()[13] = test_address; // Set stack pointer
    
    // Set up test data in registers
    cpu.R()[0] = 0x11111111;
    cpu.R()[1] = 0x22222222;
    cpu.R()[2] = 0x33333333;
    cpu.R()[3] = 0x44444444;
    
    // Test STMIA R13!, {R0-R3} (0xE8AD000F)
    uint32_t stm_instruction = 0xE8AD000F; // STMIA R13!, {R0-R3}
    arm_cpu.decodeAndExecute(stm_instruction);
    
    // Check stored values
    assert(cpu.getMemory().read32(test_address) == 0x11111111);
    assert(cpu.getMemory().read32(test_address + 4) == 0x22222222);
    assert(cpu.getMemory().read32(test_address + 8) == 0x33333333);
    assert(cpu.getMemory().read32(test_address + 12) == 0x44444444);
    assert(cpu.R()[13] == test_address + 16); // SP should be updated
    printf("✓ STMIA instruction executed correctly\n");
    
    // Clear registers and test LDMDB
    cpu.R()[0] = 0;
    cpu.R()[1] = 0;
    cpu.R()[2] = 0;
    cpu.R()[3] = 0;
    
    // Test LDMDB R13!, {R0-R3} (0xE913000F)
    uint32_t ldm_instruction = 0xE913000F; // LDMDB R13!, {R0-R3}
    arm_cpu.decodeAndExecute(ldm_instruction);
    
    // Check loaded values
    assert(cpu.R()[0] == 0x11111111);
    assert(cpu.R()[1] == 0x22222222);
    assert(cpu.R()[2] == 0x33333333);
    assert(cpu.R()[3] == 0x44444444);
    assert(cpu.R()[13] == test_address); // SP should be back to original
    printf("✓ LDMDB instruction executed correctly\n");
}

void test_arm_psr_transfer() {
    printf("Testing ARM PSR transfer instructions...\n");
    
    CPU cpu;
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Test MRS R0, CPSR (0xE10F0000)
    cpu.CPSR() = 0xF0000010; // User mode with NZCV flags set
    cpu.R()[0] = 0; // Clear destination
    
    uint32_t mrs_instruction = 0xE10F0000; // MRS R0, CPSR
    arm_cpu.decodeAndExecute(mrs_instruction);
    
    assert(cpu.R()[0] == 0xF0000010);
    printf("✓ MRS CPSR instruction executed correctly\n");
    
    // Test MSR CPSR_f, #0x20000000 (0xE328F002)
    uint32_t msr_instruction = 0xE328F002; // MSR CPSR_f, #0x20000000 (set C flag)
    arm_cpu.decodeAndExecute(msr_instruction);
    
    // Check that only the flags field was updated
    assert((cpu.CPSR() & 0xF0000000) == 0x20000000); // C flag set
    assert((cpu.CPSR() & 0x0FFFFFFF) == 0x00000010); // Other bits unchanged
    printf("✓ MSR CPSR_f instruction executed correctly\n");
}

void test_arm_conditional_execution() {
    printf("Testing ARM conditional execution...\n");
    
    CPU cpu;
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

void test_arm_shift_operations() {
    printf("Testing ARM shift operations...\n");
    
    CPU cpu;
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Test MOV R0, R1, LSL #2 (0xE1A00101)
    cpu.R()[1] = 0x12345678;
    cpu.R()[0] = 0; // Clear destination
    
    uint32_t mov_lsl_instruction = 0xE1A00101; // MOV R0, R1, LSL #2
    arm_cpu.decodeAndExecute(mov_lsl_instruction);
    
    assert(cpu.R()[0] == (0x12345678 << 2));
    printf("✓ MOV with LSL shift executed correctly\n");
    
    // Test MOV R2, R1, LSR #4 (0xE1A02221)
    cpu.R()[2] = 0; // Clear destination
    uint32_t mov_lsr_instruction = 0xE1A02221; // MOV R2, R1, LSR #4
    arm_cpu.decodeAndExecute(mov_lsr_instruction);
    
    assert(cpu.R()[2] == (0x12345678 >> 4));
    printf("✓ MOV with LSR shift executed correctly\n");
    
    // Test MOV R3, R1, ASR #8 (0xE1A03441)
    cpu.R()[1] = 0x80000000; // Negative number
    cpu.R()[3] = 0; // Clear destination
    uint32_t mov_asr_instruction = 0xE1A03441; // MOV R3, R1, ASR #8
    arm_cpu.decodeAndExecute(mov_asr_instruction);
    
    assert(cpu.R()[3] == (int32_t)0xFF800000); // Sign extended
    printf("✓ MOV with ASR shift executed correctly\n");
    
    // Test MOV R4, R1, ROR #8 (0xE1A04461)
    cpu.R()[1] = 0x12345678;
    cpu.R()[4] = 0; // Clear destination
    uint32_t mov_ror_instruction = 0xE1A04461; // MOV R4, R1, ROR #8
    arm_cpu.decodeAndExecute(mov_ror_instruction);
    
    uint32_t expected = (0x12345678 >> 8) | (0x12345678 << 24);
    assert(cpu.R()[4] == expected);
    printf("✓ MOV with ROR shift executed correctly\n");
}

void test_arm_exception_handling() {
    printf("Testing ARM exception handling...\n");
    
    CPU cpu;
    ARMCPU arm_cpu(cpu);
    setup_test_cpu(cpu);
    
    // Test SWI #0x123456 (0xEF123456)
    cpu.R()[15] = 0x8000; // Set PC
    cpu.CPSR() = 0x10; // User mode
    uint32_t old_lr = cpu.R()[14];
    
    uint32_t swi_instruction = 0xEF123456; // SWI #0x123456
    arm_cpu.decodeAndExecute(swi_instruction);
    
    // Check mode switch to SVC
    assert((cpu.CPSR() & 0x1F) == 0x13); // SVC mode
    assert((cpu.CPSR() & 0x80) != 0); // IRQ disabled
    assert(cpu.R()[14] == 0x8004); // LR_svc = PC + 4
    assert(cpu.R()[15] == 0x08); // PC = SWI vector
    printf("✓ SWI instruction executed correctly\n");
    
    // Test undefined instruction (0xE7F000F0)
    cpu.R()[15] = 0x9000; // Reset PC
    cpu.CPSR() = 0x10; // Reset to User mode
    
    uint32_t undef_instruction = 0xE7F000F0; // Undefined instruction pattern
    arm_cpu.decodeAndExecute(undef_instruction);
    
    // Check mode switch to Undefined
    assert((cpu.CPSR() & 0x1F) == 0x1B); // Undefined mode
    assert(cpu.R()[15] == 0x04); // PC = Undefined vector
    printf("✓ Undefined instruction handled correctly\n");
}

void test_arm_timing_integration() {
    printf("Testing ARM timing integration...\n");
    
    CPU cpu;
    ARMCPU arm_cpu(cpu);
    TimingState timing;
    
    timing_init(&timing);
    setup_test_cpu(cpu);
    
    // Test cycle-accurate execution
    cpu.R()[15] = 0x8000;
    
    // Execute a few instructions with timing
    arm_cpu.executeWithTiming(10, &timing);
    
    // Check that timing state was updated
    assert(timing.system_clock > 0);
    printf("✓ ARM timing integration working correctly\n");
    
    // Test instruction cycle calculation
    uint32_t add_instruction = 0xE0810002; // ADD R1, R1, R2
    uint32_t cycles = arm_cpu.calculateInstructionCycles(add_instruction);
    assert(cycles >= 1); // Should take at least 1 cycle
    printf("✓ ARM instruction cycle calculation working\n");
}

int main() {
    printf("Running comprehensive ARM CPU tests...\n\n");
    
    try {
        test_arm_multiply();
        printf("\n");
        
        test_arm_branch();
        printf("\n");
        
        test_arm_memory_transfer();
        printf("\n");
        
        test_arm_block_transfer();
        printf("\n");
        
        test_arm_psr_transfer();
        printf("\n");
        
        test_arm_conditional_execution();
        printf("\n");
        
        test_arm_shift_operations();
        printf("\n");
        
        test_arm_exception_handling();
        printf("\n");
        
        test_arm_timing_integration();
        printf("\n");
        
        printf("✅ All ARM CPU tests passed!\n");
        return 0;
    } catch (const std::exception& e) {
        printf("❌ Test failed with exception: %s\n", e.what());
        return 1;
    } catch (...) {
        printf("❌ Test failed with unknown exception\n");
        return 1;
    }
}
