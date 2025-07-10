#include "gba.h"
#include "cpu.h"
#include "memory.h"
#include "debug.h"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

// Direct GamePak cache test
// This test skips the BIOS and directly tests cache performance with GamePak code
int main() {
    std::cout << "=== Direct GamePak Cache Performance Test ===\n";
    std::cout << "Testing instruction cache performance with GamePak ROM code.\n";
    std::cout << "This test bypasses BIOS and directly executes GamePak code.\n\n";
    
    try {
        // Create GBA instance
        GBA gba(false); // Production mode
        auto& cpu = gba.getCPU();
        auto& memory = gba.getMemory();
        
        std::cout << "✓ GBA initialized\n";
        
        // Write a more comprehensive test program to GamePak ROM
        std::cout << "Writing comprehensive test program to GamePak ROM...\n";
        
        const uint32_t GAME_PAK_START = 0x08000000;
        uint32_t addr = GAME_PAK_START;
        
        // Test program: A loop that exercises the cache with different patterns
        
        // 1. Initialize counter
        memory.write32(addr, 0xE3A00A01); addr += 4; // mov r0, #0x1000  ; Loop counter
        
        // 2. Main loop label (loop_start)
        uint32_t loop_start = addr;
        
        // 3. Decrement counter and check
        memory.write32(addr, 0xE2500001); addr += 4; // subs r0, r0, #1   ; Decrement and set flags
        memory.write32(addr, 0x0A000006); addr += 4; // beq done         ; Branch if zero
        
        // 4. Cache test: Load and store operations
        memory.write32(addr, 0xE3A01000); addr += 4; // mov r1, #0        ; Clear r1
        memory.write32(addr, 0xE2811001); addr += 4; // add r1, r1, #1    ; Increment r1
        memory.write32(addr, 0xE2811001); addr += 4; // add r1, r1, #1    ; Increment r1 again
        memory.write32(addr, 0xE2811001); addr += 4; // add r1, r1, #1    ; Increment r1 again
        
        // 5. Branch back to loop start
        int32_t branch_offset = (loop_start - (addr + 8)) >> 2;
        memory.write32(addr, 0xEA000000 | (branch_offset & 0x00FFFFFF)); addr += 4; // b loop_start
        
        // 6. Done label - infinite loop
        memory.write32(addr, 0xEAFFFFFE); addr += 4; // b done  ; Infinite loop
        
        std::cout << "✓ Test program written to GamePak ROM\n";
        std::cout << "  Program size: " << (addr - GAME_PAK_START) << " bytes\n";
        std::cout << "  Loop start: 0x" << std::hex << loop_start << std::dec << std::endl;
        
        // Get ARM CPU reference for cache statistics
        auto& arm_cpu = cpu.getARMCPU();
        
        // Set CPU to start at GamePak ROM directly
        cpu.R()[15] = GAME_PAK_START;  // Set PC to GamePak start
        cpu.R()[14] = 0x08000000;      // Set LR to GamePak start
        cpu.R()[13] = 0x03007F00;      // Set SP to stack area
        
        // Clear the Thumb flag to ensure ARM mode
        cpu.clearFlag(CPU::FLAG_T);
        
        std::cout << "✓ CPU initialized for GamePak execution\n";
        std::cout << "  PC: 0x" << std::hex << cpu.R()[15] << std::dec << std::endl;
        std::cout << "  Mode: " << (cpu.getFlag(CPU::FLAG_T) ? "Thumb" : "ARM") << std::endl;
        
        // Reset cache statistics
        arm_cpu.resetInstructionCacheStats();
        
        std::cout << "\n=== GamePak Cache Performance Analysis ===\n";
        
        // Execute in phases and monitor cache performance
        const uint32_t PHASE_SIZE = 1000;
        const uint32_t MAX_PHASES = 20;
        
        std::cout << std::setw(6) << "Phase"
                  << std::setw(12) << "Instructions"
                  << std::setw(12) << "PC"
                  << std::setw(8) << "Hits"
                  << std::setw(8) << "Misses"
                  << std::setw(10) << "Hit Rate"
                  << std::setw(8) << "Mode"
                  << std::endl;
        std::cout << std::string(70, '-') << std::endl;
        
        uint32_t total_instructions = 0;
        
        for (uint32_t phase = 0; phase < MAX_PHASES; phase++) {
            uint32_t pc_before = cpu.R()[15];
            auto stats_before = arm_cpu.getInstructionCacheStats();
            bool is_arm_mode = !(cpu.getFlag(CPU::FLAG_T));
            
            // Execute one phase
            auto exec_start = std::chrono::high_resolution_clock::now();
            cpu.execute(PHASE_SIZE);
            auto exec_end = std::chrono::high_resolution_clock::now();
            
            uint32_t pc_after = cpu.R()[15];
            auto stats_after = arm_cpu.getInstructionCacheStats();
            
            // Calculate phase statistics
            uint64_t phase_hits = stats_after.hits - stats_before.hits;
            uint64_t phase_misses = stats_after.misses - stats_before.misses;
            uint64_t phase_total = phase_hits + phase_misses;
            double phase_hit_rate = phase_total > 0 ? (double)phase_hits / phase_total * 100.0 : 0.0;
            
            total_instructions += PHASE_SIZE;
            
            std::cout << std::setw(6) << phase
                      << std::setw(12) << total_instructions
                      << " 0x" << std::hex << std::setfill('0') << std::setw(8) << pc_after << std::dec << std::setfill(' ')
                      << std::setw(8) << phase_hits
                      << std::setw(8) << phase_misses
                      << std::setw(9) << std::fixed << std::setprecision(1) << phase_hit_rate << "%"
                      << std::setw(8) << (is_arm_mode ? "ARM" : "Thumb")
                      << std::endl;
            
            // Check if we're still in GamePak region
            if (pc_after < GAME_PAK_START || pc_after > GAME_PAK_START + 0x1000) {
                std::cout << "⚠ PC outside expected GamePak region, stopping.\n";
                break;
            }
        }
        
        // Final analysis
        std::cout << "\n=== Final Cache Performance Analysis ===\n";
        auto final_stats = arm_cpu.getInstructionCacheStats();
        
        std::cout << "Execution summary:\n";
        std::cout << "  Total instructions executed: " << total_instructions << std::endl;
        std::cout << "  Final PC: 0x" << std::hex << cpu.R()[15] << std::dec << std::endl;
        std::cout << "  Execution mode: " << (cpu.getFlag(CPU::FLAG_T) ? "Thumb" : "ARM") << std::endl;
        
        std::cout << "\nCache performance:\n";
        std::cout << "  Total hits: " << final_stats.hits << std::endl;
        std::cout << "  Total misses: " << final_stats.misses << std::endl;
        std::cout << "  Overall hit rate: " << std::fixed << std::setprecision(2) << final_stats.hit_rate << "%" << std::endl;
        std::cout << "  Total invalidations: " << final_stats.invalidations << std::endl;
        
        // Analysis
        std::cout << "\n=== Cache Analysis ===\n";
        if (final_stats.hit_rate > 80.0) {
            std::cout << "✓ Excellent cache performance - High instruction reuse detected\n";
        } else if (final_stats.hit_rate > 50.0) {
            std::cout << "✓ Good cache performance - Moderate instruction reuse\n";
        } else if (final_stats.hit_rate > 20.0) {
            std::cout << "◐ Fair cache performance - Some instruction reuse\n";
        } else {
            std::cout << "✗ Poor cache performance - Limited instruction reuse\n";
        }
        
        std::cout << "\nThis test demonstrates cache behavior with GamePak ROM code.\n";
        std::cout << "The loop should show high cache hit rates after the first iteration.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error during GamePak cache test: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
