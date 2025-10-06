// Test memory wait states for cycle-accurate timing

#include <gtest/gtest.h>
#include "memory.h"
#include "scheduler.h"

class MemoryWaitStatesTest : public ::testing::Test {
protected:
    Memory memory;
    Scheduler scheduler;
    
    void SetUp() override {
        memory.setScheduler(&scheduler);
        scheduler.reset();
    }
};

// ==========================================================================
// BIOS Region (0x00000000 - 0x00003FFF)
// ==========================================================================

TEST_F(MemoryWaitStatesTest, BIOS_8bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x00000000, 8), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x00001000, 8), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x00003FFF, 8), 1UL);
}

TEST_F(MemoryWaitStatesTest, BIOS_16bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x00000000, 16), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x00002000, 16), 1UL);
}

TEST_F(MemoryWaitStatesTest, BIOS_32bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x00000000, 32), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x00003000, 32), 1UL);
}

// ==========================================================================
// EWRAM - External Work RAM (0x02000000 - 0x0203FFFF)
// 16-bit bus: 3 cycles for 8/16-bit, 6 cycles for 32-bit
// ==========================================================================

TEST_F(MemoryWaitStatesTest, EWRAM_8bit_3cycles) {
    EXPECT_EQ(memory.getWaitStates(0x02000000, 8), 3UL);
    EXPECT_EQ(memory.getWaitStates(0x02010000, 8), 3UL);
    EXPECT_EQ(memory.getWaitStates(0x0203FFFF, 8), 3UL);
}

TEST_F(MemoryWaitStatesTest, EWRAM_16bit_3cycles) {
    EXPECT_EQ(memory.getWaitStates(0x02000000, 16), 3UL);
    EXPECT_EQ(memory.getWaitStates(0x02020000, 16), 3UL);
}

TEST_F(MemoryWaitStatesTest, EWRAM_32bit_6cycles) {
    EXPECT_EQ(memory.getWaitStates(0x02000000, 32), 6UL);
    EXPECT_EQ(memory.getWaitStates(0x02030000, 32), 6UL);
}

// ==========================================================================
// IWRAM - Internal Work RAM (0x03000000 - 0x03007FFF)
// 32-bit bus: 1 cycle for any access (fastest memory)
// ==========================================================================

TEST_F(MemoryWaitStatesTest, IWRAM_8bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x03000000, 8), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x03004000, 8), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x03007FFF, 8), 1UL);
}

TEST_F(MemoryWaitStatesTest, IWRAM_16bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x03000000, 16), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x03007000, 16), 1UL);
}

TEST_F(MemoryWaitStatesTest, IWRAM_32bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x03000000, 32), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x03007FFC, 32), 1UL);
}

// ==========================================================================
// I/O Registers (0x04000000 - 0x040003FE)
// 1 cycle for any access
// ==========================================================================

TEST_F(MemoryWaitStatesTest, IO_Registers_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x04000000, 8), 1UL);   // DISPCNT
    EXPECT_EQ(memory.getWaitStates(0x04000004, 16), 1UL);  // DISPSTAT
    EXPECT_EQ(memory.getWaitStates(0x04000100, 32), 1UL);  // TM0CNT_L
    EXPECT_EQ(memory.getWaitStates(0x04000200, 16), 1UL);  // IE
}

// ==========================================================================
// Palette RAM (0x05000000 - 0x050003FF)
// 16-bit bus: 1 cycle for 16-bit, 2 cycles for 32-bit
// ==========================================================================

TEST_F(MemoryWaitStatesTest, PaletteRAM_8bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x05000000, 8), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x05000100, 8), 1UL);
}

TEST_F(MemoryWaitStatesTest, PaletteRAM_16bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x05000000, 16), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x05000200, 16), 1UL);
}

TEST_F(MemoryWaitStatesTest, PaletteRAM_32bit_2cycles) {
    EXPECT_EQ(memory.getWaitStates(0x05000000, 32), 2UL);
    EXPECT_EQ(memory.getWaitStates(0x05000100, 32), 2UL);
}

// ==========================================================================
// VRAM (0x06000000 - 0x06017FFF)
// 16-bit bus: 1 cycle for 16-bit, 2 cycles for 32-bit
// ==========================================================================

TEST_F(MemoryWaitStatesTest, VRAM_8bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x06000000, 8), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x06010000, 8), 1UL);
}

TEST_F(MemoryWaitStatesTest, VRAM_16bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x06000000, 16), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x06017000, 16), 1UL);
}

TEST_F(MemoryWaitStatesTest, VRAM_32bit_2cycles) {
    EXPECT_EQ(memory.getWaitStates(0x06000000, 32), 2UL);
    EXPECT_EQ(memory.getWaitStates(0x06010000, 32), 2UL);
}

// ==========================================================================
// OAM (0x07000000 - 0x070003FF)
// 32-bit bus: 1 cycle for any access
// ==========================================================================

TEST_F(MemoryWaitStatesTest, OAM_8bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x07000000, 8), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x07000200, 8), 1UL);
}

TEST_F(MemoryWaitStatesTest, OAM_16bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x07000000, 16), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x070003FE, 16), 1UL);
}

TEST_F(MemoryWaitStatesTest, OAM_32bit_1cycle) {
    EXPECT_EQ(memory.getWaitStates(0x07000000, 32), 1UL);
    EXPECT_EQ(memory.getWaitStates(0x07000100, 32), 1UL);
}

// ==========================================================================
// Game Pak ROM (0x08000000 - 0x0DFFFFFF)
// 16-bit bus: 5 cycles for 8/16-bit, 8 cycles for 32-bit (default)
// ==========================================================================

TEST_F(MemoryWaitStatesTest, GamePakROM_WS0_8bit_5cycles) {
    EXPECT_EQ(memory.getWaitStates(0x08000000, 8), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x08100000, 8), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x09FFFFFF, 8), 5UL);
}

TEST_F(MemoryWaitStatesTest, GamePakROM_WS0_16bit_5cycles) {
    EXPECT_EQ(memory.getWaitStates(0x08000000, 16), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x09000000, 16), 5UL);
}

TEST_F(MemoryWaitStatesTest, GamePakROM_WS0_32bit_8cycles) {
    EXPECT_EQ(memory.getWaitStates(0x08000000, 32), 8UL);
    EXPECT_EQ(memory.getWaitStates(0x09000000, 32), 8UL);
}

TEST_F(MemoryWaitStatesTest, GamePakROM_WS1_AllWidths) {
    EXPECT_EQ(memory.getWaitStates(0x0A000000, 8), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x0A000000, 16), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x0A000000, 32), 8UL);
    EXPECT_EQ(memory.getWaitStates(0x0BFFFFFF, 16), 5UL);
}

TEST_F(MemoryWaitStatesTest, GamePakROM_WS2_AllWidths) {
    EXPECT_EQ(memory.getWaitStates(0x0C000000, 8), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x0C000000, 16), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x0C000000, 32), 8UL);
    EXPECT_EQ(memory.getWaitStates(0x0DFFFFFF, 16), 5UL);
}

// ==========================================================================
// Game Pak SRAM (0x0E000000 - 0x0E00FFFF)
// 8-bit bus: 5 cycles for any access
// ==========================================================================

TEST_F(MemoryWaitStatesTest, GamePakSRAM_8bit_5cycles) {
    EXPECT_EQ(memory.getWaitStates(0x0E000000, 8), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x0E008000, 8), 5UL);
    EXPECT_EQ(memory.getWaitStates(0x0E00FFFF, 8), 5UL);
}

TEST_F(MemoryWaitStatesTest, GamePakSRAM_16bit_5cycles) {
    EXPECT_EQ(memory.getWaitStates(0x0E000000, 16), 5UL);
}

TEST_F(MemoryWaitStatesTest, GamePakSRAM_32bit_5cycles) {
    EXPECT_EQ(memory.getWaitStates(0x0E000000, 32), 5UL);
}

// ==========================================================================
// Unmapped Regions
// ==========================================================================

TEST_F(MemoryWaitStatesTest, UnmappedMemory_Returns1Cycle) {
    EXPECT_EQ(memory.getWaitStates(0x01000000, 32), 1UL); // Between BIOS and EWRAM
    EXPECT_EQ(memory.getWaitStates(0x0F000000, 32), 1UL); // After SRAM
}

// ==========================================================================
// Performance Comparison Tests
// ==========================================================================

TEST_F(MemoryWaitStatesTest, FastestMemory_IWRAM) {
    // IWRAM should be fastest for all access widths
    uint32_t iwram_cycles = memory.getWaitStates(0x03000000, 32);
    
    EXPECT_LE(iwram_cycles, memory.getWaitStates(0x02000000, 32)); // vs EWRAM
    EXPECT_LE(iwram_cycles, memory.getWaitStates(0x08000000, 32)); // vs ROM
    EXPECT_LE(iwram_cycles, memory.getWaitStates(0x0E000000, 32)); // vs SRAM
}

TEST_F(MemoryWaitStatesTest, SlowestMemory_GamePakROM_32bit) {
    // GamePak ROM 32-bit access should be slowest
    uint32_t rom_32bit = memory.getWaitStates(0x08000000, 32);
    
    EXPECT_GE(rom_32bit, memory.getWaitStates(0x03000000, 32)); // vs IWRAM
    EXPECT_GE(rom_32bit, memory.getWaitStates(0x06000000, 32)); // vs VRAM
    EXPECT_GE(rom_32bit, memory.getWaitStates(0x07000000, 32)); // vs OAM
}

TEST_F(MemoryWaitStatesTest, BusWidth_EWRAM_32bit_DoubleTime) {
    // 32-bit access on 16-bit bus should take double time
    uint32_t ewram_16bit = memory.getWaitStates(0x02000000, 16);
    uint32_t ewram_32bit = memory.getWaitStates(0x02000000, 32);
    
    EXPECT_EQ(ewram_32bit, ewram_16bit * 2);
}

TEST_F(MemoryWaitStatesTest, BusWidth_VRAM_32bit_DoubleTime) {
    // 32-bit access on 16-bit bus should take double time
    uint32_t vram_16bit = memory.getWaitStates(0x06000000, 16);
    uint32_t vram_32bit = memory.getWaitStates(0x06000000, 32);
    
    EXPECT_EQ(vram_32bit, vram_16bit * 2);
}

// ==========================================================================
// Summary Test
// ==========================================================================

TEST_F(MemoryWaitStatesTest, ComprehensiveSummary) {
    printf("\n========================================\n");
    printf("  Memory Region Wait States Summary\n");
    printf("========================================\n\n");
    
    printf("%-20s %6s %6s %6s\n", "Region", "8-bit", "16-bit", "32-bit");
    printf("%-20s %6s %6s %6s\n", "--------------------", "------", "------", "------");
    
    printf("%-20s %6u %6u %6u\n", "BIOS",
           memory.getWaitStates(0x00000000, 8),
           memory.getWaitStates(0x00000000, 16),
           memory.getWaitStates(0x00000000, 32));
    
    printf("%-20s %6u %6u %6u\n", "EWRAM",
           memory.getWaitStates(0x02000000, 8),
           memory.getWaitStates(0x02000000, 16),
           memory.getWaitStates(0x02000000, 32));
    
    printf("%-20s %6u %6u %6u  ← Fastest!\n", "IWRAM",
           memory.getWaitStates(0x03000000, 8),
           memory.getWaitStates(0x03000000, 16),
           memory.getWaitStates(0x03000000, 32));
    
    printf("%-20s %6u %6u %6u\n", "I/O Registers",
           memory.getWaitStates(0x04000000, 8),
           memory.getWaitStates(0x04000000, 16),
           memory.getWaitStates(0x04000000, 32));
    
    printf("%-20s %6u %6u %6u\n", "Palette RAM",
           memory.getWaitStates(0x05000000, 8),
           memory.getWaitStates(0x05000000, 16),
           memory.getWaitStates(0x05000000, 32));
    
    printf("%-20s %6u %6u %6u\n", "VRAM",
           memory.getWaitStates(0x06000000, 8),
           memory.getWaitStates(0x06000000, 16),
           memory.getWaitStates(0x06000000, 32));
    
    printf("%-20s %6u %6u %6u\n", "OAM",
           memory.getWaitStates(0x07000000, 8),
           memory.getWaitStates(0x07000000, 16),
           memory.getWaitStates(0x07000000, 32));
    
    printf("%-20s %6u %6u %6u\n", "GamePak ROM (WS0)",
           memory.getWaitStates(0x08000000, 8),
           memory.getWaitStates(0x08000000, 16),
           memory.getWaitStates(0x08000000, 32));
    
    printf("%-20s %6u %6u %6u\n", "GamePak SRAM",
           memory.getWaitStates(0x0E000000, 8),
           memory.getWaitStates(0x0E000000, 16),
           memory.getWaitStates(0x0E000000, 32));
    
    printf("\n");
}

// ==========================================================================
// Main
// ==========================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
