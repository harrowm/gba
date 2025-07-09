#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

// Simple benchmark without Google Test to test optimized builds

int main() {
    // Note: Debug output automatically disabled in NDEBUG/BENCHMARK_MODE builds
    
    // Create GBA in test mode with minimal memory
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    
    // Switch to ARM mode
    cpu.CPSR() &= ~CPU::FLAG_T; // Clear Thumb bit to use ARM mode
    
    // Initialize all registers to 0
    auto& registers = cpu.R();
    registers.fill(0);
    
    // PC starts at address 0
    registers[15] = 0;
    
    // Create a program of ADD instructions
    // E0811002: ADD R1, R1, R2 (R1 = R1 + R2)
    std::vector<uint32_t> addProgram(100, 0xE0811002);
    
    // Load program into memory
    auto& memory = cpu.getMemory();
    for (size_t i = 0; i < addProgram.size(); ++i) {
        memory.write32(i * 4, addProgram[i]);
    }
    
    // Initialize operand registers
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    
    // Run the benchmark with different iteration counts
    std::vector<uint32_t> iterations = {1000, 10000, 100000};
    
    std::cout << "\n=== ARM Arithmetic Instruction Benchmark ===\n";
    std::cout << "Instruction: ADD R1, R1, R2 (R1 = R1 + R2)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    for (auto iter : iterations) {
        // Reset PC to beginning of program
        cpu.R()[15] = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute the program for the specified number of iterations
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(addProgram.size());
            cpu.R()[15] = 0; // Reset PC back to start of program after each iteration
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        // Calculate instructions per second
        double total_instructions = static_cast<double>(addProgram.size()) * iter;
        double ips = total_instructions / elapsed.count();
        
        std::cout << std::setw(12) << iter 
                  << std::setw(15) << addProgram.size() * iter
                  << std::setw(15) << std::fixed << std::setprecision(0) << ips << std::endl;
    }
    
    // Create an alternating program of LDR/STR instructions
    // E5801000: STR R1, [R0]    (store R1 to address in R0)
    // E5902000: LDR R2, [R0]    (load from address in R0 to R2)
    std::vector<uint32_t> memProgram;
    for (int i = 0; i < 50; i++) {
        memProgram.push_back(0xE5801000); // STR R1, [R0]
        memProgram.push_back(0xE5902000); // LDR R2, [R0]
    }
    
    // Load memory program into memory
    for (size_t i = 0; i < memProgram.size(); ++i) {
        memory.write32(i * 4, memProgram[i]);
    }
    
    // Initialize registers
    cpu.R()[0] = 0x100; // Memory address to use (within 0x0000-0x1FFF range)
    cpu.R()[1] = 0x12345678; // Value to store
    
    // Run the benchmark with different iteration counts
    iterations = {1000, 10000};
    
    std::cout << "\n=== ARM Memory Access Instruction Benchmark ===\n";
    std::cout << "Instructions: STR R1, [R0] / LDR R2, [R0] (alternating)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    for (auto iter : iterations) {
        // Reset PC to beginning of program
        cpu.R()[15] = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute the program for the specified number of iterations
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(memProgram.size());
            cpu.R()[15] = 0; // Reset PC back to start of program after each iteration
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        
        // Calculate instructions per second
        double total_instructions = static_cast<double>(memProgram.size()) * iter;
        double ips = total_instructions / elapsed.count();
        
        std::cout << std::setw(12) << iter 
                  << std::setw(15) << memProgram.size() * iter
                  << std::setw(15) << std::fixed << std::setprecision(0) << ips << std::endl;
    }
    
    return 0;
}
