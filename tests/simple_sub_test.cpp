#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "debug.h"
#include <iostream>
#include <iomanip>
#include <vector>

// Simple SUB instruction test
int main() {
    std::cout << "=== Simple SUB Instruction Test ===\n";
    
    try {
        // Create GBA instance
        GBA gba(false);
        auto& cpu = gba.getCPU();
        auto& memory = gba.getMemory();
        
        // Load simple test ROM
        std::cout << "Loading simple test ROM...\n";
        std::ifstream testRom("assets/roms/simple_test.bin", std::ios::binary);
        if (!testRom.is_open()) {
            std::cerr << "Error: Could not open simple_test.bin\n";
            return 1;
        }
        
        testRom.seekg(0, std::ios::end);
        size_t romSize = testRom.tellg();
        testRom.seekg(0, std::ios::beg);
        std::vector<uint8_t> romData(romSize);
        testRom.read(reinterpret_cast<char*>(romData.data()), romSize);
        testRom.close();
        
        // Install ROM data
        for (size_t i = 0; i < romSize && i < 256; ++i) {
            int mappedAddr = memory.mapAddress(0x08000000 + i, false);
            if (mappedAddr >= 0) {
                memory.getRawData()[mappedAddr] = romData[i];
            }
        }
        
        // Verify ROM instructions
        std::cout << "ROM Instructions:\n";
        for (int i = 0; i < 5; i++) {
            uint32_t addr = 0x08000000 + i * 4;
            uint32_t instr = memory.read32(addr);
            std::cout << "  0x" << std::hex << addr << ": 0x" << instr << std::dec << std::endl;
        }
        
        // Setup CPU
        cpu.R()[15] = 0x08000000;
        cpu.clearFlag(CPU::FLAG_Z);
        cpu.clearFlag(CPU::FLAG_N);
        cpu.clearFlag(CPU::FLAG_C);
        cpu.clearFlag(CPU::FLAG_V);
        cpu.clearFlag(CPU::FLAG_T);
        
        std::cout << "\nInitial state: R0=0x" << std::hex << cpu.R()[0] << std::dec << std::endl;
        
        // Execute step by step
        for (int step = 0; step < 10; step++) {
            uint32_t pc = cpu.R()[15];
            uint32_t r0_before = cpu.R()[0];
            uint32_t instruction = memory.read32(pc);
            
            std::cout << "Step " << step << ": PC=0x" << std::hex << pc 
                      << ", R0_before=0x" << r0_before
                      << ", Instr=0x" << instruction << std::dec;
            
            // Execute instruction
            cpu.execute(1);
            
            uint32_t r0_after = cpu.R()[0];
            uint32_t pc_after = cpu.R()[15];
            
            std::cout << " → R0_after=0x" << std::hex << r0_after
                      << ", PC_after=0x" << pc_after << std::dec << std::endl;
            
            // Stop if we reach the infinite loop
            if (pc_after == 0x08000010) {
                std::cout << "Reached infinite loop, stopping.\n";
                break;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
