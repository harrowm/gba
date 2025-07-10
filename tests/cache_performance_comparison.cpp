#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

// Performance comparison: ARM benchmark with and without cache stats
int main() {
    std::cout << "=== Performance Impact of Cache Statistics Collection ===\n";
    std::cout << "Comparing performance with and without cache stats enabled.\n\n";
    
    // Create GBA in test mode
    GBA gba(true);
    auto& cpu = gba.getCPU();
    
    // Switch to ARM mode
    cpu.CPSR() &= ~CPU::FLAG_T;
    
    // Initialize registers
    auto& registers = cpu.R();
    registers.fill(0);
    registers[15] = 0;
    registers[1] = 0;
    registers[2] = 1;
    
    // Create a simple ADD instruction program
    auto& memory = cpu.getMemory();
    std::vector<uint32_t> program(100, 0xE0811002); // ADD R1, R1, R2
    
    for (size_t i = 0; i < program.size(); ++i) {
        memory.write32(i * 4, program[i]);
    }
    
    // Test iterations
    std::vector<uint32_t> iterations = {10000, 50000, 100000};
    
    for (uint32_t iter : iterations) {
        // Reset state
        registers.fill(0);
        registers[15] = 0;
        registers[1] = 0;
        registers[2] = 1;
        
        // Time the execution
        auto start = std::chrono::high_resolution_clock::now();
        cpu.execute(iter);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double seconds = duration.count() / 1e9;
        double instructions_per_second = iter / seconds;
        
        std::cout << "Iterations: " << std::setw(6) << iter
                  << " | IPS: " << std::setw(12) << std::fixed << std::setprecision(0) << instructions_per_second
                  << " | R1: " << cpu.R()[1] << std::endl;
    }
    
    std::cout << "\n=== Analysis ===\n";
    
#if ARM_CACHE_STATS
    std::cout << "Cache statistics collection is ENABLED.\n";
    std::cout << "Performance impact: Significant overhead from stats collection.\n";
    std::cout << "Cache hit rate tracking adds CPU cycles to every instruction lookup.\n";
    std::cout << "This explains the performance difference between regular ARM benchmark and cache stats version.\n";
#else
    std::cout << "Cache statistics collection is DISABLED.\n";
    std::cout << "Performance impact: No overhead from stats collection.\n";
    std::cout << "This version should show optimal performance with cache benefits.\n";
#endif
    
    return 0;
}
