#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include <iostream>
#include <iomanip>

// Test to verify ARM instruction cache invalidation works correctly
int main() {
    std::cout << "=== ARM Instruction Cache Invalidation Test ===" << std::endl;
    
    // Create GBA in test mode
    GBA gba(true);
    auto& cpu = gba.getCPU();
    auto& memory = cpu.getMemory();
    
    // Switch to ARM mode
    cpu.CPSR() &= ~CPU::FLAG_T; // Clear Thumb bit to use ARM mode
    
    // Initialize registers
    auto& registers = cpu.R();
    registers.fill(0);
    registers[15] = 0; // PC starts at 0
    
    // Test 1: Load ARM instruction and execute to populate cache
    std::cout << "\n1. Testing cache population..." << std::endl;
    
    // Write ADD R1, R1, R2 instruction at address 0x0000
    uint32_t add_instruction = 0xE0811002; // ADD R1, R1, R2 (condition=always, data processing)
    memory.write32(0x0000, add_instruction);
    
    // Set up registers for the test
    registers[1] = 10; // R1 = 10
    registers[2] = 5;  // R2 = 5
    registers[15] = 0; // PC = 0
    
    // Execute instruction to populate cache
    cpu.execute(1);
    
    // Check that R1 = 15 (10 + 5)
    if (registers[1] == 15) {
        std::cout << "✓ Instruction executed correctly, R1 = " << registers[1] << std::endl;
    } else {
        std::cout << "✗ Instruction execution failed, R1 = " << registers[1] << std::endl;
    }
    
    // Test 2: Get cache statistics
    std::cout << "\n2. Checking cache statistics..." << std::endl;
    auto stats = cpu.getARMCPU().getInstructionCacheStats();
    std::cout << "Cache hits: " << stats.hits << std::endl;
    std::cout << "Cache misses: " << stats.misses << std::endl;
    std::cout << "Cache invalidations: " << stats.invalidations << std::endl;
    std::cout << "Hit rate: " << std::fixed << std::setprecision(2) << (stats.hit_rate * 100) << "%" << std::endl;
    
    // Test 3: Execute same instruction again (should be cache hit)
    std::cout << "\n3. Testing cache hit..." << std::endl;
    registers[1] = 10; // Reset R1
    registers[15] = 0; // Reset PC
    cpu.execute(1);
    
    if (registers[1] == 15) {
        std::cout << "✓ Second execution successful, R1 = " << registers[1] << std::endl;
    } else {
        std::cout << "✗ Second execution failed, R1 = " << registers[1] << std::endl;
    }
    
    // Check cache statistics after hit
    auto stats2 = cpu.getARMCPU().getInstructionCacheStats();
    std::cout << "Cache hits: " << stats2.hits << " (+" << (stats2.hits - stats.hits) << ")" << std::endl;
    std::cout << "Cache misses: " << stats2.misses << " (+" << (stats2.misses - stats.misses) << ")" << std::endl;
    
    // Test 4: Write to instruction memory to trigger cache invalidation
    std::cout << "\n4. Testing cache invalidation..." << std::endl;
    
    // Write a different instruction at the same address (SUB R1, R1, R2)
    uint32_t sub_instruction = 0xE0411002; // SUB R1, R1, R2
    memory.write32(0x0000, sub_instruction);
    
    // Check invalidation count
    auto stats3 = cpu.getARMCPU().getInstructionCacheStats();
    std::cout << "Cache invalidations: " << stats3.invalidations << " (+" << (stats3.invalidations - stats2.invalidations) << ")" << std::endl;
    
    if (stats3.invalidations > stats2.invalidations) {
        std::cout << "✓ Cache invalidation triggered by memory write" << std::endl;
    } else {
        std::cout << "✗ Cache invalidation not triggered" << std::endl;
    }
    
    // Test 5: Execute modified instruction
    std::cout << "\n5. Testing execution after invalidation..." << std::endl;
    registers[1] = 10; // Reset R1
    registers[15] = 0; // Reset PC
    cpu.execute(1);
    
    if (registers[1] == 5) { // Should be 10 - 5 = 5
        std::cout << "✓ Modified instruction executed correctly, R1 = " << registers[1] << std::endl;
        std::cout << "✓ Cache invalidation working properly - new instruction was decoded and executed" << std::endl;
    } else {
        std::cout << "✗ Modified instruction execution failed, R1 = " << registers[1] << std::endl;
        std::cout << "✗ Cache invalidation may not be working - old instruction may have been cached" << std::endl;
    }
    
    // Final cache statistics
    auto final_stats = cpu.getARMCPU().getInstructionCacheStats();
    std::cout << "\n=== Final Cache Statistics ===" << std::endl;
    std::cout << "Total hits: " << final_stats.hits << std::endl;
    std::cout << "Total misses: " << final_stats.misses << std::endl;
    std::cout << "Total invalidations: " << final_stats.invalidations << std::endl;
    std::cout << "Final hit rate: " << std::fixed << std::setprecision(2) << (final_stats.hit_rate * 100) << "%" << std::endl;
    
    std::cout << "\n=== Cache Invalidation Test Complete ===" << std::endl;
    
    return 0;
}
