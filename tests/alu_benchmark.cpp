#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "arm_instruction_cache.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>

// Focused ALU benchmark to test the optimization

int main(int argc, char* argv[]) {
    // Create GBA in test mode
    GBA gba(true);
    auto& cpu = gba.getCPU();
    auto& memory = cpu.getMemory();
    
    // Switch to ARM mode
    cpu.CPSR() &= ~CPU::FLAG_T;
    
    // Test each ALU operation that we optimized
    std::vector<std::pair<std::string, uint32_t>> aluTests = {
        {"ADD R1, R1, R2", 0xE0811002},  // ADD - opcode 0x4
        {"SUB R1, R1, R2", 0xE0411002},  // SUB - opcode 0x2  
        {"MOV R1, R2",     0xE1A01002},  // MOV - opcode 0xD
        {"ORR R1, R1, R2", 0xE1811002},  // ORR - opcode 0xC
        {"AND R1, R1, R2", 0xE0011002},  // AND - opcode 0x0
        {"CMP R1, R2",     0xE1510002},  // CMP - opcode 0xA
    };
    
    std::cout << "=== ALU Operation Focused Benchmark ===\n\n";
    
    for (const auto& test : aluTests) {
        // Create program with 1000 of this instruction
        std::vector<uint32_t> program(1000, test.second);
        
        // Load into memory
        for (size_t i = 0; i < program.size(); ++i) {
            memory.write32(i * 4, program[i]);
        }
        
        // Initialize registers
        auto& registers = cpu.R();
        registers.fill(0);
        registers[1] = 0x12345678;
        registers[2] = 0x1;
        registers[15] = 0; // PC
        
        // Warm up
        cpu.execute(10);
        registers[15] = 0;
        
        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();
        
        const int iterations = 10000;
        for (int i = 0; i < iterations; ++i) {
            cpu.execute(program.size());
            registers[15] = 0; // Reset PC
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double total_instructions = static_cast<double>(program.size()) * iterations;
        double ips = total_instructions / (duration.count() / 1e6);
        
        std::cout << std::left << std::setw(15) << test.first
                  << std::right << std::setw(15) << std::fixed << std::setprecision(0) << ips
                  << " IPS" << std::endl;
    }
    
    // Display cache statistics if command line argument --cache-stats is passed
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cache-stats") == 0) {
            auto& armCPU = cpu.getARMCPU();
            auto stats = armCPU.getInstructionCacheStats();
            
            std::cout << "\n=== ARM Instruction Cache Statistics ===\n";
#ifdef ARM_ICACHE_SIZE
            std::cout << "Cache size: " << ARM_ICACHE_SIZE << " entries\n";
#endif
            std::cout << "Cache hits: " << stats.hits << "\n";
            std::cout << "Cache misses: " << stats.misses << "\n";
            std::cout << "Cache invalidations: " << stats.invalidations << "\n";
            std::cout << "Cache hit rate: " << std::fixed << std::setprecision(2)
                      << stats.hit_rate * 100.0 << "%\n";
            break;
        }
    }
    
    return 0;
}
