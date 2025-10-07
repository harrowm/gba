/**
 * Day 4 Session 2: Background Priority System Tests
 * 
 * Tests for proper priority-based compositing of multiple backgrounds.
 * 
 * Priority Rules:
 * 1. Lower priority value = higher priority (0 is highest, 3 is lowest)
 * 2. When priorities match, lower BG number wins (BG0 > BG1 > BG2 > BG3)
 * 3. Transparent pixels (palette index 0) don't draw, show next layer
 * 4. Backdrop color has lowest priority (below all BGs)
 * 
 * Test Coverage (18 tests):
 * - Basic priority levels (4 tests)
 * - BG number tiebreaker (4 tests)
 * - Transparent pixel handling (4 tests)
 * - Complex multi-layer scenes (4 tests)
 * - Edge cases (2 tests)
 */

#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

class PriorityTest : public ::testing::Test {
protected:
    GBA* gba;
    GPU* gpu;
    Memory* memory;
    
    // Register addresses
    static constexpr uint32_t VRAM_BASE = 0x06000000;
    static constexpr uint32_t PALETTE_RAM_BASE = 0x05000000;
    static constexpr uint32_t REG_DISPCNT = 0x04000000;
    static constexpr uint32_t REG_BG0CNT = 0x04000008;
    static constexpr uint32_t REG_BG1CNT = 0x0400000A;
    static constexpr uint32_t REG_BG2CNT = 0x0400000C;
    static constexpr uint32_t REG_BG3CNT = 0x0400000E;
    
    // DISPCNT bits
    static constexpr uint16_t DISPCNT_MODE_0 = 0x0000;
    static constexpr uint16_t DISPCNT_BG0 = 0x0100;
    static constexpr uint16_t DISPCNT_BG1 = 0x0200;
    static constexpr uint16_t DISPCNT_BG2 = 0x0400;
    static constexpr uint16_t DISPCNT_BG3 = 0x0800;
    
    void SetUp() override {
        gba = new GBA(false);
        gpu = &gba->getGPU();
        memory = &gba->getMemory();
        
        // Initialize with Mode 0
        memory->write16(REG_DISPCNT, DISPCNT_MODE_0);
        
        // Setup backdrop to black
        memory->write16(PALETTE_RAM_BASE, 0x0000);
    }
    
    void TearDown() override {
        delete gba;
    }
    
    /**
     * Setup a background with a solid color tile
     * @param bgNum Background number (0-3)
     * @param priority Priority level (0-3, 0=highest)
     * @param color RGB555 color for the tile
     * @param screenBase Screen base block
     * @param charBase Character base block
     */
    void setupBGWithPriority(int bgNum, uint8_t priority, uint16_t color, 
                              uint16_t screenBase, uint16_t charBase) {
        // Setup BGxCNT with priority in bits 0-1
        uint32_t bgcntReg = REG_BG0CNT + (bgNum * 2);
        uint16_t bgcnt = priority | (charBase << 2) | (screenBase << 8);
        memory->write16(bgcntReg, bgcnt);
        
        // Use different palette indices for different BGs to avoid conflicts
        uint8_t paletteIndex = bgNum + 1; // BG0=1, BG1=2, BG2=3, BG3=4
        
        // Setup tile data (tile bgNum+1, solid color with unique palette index)
        uint32_t tileNum = bgNum + 1;
        uint32_t tileAddr = VRAM_BASE + (charBase * 0x4000) + (tileNum * 32);
        uint8_t pixelData = (paletteIndex & 0x0F) | ((paletteIndex & 0x0F) << 4);
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, pixelData);
        }
        
        // Setup tilemap (tile 0,0 uses tile bgNum+1)
        uint32_t tilemapAddr = VRAM_BASE + (screenBase * 0x800);
        memory->write16(tilemapAddr, tileNum); // Tile number, palette 0
        
        // Setup palette color at unique index
        uint32_t paletteAddr = PALETTE_RAM_BASE + (paletteIndex * 2);
        memory->write16(paletteAddr, color);
    }
    
    /**
     * Setup a background with transparent tile (tile 0)
     */
    void setupBGTransparent(int bgNum, uint8_t priority, uint16_t screenBase, uint16_t charBase) {
        uint32_t bgcntReg = REG_BG0CNT + (bgNum * 2);
        uint16_t bgcnt = priority | (charBase << 2) | (screenBase << 8);
        memory->write16(bgcntReg, bgcnt);
        
        // Setup tilemap with tile 0 (transparent)
        uint32_t tilemapAddr = VRAM_BASE + (screenBase * 0x800);
        memory->write16(tilemapAddr, 0);
    }
    
    /**
     * Setup a background with a checkerboard pattern (transparent and solid alternating)
     */
    void setupBGCheckerboard(int bgNum, uint8_t priority, uint16_t color,
                              uint16_t screenBase, uint16_t charBase) {
        uint32_t bgcntReg = REG_BG0CNT + (bgNum * 2);
        uint16_t bgcnt = priority | (charBase << 2) | (screenBase << 8);
        memory->write16(bgcntReg, bgcnt);
        
        // Setup tile 1 (solid color)
        uint32_t tileAddr = VRAM_BASE + (charBase * 0x4000) + (1 * 32);
        uint8_t pixelData = 0x11; // Palette index 1
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, pixelData);
        }
        
        // Setup palette
        uint32_t paletteAddr = PALETTE_RAM_BASE + 2;
        memory->write16(paletteAddr, color);
        
        // Setup tilemap: alternating tile 0 and tile 1
        uint32_t tilemapAddr = VRAM_BASE + (screenBase * 0x800);
        for (int x = 0; x < 32; x++) {
            memory->write16(tilemapAddr + x * 2, (x % 2 == 0) ? 0 : 1);
        }
    }
    
    const uint16_t* getFramebuffer() {
        return gpu->getTiledFramebuffer();
    }
};

/**
 * Test Suite 1: Basic Priority Levels (4 tests)
 */

TEST_F(PriorityTest, Priority_Level0BeatsLevel1) {
    // BG0 with priority 0 (red), BG1 with priority 1 (green)
    setupBGWithPriority(0, 0, 0x001F, 30, 0); // Red, priority 0
    setupBGWithPriority(1, 1, 0x03E0, 29, 0); // Green, priority 1
    
    // Enable both backgrounds at the same position
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 (priority 0) should win, showing red
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (priority 0 wins)";
    }
}

TEST_F(PriorityTest, Priority_Level1BeatsLevel2) {
    // BG0 with priority 1 (red), BG1 with priority 2 (green)
    setupBGWithPriority(0, 1, 0x001F, 30, 0); // Red, priority 1
    setupBGWithPriority(1, 2, 0x03E0, 29, 0); // Green, priority 2
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 (priority 1) should win
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (priority 1 wins)";
    }
}

TEST_F(PriorityTest, Priority_Level2BeatsLevel3) {
    // BG0 with priority 2 (red), BG1 with priority 3 (green)
    setupBGWithPriority(0, 2, 0x001F, 30, 0); // Red, priority 2
    setupBGWithPriority(1, 3, 0x03E0, 29, 0); // Green, priority 3
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 (priority 2) should win
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (priority 2 wins)";
    }
}

TEST_F(PriorityTest, Priority_Level3BeatsBackdrop) {
    // BG0 with priority 3 (red), backdrop is black
    setupBGWithPriority(0, 3, 0x001F, 30, 0); // Red, priority 3
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 (priority 3) should beat backdrop
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (priority 3 beats backdrop)";
    }
    
    // After tile, should show backdrop
    for (int x = 8; x < 16; x++) {
        EXPECT_EQ(fb[x], 0x0000) << "Pixel " << x << " should be black (backdrop)";
    }
}

/**
 * Test Suite 2: BG Number Tiebreaker (4 tests)
 */

TEST_F(PriorityTest, Tiebreaker_BG0BeatsBG1) {
    // Both BG0 and BG1 have priority 1
    setupBGWithPriority(0, 1, 0x001F, 30, 0); // Red, priority 1
    setupBGWithPriority(1, 1, 0x03E0, 29, 0); // Green, priority 1
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 should win the tiebreaker
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0 wins tiebreaker)";
    }
}

TEST_F(PriorityTest, Tiebreaker_BG1BeatsBG2) {
    // Both BG1 and BG2 have priority 2
    setupBGWithPriority(1, 2, 0x001F, 29, 0); // Red, priority 2
    setupBGWithPriority(2, 2, 0x03E0, 28, 0); // Green, priority 2
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG1 | DISPCNT_BG2);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG1 should win
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG1 wins tiebreaker)";
    }
}

TEST_F(PriorityTest, Tiebreaker_BG2BeatsBG3) {
    // Both BG2 and BG3 have priority 3
    setupBGWithPriority(2, 3, 0x001F, 28, 0); // Red, priority 3
    setupBGWithPriority(3, 3, 0x03E0, 27, 0); // Green, priority 3
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG2 | DISPCNT_BG3);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG2 should win
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG2 wins tiebreaker)";
    }
}

TEST_F(PriorityTest, Tiebreaker_AllFourSamePriority) {
    // All four BGs have priority 1
    setupBGWithPriority(0, 1, 0x001F, 30, 0); // Red
    setupBGWithPriority(1, 1, 0x03E0, 29, 0); // Green
    setupBGWithPriority(2, 1, 0x7C00, 28, 0); // Blue
    setupBGWithPriority(3, 1, 0x7FFF, 27, 0); // White
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1 | DISPCNT_BG2 | DISPCNT_BG3);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 should win (lowest BG number)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0 wins tiebreaker)";
    }
}

/**
 * Test Suite 3: Transparent Pixel Handling (4 tests)
 */

TEST_F(PriorityTest, Transparent_HighPriorityTransparentShowsLower) {
    // BG0 priority 0 but transparent, BG1 priority 1 solid
    setupBGTransparent(0, 0, 30, 0);
    setupBGWithPriority(1, 1, 0x03E0, 29, 0); // Green
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // Should show BG1 since BG0 is transparent
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x03E0) << "Pixel " << x << " should be green (BG0 transparent)";
    }
}

TEST_F(PriorityTest, Transparent_BothTransparentShowsBackdrop) {
    // Both BG0 and BG1 transparent
    setupBGTransparent(0, 0, 30, 0);
    setupBGTransparent(1, 1, 29, 0);
    
    // Set backdrop to cyan
    memory->write16(PALETTE_RAM_BASE, 0x7FE0);
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // Should show backdrop (cyan)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x7FE0) << "Pixel " << x << " should be cyan (backdrop)";
    }
}

TEST_F(PriorityTest, Transparent_LowerPrioritySolidBeatsBackdrop) {
    // BG0 priority 0 transparent, BG1 priority 3 solid
    setupBGTransparent(0, 0, 30, 0);
    setupBGWithPriority(1, 3, 0x001F, 29, 0); // Red
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // Should show BG1 (even though lower priority, it's the only solid layer)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG1 solid)";
    }
}

TEST_F(PriorityTest, Transparent_CheckerboardCompositing) {
    // BG0 checkerboard (transparent/red), BG1 solid green
    setupBGCheckerboard(0, 0, 0x001F, 30, 0); // Red checkerboard, priority 0
    setupBGWithPriority(1, 1, 0x03E0, 29, 0);  // Green solid, priority 1
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // First tile (0-7): transparent, should show green from BG1
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x03E0) << "Pixel " << x << " should be green (BG0 transparent)";
    }
    
    // Second tile (8-15): red from BG0
    for (int x = 8; x < 16; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0 solid)";
    }
}

/**
 * Test Suite 4: Complex Multi-Layer (4 tests)
 */

TEST_F(PriorityTest, Complex_ThreeLayers) {
    // BG0: priority 0, red
    // BG1: priority 1, green
    // BG2: priority 2, blue
    setupBGWithPriority(0, 0, 0x001F, 30, 0); // Red
    setupBGWithPriority(1, 1, 0x03E0, 29, 0); // Green
    setupBGWithPriority(2, 2, 0x7C00, 28, 0); // Blue
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1 | DISPCNT_BG2);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 (highest priority) should win
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0 highest priority)";
    }
}

TEST_F(PriorityTest, Complex_FourLayersMixed) {
    // BG0: priority 2, red
    // BG1: priority 0, green (highest)
    // BG2: priority 1, blue
    // BG3: priority 3, white (lowest)
    setupBGWithPriority(0, 2, 0x001F, 30, 0);
    setupBGWithPriority(1, 0, 0x03E0, 29, 0); // Highest priority
    setupBGWithPriority(2, 1, 0x7C00, 28, 0);
    setupBGWithPriority(3, 3, 0x7FFF, 27, 0);
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1 | DISPCNT_BG2 | DISPCNT_BG3);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG1 has priority 0 (highest), should win
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x03E0) << "Pixel " << x << " should be green (BG1 priority 0)";
    }
}

TEST_F(PriorityTest, Complex_PartialOverlap) {
    // Setup BG0 and BG1 at different horizontal positions
    // This tests that priority works per-pixel, not per-layer
    
    // BG0: priority 1, red at tile X=0
    setupBGWithPriority(0, 1, 0x001F, 30, 0);
    
    // BG1: priority 0, green
    // setupBGWithPriority creates tile 2 for BG1 (bgNum+1)
    setupBGWithPriority(1, 0, 0x03E0, 29, 0);
    
    // Modify BG1's tilemap: tile 0 transparent at X=0, tile 2 (BG1's tile) at X=1
    uint32_t tilemapAddr1 = VRAM_BASE + (29 * 0x800);
    memory->write16(tilemapAddr1, 0); // Tile 0 transparent at X=0
    memory->write16(tilemapAddr1 + 2, 2); // Tile 2 (BG1's tile) at X=1
    
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // Pixels 0-7: BG1 transparent, should show BG0 red
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG1 transparent)";
    }
    
    // Pixels 8-15: BG1 solid green (priority 0), should show green
    for (int x = 8; x < 16; x++) {
        EXPECT_EQ(fb[x], 0x03E0) << "Pixel " << x << " should be green (BG1 priority 0)";
    }
}

TEST_F(PriorityTest, Complex_SelectiveDisable) {
    // Setup all four BGs with different priorities
    setupBGWithPriority(0, 3, 0x001F, 30, 0); // Red, priority 3
    setupBGWithPriority(1, 2, 0x03E0, 29, 0); // Green, priority 2
    setupBGWithPriority(2, 1, 0x7C00, 28, 0); // Blue, priority 1
    setupBGWithPriority(3, 0, 0x7FFF, 27, 0); // White, priority 0 (highest)
    
    // Enable only BG0, BG1, BG2 (disable BG3 which has highest priority)
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1 | DISPCNT_BG2);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG2 should win (priority 1, BG3 disabled)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x7C00) << "Pixel " << x << " should be blue (BG2 priority 1)";
    }
}

/**
 * Test Suite 5: Edge Cases (2 tests)
 */

TEST_F(PriorityTest, EdgeCase_OnlyBackdrop) {
    // No backgrounds enabled
    memory->write16(PALETTE_RAM_BASE, 0x7C00); // Blue backdrop
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // Should show only backdrop
    for (int x = 0; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x7C00) << "Pixel " << x << " should be blue (backdrop only)";
    }
}

TEST_F(PriorityTest, EdgeCase_AllSamePriorityReverseBGOrder) {
    // All BGs priority 2, but enable in reverse order
    setupBGWithPriority(0, 2, 0x001F, 30, 0); // Red
    setupBGWithPriority(1, 2, 0x03E0, 29, 0); // Green
    setupBGWithPriority(2, 2, 0x7C00, 28, 0); // Blue
    setupBGWithPriority(3, 2, 0x7FFF, 27, 0); // White
    
    // Enable in order BG3, BG2, BG1, BG0
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG3 | DISPCNT_BG2 | DISPCNT_BG1 | DISPCNT_BG0);
    
    gpu->renderScanline();
    const uint16_t* fb = getFramebuffer();
    
    // BG0 should still win (tiebreaker is BG number, not enable order)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0 wins regardless of enable order)";
    }
}
