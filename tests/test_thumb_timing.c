#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "thumb_timing.h"
#include "timing.h"

// Test cycle calculation for various Thumb instructions
void test_thumb_cycle_calculation() {
    printf("Testing Thumb instruction cycle calculation...\n");
    
    uint32_t registers[16] = {0}; // Dummy register state
    uint32_t pc = 0x08000000;     // Typical ROM address
    
    // Test shift immediate (Format 1)
    uint16_t lsl_imm = 0x0020; // LSL R0, R4, #0
    uint32_t cycles = thumb_calculate_instruction_cycles(lsl_imm, pc, registers);
    assert(cycles == THUMB_CYCLES_SHIFT_IMM);
    printf("  LSL immediate: %d cycles ✓\n", cycles);
    
    // Test MOV immediate (Format 3)
    uint16_t mov_imm = 0x2010; // MOV R0, #16
    cycles = thumb_calculate_instruction_cycles(mov_imm, pc, registers);
    assert(cycles == THUMB_CYCLES_MOV_IMM);
    printf("  MOV immediate: %d cycles ✓\n", cycles);
    
    // Test ALU operation (Format 4)
    uint16_t and_op = 0x4008; // AND R0, R1
    cycles = thumb_calculate_instruction_cycles(and_op, pc, registers);
    assert(cycles == THUMB_CYCLES_ALU);
    printf("  AND operation: %d cycles ✓\n", cycles);
    
    // Test multiply (Format 4)
    uint16_t mul_op = 0x4348; // MUL R0, R1
    registers[1] = 0x12345678; // Set up operand
    cycles = thumb_calculate_instruction_cycles(mul_op, pc, registers);
    assert(cycles >= THUMB_CYCLES_ALU + THUMB_CYCLES_MULTIPLY_MIN);
    printf("  MUL operation: %d cycles ✓\n", cycles);
    
    // Test PC-relative load (Format 6)
    uint16_t ldr_pc = 0x4810; // LDR R0, [PC, #64]
    cycles = thumb_calculate_instruction_cycles(ldr_pc, pc, registers);
    assert(cycles > THUMB_CYCLES_PC_REL_LOAD); // Should include memory access
    printf("  LDR PC-relative: %d cycles ✓\n", cycles);
    
    // Test push multiple (Format 14)
    uint16_t push_regs = 0xB4F0; // PUSH {R4-R7}
    cycles = thumb_calculate_instruction_cycles(push_regs, pc, registers);
    assert(cycles == THUMB_CYCLES_PUSH_POP_BASE + (4 * THUMB_CYCLES_TRANSFER_REG));
    printf("  PUSH multiple: %d cycles ✓\n", cycles);
    
    // Test unconditional branch (Format 18)
    uint16_t branch = 0xE010; // B #32
    cycles = thumb_calculate_instruction_cycles(branch, pc, registers);
    assert(cycles == THUMB_CYCLES_BRANCH_TAKEN);
    printf("  Branch: %d cycles ✓\n", cycles);
    
    printf("All Thumb cycle calculation tests passed!\n\n");
}

// Test timing system integration
void test_timing_integration() {
    printf("Testing timing system integration...\n");
    
    TimingState timing;
    timing_init(&timing);
    
    // Test basic timing advance
    uint32_t initial_cycles = timing.total_cycles;
    timing_advance(&timing, 100);
    assert(timing.total_cycles == initial_cycles + 100);
    printf("  Basic timing advance: ✓\n");
    
    // Test cycles until next event
    uint32_t cycles_to_event = timing_cycles_until_next_event(&timing);
    assert(cycles_to_event > 0);
    printf("  Cycles until next event: %d ✓\n", cycles_to_event);
    
    // Test video timing update
    timing_advance(&timing, 1000);
    timing_update_video(&timing);
    printf("  Video timing update: scanline %d, cycles %d ✓\n", 
           timing.current_scanline, timing.scanline_cycles);
    
    printf("Timing integration tests passed!\n\n");
}

// Test conditional branch detection
void test_conditional_branch() {
    printf("Testing conditional branch evaluation...\n");
    
    uint32_t cpsr;
    
    // Test BEQ (branch if equal)
    uint16_t beq = 0xD010; // BEQ #32
    
    // Z flag set (equal)
    cpsr = (1 << 30); // Z flag
    assert(thumb_is_branch_taken(beq, cpsr) == true);
    printf("  BEQ with Z=1: taken ✓\n");
    
    // Z flag clear (not equal)
    cpsr = 0;
    assert(thumb_is_branch_taken(beq, cpsr) == false);
    printf("  BEQ with Z=0: not taken ✓\n");
    
    // Test BCS (branch if carry set)
    uint16_t bcs = 0xD210; // BCS #32
    
    // C flag set
    cpsr = (1 << 29); // C flag
    assert(thumb_is_branch_taken(bcs, cpsr) == true);
    printf("  BCS with C=1: taken ✓\n");
    
    // C flag clear
    cpsr = 0;
    assert(thumb_is_branch_taken(bcs, cpsr) == false);
    printf("  BCS with C=0: not taken ✓\n");
    
    printf("Conditional branch tests passed!\n\n");
}

// Benchmark cycle calculation performance
void benchmark_cycle_calculation() {
    printf("Benchmarking cycle calculation performance...\n");
    
    uint32_t registers[16] = {0};
    uint32_t pc = 0x08000000;
    
    // Create a mix of different instruction types
    uint16_t instructions[] = {
        0x0020, // LSL
        0x2010, // MOV
        0x4008, // AND
        0x4810, // LDR PC-rel
        0xB4F0, // PUSH
        0xE010  // Branch
    };
    int num_instructions = sizeof(instructions) / sizeof(instructions[0]);
    
    const int iterations = 100000;
    printf("  Calculating cycles for %d instructions %d times...\n", 
           num_instructions, iterations);
    
    uint64_t total_cycles = 0;
    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < num_instructions; j++) {
            total_cycles += thumb_calculate_instruction_cycles(instructions[j], pc, registers);
        }
    }
    
    printf("  Total cycles calculated: %llu\n", total_cycles);
    printf("  Performance benchmark completed ✓\n\n");
}

int main() {
    printf("=== Thumb Timing System Tests ===\n\n");
    
    test_thumb_cycle_calculation();
    test_timing_integration();
    test_conditional_branch();
    benchmark_cycle_calculation();
    
    printf("=== All Tests Passed! ===\n");
    return 0;
}
