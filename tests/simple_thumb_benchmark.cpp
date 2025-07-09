#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

// Simple Thumb benchmark without Google Test to test optimized builds

int main() {
    // Note: Debug output automatically disabled in NDEBUG/BENCHMARK_MODE builds
    
    // Create GBA in test mode with minimal memory
    GBA gba(true); // Test mode
    auto& cpu = gba.getCPU();
    
    // Switch to Thumb mode
    cpu.CPSR() |= CPU::FLAG_T; // Set Thumb bit to use Thumb mode
    
    // Initialize all registers to 0
    auto& registers = cpu.R();
    registers.fill(0);
    
    // PC starts at address 0 (must be halfword aligned for Thumb)
    registers[15] = 0;
    
    // Create a program of Thumb ADD instructions
    // 1889: ADD R1, R1, R2 (R1 = R1 + R2) - Thumb format
    std::vector<uint16_t> addProgram(200, 0x1889);
    
    // Load program into memory
    auto& memory = cpu.getMemory();
    for (size_t i = 0; i < addProgram.size(); ++i) {
        memory.write16(i * 2, addProgram[i]);
    }
    
    // Initialize operand registers
    cpu.R()[1] = 0;
    cpu.R()[2] = 1;
    
    // Run the benchmark with different iteration counts
    std::vector<uint32_t> iterations = {1000, 10000, 100000};
    
    std::cout << "\n=== Thumb Arithmetic Instruction Benchmark ===\n";
    std::cout << "Instruction: ADD R1, R1, R2 (R1 = R1 + R2)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    for (uint32_t iter : iterations) {
        // Reset CPU state
        cpu.R()[1] = 0;
        cpu.R()[2] = 1;
        cpu.R()[15] = 0;
        
        uint32_t totalInstructions = iter * 100; // 100 instructions per iteration
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute instructions
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(100); // Execute 100 Thumb instructions
            
            // Reset PC to loop the program
            cpu.R()[15] = 0;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Calculate instructions per second
        double seconds = duration.count() / 1000000.0;
        uint64_t ips = static_cast<uint64_t>(totalInstructions / seconds);
        
        std::cout << std::setw(12) << iter << std::setw(15) << totalInstructions 
                  << std::setw(15) << ips << std::endl;
    }
    
    // Test different Thumb instruction types
    std::cout << "\n=== Thumb Memory Access Instruction Benchmark ===\n";
    std::cout << "Instructions: STR R1, [R0] / LDR R2, [R0] (alternating)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create alternating STR/LDR program
    std::vector<uint16_t> memProgram;
    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0) {
            memProgram.push_back(0x6001); // STR R1, [R0] - Format 9 immediate offset
        } else {
            memProgram.push_back(0x6802); // LDR R2, [R0] - Format 9 immediate offset
        }
    }
    
    // Load memory access program
    for (size_t i = 0; i < memProgram.size(); ++i) {
        memory.write16(i * 2, memProgram[i]);
    }
    
    for (uint32_t iter : iterations) {
        // Reset CPU state
        cpu.R()[0] = 0x02000000; // EWRAM address
        cpu.R()[1] = 0x12345678; // Test value
        cpu.R()[2] = 0;
        cpu.R()[15] = 0;
        
        uint32_t totalInstructions = iter * 100;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute instructions
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(100);
            
            // Reset PC to loop the program
            cpu.R()[15] = 0;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Calculate instructions per second
        double seconds = duration.count() / 1000000.0;
        uint64_t ips = static_cast<uint64_t>(totalInstructions / seconds);
        
        std::cout << std::setw(12) << iter << std::setw(15) << totalInstructions 
                  << std::setw(15) << ips << std::endl;
    }
    
    // Test Thumb ALU operations
    std::cout << "\n=== Thumb ALU Operation Benchmark ===\n";
    std::cout << "Instructions: AND R1, R2 / EOR R1, R2 / LSL R1, R2 (cycling)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create ALU operations program
    std::vector<uint16_t> aluProgram;
    for (int i = 0; i < 100; ++i) {
        switch (i % 3) {
            case 0: aluProgram.push_back(0x4051); break; // AND R1, R2 - Format 4 ALU op (opcode 0)
            case 1: aluProgram.push_back(0x4051 | (1 << 6)); break; // EOR R1, R2 (opcode 1)
            case 2: aluProgram.push_back(0x4051 | (2 << 6)); break; // LSL R1, R2 (opcode 2)
        }
    }
    
    // Load ALU program
    for (size_t i = 0; i < aluProgram.size(); ++i) {
        memory.write16(i * 2, aluProgram[i]);
    }
    
    for (uint32_t iter : iterations) {
        // Reset CPU state
        cpu.R()[1] = 0xFFFFFFFF;
        cpu.R()[2] = 0x12345678;
        cpu.R()[15] = 0;
        
        uint32_t totalInstructions = iter * 100;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute instructions
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(100);
            
            // Reset PC and registers to maintain consistent state
            cpu.R()[1] = 0xFFFFFFFF;
            cpu.R()[2] = 0x12345678;
            cpu.R()[15] = 0;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Calculate instructions per second
        double seconds = duration.count() / 1000000.0;
        uint64_t ips = static_cast<uint64_t>(totalInstructions / seconds);
        
        std::cout << std::setw(12) << iter << std::setw(15) << totalInstructions 
                  << std::setw(15) << ips << std::endl;
    }
    
    // Test Thumb branch instructions
    std::cout << "\n=== Thumb Branch Instruction Benchmark ===\n";
    std::cout << "Instructions: B #2 (short forward branch)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create branch program (Format 18: unconditional branch)
    std::vector<uint16_t> branchProgram;
    for (int i = 0; i < 50; ++i) {
        branchProgram.push_back(0xE001); // B #2 - branch forward by 2 halfwords
        branchProgram.push_back(0x46C0); // NOP (MOV R8, R8) - filler instruction
    }
    
    // Load branch program
    for (size_t i = 0; i < branchProgram.size(); ++i) {
        memory.write16(i * 2, branchProgram[i]);
    }
    
    for (uint32_t iter : iterations) {
        // Reset CPU state
        cpu.R()[15] = 0;
        
        uint32_t totalInstructions = iter * 100;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute instructions
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(100);
            
            // Reset PC to loop the program
            cpu.R()[15] = 0;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Calculate instructions per second
        double seconds = duration.count() / 1000000.0;
        uint64_t ips = static_cast<uint64_t>(totalInstructions / seconds);
        
        std::cout << std::setw(12) << iter << std::setw(15) << totalInstructions 
                  << std::setw(15) << ips << std::endl;
    }
    
    std::cout << "\n=== Thumb Benchmark Complete ===\n";
    std::cout << "This benchmark tested:\n";
    std::cout << "  • Thumb arithmetic instructions (ADD)\n";
    std::cout << "  • Thumb memory access instructions (STR/LDR)\n";
    std::cout << "  • Thumb ALU operations (AND/EOR/LSL)\n";
    std::cout << "  • Thumb branch instructions (B)\n";
    std::cout << "\nCompare with ARM benchmark results to evaluate relative performance.\n";
    
    return 0;
}
