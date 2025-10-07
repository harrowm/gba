/**
 * Day 4 Session 1: Main Render Loop Integration Tests
 * 
 * Tests for Mode 0 rendering with multiple backgrounds, backdrop color,
 * and forced blank handling.
 * 
 * Test Coverage (15 tests):
 * - Backdrop color rendering (3 tests)
 * - Forced blank handling (2 tests)
 * - Single background rendering (4 tests)
 * - Multiple background compositing (4 tests)
 * - Mode switching (2 tests)
 */

#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

class Mode0Test : public ::testing::Test {
protected:
    GBA* gba;
    GPU* gpu;
    Memory* memory;
    
    // VRAM and Palette RAM base addresses
    static constexpr uint32_t VRAM_BASE = 0x06000000;
    static constexpr uint32_t PALETTE_RAM_BASE = 0x05000000;
    
    // Register addresses
    static constexpr uint32_t REG_DISPCNT = 0x04000000;
    static constexpr uint32_t REG_BG0CNT = 0x04000008;
    static constexpr uint32_t REG_BG1CNT = 0x0400000A;
    static constexpr uint32_t REG_BG2CNT = 0x0400000C;
    static constexpr uint32_t REG_BG3CNT = 0x0400000E;
    static constexpr uint32_t REG_BG0HOFS = 0x04000010;
    static constexpr uint32_t REG_BG0VOFS = 0x04000012;
    
    // DISPCNT bits
    static constexpr uint16_t DISPCNT_MODE_0 = 0x0000;
    static constexpr uint16_t DISPCNT_BG0 = 0x0100;
    static constexpr uint16_t DISPCNT_BG1 = 0x0200;
    static constexpr uint16_t DISPCNT_BG2 = 0x0400;
    static constexpr uint16_t DISPCNT_BG3 = 0x0800;
    static constexpr uint16_t DISPCNT_FORCED_BLANK = 0x0080;
    
    void SetUp() override {
        gba = new GBA(false);  // Full memory map
        gpu = &gba->getGPU();
        memory = &gba->getMemory();
        
        // Initialize with Mode 0
        memory->write16(REG_DISPCNT, DISPCNT_MODE_0);
    }
    
    void TearDown() override {
        delete gba;
    }
    
    /**
     * Helper to set backdrop color (palette RAM index 0)
     */
    void setBackdropColor(uint16_t color) {
        memory->write16(PALETTE_RAM_BASE, color);
    }
    
    /**
     * Helper to setup a simple 4bpp tile with solid color
     */
    void setupSimpleTile(uint32_t tileAddr, uint8_t paletteIndex) {
        // Each pixel is 4 bits, 2 pixels per byte
        uint8_t pixelData = (paletteIndex & 0x0F) | ((paletteIndex & 0x0F) << 4);
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, pixelData);
        }
    }
    
    /**
     * Helper to setup a tilemap entry
     */
    void setupTilemapEntry(uint32_t tilemapAddr, int tileX, int tileY, uint16_t entry) {
        uint32_t entryAddr = tilemapAddr + (tileY * 32 + tileX) * 2;
        memory->write16(entryAddr, entry);
    }
    
    /**
     * Helper to setup a BG configuration
     */
    void setupBG(int bgNum, uint16_t screenBase, uint16_t charBase, uint8_t paletteNum) {
        uint32_t bgcntReg = REG_BG0CNT + (bgNum * 2);
        // BGxCNT: screen base in bits 8-12, char base in bits 2-3, palette mode bit 7 = 0 (4bpp)
        uint16_t bgcnt = (screenBase << 8) | (charBase << 2);
        memory->write16(bgcntReg, bgcnt);
        
        // Setup tile in character base
        uint32_t tileAddr = VRAM_BASE + (charBase * 0x4000) + (1 * 32); // Tile 1
        setupSimpleTile(tileAddr, paletteNum * 16 + 1); // Use color 1 of palette
        
        // Setup tilemap at screen base
        uint32_t tilemapAddr = VRAM_BASE + (screenBase * 0x800);
        // Top-left tile (0,0) uses tile 1 with this palette
        setupTilemapEntry(tilemapAddr, 0, 0, (paletteNum << 12) | 1);
        
        // Setup palette color
        uint32_t paletteAddr = PALETTE_RAM_BASE + (paletteNum * 16 + 1) * 2;
        memory->write16(paletteAddr, 0x001F); // Red
    }
    
    /**
     * Get the framebuffer from GPU for inspection
     */
    const uint16_t* getFramebuffer() {
        return gpu->getTiledFramebuffer();
    }
};

/**
 * Test Suite 1: Backdrop Color (3 tests)
 */

TEST_F(Mode0Test, BackdropColor_Black) {
    // Set backdrop to black (0x0000)
    setBackdropColor(0x0000);
    
    // No backgrounds enabled
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // All pixels should be black
    for (int x = 0; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x0000) << "Pixel " << x << " should be black";
    }
}

TEST_F(Mode0Test, BackdropColor_White) {
    // Set backdrop to white (0x7FFF)
    setBackdropColor(0x7FFF);
    
    // No backgrounds enabled
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // All pixels should be white
    for (int x = 0; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x7FFF) << "Pixel " << x << " should be white";
    }
}

TEST_F(Mode0Test, BackdropColor_Custom) {
    // Set backdrop to cyan (0x7FE0 = max green + max blue)
    setBackdropColor(0x7FE0);
    
    // No backgrounds enabled
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // All pixels should be cyan
    for (int x = 0; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x7FE0) << "Pixel " << x << " should be cyan";
    }
}

/**
 * Test Suite 2: Forced Blank (2 tests)
 */

TEST_F(Mode0Test, ForcedBlank_White) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Enable forced blank (bit 7)
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_FORCED_BLANK);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // All pixels should be white (forced blank overrides backdrop)
    for (int x = 0; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x7FFF) << "Pixel " << x << " should be white (forced blank)";
    }
}

TEST_F(Mode0Test, ForcedBlank_OverridesBG) {
    // Setup BG0 with red tiles
    setupBG(0, 30, 0, 0);
    
    // Enable BG0 with forced blank
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_FORCED_BLANK);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // All pixels should be white (forced blank overrides everything)
    for (int x = 0; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x7FFF) << "Pixel " << x << " should be white (forced blank)";
    }
}

/**
 * Test Suite 3: Single Background (4 tests)
 */

TEST_F(Mode0Test, SingleBG_BG0Only) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup BG0 with red tile at position (0,0)
    setupBG(0, 30, 0, 0);
    
    // Enable only BG0
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // First 8 pixels should be red (tile 0)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
    
    // Remaining pixels should be backdrop (black)
    for (int x = 8; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x0000) << "Pixel " << x << " should be black (backdrop)";
    }
}

TEST_F(Mode0Test, SingleBG_BG1Only) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup BG1 with red tile
    setupBG(1, 29, 0, 1);
    
    // Enable only BG1
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG1);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // First 8 pixels should be red
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
}

TEST_F(Mode0Test, SingleBG_BG2Only) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup BG2 with red tile
    setupBG(2, 28, 0, 2);
    
    // Enable only BG2
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG2);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // First 8 pixels should be red
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
}

TEST_F(Mode0Test, SingleBG_BG3Only) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup BG3 with red tile
    setupBG(3, 27, 0, 3);
    
    // Enable only BG3
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG3);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // First 8 pixels should be red
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
}

/**
 * Test Suite 4: Multiple Backgrounds (4 tests)
 * Note: These tests will show simple overwriting behavior.
 * Proper priority handling will be tested in Day 4 Session 2.
 */

TEST_F(Mode0Test, MultipleBG_TwoBackgrounds) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup BG0 with red tile at (0,0)
    setupBG(0, 30, 0, 0);
    
    // Setup BG1 with different color at (1,0) - tile X position 1
    uint32_t tileAddr = VRAM_BASE + (0 * 0x4000) + (2 * 32); // Tile 2
    setupSimpleTile(tileAddr, 1 * 16 + 2); // Palette 1, color 2
    uint32_t tilemapAddr = VRAM_BASE + (29 * 0x800);
    setupTilemapEntry(tilemapAddr, 1, 0, (1 << 12) | 2); // Tile 2, palette 1
    uint32_t paletteAddr = PALETTE_RAM_BASE + (1 * 16 + 2) * 2;
    memory->write16(paletteAddr, 0x03E0); // Green
    
    setupBG(1, 29, 0, 1);
    
    // Enable both BG0 and BG1
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // First 8 pixels should be red (BG0)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0)";
    }
    
    // Next 8 pixels should be green (BG1)
    for (int x = 8; x < 16; x++) {
        EXPECT_EQ(fb[x], 0x03E0) << "Pixel " << x << " should be green (BG1)";
    }
}

TEST_F(Mode0Test, MultipleBG_ThreeBackgrounds) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup BG0, BG1, BG2 with tiles at different X positions
    setupBG(0, 30, 0, 0); // Red at X=0
    setupBG(1, 29, 0, 1); // Red at X=0 (will be overwritten by BG0)
    setupBG(2, 28, 0, 2); // Red at X=0 (will be overwritten by BG1 and BG0)
    
    // Enable all three
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1 | DISPCNT_BG2);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // Since all three BGs have tiles at X=0, BG0 should win (rendered last)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0 wins)";
    }
}

TEST_F(Mode0Test, MultipleBG_AllFourBackgrounds) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup all four BGs
    setupBG(0, 30, 0, 0);
    setupBG(1, 29, 0, 1);
    setupBG(2, 28, 0, 2);
    setupBG(3, 27, 0, 3);
    
    // Enable all four
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0 | DISPCNT_BG1 | DISPCNT_BG2 | DISPCNT_BG3);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // BG0 should win (rendered last)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0 wins)";
    }
}

TEST_F(Mode0Test, MultipleBG_SelectiveEnable) {
    // Set backdrop to black
    setBackdropColor(0x0000);
    
    // Setup all four BGs
    setupBG(0, 30, 0, 0);
    setupBG(1, 29, 0, 1);
    setupBG(2, 28, 0, 2);
    setupBG(3, 27, 0, 3);
    
    // Enable only BG1 and BG3 (skip BG0 and BG2)
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG1 | DISPCNT_BG3);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // BG1 should win (BG0 is disabled, BG1 rendered after BG3)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG1 wins, BG0 disabled)";
    }
}

/**
 * Test Suite 5: Mode Switching (2 tests)
 */

TEST_F(Mode0Test, ModeSwitching_Mode0ToMode3) {
    // Setup Mode 0 with BG0
    setupBG(0, 30, 0, 0);
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0);
    
    // Render scanline 0 in Mode 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // First 8 pixels should be red in tiled framebuffer
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " in Mode 0 should be red";
    }
    
    // Switch to Mode 3
    memory->write16(REG_DISPCNT, 0x0003 | DISPCNT_BG2); // Mode 3 with BG2
    
    // Write some pixels to Mode 3 framebuffer (which is in VRAM, not tiledFramebuffer)
    uint32_t mode3FB = VRAM_BASE;
    memory->write16(mode3FB, 0x03E0); // Green pixel at (0,0)
    
    // Render scanline 0 in Mode 3 (this doesn't modify tiledFramebuffer in Mode 3)
    gpu->renderScanline();
    
    // In Mode 3, getFrameBuffer() returns VRAM directly, not tiledFramebuffer
    uint16_t* mode3Buffer = gpu->getFrameBuffer();
    EXPECT_EQ(mode3Buffer[0], 0x03E0) << "Pixel 0 in Mode 3 should be green";
    
    // tiledFramebuffer should still have the old Mode 0 content
    EXPECT_EQ(fb[0], 0x001F) << "Tiled framebuffer pixel 0 should still be red";
}

TEST_F(Mode0Test, ModeSwitching_DisableAllBackgrounds) {
    // Setup BG0
    setupBG(0, 30, 0, 0);
    
    // Set backdrop to blue
    setBackdropColor(0x7C00); // Blue
    
    // Enable BG0
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG0);
    
    // Render scanline 0
    gpu->renderScanline();
    
    const uint16_t* fb = getFramebuffer();
    
    // First 8 pixels should be red (BG0)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (BG0)";
    }
    
    // Disable all backgrounds
    memory->write16(REG_DISPCNT, DISPCNT_MODE_0);
    
    // Render scanline 0 again
    gpu->renderScanline();
    
    // All pixels should now be blue (backdrop)
    for (int x = 0; x < 240; x++) {
        EXPECT_EQ(fb[x], 0x7C00) << "Pixel " << x << " should be blue (backdrop)";
    }
}
