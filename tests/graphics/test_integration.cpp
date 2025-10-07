/**
 * test_integration.cpp
 * 
 * Day 4 Session 4: Integration Testing & Visual Verification
 * 
 * Comprehensive integration tests for Mode 0 rendering with realistic scenes.
 * Tests complex interactions, edge cases, and visual correctness.
 */

#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"
#include <vector>
#include <cstring>

class IntegrationTest : public ::testing::Test {
protected:
    GBA* gba;
    Memory* memory;
    GPU* gpu;
    
    // Register addresses
    static constexpr uint32_t PALETTE_RAM_BASE = 0x05000000u;
    static constexpr uint32_t VRAM_BASE = 0x06000000u;
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
        gba = new GBA(false);
        memory = &gba->getMemory();
        gpu = &gba->getGPU();
        
        // Initialize to Mode 0
        memory->write16(REG_DISPCNT, 0x0000);
    }
    
    void TearDown() override {
        delete gba;
    }
    
    // Helper to create a tile with specific pattern
    void createTile(int tileNum, uint8_t pattern[64]) {
        uint8_t* vram = memory->getVRAM();
        uint32_t tileAddr = tileNum * 32;  // 4bpp tiles are 32 bytes
        
        for (int i = 0; i < 32; i++) {
            vram[tileAddr + i] = (pattern[i * 2] & 0x0Fu) | ((pattern[i * 2 + 1] & 0x0Fu) << 4);
        }
    }
    
    // Helper to create an 8bpp tile
    void createTile8bpp(int tileNum, uint8_t pattern[64]) {
        uint8_t* vram = memory->getVRAM();
        uint32_t tileAddr = tileNum * 64;  // 8bpp tiles are 64 bytes
        
        for (int i = 0; i < 64; i++) {
            vram[tileAddr + i] = pattern[i];
        }
    }
    
    // Helper to setup palette colors
    void setupPalette(int paletteNum, const std::vector<uint16_t>& colors) {
        uint8_t* paletteRAM = memory->getPaletteRAM();
        for (size_t i = 0; i < colors.size() && i < 16; i++) {
            uint32_t addr = (paletteNum * 16 + i) * 2;
            paletteRAM[addr] = colors[i] & 0xFFu;
            paletteRAM[addr + 1] = (colors[i] >> 8) & 0xFFu;
        }
    }
    
    // Helper to setup BG config (use screen base 28+bgNum for each BG)
    void setupBGConfig(int bgNum, uint8_t priority, bool is8bpp = false, uint8_t screenSize = 0) {
        uint16_t config = priority | ((28u + bgNum) << 8) | (screenSize << 14);
        if (is8bpp) {
            config |= 0x0080u;  // Bit 7 for 8bpp
        }
        memory->write16(REG_BG0CNT + bgNum * 2, config);
    }
    
    // Helper to fill tilemap at screen base 28+bgNum
    void fillTilemap(int bgNum, const std::vector<uint16_t>& entries) {
        uint8_t* vram = memory->getVRAM();
        // Screen base 28+bgNum = different location for each BG
        uint32_t tilemapBase = 0xE000u + (bgNum * 0x800u);
        
        for (size_t i = 0; i < entries.size() && i < 1024; i++) {
            uint32_t addr = tilemapBase + i * 2;
            vram[addr] = entries[i] & 0xFFu;
            vram[addr + 1] = (entries[i] >> 8) & 0xFFu;
        }
    }
    
    // Helper to render full frame
    void renderFrame() {
        for (int scanline = 0; scanline < 160; scanline++) {
            gpu->setCurrentScanline(scanline);
            gpu->renderScanline();
        }
    }
};

// Test 1: Simple Scene - One background with basic tiles
TEST_F(IntegrationTest, SimpleScene) {
    // Setup BG0 with a simple checkerboard pattern
    setupBGConfig(0, 0);  // BG0, priority 0
    
    // Create two tiles: solid red (tile 1) and solid blue (tile 2)
    uint8_t redTile[64];
    uint8_t blueTile[64];
    std::fill_n(redTile, 64, 1);   // Palette index 1
    std::fill_n(blueTile, 64, 2);  // Palette index 2
    
    createTile(1, redTile);
    createTile(2, blueTile);
    
    // Setup palette
    setupPalette(0, {0x0000, 0x001F, 0x7C00});  // Black, red, blue
    
    // Create 4x4 checkerboard in top-left
    std::vector<uint16_t> tilemap(1024, 0);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            int tileNum = ((x + y) % 2 == 0) ? 1 : 2;
            tilemap[y * 32 + x] = tileNum;
        }
    }
    fillTilemap(0, tilemap);
    
    // Enable BG0
    memory->write16(REG_DISPCNT, 0x0100u);
    
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Check checkerboard pattern in top-left
    // Tile (0,0) at pixel (0,0)
    EXPECT_EQ(fb[0], 0x001Fu) << "Tile (0,0) should be red";
    // Tile (1,0) at pixel (8,0)  
    EXPECT_EQ(fb[8], 0x7C00u) << "Tile (1,0) should be blue";
    // Tile (0,1) at pixel (0,8) = row 8
    EXPECT_EQ(fb[8 * 240], 0x7C00u) << "Tile (0,1) should be blue";
    // Tile (1,1) at pixel (8,8)
    EXPECT_EQ(fb[8 * 240 + 8], 0x001Fu) << "Tile (1,1) should be red";
}

// Test 2: Two Layer Scene - Two backgrounds with different priorities
TEST_F(IntegrationTest, TwoLayerScene) {
    // BG0: Priority 0, foreground
    setupBGConfig(0, 0);
    
    // BG1: Priority 1, background
    setupBGConfig(1, 1);
    
    // Create tiles
    uint8_t solidRed[64], solidBlue[64];
    std::fill_n(solidRed, 64, 1);
    std::fill_n(solidBlue, 64, 1);  // Use palette index 1 (will be blue in palette 1)
    
    createTile(1, solidRed);
    createTile(2, solidBlue);
    
    // Setup palettes (different for each BG)
    setupPalette(0, {0x0000, 0x001F, 0x03E0});  // BG0: black, red, green
    setupPalette(1, {0x0000, 0x7C00, 0x7FFF});  // BG1: black, blue, white
    
    // BG0: Red tile at position (2, 2)
    std::vector<uint16_t> tilemap0(1024, 0);
    tilemap0[2 * 32 + 2] = 1;  // Tile 1, palette 0
    fillTilemap(0, tilemap0);
    
    // BG1: Blue tiles everywhere
    std::vector<uint16_t> tilemap1(1024, 0x1002u);  // Tile 2, palette 1 (tile pixels are value 1 = blue)
    fillTilemap(1, tilemap1);
    
    // Enable both BGs
    memory->write16(REG_DISPCNT, 0x0300u);
    
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Check that BG0 (priority 0) shows on top at (16, 16) (pixel coords of tile 2,2)
    EXPECT_EQ(fb[16 * 240 + 16], 0x001Fu) << "BG0 red should be on top";
    
    // Check that BG1 shows elsewhere
    EXPECT_EQ(fb[0], 0x7C00u) << "BG1 blue should show where BG0 is transparent";
}

// Test 3: Four Layer Scene - All backgrounds enabled
TEST_F(IntegrationTest, FourLayerScene) {
    // Setup all 4 BGs with different priorities
    setupBGConfig(0, 0);
    setupBGConfig(1, 1);
    setupBGConfig(2, 2);
    setupBGConfig(3, 3);
    
    // Create tiles
    uint8_t tile[64];
    for (int i = 1; i <= 4; i++) {
        std::fill_n(tile, 64, i);
        createTile(i, tile);
    }
    
    // Setup palettes
    setupPalette(0, {0x0000, 0x001F, 0x03E0, 0x7C00, 0x7FFF});  // Red, green, blue, white
    
    // Each BG has one tile in a different position
    for (int bg = 0; bg < 4; bg++) {
        std::vector<uint16_t> tilemap(1024, 0);
        tilemap[bg * 32 + bg] = bg + 1;  // Different position for each BG
        fillTilemap(bg, tilemap);
    }
    
    // Enable all BGs
    memory->write16(REG_DISPCNT, 0x0F00u);
    
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Each BG should show in its respective position
    EXPECT_EQ(fb[0], 0x001Fu) << "BG0 at (0,0)";
    EXPECT_EQ(fb[8 * 240 + 8], 0x03E0u) << "BG1 at (1,1)";
    EXPECT_EQ(fb[16 * 240 + 16], 0x7C00u) << "BG2 at (2,2)";
    EXPECT_EQ(fb[24 * 240 + 24], 0x7FFFu) << "BG3 at (3,3)";
}

// Test 4: Scrolling Background - Single background with offset
TEST_F(IntegrationTest, ScrollingBackground) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    // Create gradient tiles
    for (int tileNum = 1; tileNum <= 8; tileNum++) {
        uint8_t tile[64];
        std::fill_n(tile, 64, tileNum);
        createTile(tileNum, tile);
    }
    
    // Setup palette with gradient
    std::vector<uint16_t> colors = {0x0000};
    for (int i = 1; i <= 8; i++) {
        colors.push_back((uint16_t)(i * 4));  // Gradient
    }
    setupPalette(0, colors);
    
    // Fill tilemap with horizontal pattern
    std::vector<uint16_t> tilemap(1024, 0);
    for (int i = 0; i < 32; i++) {
        tilemap[i] = (i % 8) + 1;
    }
    fillTilemap(0, tilemap);
    
    // Enable BG0
    memory->write16(REG_DISPCNT, 0x0100u);
    
    // Render without scroll
    renderFrame();
    uint16_t* fb = gpu->getTiledFramebuffer();
    uint16_t pixel0 = fb[0];
    
    // Now scroll by 8 pixels
    memory->write16(REG_BG0HOFS, 8);
    renderFrame();
    uint16_t pixel1 = fb[0];
    
    // The pixel should be different after scrolling
    EXPECT_NE(pixel0, pixel1) << "Scrolling should change rendered pixels";
}

// Test 5: Parallax Scrolling - Multiple layers at different speeds
TEST_F(IntegrationTest, ParallaxScrolling) {
    // Setup 3 BGs with different priorities
    memory->write16(REG_BG0CNT, 0x1C00u);  // Priority 0 (foreground)
    memory->write16(REG_BG1CNT, 0x1C01u | (1 << 11));  // Priority 1 (middle)
    memory->write16(REG_BG2CNT, 0x1C02u | (2 << 11));  // Priority 2 (background)
    
    // Create distinct tiles for each layer
    for (int i = 1; i <= 3; i++) {
        uint8_t tile[64];
        std::fill_n(tile, 64, i);
        createTile(i, tile);
    }
    
    // Setup palettes
    setupPalette(0, {0x0000, 0x001F, 0x03E0, 0x7C00});
    
    // Fill tilemaps with repeating patterns
    for (int bg = 0; bg < 3; bg++) {
        std::vector<uint16_t> tilemap(1024, bg + 1);
        fillTilemap(bg, tilemap);
    }
    
    // Enable BGs
    memory->write16(REG_DISPCNT, 0x0700u);
    
    // Apply different scroll speeds (parallax effect)
    memory->write16(REG_BG0HOFS, 16);   // Fast foreground
    memory->write16(REG_BG1HOFS, 8);    // Medium middle
    memory->write16(REG_BG2HOFS, 4);    // Slow background
    
    renderFrame();
    
    // All layers should render (BG0 on top)
    uint16_t* fb = gpu->getTiledFramebuffer();
    EXPECT_EQ(fb[0], 0x001Fu) << "BG0 should be visible (highest priority)";
}

// Test 6: Priority Demo - Visual priority demonstration
TEST_F(IntegrationTest, PriorityDemo) {
    // Create a scene where priority order is clearly visible
    // BG0 and BG1 both have priority 0 (BG0 should win)
    memory->write16(REG_BG0CNT, 0x1C00u);  // Priority 0
    memory->write16(REG_BG1CNT, 0x1C00u | (1 << 11));  // Also priority 0
    
    // Create overlapping tiles
    uint8_t tile1[64], tile2[64];
    std::fill_n(tile1, 64, 1);
    std::fill_n(tile2, 64, 2);
    createTile(1, tile1);
    createTile(2, tile2);
    
    setupPalette(0, {0x0000, 0x001F, 0x03E0});
    
    // Both BGs have tiles at same position
    std::vector<uint16_t> tilemap(1024, 0);
    tilemap[0] = 1;  // BG0 tile
    fillTilemap(0, tilemap);
    
    tilemap[0] = 2;  // BG1 tile (same position)
    fillTilemap(1, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0300u);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    EXPECT_EQ(fb[0], 0x001Fu) << "BG0 should win tiebreaker at same priority";
}

// Test 7: Transparency Showcase - Transparent tiles revealing layers
TEST_F(IntegrationTest, TransparencyShowcase) {
    setupBGConfig(0, 0);  // Priority 0
    setupBGConfig(1, 1);  // Priority 1
    
    // Create tile with transparent center (palette 0)
    uint8_t frameTile[64];
    std::fill_n(frameTile, 64, 0);  // Start transparent
    // Make border solid
    for (int y = 0; y < 8; y++) {
        frameTile[y * 8] = 1;      // Left edge
        frameTile[y * 8 + 7] = 1;  // Right edge
    }
    for (int x = 0; x < 8; x++) {
        frameTile[x] = 1;          // Top edge
        frameTile[56 + x] = 1;     // Bottom edge
    }
    
    uint8_t solidTile[64];
    std::fill_n(solidTile, 64, 2);
    
    createTile(1, frameTile);
    createTile(2, solidTile);
    
    setupPalette(0, {0x0000, 0x001F, 0x7C00});
    
    // BG0: Frame tile
    std::vector<uint16_t> tilemap0(1024, 0);
    tilemap0[0] = 1;
    fillTilemap(0, tilemap0);
    
    // BG1: Solid tile underneath
    std::vector<uint16_t> tilemap1(1024, 2);
    fillTilemap(1, tilemap1);
    
    memory->write16(REG_DISPCNT, 0x0300u);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Border should show BG0 (red)
    EXPECT_EQ(fb[0], 0x001Fu) << "Border should be red (BG0)";
    
    // Center should show BG1 through transparency (blue)
    EXPECT_EQ(fb[3 * 240 + 3], 0x7C00u) << "Center should show BG1 (blue)";
}

// Test 8: Complex Tileset - Many unique tiles
TEST_F(IntegrationTest, ComplexTileset) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    // Create 64 unique tiles
    for (int tileNum = 1; tileNum <= 64; tileNum++) {
        uint8_t tile[64];
        for (int i = 0; i < 64; i++) {
            tile[i] = (tileNum % 15) + 1;  // Cycle through palette colors
        }
        createTile(tileNum, tile);
    }
    
    // Setup palette with many colors
    std::vector<uint16_t> colors = {0x0000};
    for (int i = 1; i < 16; i++) {
        colors.push_back((uint16_t)(i * 0x0842));  // Various colors
    }
    setupPalette(0, colors);
    
    // Fill tilemap with all tiles
    std::vector<uint16_t> tilemap(1024, 0);
    for (int i = 0; i < 64; i++) {
        tilemap[i] = i + 1;
    }
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Verify different tiles produce different colors
    EXPECT_NE(fb[0], fb[8]) << "Different tiles should produce different output";
}

// Test 9: Large 512x512 Map - Maximum size tilemap (size code 3)
TEST_F(IntegrationTest, Large512x512Map) {
    // BGxCNT with screen size = 3 (512x512 pixels = 64x64 tiles)
    setupBGConfig(0, 0, false, 3);  // Screen size 3
    
    // Create a tile
    uint8_t tile[64];
    std::fill_n(tile, 64, 1);
    createTile(1, tile);
    
    setupPalette(0, {0x0000, 0x001F});
    
    // Place tiles in first screen block (visible area)
    std::vector<uint16_t> tilemap(1024, 0);
    for (int i = 0; i < 32 * 32; i++) {
        tilemap[i] = 1;  // Fill with tile 1
    }
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    
    // Render without scrolling
    memory->write16(REG_BG0HOFS, 0);
    memory->write16(REG_BG0VOFS, 0);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    EXPECT_EQ(fb[0], 0x001Fu) << "Large map renders correctly";
    
    // Test scrolling
    memory->write16(REG_BG0HOFS, 64);
    memory->write16(REG_BG0VOFS, 32);
    renderFrame();
    
    // Should render without crashing
    EXPECT_GT(fb[120 * 240 + 120], 0u) << "Scrolling on large map works";
}

// Test 10: Animated Tiles - Simulate tile changes
TEST_F(IntegrationTest, AnimatedTiles) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    // Create initial tile
    uint8_t tile1[64];
    std::fill_n(tile1, 64, 1);
    createTile(1, tile1);
    
    setupPalette(0, {0x0000, 0x001F, 0x03E0});
    
    std::vector<uint16_t> tilemap(1024, 0);
    tilemap[0] = 1;
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    uint16_t frame1 = fb[0];
    
    // Change tile data (simulate animation)
    uint8_t tile2[64];
    std::fill_n(tile2, 64, 2);
    createTile(1, tile2);
    
    renderFrame();
    uint16_t frame2 = fb[0];
    
    EXPECT_NE(frame1, frame2) << "Tile changes should reflect in rendering";
}

// Test 11: Color Palettes - Multiple palettes in use
TEST_F(IntegrationTest, ColorPalettes) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    // Create one tile
    uint8_t tile[64];
    std::fill_n(tile, 64, 1);
    createTile(1, tile);
    
    // Setup 4 different palettes
    setupPalette(0, {0x0000, 0x001F});  // Red
    setupPalette(1, {0x0000, 0x03E0});  // Green
    setupPalette(2, {0x0000, 0x7C00});  // Blue
    setupPalette(3, {0x0000, 0x7FFF});  // White
    
    // Use same tile with different palettes
    std::vector<uint16_t> tilemap(1024, 0);
    tilemap[0] = 1 | (0 << 12);  // Palette 0
    tilemap[1] = 1 | (1 << 12);  // Palette 1
    tilemap[2] = 1 | (2 << 12);  // Palette 2
    tilemap[3] = 1 | (3 << 12);  // Palette 3
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    EXPECT_EQ(fb[0], 0x001Fu) << "Palette 0: Red";
    EXPECT_EQ(fb[8], 0x03E0u) << "Palette 1: Green";
    EXPECT_EQ(fb[16], 0x7C00u) << "Palette 2: Blue";
    EXPECT_EQ(fb[24], 0x7FFFu) << "Palette 3: White";
}

// Test 12: 8bpp Scene - 256-color background
TEST_F(IntegrationTest, Scene8bpp) {
    memory->write16(REG_BG0CNT, 0x1C80u);  // 8bpp mode (bit 7)
    
    // Create 8bpp tile with gradient
    uint8_t tile[64];
    for (int i = 0; i < 64; i++) {
        tile[i] = i;  // 0-63 gradient
    }
    createTile8bpp(1, tile);
    
    // Setup 256-color palette
    uint8_t* paletteRAM = memory->getPaletteRAM();
    for (int i = 0; i < 256; i++) {
        uint16_t color = (uint16_t)(i * 0x20);
        paletteRAM[i * 2] = color & 0xFFu;
        paletteRAM[i * 2 + 1] = (color >> 8) & 0xFFu;
    }
    
    std::vector<uint16_t> tilemap(1024, 0);
    tilemap[0] = 1;
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Should show gradient colors
    EXPECT_NE(fb[0], fb[7]) << "8bpp gradient should show variation";
}

// Test 13: Mixed 4bpp and 8bpp - Different color modes per BG
TEST_F(IntegrationTest, Mixed4and8bpp) {
    // BG0: 4bpp mode
    setupBGConfig(0, 0, false);
    
    // BG1: 8bpp mode
    setupBGConfig(1, 1, true);  // 8bpp, priority 1
    
    // Create 4bpp tile
    uint8_t tile4bpp[64];
    std::fill_n(tile4bpp, 64, 1);
    createTile(1, tile4bpp);
    
    // Create 8bpp tile
    uint8_t tile8bpp[64];
    std::fill_n(tile8bpp, 64, 10);
    createTile8bpp(2, tile8bpp);
    
    // Setup palettes
    setupPalette(0, {0x0000, 0x001F});
    
    uint8_t* paletteRAM = memory->getPaletteRAM();
    uint16_t color = 0x03E0;
    paletteRAM[10 * 2] = color & 0xFFu;
    paletteRAM[10 * 2 + 1] = (color >> 8) & 0xFFu;
    
    // BG0 tile at (0, 0)
    std::vector<uint16_t> tilemap0(1024, 0);
    tilemap0[0] = 1;
    fillTilemap(0, tilemap0);
    
    // BG1 tiles everywhere else
    std::vector<uint16_t> tilemap1(1024, 2);
    tilemap1[0] = 0;  // Transparent at (0,0)
    fillTilemap(1, tilemap1);
    
    memory->write16(REG_DISPCNT, 0x0300u);
    renderFrame();
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    EXPECT_EQ(fb[0], 0x001Fu) << "4bpp BG0 on top";
    EXPECT_EQ(fb[8], 0x03E0u) << "8bpp BG1 elsewhere";
}

// Test 14: Edge Cases - Screen wrapping and boundaries
TEST_F(IntegrationTest, EdgeCases) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    uint8_t tile[64];
    std::fill_n(tile, 64, 1);
    createTile(1, tile);
    
    setupPalette(0, {0x0000, 0x001F});
    
    // Place tile at edge of 256x256 screen
    std::vector<uint16_t> tilemap(1024, 0);
    tilemap[31] = 1;      // Right edge (tile 31)
    tilemap[31 * 32] = 1; // Bottom edge
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    
    // Scroll to show right edge
    memory->write16(REG_BG0HOFS, 8);
    renderFrame();
    
    // Should render without crashing
    uint16_t* fb = gpu->getTiledFramebuffer();
    EXPECT_GE(fb[239], 0u) << "Right edge renders";
}

// Test 15: Rapid Scrolling - Fast scroll changes
TEST_F(IntegrationTest, RapidScrolling) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    uint8_t tile[64];
    std::fill_n(tile, 64, 1);
    createTile(1, tile);
    
    setupPalette(0, {0x0000, 0x001F});
    
    std::vector<uint16_t> tilemap(1024, 1);
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    
    // Rapidly change scroll values
    for (int scroll = 0; scroll < 256; scroll += 32) {
        memory->write16(REG_BG0HOFS, scroll);
        memory->write16(REG_BG0VOFS, scroll);
        renderFrame();
    }
    
    // Should complete without errors
    SUCCEED();
}

// Test 16: Mid-Frame Changes - Change config during rendering
TEST_F(IntegrationTest, MidFrameChanges) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    uint8_t tile[64];
    std::fill_n(tile, 64, 1);
    createTile(1, tile);
    
    setupPalette(0, {0x0000, 0x001F, 0x03E0});
    
    std::vector<uint16_t> tilemap(1024, 1);
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    
    // Render first half
    for (int scanline = 0; scanline < 80; scanline++) {
        memory->write16(0x04000006u, scanline);
        gpu->renderScanline();
    }
    
    // Change palette mid-frame
    setupPalette(0, {0x0000, 0x03E0, 0x7C00});
    
    // Render second half
    for (int scanline = 80; scanline < 160; scanline++) {
        memory->write16(0x04000006u, scanline);
        gpu->renderScanline();
    }
    
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Top half should be different from bottom half
    EXPECT_NE(fb[0], fb[80 * 240]) << "Mid-frame changes should affect rendering";
}

// Test 17: Full Game Scene - Realistic game-like setup
TEST_F(IntegrationTest, FullGameScene) {
    // Simulate a platformer game scene
    // BG0: UI/HUD (priority 0)
    // BG1: Player layer (priority 1)
    // BG2: Foreground scenery (priority 2)
    // BG3: Background sky (priority 3)
    
    memory->write16(REG_BG0CNT, 0x1C00u);
    memory->write16(REG_BG1CNT, 0x1C01u | (1 << 11));
    memory->write16(REG_BG2CNT, 0x1C02u | (2 << 11));
    memory->write16(REG_BG3CNT, 0x1C03u | (3 << 11));
    
    // Create different tiles for each layer
    for (int i = 1; i <= 4; i++) {
        uint8_t tile[64];
        std::fill_n(tile, 64, i);
        createTile(i, tile);
    }
    
    setupPalette(0, {0x0000, 0x7FFF, 0x03E0, 0x421F, 0x318C});
    
    // Setup each layer
    for (int bg = 0; bg < 4; bg++) {
        std::vector<uint16_t> tilemap(1024, 0);
        // Add some tiles to each layer
        for (int i = 0; i < 10; i++) {
            tilemap[bg * 32 + i] = bg + 1;
        }
        fillTilemap(bg, tilemap);
    }
    
    // Add different scroll speeds for parallax
    memory->write16(REG_BG1HOFS, 10);
    memory->write16(REG_BG2HOFS, 5);
    memory->write16(REG_BG3HOFS, 2);
    
    memory->write16(REG_DISPCNT, 0x0F00u);
    renderFrame();
    
    // Should render complete scene
    uint16_t* fb = gpu->getTiledFramebuffer();
    EXPECT_NE(fb[0], 0u) << "Game scene renders";
}

// Test 18: Stress Test - Maximum complexity
TEST_F(IntegrationTest, StressTest) {
    // All 4 BGs enabled, all scrolling, maximum tiles
    for (int bg = 0; bg < 4; bg++) {
        uint16_t config = 0x1C00u | bg | (bg << 11);  // Different screen base and priority
        memory->write16(REG_BG0CNT + bg * 2, config);
    }
    
    // Create many tiles
    for (int tileNum = 1; tileNum <= 128; tileNum++) {
        uint8_t tile[64];
        for (int i = 0; i < 64; i++) {
            tile[i] = (tileNum % 15) + 1;
        }
        createTile(tileNum, tile);
    }
    
    // Setup palettes
    for (int pal = 0; pal < 16; pal++) {
        std::vector<uint16_t> colors = {0x0000};
        for (int i = 1; i < 16; i++) {
            colors.push_back((uint16_t)((pal * 16 + i) * 0x200));
        }
        setupPalette(pal, colors);
    }
    
    // Fill all tilemaps
    for (int bg = 0; bg < 4; bg++) {
        std::vector<uint16_t> tilemap(1024, 0);
        for (int i = 0; i < 1024; i++) {
            int tileNum = (i % 128) + 1;
            int palNum = i % 16;
            tilemap[i] = (uint16_t)(tileNum | (palNum << 12));
        }
        fillTilemap(bg, tilemap);
    }
    
    // Apply scrolling to all
    memory->write16(REG_BG0HOFS, 13);
    memory->write16(REG_BG0VOFS, 7);
    memory->write16(REG_BG1HOFS, 23);
    memory->write16(REG_BG1VOFS, 11);
    memory->write16(REG_BG2HOFS, 37);
    memory->write16(REG_BG2VOFS, 17);
    memory->write16(REG_BG3HOFS, 53);
    memory->write16(REG_BG3VOFS, 29);
    
    memory->write16(REG_DISPCNT, 0x0F00u);
    renderFrame();
    
    // Should complete without crashing
    uint16_t* fb = gpu->getTiledFramebuffer();
    EXPECT_NE(fb[120 * 240 + 120], 0u) << "Stress test completes";
}

// Test 19: Visual Regression - Consistent output
TEST_F(IntegrationTest, VisualRegression) {
    // Setup a known configuration
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    uint8_t tile[64];
    for (int i = 0; i < 64; i++) {
        tile[i] = (i % 8) + 1;
    }
    createTile(1, tile);
    
    setupPalette(0, {0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008});
    
    std::vector<uint16_t> tilemap(1024, 0);
    for (int i = 0; i < 100; i++) {
        tilemap[i] = 1;
    }
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    
    // Render twice
    renderFrame();
    uint16_t* fb1 = gpu->getTiledFramebuffer();
    uint16_t frame1[240 * 160];
    std::memcpy(frame1, fb1, sizeof(frame1));
    
    renderFrame();
    uint16_t* fb2 = gpu->getTiledFramebuffer();
    
    // Should produce identical output
    bool identical = true;
    for (int i = 0; i < 240 * 160; i++) {
        if (frame1[i] != fb2[i]) {
            identical = false;
            break;
        }
    }
    
    EXPECT_TRUE(identical) << "Rendering should be deterministic";
}

// Test 20: Compare Frames - Frame-to-frame comparison
TEST_F(IntegrationTest, CompareFrames) {
    memory->write16(REG_BG0CNT, 0x1C00u);
    
    uint8_t tile[64];
    std::fill_n(tile, 64, 1);
    createTile(1, tile);
    
    setupPalette(0, {0x0000, 0x001F});
    
    std::vector<uint16_t> tilemap(1024, 1);
    fillTilemap(0, tilemap);
    
    memory->write16(REG_DISPCNT, 0x0100u);
    
    // Render frame 1
    renderFrame();
    uint16_t* fb = gpu->getTiledFramebuffer();
    uint16_t checksum1 = 0;
    for (int i = 0; i < 240 * 160; i++) {
        checksum1 += fb[i];
    }
    
    // Change nothing, render frame 2
    renderFrame();
    uint16_t checksum2 = 0;
    for (int i = 0; i < 240 * 160; i++) {
        checksum2 += fb[i];
    }
    
    EXPECT_EQ(checksum1, checksum2) << "Identical frames should have same checksum";
    
    // Change tile, render frame 3
    std::fill_n(tile, 64, 2);
    createTile(1, tile);
    setupPalette(0, {0x0000, 0x001F, 0x03E0});
    
    renderFrame();
    uint16_t checksum3 = 0;
    for (int i = 0; i < 240 * 160; i++) {
        checksum3 += fb[i];
    }
    
    EXPECT_NE(checksum1, checksum3) << "Different frames should have different checksums";
}
