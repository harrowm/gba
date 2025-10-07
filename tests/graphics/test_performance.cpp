/**
 * test_performance.cpp
 * 
 * Performance benchmarks for Mode 0 rendering.
 * Target: <1ms per frame (60 FPS capable)
 */

#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"
#include <chrono>
#include <vector>

class PerformanceTest : public ::testing::Test {
protected:
    GBA* gba;
    Memory* memory;
    GPU* gpu;
    
    // Register addresses
    static constexpr uint32_t PALETTE_RAM_BASE = 0x05000000u;
    static constexpr uint32_t REG_DISPCNT = 0x04000000u;
    static constexpr uint32_t REG_BG0CNT = 0x04000008u;
    static constexpr uint32_t REG_BG1CNT = 0x0400000Au;
    static constexpr uint32_t REG_BG2CNT = 0x0400000Cu;
    static constexpr uint32_t REG_BG3CNT = 0x0400000Eu;
    static constexpr uint32_t REG_BG0HOFS = 0x04000010u;
    static constexpr uint32_t REG_BG0VOFS = 0x04000012u;
    static constexpr uint32_t REG_BG1HOFS = 0x04000014u;
    static constexpr uint32_t REG_BG1VOFS = 0x04000016u;
    static constexpr uint32_t REG_BG2HOFS = 0x04000018u;
    static constexpr uint32_t REG_BG2VOFS = 0x0400001Au;
    static constexpr uint32_t REG_BG3HOFS = 0x0400001Cu;
    static constexpr uint32_t REG_BG3VOFS = 0x0400001Eu;
    
    void SetUp() override {
        gba = new GBA(false);  // false = don't show debug output
        memory = &gba->getMemory();
        gpu = &gba->getGPU();
        
        // Initialize to Mode 0
        memory->write16(REG_DISPCNT, 0x0000);  // Mode 0
    }
    
    void TearDown() override {
        delete gba;
    }
    
    // Helper to setup a background
    void setupBG(int bgNum, uint8_t priority, bool is8bpp, int tileNum) {
        uint32_t bgcnt = REG_BG0CNT + (bgNum * 2);
        
        // BGxCNT: priority, screen base, color mode
        uint16_t config = priority | (28u << 8);  // Screen base = 28 (0x7000)
        if (is8bpp) {
            config |= 0x0080u;  // Bit 7 for 8bpp
        }
        memory->write16(bgcnt, config);
        
        // Write screen entry (tile number)
        uint32_t tilemapAddr = 0x06007000u + (bgNum * 0x800u);
        memory->write16(tilemapAddr, tileNum);
        
        // Create tile data
        uint8_t* vram = memory->getVRAM();
        uint32_t tileAddr = 0x06000000u + (tileNum * 32u);
        uint32_t offset = tileAddr - 0x06000000u;
        
        if (is8bpp) {
            // Fill with pattern
            for (int i = 0; i < 64; i++) {
                vram[offset + i] = (uint8_t)((i % 8) + 1);
            }
        } else {
            // 4bpp pattern
            for (int i = 0; i < 32; i++) {
                vram[offset + i] = (uint8_t)(0x12);  // Two pixels per byte
            }
        }
        
        // Write palette colors
        uint8_t* paletteRAM = memory->getPaletteRAM();
        for (int i = 1; i < 16; i++) {
            // Different colors per palette
            uint16_t color = (uint16_t)(((bgNum + 1) << 10) | ((bgNum + 1) << 5) | (bgNum + 1));
            paletteRAM[(bgNum * 16 + i) * 2] = color & 0xFFu;
            paletteRAM[(bgNum * 16 + i) * 2 + 1] = (color >> 8) & 0xFFu;
        }
    }
    
    // Measure time to render N frames
    double measureFrameTime(int numFrames) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int frame = 0; frame < numFrames; frame++) {
            for (int scanline = 0; scanline < 160; scanline++) {
                // Set VCOUNT to simulate scanline progression
                memory->write16(0x04000006u, scanline);
                gpu->renderScanline();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        
        return duration.count() / numFrames;  // Average time per frame
    }
};

// Test 1: Backdrop Only (Baseline)
TEST_F(PerformanceTest, Perf_BackdropOnly) {
    // Set backdrop color
    memory->write16(PALETTE_RAM_BASE, 0x001Fu);  // Red backdrop
    
    // Disable all BGs
    memory->write16(REG_DISPCNT, 0x0000);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Backdrop-only rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Backdrop Only] Average frame time: %.3f ms\n", avgTime);
}

// Test 2: Single BG 4bpp
TEST_F(PerformanceTest, Perf_SingleBG4bpp) {
    setupBG(0, 0, false, 1);
    
    // Enable BG0
    memory->write16(REG_DISPCNT, 0x0100);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Single BG 4bpp rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Single BG 4bpp] Average frame time: %.3f ms\n", avgTime);
}

// Test 3: Single BG 8bpp
TEST_F(PerformanceTest, Perf_SingleBG8bpp) {
    setupBG(0, 0, true, 1);
    
    // Enable BG0
    memory->write16(REG_DISPCNT, 0x0100);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Single BG 8bpp rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Single BG 8bpp] Average frame time: %.3f ms\n", avgTime);
}

// Test 4: Two BGs (4bpp)
TEST_F(PerformanceTest, Perf_TwoBGs4bpp) {
    setupBG(0, 0, false, 1);
    setupBG(1, 1, false, 2);
    
    // Enable BG0 and BG1
    memory->write16(REG_DISPCNT, 0x0300);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Two BGs 4bpp rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Two BGs 4bpp] Average frame time: %.3f ms\n", avgTime);
}

// Test 5: Four BGs (4bpp) - Worst Case
TEST_F(PerformanceTest, Perf_FourBGs4bpp) {
    setupBG(0, 0, false, 1);
    setupBG(1, 1, false, 2);
    setupBG(2, 2, false, 3);
    setupBG(3, 3, false, 4);
    
    // Enable all BGs
    memory->write16(REG_DISPCNT, 0x0F00);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Four BGs 4bpp rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Four BGs 4bpp] Average frame time: %.3f ms\n", avgTime);
}

// Test 6: Four BGs (mixed 4bpp/8bpp)
TEST_F(PerformanceTest, Perf_FourBGsMixed) {
    setupBG(0, 0, false, 1);  // 4bpp
    setupBG(1, 1, true, 2);   // 8bpp
    setupBG(2, 2, false, 3);  // 4bpp
    setupBG(3, 3, true, 4);   // 8bpp
    
    // Enable all BGs
    memory->write16(REG_DISPCNT, 0x0F00);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Four BGs mixed rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Four BGs Mixed] Average frame time: %.3f ms\n", avgTime);
}

// Test 7: Four BGs with Scrolling
TEST_F(PerformanceTest, Perf_FourBGsScrolling) {
    setupBG(0, 0, false, 1);
    setupBG(1, 1, false, 2);
    setupBG(2, 2, false, 3);
    setupBG(3, 3, false, 4);
    
    // Set scrolling for variety
    memory->write16(REG_BG0HOFS, 4);
    memory->write16(REG_BG0VOFS, 8);
    memory->write16(REG_BG1HOFS, 16);
    memory->write16(REG_BG1VOFS, 24);
    memory->write16(REG_BG2HOFS, 32);
    memory->write16(REG_BG2VOFS, 40);
    memory->write16(REG_BG3HOFS, 48);
    memory->write16(REG_BG3VOFS, 56);
    
    // Enable all BGs
    memory->write16(REG_DISPCNT, 0x0F00);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Four BGs with scrolling took " << avgTime << "ms (should be <1ms)";
    printf("  [Four BGs Scrolling] Average frame time: %.3f ms\n", avgTime);
}

// Test 8: Forced Blank (should be fastest)
TEST_F(PerformanceTest, Perf_ForcedBlank) {
    setupBG(0, 0, false, 1);
    setupBG(1, 1, false, 2);
    setupBG(2, 2, false, 3);
    setupBG(3, 3, false, 4);
    
    // Enable all BGs + forced blank
    memory->write16(REG_DISPCNT, 0x8F00);  // Bit 7 = forced blank
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 0.5) << "Forced blank took " << avgTime << "ms (should be <0.5ms)";
    printf("  [Forced Blank] Average frame time: %.3f ms\n", avgTime);
}

// Test 9: Full Screen Tile Map (30x20 tiles)
TEST_F(PerformanceTest, Perf_FullScreenTilemap) {
    setupBG(0, 0, false, 1);
    
    // Fill entire screen with tiles
    uint8_t* vram = memory->getVRAM();
    uint32_t tilemapAddr = 0x06007000u;
    uint32_t tilemapOffset = tilemapAddr - 0x06000000u;
    
    // Create multiple different tiles
    for (int tileNum = 1; tileNum <= 32; tileNum++) {
        uint32_t tileAddr = 0x06000000u + (tileNum * 32u);
        uint32_t offset = tileAddr - 0x06000000u;
        
        for (int i = 0; i < 32; i++) {
            vram[offset + i] = (uint8_t)(((tileNum + i) & 0x0F) | (((tileNum + i + 1) & 0x0F) << 4));
        }
    }
    
    // Fill 30x20 tilemap (full screen)
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 30; x++) {
            int tileNum = 1 + ((x + y) % 32);
            int paletteNum = (x + y) % 4;
            uint16_t entry = (uint16_t)(tileNum | (paletteNum << 12));
            
            uint32_t addr = tilemapOffset + (y * 32 + x) * 2;
            vram[addr] = entry & 0xFFu;
            vram[addr + 1] = (entry >> 8) & 0xFFu;
        }
    }
    
    // Enable BG0
    memory->write16(REG_DISPCNT, 0x0100);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Full screen tilemap rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Full Screen Tilemap] Average frame time: %.3f ms\n", avgTime);
}

// Test 10: Stress Test - Complex Scene
TEST_F(PerformanceTest, Perf_ComplexScene) {
    // Setup all 4 BGs with different configs
    setupBG(0, 0, false, 1);
    setupBG(1, 1, true, 2);
    setupBG(2, 2, false, 3);
    setupBG(3, 3, true, 4);
    
    // Fill each BG's tilemap
    uint8_t* vram = memory->getVRAM();
    
    for (int bgNum = 0; bgNum < 4; bgNum++) {
        uint32_t tilemapAddr = 0x06007000u + (bgNum * 0x800u);
        uint32_t tilemapOffset = tilemapAddr - 0x06000000u;
        
        for (int i = 0; i < 32 * 32; i++) {
            uint16_t entry = (uint16_t)((bgNum + 1) | ((i % 16) << 12));
            vram[tilemapOffset + i * 2] = entry & 0xFFu;
            vram[tilemapOffset + i * 2 + 1] = (entry >> 8) & 0xFFu;
        }
    }
    
    // Add scrolling
    memory->write16(REG_BG0HOFS, 7);
    memory->write16(REG_BG0VOFS, 3);
    memory->write16(REG_BG1HOFS, 13);
    memory->write16(REG_BG1VOFS, 19);
    memory->write16(REG_BG2HOFS, 29);
    memory->write16(REG_BG2VOFS, 37);
    memory->write16(REG_BG3HOFS, 53);
    memory->write16(REG_BG3VOFS, 61);
    
    // Enable all BGs
    memory->write16(REG_DISPCNT, 0x0F00);
    
    double avgTime = measureFrameTime(100);
    
    EXPECT_LT(avgTime, 1.0) << "Complex scene rendering took " << avgTime << "ms (should be <1ms)";
    printf("  [Complex Scene] Average frame time: %.3f ms\n", avgTime);
}
