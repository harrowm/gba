#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "debug.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

// Detailed ARM instruction execution debugging test
int main() {
    std::cout << "=== ARM Branch Execution Investigation ===\n";
    std::cout << "Testing ARM branch instruction execution in tight loop.\n\n";
    
    try {
        // Create GBA instance and load our test ROM
        GBA gba(false);
        auto& cpu = gba.getCPU();
        auto& memory = gba.getMemory();
        
        // Load our test ROM
        std::cout << "Loading test GamePak ROM with simple loop...\n";
        std::ifstream testRom("assets/roms/test_gamepak.bin", std::ios::binary);
        if (!testRom.is_open()) {
            std::cerr << "Error: Could not open test_gamepak.bin. Run create_test_rom.py first.\n";
            return 1;
        }
        
        testRom.seekg(0, std::ios::end);
        size_t romSize = testRom.tellg();
        testRom.seekg(0, std::ios::beg);
        std::vector<uint8_t> romData(romSize);
        testRom.read(reinterpret_cast<char*>(romData.data()), romSize);
        testRom.close();
        
        // Install ROM data directly
        for (size_t i = 0; i < romSize && i < 32 * 1024 * 1024; ++i) {
            int mappedAddr = memory.mapAddress(0x08000000 + i, false);
            if (mappedAddr >= 0) {
                memory.getRawData()[mappedAddr] = romData[i];
            }
        }
        
        // Verify ROM loaded correctly
        uint32_t instr0 = memory.read32(0x08000000);
        uint32_t instr1 = memory.read32(0x08000004);
        uint32_t instr2 = memory.read32(0x08000008);
        uint32_t instr3 = memory.read32(0x0800000C);
        uint32_t instr4 = memory.read32(0x08000010);
        
        std::cout << "ROM Instructions:\n";
        std::cout << "  0x08000000: 0x" << std::hex << instr0 << " (mov r0, #0xFF)\n";
        std::cout << "  0x08000004: 0x" << std::hex << instr1 << " (sub r0, r0, #1)\n";
        std::cout << "  0x08000008: 0x" << std::hex << instr2 << " (cmp r0, #0)\n";
        std::cout << "  0x0800000C: 0x" << std::hex << instr3 << " (bne -4)\n";
        std::cout << "  0x08000010: 0x" << std::hex << instr4 << " (b 0x08000010)\n";
        std::cout << std::dec << std::endl;
        
        // Force jump to GamePak code
        std::cout << "Setting up CPU to execute GamePak code...\n";
        cpu.R()[15] = 0x08000000;  // Set PC to start of our loop
        cpu.R()[14] = 0x00000000;  // Set LR to BIOS (for safety)
        
        // Clear ARM flags to ensure known state
        cpu.clearFlag(CPU::FLAG_Z);  // Clear Zero flag
        cpu.clearFlag(CPU::FLAG_N);  // Clear Negative flag
        cpu.clearFlag(CPU::FLAG_C);  // Clear Carry flag
        cpu.clearFlag(CPU::FLAG_V);  // Clear Overflow flag
        cpu.clearFlag(CPU::FLAG_T);  // Ensure ARM mode (not Thumb)
        
        std::cout << "Initial CPU state:\n";
        std::cout << "  PC: 0x" << std::hex << cpu.R()[15] << std::dec << std::endl;
        std::cout << "  ARM mode: " << (cpu.getFlag(CPU::FLAG_T) ? "Thumb" : "ARM") << std::endl;
        std::cout << "  Flags: Z=" << cpu.getFlag(CPU::FLAG_Z) 
                  << " N=" << cpu.getFlag(CPU::FLAG_N)
                  << " C=" << cpu.getFlag(CPU::FLAG_C) 
                  << " V=" << cpu.getFlag(CPU::FLAG_V) << std::endl;
        
        // Execute step-by-step through the loop
        std::cout << "\n=== Step-by-Step Execution Analysis ===\n";
        
        int step = 0;
        uint32_t last_pc = 0xFFFFFFFF;
        int same_pc_count = 0;
        
        for (int i = 0; i < 1000; i++) {  // Maximum 1000 steps to avoid infinite loop
            uint32_t pc = cpu.R()[15];
            uint32_t r0_value = cpu.R()[0];
            uint32_t instruction = memory.read32(pc);
            
            // Break if we're stuck at the same PC too long
            if (pc == last_pc) {
                same_pc_count++;
                if (same_pc_count > 5) {
                    std::cout << "  ⚠ PC stuck at 0x" << std::hex << pc << std::dec << " for " << same_pc_count << " steps - breaking\n";
                    break;
                }
            } else {
                same_pc_count = 0;
            }
            last_pc = pc;
            
            // Show first 20 steps in detail, then every 50th step
            if (step < 20 || step % 50 == 0) {
                std::cout << "  Step " << std::setw(3) << step 
                          << ": PC=0x" << std::hex << std::setw(8) << std::setfill('0') << pc
                          << ", R0=0x" << std::setw(8) << r0_value
                          << ", Instr=0x" << std::setw(8) << instruction << std::dec << std::setfill(' ');
                
                // Decode instruction type
                if (pc == 0x08000000) {
                    std::cout << " (mov r0,#0xFF)";
                } else if (pc == 0x08000004) {
                    std::cout << " (sub r0,r0,#1)";
                } else if (pc == 0x08000008) {
                    std::cout << " (cmp r0,#0)";
                } else if (pc == 0x0800000C) {
                    std::cout << " (bne loop)";
                } else if (pc == 0x08000010) {
                    std::cout << " (b infinite)";
                } else {
                    std::cout << " (unknown)";
                }
            }
            
            // Execute single instruction
            cpu.execute(1);
            
            uint32_t new_pc = cpu.R()[15];
            uint32_t new_r0 = cpu.R()[0];
            
            if (step < 20 || step % 50 == 0) {
                std::cout << " → PC=0x" << std::hex << new_pc 
                          << ", R0=0x" << new_r0 << std::dec;
                          
                // Show flags after instruction
                std::cout << " [Z=" << cpu.getFlag(CPU::FLAG_Z) 
                          << " N=" << cpu.getFlag(CPU::FLAG_N)
                          << " C=" << cpu.getFlag(CPU::FLAG_C) 
                          << " V=" << cpu.getFlag(CPU::FLAG_V) << "]";
                
                // Add specific debugging for SUB instruction
                if (pc == 0x08000004) {
                    std::cout << "\n    SUB DEBUG: R0 = R0(0x" << std::hex << r0_value 
                              << ") - 0x1 = 0x" << new_r0 << std::dec;
                    std::cout << "\n    SUB VERIFY: R0 now contains 0x" << std::hex << new_r0 << std::dec;
                }
                
                std::cout << std::endl;
            }
            
            step++;
            
            // Check if we've finished the loop (reached infinite branch)
            if (new_pc == 0x08000010) {
                std::cout << "  ✓ Loop completed! Reached infinite branch at step " << step << std::endl;
                std::cout << "  Final R0 value: 0x" << std::hex << new_r0 << std::dec << std::endl;
                break;
            }
            
            // Break if we've gone outside our expected region
            if (new_pc < 0x08000000 || new_pc > 0x08000020) {
                std::cout << "  ✗ PC went outside expected region at step " << step << std::endl;
                break;
            }
            
            // Safety check - if R0 becomes very large, something's wrong
            if (new_r0 > 0x1000) {
                std::cout << "  ✗ R0 value unexpectedly large: 0x" << std::hex << new_r0 << std::dec << " at step " << step << std::endl;
                break;
            }
        }
        
        std::cout << "\n=== Analysis Results ===\n";
        std::cout << "Total steps executed: " << step << std::endl;
        std::cout << "Final PC: 0x" << std::hex << cpu.R()[15] << std::dec << std::endl;
        std::cout << "Final R0: 0x" << std::hex << cpu.R()[0] << std::dec << std::endl;
        
        // Expected: 255 iterations of the loop (sub + cmp + bne) + initial mov + final infinite branch
        // Each loop iteration = 3 instructions (sub, cmp, bne)
        // Total expected = 1 (mov) + 255*3 (loop) + 1 (final branch) = 767 instructions
        int expected_steps = 1 + 255 * 3 + 1;
        std::cout << "Expected steps for 255 iterations: " << expected_steps << std::endl;
        
        if (step == expected_steps) {
            std::cout << "✓ Instruction count matches expected value!\n";
        } else {
            std::cout << "✗ Instruction count mismatch - indicates branch execution issue\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during analysis: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
