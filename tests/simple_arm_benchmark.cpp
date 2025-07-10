#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

// Simple ARM benchmark without Google Test to test optimized builds

int main() {
    // Note: Debug output automatically disabled in NDEBUG/BENCHMARK_MODE builds
    
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
    
    // Create a program of ARM ADD instructions
    // 0xE0811002: ADD R1, R1, R2 (R1 = R1 + R2) - ARM format
    // Condition: AL (1110), Opcode: ADD (0100), S=0, Rn=R1, Rd=R1, Operand2=R2
    std::vector<uint32_t> addProgram(200, 0xE0811002);
    
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
    
    for (uint32_t iter : iterations) {
        // Reset CPU state
        cpu.R()[1] = 0;
        cpu.R()[2] = 1;
        cpu.R()[15] = 0;
        
        uint32_t totalInstructions = iter * 100; // 100 instructions per iteration
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute instructions
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(100); // Execute 100 ARM instructions
            
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
    
    // Test different ARM instruction types
    std::cout << "\n=== ARM Memory Access Instruction Benchmark ===\n";
    std::cout << "Instructions: STR R1, [R0] / LDR R2, [R0] (alternating)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create alternating STR/LDR program
    std::vector<uint32_t> memProgram;
    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0) {
            memProgram.push_back(0xE5801000); // STR R1, [R0] - Single Data Transfer
        } else {
            memProgram.push_back(0xE5902000); // LDR R2, [R0] - Single Data Transfer
        }
    }
    
    // Load memory access program
    for (size_t i = 0; i < memProgram.size(); ++i) {
        memory.write32(i * 4, memProgram[i]);
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
    
    // Test ARM ALU operations
    std::cout << "\n=== ARM ALU Operation Benchmark ===\n";
    std::cout << "Instructions: AND R1, R1, R2 / EOR R1, R1, R2 / LSL R1, R1, R2 (cycling)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create ALU operations program
    std::vector<uint32_t> aluProgram;
    for (int i = 0; i < 100; ++i) {
        switch (i % 3) {
            case 0: aluProgram.push_back(0xE0011002); break; // AND R1, R1, R2 - Data Processing
            case 1: aluProgram.push_back(0xE0211002); break; // EOR R1, R1, R2 - Data Processing
            case 2: aluProgram.push_back(0xE1A01112); break; // LSL R1, R2, #2 - Data Processing (MOV with shift)
        }
    }
    
    // Load ALU program
    for (size_t i = 0; i < aluProgram.size(); ++i) {
        memory.write32(i * 4, aluProgram[i]);
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
    
    // Test ARM branch instructions
    std::cout << "\n=== ARM Branch Instruction Benchmark ===\n";
    std::cout << "Instructions: B #8 (forward branch)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create branch program
    std::vector<uint32_t> branchProgram;
    for (int i = 0; i < 50; ++i) {
        branchProgram.push_back(0xEA000001); // B #8 - branch forward by 2 instructions (8 bytes)
        branchProgram.push_back(0xE1A00000); // NOP (MOV R0, R0) - filler instruction
    }
    
    // Load branch program
    for (size_t i = 0; i < branchProgram.size(); ++i) {
        memory.write32(i * 4, branchProgram[i]);
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
    
    // Test ARM multiple data transfer (LDM/STM)
    std::cout << "\n=== ARM Multiple Data Transfer Benchmark ===\n";
    std::cout << "Instructions: LDMIA R0!, {R1-R4} / STMIA R0!, {R1-R4} (alternating)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create multiple data transfer program
    std::vector<uint32_t> mdtProgram;
    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0) {
            mdtProgram.push_back(0xE8A0001E); // STMIA R0!, {R1-R4} - Store Multiple
        } else {
            mdtProgram.push_back(0xE8B0001E); // LDMIA R0!, {R1-R4} - Load Multiple
        }
    }
    
    // Load multiple data transfer program
    for (size_t i = 0; i < mdtProgram.size(); ++i) {
        memory.write32(i * 4, mdtProgram[i]);
    }
    
    for (uint32_t iter : iterations) {
        // Reset CPU state
        cpu.R()[0] = 0x02000000; // EWRAM address
        cpu.R()[1] = 0x11111111; // Test values
        cpu.R()[2] = 0x22222222;
        cpu.R()[3] = 0x33333333;
        cpu.R()[4] = 0x44444444;
        cpu.R()[15] = 0;
        
        uint32_t totalInstructions = iter * 100;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute instructions
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(100);
            
            // Reset PC and R0 to loop the program
            cpu.R()[0] = 0x02000000;
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
    
    // Test ARM multiply instructions
    std::cout << "\n=== ARM Multiply Instruction Benchmark ===\n";
    std::cout << "Instructions: MUL R1, R2, R3 / MLA R1, R2, R3, R4 (alternating)\n\n";
    std::cout << std::setw(12) << "Iterations" << std::setw(15) << "Instructions" 
              << std::setw(15) << "IPS" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    
    // Create multiply program
    std::vector<uint32_t> mulProgram;
    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0) {
            mulProgram.push_back(0xE0010392); // MUL R1, R2, R3 - Multiply
        } else {
            mulProgram.push_back(0xE0214392); // MLA R1, R2, R3, R4 - Multiply and Accumulate
        }
    }
    
    // Load multiply program
    for (size_t i = 0; i < mulProgram.size(); ++i) {
        memory.write32(i * 4, mulProgram[i]);
    }
    
    for (uint32_t iter : iterations) {
        // Reset CPU state
        cpu.R()[1] = 0;
        cpu.R()[2] = 123;
        cpu.R()[3] = 456;
        cpu.R()[4] = 1000;
        cpu.R()[15] = 0;
        
        uint32_t totalInstructions = iter * 100;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Execute instructions
        for (uint32_t i = 0; i < iter; ++i) {
            cpu.execute(100);
            
            // Reset PC and registers to maintain consistent state
            cpu.R()[1] = 0;
            cpu.R()[2] = 123;
            cpu.R()[3] = 456;
            cpu.R()[4] = 1000;
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
    
    std::cout << "\n=== ARM Benchmark Complete ===\n";
    std::cout << "This benchmark tested:\n";
    std::cout << "  • ARM arithmetic instructions (ADD)\n";
    std::cout << "  • ARM memory access instructions (STR/LDR)\n";
    std::cout << "  • ARM ALU operations (AND/EOR/LSL)\n";
    std::cout << "  • ARM branch instructions (B)\n";
    std::cout << "  • ARM multiple data transfer (LDM/STM)\n";
    std::cout << "  • ARM multiply instructions (MUL/MLA)\n";
    std::cout << "\nCompare with Thumb benchmark results to evaluate relative performance.\n";
    std::cout << "The ARM instruction cache should improve performance significantly.\n";
    
    return 0;
}
