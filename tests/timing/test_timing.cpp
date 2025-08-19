#include "gba.h"
#include "cpu.h"
#include "debug.h"
#include <iostream>
#include <iomanip>
#include <string>

int main() {
    // Disable debug output
    extern int g_debug_level;
    extern const int DEBUG_LEVEL_OFF;
    g_debug_level = DEBUG_LEVEL_OFF;

    extern bool g_disassemble_enabled;
    g_disassemble_enabled = true;

    // Create a real GBA (full memory map, BIOS and GamePak auto-loaded)
    GBA gba(false);
    auto& cpu = gba.getCPU();

    // Set PC to 0
    cpu.R()[15] = 0;

    // Run for 1,000,000 cycles
    constexpr uint32_t total_cycles = 10000;
    constexpr uint32_t report_interval = 200;
    std::cout << "\n=== GBA Timing Test ===\n";
    std::cout << "Running for " << total_cycles << " instructions...\n";

    for (uint32_t i = 0; i < total_cycles; ++i) {
        cpu.execute(1);
        if (i % report_interval == 0) {
            uint32_t pc = cpu.R()[15];
            bool thumb = cpu.getFlag(CPU::FLAG_T);
            std::cout << "[" << std::setw(6) << i << "] PC=0x" << std::hex << std::setw(8) << std::setfill('0') << pc
                      << std::dec << " Mode=" << (thumb ? "Thumb" : "ARM") << std::endl;
        }
    }
    std::cout << "\nTiming test complete.\n";
    return 0;
}
