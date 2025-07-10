#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <iostream>
#include <iomanip>

// Simple test to debug cache behavior
int main() {
    // Create GBA in test mode
    GBA gba(true);
    auto& cpu = gba.getCPU();
    
    // Switch to ARM mode
    cpu.CPSR() &= ~CPU::FLAG_T;
    
    // Initialize registers
    auto& registers = cpu.R();
    registers.fill(0);
    registers[15] = 0; // PC
    registers[1] = 5;  // R1
    registers[2] = 10; // R2
    
    // Get ARM CPU reference
    auto& arm_cpu = cpu.getARMCPU();
    
    // Create a simple program: ADD R1, R1, R2 (0xE0811002)
    auto& memory = cpu.getMemory();
    memory.write32(0x0000, 0xE0811002); // ADD R1, R1, R2
    memory.write32(0x0004, 0xE0811002); // ADD R1, R1, R2
    memory.write32(0x0008, 0xE0811002); // ADD R1, R1, R2
    memory.write32(0x000C, 0xE0811002); // ADD R1, R1, R2
    
    std::cout << "=== ARM Cache Debug Test ===\n";
    std::cout << "Testing cache behavior with identical instructions at different addresses.\n\n";
    
    // Reset cache stats
    arm_cpu.resetInstructionCacheStats();
    
    // Execute each instruction individually and check cache stats
    for (int i = 0; i < 4; i++) {
        uint32_t pc = i * 4;
        cpu.R()[15] = pc;
        
        std::cout << "Before instruction " << i << " (PC=0x" << std::hex << pc << std::dec << "):\n";
        auto stats_before = arm_cpu.getInstructionCacheStats();
        std::cout << "  Hits: " << stats_before.hits << ", Misses: " << stats_before.misses << std::endl;
        
        // Execute one instruction
        cpu.execute(1);
        
        auto stats_after = arm_cpu.getInstructionCacheStats();
        std::cout << "After instruction " << i << " (R1=" << cpu.R()[1] << "):\n";
        std::cout << "  Hits: " << stats_after.hits << ", Misses: " << stats_after.misses << std::endl;
        std::cout << "  Hit rate: " << std::fixed << std::setprecision(2) << stats_after.hit_rate << "%\n\n";
    }
    
    std::cout << "=== Cache Analysis ===\n";
    std::cout << "The same instruction (0xE0811002) was executed at 4 different addresses.\n";
    std::cout << "Expected behavior: Each address should be a cache miss (different PC).\n";
    std::cout << "Cache uses PC as part of the key, so same instruction at different addresses won't hit.\n\n";
    
    // Now test with a loop that repeats the same PC
    std::cout << "=== Loop Test ===\n";
    std::cout << "Testing cache with repeated execution of the same PC.\n\n";
    
    // Reset everything
    registers.fill(0);
    registers[15] = 0;
    registers[1] = 1;
    registers[2] = 1;
    arm_cpu.resetInstructionCacheStats();
    
    // Execute the same instruction multiple times (same PC)
    for (int i = 0; i < 10; i++) {
        cpu.R()[15] = 0; // Always execute from PC = 0
        
        auto stats_before = arm_cpu.getInstructionCacheStats();
        cpu.execute(1);
        auto stats_after = arm_cpu.getInstructionCacheStats();
        
        std::cout << "Iteration " << i << ": Hits=" << stats_after.hits 
                  << ", Misses=" << stats_after.misses
                  << ", Hit Rate=" << std::fixed << std::setprecision(2) << stats_after.hit_rate << "%"
                  << " (R1=" << cpu.R()[1] << ")" << std::endl;
    }
    
    return 0;
}
