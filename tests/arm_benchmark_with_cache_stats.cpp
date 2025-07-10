#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

// ARM benchmark with cache statistics enabled
// This version shows cache performance metrics

int main() {
    // Create GBA in test mode with minimal memory
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    
    // Switch to ARM mode (clear Thumb bit)
    cpu.CPSR() &= ~CPU::FLAG_T; // Clear Thumb bit to use ARM mode
    
    // Initialize all registers to 0
    auto& registers = cpu.R();
    registers.fill(0);
    
    // PC starts at address 0 (must be word aligned for ARM)
    registers[15] = 0;
    
    // Get ARM CPU reference for cache stats
    auto& arm_cpu = cpu.getARMCPU();
    
    // Test 1: Simple loop to demonstrate cache effectiveness
    std::cout << "\n=== ARM Cache Performance Test ===\n";
    std::cout << "Testing instruction cache effectiveness with a simple loop.\n\n";
    
    // Create a simple loop program
    // Instructions:
    // 0x00000000: ADD R1, R1, R2    (0xE0811002)
    // 0x00000004: ADD R2, R2, R3    (0xE0822003)
    // 0x00000008: ADD R3, R3, R1    (0xE0833001)
    // 0x0000000C: B   0x00000000    (0xEAFFFFFB) - branch back to start
    
    std::vector<uint32_t> loopProgram = {
        0xE0811002,  // ADD R1, R1, R2
        0xE0822003,  // ADD R2, R2, R3  
        0xE0833001,  // ADD R3, R3, R1
        0xEAFFFFFB   // B #-20 (branch back to start)
    };
    
    // Load program into memory
    auto& memory = cpu.getMemory();
    for (size_t i = 0; i < loopProgram.size(); ++i) {
        memory.write32(i * 4, loopProgram[i]);
    }
    
    // Initialize operand registers
    cpu.R()[1] = 1;
    cpu.R()[2] = 2;
    cpu.R()[3] = 3;
    
    // Reset cache stats
    arm_cpu.resetInstructionCacheStats();
    
    // Execute the loop multiple times to demonstrate cache benefits
    std::vector<uint32_t> iterations = {10, 50, 100, 500, 1000};
    
    for (uint32_t iter : iterations) {
        // Reset PC to start of program
        cpu.R()[15] = 0;
        
        // Reset cache stats for this run
        arm_cpu.resetInstructionCacheStats();
        
        // Time the execution
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute the specified number of cycles
        cpu.execute(iter);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        // Get cache statistics
        auto stats = arm_cpu.getInstructionCacheStats();
        
        // Calculate performance metrics
        double seconds = duration.count() / 1e9;
        double instructions_per_second = iter / seconds;
        
        std::cout << "Cycles: " << std::setw(4) << iter
                  << " | IPS: " << std::setw(12) << std::fixed << std::setprecision(0) << instructions_per_second
                  << " | Hits: " << std::setw(3) << stats.hits
                  << " | Misses: " << std::setw(3) << stats.misses
                  << " | Hit Rate: " << std::setw(6) << std::fixed << std::setprecision(2) << stats.hit_rate << "%"
                  << " | Invalidations: " << stats.invalidations << std::endl;
    }
    
    std::cout << "\n=== ARM Arithmetic Benchmark (with Cache Stats) ===\n";
    std::cout << "Testing large-scale arithmetic performance with cache statistics.\n\n";
    
    // Create a program of ARM ADD instructions for arithmetic benchmark
    std::vector<uint32_t> addProgram(1000, 0xE0811002); // ADD R1, R1, R2
    
    // Load program into memory
    for (size_t i = 0; i < addProgram.size(); ++i) {
        memory.write32(i * 4, addProgram[i]);
    }
    
    // Initialize operand registers
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    cpu.R()[15] = 0; // Reset PC
    
    // Run arithmetic benchmark with cache stats
    std::vector<uint32_t> arith_iterations = {1000, 10000, 100000};
    
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::setw(10) << "Hits" << std::setw(10) << "Misses"
              << std::setw(12) << "Hit Rate" << std::setw(15) << "Invalidations" << std::endl;
    std::cout << std::string(95, '-') << std::endl;
    
    for (uint32_t iter : arith_iterations) {
        // Reset state for each test
        cpu.R()[1] = 0;
        cpu.R()[2] = 1;
        cpu.R()[15] = 0;
        arm_cpu.resetInstructionCacheStats();
        
        // Calculate the number of instructions that will be executed
        uint32_t instructions_executed = iter;
        
        // Time the execution
        auto start = std::chrono::high_resolution_clock::now();
        cpu.execute(iter);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double seconds = duration.count() / 1e9;
        double instructions_per_second = instructions_executed / seconds;
        
        // Get cache statistics
        auto stats = arm_cpu.getInstructionCacheStats();
        
        std::cout << std::setw(12) << iter
                  << std::setw(15) << instructions_executed
                  << std::setw(15) << std::fixed << std::setprecision(0) << instructions_per_second
                  << std::setw(10) << stats.hits
                  << std::setw(10) << stats.misses
                  << std::setw(11) << std::fixed << std::setprecision(2) << stats.hit_rate << "%"
                  << std::setw(15) << stats.invalidations << std::endl;
    }
    
    std::cout << "\n=== Cache Performance Analysis ===\n";
    std::cout << "Cache Statistics Interpretation:\n";
    std::cout << "• High hit rate (>90%) indicates effective instruction caching\n";
    std::cout << "• Low miss count suggests good cache utilization\n";
    std::cout << "• Zero invalidations indicate no self-modifying code\n";
    std::cout << "• Performance improvement should be visible with high hit rates\n\n";
    
    return 0;
}
