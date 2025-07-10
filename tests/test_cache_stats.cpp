#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <iostream>
#include <iomanip>

// Quick test to show ARM cache statistics during normal execution
int main() {
    std::cout << "=== ARM Cache Statistics Test ===" << std::endl;
    
    // Create GBA in test mode
    GBA gba(true);
    auto& cpu = gba.getCPU();
    
    // Switch to ARM mode
    cpu.CPSR() &= ~CPU::FLAG_T; // Clear Thumb bit
    
    // Initialize registers
    auto& registers = cpu.R();
    registers.fill(0);
    registers[15] = 0; // PC starts at 0
    
    // Create a simple ARM program with repeated instructions
    auto& memory = cpu.getMemory();
    
    // Write some ARM instructions
    memory.write32(0x0000, 0xE0811002); // ADD R1, R1, R2
    memory.write32(0x0004, 0xE0822003); // ADD R2, R2, R3  
    memory.write32(0x0008, 0xE0833001); // ADD R3, R3, R1
    memory.write32(0x000C, 0xEAFFFFFB); // B -20 (branch back to start)
    
    // Set up initial values
    registers[1] = 1;
    registers[2] = 2; 
    registers[3] = 3;
    
    std::cout << "\nExecuting ARM instructions to populate cache..." << std::endl;
    
    // Execute several cycles to build up cache hits
    for (int i = 0; i < 5; i++) {
        registers[15] = 0; // Reset PC
        cpu.execute(10); // Execute 10 instructions
        
        auto stats = cpu.getARMCPU().getInstructionCacheStats();
        std::cout << "Iteration " << (i+1) << ": "
                  << "Hits=" << stats.hits 
                  << ", Misses=" << stats.misses
                  << ", Hit Rate=" << std::fixed << std::setprecision(1) 
                  << (stats.hit_rate * 100) << "%" << std::endl;
    }
    
    // Final statistics
    auto final_stats = cpu.getARMCPU().getInstructionCacheStats();
    std::cout << "\n=== Final Cache Statistics ===" << std::endl;
    std::cout << "Total Hits: " << final_stats.hits << std::endl;
    std::cout << "Total Misses: " << final_stats.misses << std::endl;
    std::cout << "Total Invalidations: " << final_stats.invalidations << std::endl;
    std::cout << "Final Hit Rate: " << std::fixed << std::setprecision(2) 
              << (final_stats.hit_rate * 100) << "%" << std::endl;
    
    std::cout << "\n=== Cache Statistics Test Complete ===" << std::endl;
    
    return 0;
}
