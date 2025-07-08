#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "thumb_timing.h"
#include "timing.h"

// Simulated instruction sequence for demonstration
uint16_t demo_program[] = {
    0x2010, // MOV R0, #16        - 1 cycle
    0x2120, // MOV R1, #32        - 1 cycle  
    0x1848, // ADD R0, R1, R0     - 1 cycle
    0x4348, // MUL R0, R1         - 4 cycles (depends on operand)
    0x4810, // LDR R0, [PC, #64]  - 6 cycles (with memory access)
    0xB4F0, // PUSH {R4-R7}       - 5 cycles (4 registers + base)
    0xD001, // BEQ +2             - 1 cycle (not taken) or 3 cycles (taken)
    0x2000, // MOV R0, #0         - 1 cycle
    0xE7FE, // B -4 (infinite loop) - 3 cycles
};

void simulate_cycle_driven_execution() {
    printf("=== Cycle-Driven Execution Simulation ===\n\n");
    
    TimingState timing;
    timing_init(&timing);
    
    uint32_t registers[16] = {0};
    uint32_t pc = 0x08000000;
    uint32_t cpsr = 0; // ARM mode, no flags set
    
    printf("Simulating instruction execution with timing events...\n");
    printf("Initial state: PC=0x%08X, Total cycles=%llu\n\n", pc, timing.total_cycles);
    
    // Simulate several instructions
    for (int i = 0; i < 9 && i < sizeof(demo_program)/sizeof(demo_program[0]); i++) {
        uint16_t instruction = demo_program[i];
        uint32_t instruction_cycles = thumb_calculate_instruction_cycles(instruction, pc, registers);
        
        printf("Instruction %d: 0x%04X at PC=0x%08X\n", i+1, instruction, pc);
        printf("  Predicted cycles: %d\n", instruction_cycles);
        
        // Check cycles until next timing event
        uint32_t cycles_until_event = timing_cycles_until_next_event(&timing);
        printf("  Cycles until next event: %d\n", cycles_until_event);
        
        if (instruction_cycles <= cycles_until_event) {
            printf("  → Executing instruction (completes before next event)\n");
            timing_advance(&timing, instruction_cycles);
            pc += 2;
        } else {
            printf("  → Processing timing event first\n");
            timing_advance(&timing, cycles_until_event);
            timing_process_timer_events(&timing);
            timing_process_video_events(&timing);
            printf("  → Event processed, instruction will execute next\n");
            // Don't increment i, will retry this instruction
            i--;
        }
        
        printf("  Current state: PC=0x%08X, Total cycles=%llu, Scanline=%d\n", 
               pc, timing.total_cycles, timing.current_scanline);
        printf("\n");
        
        // Update some register values for demonstration
        if (i == 0) registers[0] = 16;
        if (i == 1) registers[1] = 32;
        if (i == 2) registers[0] = 48;
        if (i == 3) registers[0] = 1536; // 48 * 32
        
        // Set Z flag for conditional branch demo
        if (i == 5) cpsr |= (1 << 30); // Set Z flag
        
        // Break if we hit the infinite loop
        if (instruction == 0xE7FE) {
            printf("Hit infinite loop, stopping simulation.\n");
            break;
        }
    }
    
    printf("Final state: Total cycles=%llu, Scanline=%d, Scanline cycles=%d\n", 
           timing.total_cycles, timing.current_scanline, timing.scanline_cycles);
}

void demonstrate_timer_integration() {
    printf("=== Timer Integration Example ===\n\n");
    
    TimingState timing;
    timing_init(&timing);
    
    printf("Simulating timer events during instruction execution...\n");
    printf("Timer frequency: %d Hz (every %d cycles)\n", 
           TIMER_FREQUENCY_HZ(1), 65536);
    
    // Simulate running for several timer periods
    uint32_t total_cycles_to_run = 200000; // About 3 timer overflows
    uint32_t cycles_run = 0;
    
    while (cycles_run < total_cycles_to_run) {
        // Simulate fetching an instruction
        uint16_t instruction = 0x2010; // MOV R0, #16
        uint32_t instruction_cycles = 1;
        
        uint32_t cycles_until_event = timing_cycles_until_next_event(&timing);
        
        if (instruction_cycles <= cycles_until_event) {
            // Execute instruction
            timing_advance(&timing, instruction_cycles);
            cycles_run += instruction_cycles;
        } else {
            // Process timing event
            timing_advance(&timing, cycles_until_event);
            cycles_run += cycles_until_event;
            
            // Check for timer overflow (simplified)
            if (timing.total_cycles % 65536 < cycles_until_event) {
                printf("Timer overflow at cycle %llu (scanline %d)\n", 
                       timing.total_cycles, timing.current_scanline);
            }
        }
        
        // Progress indicator
        if (cycles_run % 50000 == 0) {
            printf("Progress: %d/%d cycles, scanline %d\n", 
                   cycles_run, total_cycles_to_run, timing.current_scanline);
        }
    }
    
    printf("Simulation complete: %d cycles executed\n", cycles_run);
}

int main() {
    simulate_cycle_driven_execution();
    printf("\n");
    demonstrate_timer_integration();
    
    return 0;
}
