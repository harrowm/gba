#include <gtest/gtest.h>
#include "../../include/gba.h"
#include "../../include/gpu.h"
#include "../../include/memory.h"

class BGRenderTest : public ::testing::Test {
protected:
    GBA* gba;
    GPU* gpu;
    Memory* memory;

    void SetUp() override {
        gba = new GBA(false);  // Full memory map
        gpu = &gba->getGPU();
        memory = &gba->getMemory();
    }

    void TearDown() override {
        delete gba;
    }

    void setupSimpleBG0() {
        // Configure Mode 0, BG0 enabled
        memory->write16(0x04000000, 0x0100);  // DISPCNT: Mode 0, BG0 enabled
        
        // Configure BG0: char base 0, screen base 30, 256x256, 4bpp
        memory->write16(0x04000008, 0x0F00);  // BG0CNT: screen base 30 (0x7800)
        
        // Zero scroll
        memory->write16(0x04000010, 0);  // BG0HOFS
        memory->write16(0x04000012, 0);  // BG0VOFS
    }

    void setupPalette() {
        // Setup a simple palette
        // Color 0: Transparent (black)
        memory->write16(0x05000000, 0x0000);
        // Color 1: Red
        memory->write16(0x05000002, 0x001F);
        // Color 2: Green
        memory->write16(0x05000004, 0x03E0);
        // Color 3: Blue
        memory->write16(0x05000006, 0x7C00);
        // Color 4: White
        memory->write16(0x05000008, 0x7FFF);
    }

    void setupSimpleTile(uint16_t tileNum, uint8_t color) {
        // Create a solid color tile (8x8 pixels, 4bpp)
        uint32_t tileAddr = 0x06000000 + tileNum * 32;
        for (int i = 0; i < 32; i++) {
            uint8_t byte = (color << 4) | color;
            memory->write8(tileAddr + i, byte);
        }
    }

    void setupCheckerboardTile(uint16_t tileNum, uint8_t color1, uint8_t color2) {
        // Create a checkerboard pattern tile
        uint32_t tileAddr = 0x06000000 + tileNum * 32;
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 4; col++) {  // 4 bytes per row
                uint8_t c1 = (row + col * 2) % 2 == 0 ? color1 : color2;
                uint8_t c2 = (row + col * 2 + 1) % 2 == 0 ? color1 : color2;
                memory->write8(tileAddr + row * 4 + col, (c2 << 4) | c1);
            }
        }
    }

    void setupTilemap(uint16_t x, uint16_t y, uint16_t tileNum, uint8_t paletteNum = 0, bool hFlip = false, bool vFlip = false) {
        // Write a screen entry to the tilemap
        // Screen base is at 0x7800 (block 30)
        uint32_t tilemapBase = 0x06000000 + 0x7800;
        uint32_t offset = (y * 32 + x) * 2;
        
        uint16_t entry = tileNum;
        if (hFlip) entry |= 0x0400;
        if (vFlip) entry |= 0x0800;
        entry |= (paletteNum << 12);
        
        memory->write16(tilemapBase + offset, entry);
    }
};

TEST_F(BGRenderTest, RenderSingleTile) {
    // Setup a simple background with one red tile
    setupSimpleBG0();
    setupPalette();
    setupSimpleTile(1, 1);  // Tile 1 = solid red
    setupTilemap(0, 0, 1);  // Top-left tile
    
    // Render the first scanline
    gpu->renderBGScanline(0, 0);
    
    // Check that the first 8 pixels are red
    uint16_t* fb = gpu->getFrameBuffer();
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
    
    // Check that pixels beyond the tile are unrendered (would be 0 or backdrop)
    // Since we only placed one tile, tile 0 is transparent
}

TEST_F(BGRenderTest, RenderMultipleTiles) {
    // Setup background with multiple colored tiles
    setupSimpleBG0();
    setupPalette();
    
    // Create different colored tiles
    setupSimpleTile(1, 1);  // Red
    setupSimpleTile(2, 2);  // Green
    setupSimpleTile(3, 3);  // Blue
    setupSimpleTile(4, 4);  // White
    
    // Place them in a row
    setupTilemap(0, 0, 1);  // Red
    setupTilemap(1, 0, 2);  // Green
    setupTilemap(2, 0, 3);  // Blue
    setupTilemap(3, 0, 4);  // White
    
    // Render the first scanline
    gpu->renderBGScanline(0, 0);
    
    // Check colors
    uint16_t* fb = gpu->getFrameBuffer();
    
    // First tile (pixels 0-7): Red
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
    
    // Second tile (pixels 8-15): Green
    for (int x = 8; x < 16; x++) {
        EXPECT_EQ(fb[x], 0x03E0) << "Pixel " << x << " should be green";
    }
    
    // Third tile (pixels 16-23): Blue
    for (int x = 16; x < 24; x++) {
        EXPECT_EQ(fb[x], 0x7C00) << "Pixel " << x << " should be blue";
    }
    
    // Fourth tile (pixels 24-31): White
    for (int x = 24; x < 32; x++) {
        EXPECT_EQ(fb[x], 0x7FFF) << "Pixel " << x << " should be white";
    }
}

TEST_F(BGRenderTest, RenderWithScroll) {
    // Test rendering with horizontal scrolling
    setupSimpleBG0();
    setupPalette();
    
    // Create a pattern of tiles
    setupSimpleTile(1, 1);  // Red
    setupSimpleTile(2, 2);  // Green
    
    setupTilemap(0, 0, 1);  // Red at position 0
    setupTilemap(1, 0, 2);  // Green at position 1
    
    // Scroll right by 4 pixels
    memory->write16(0x04000010, 4);  // BG0HOFS = 4
    
    // Render scanline 0
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // First 4 pixels should be from the right side of the red tile
    for (int x = 0; x < 4; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red (scrolled)";
    }
    
    // Next 8 pixels should be from the green tile
    for (int x = 4; x < 12; x++) {
        EXPECT_EQ(fb[x], 0x03E0) << "Pixel " << x << " should be green";
    }
}

TEST_F(BGRenderTest, RenderWithVerticalScroll) {
    // Test rendering with vertical scrolling
    setupSimpleBG0();
    setupPalette();
    
    // Create tiles in two rows
    setupSimpleTile(1, 1);  // Red
    setupSimpleTile(2, 2);  // Green
    
    setupTilemap(0, 0, 1);  // Red at row 0
    setupTilemap(0, 1, 2);  // Green at row 1
    
    // Scroll down by 4 pixels
    memory->write16(0x04000012, 4);  // BG0VOFS = 4
    
    // Render scanline 0 (which will see row 0, pixels 4-7 and row 1, pixels 0-3)
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // Since we're rendering scanline 0 with VOFS=4, we're looking at background line 4
    // Line 4 is still in tile row 0, but at pixel row 4 of the tile
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
    
    // Now render scanline 4 with scroll
    gpu->renderBGScanline(0, 4);
    // Scanline 4 + scroll 4 = background line 8 (start of row 1)
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[4 * 240 + x], 0x03E0) << "Pixel " << x << " on line 4 should be green";
    }
}

TEST_F(BGRenderTest, RenderHorizontalFlip) {
    // Test horizontal flip
    setupSimpleBG0();
    setupPalette();
    
    // Create a gradient tile (different colors across)
    uint32_t tileAddr = 0x06000000 + 1 * 32;
    for (int row = 0; row < 8; row++) {
        memory->write8(tileAddr + row * 4 + 0, 0x21);  // Colors 1,2
        memory->write8(tileAddr + row * 4 + 1, 0x21);  // Colors 1,2
        memory->write8(tileAddr + row * 4 + 2, 0x43);  // Colors 3,4
        memory->write8(tileAddr + row * 4 + 3, 0x43);  // Colors 3,4
    }
    
    // Place without flip
    setupTilemap(0, 0, 1, 0, false, false);
    
    // Place with horizontal flip
    setupTilemap(1, 0, 1, 0, true, false);
    
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // First tile: 1,2,1,2,3,4,3,4
    EXPECT_EQ(fb[0], 0x001F);  // Color 1 (red)
    EXPECT_EQ(fb[1], 0x03E0);  // Color 2 (green)
    EXPECT_EQ(fb[4], 0x7C00);  // Color 3 (blue)
    EXPECT_EQ(fb[5], 0x7FFF);  // Color 4 (white)
    
    // Second tile (flipped): 4,3,4,3,2,1,2,1
    EXPECT_EQ(fb[8], 0x7FFF);   // Color 4 (white)
    EXPECT_EQ(fb[9], 0x7C00);   // Color 3 (blue)
    EXPECT_EQ(fb[12], 0x03E0);  // Color 2 (green)
    EXPECT_EQ(fb[13], 0x001F);  // Color 1 (red)
}

TEST_F(BGRenderTest, RenderVerticalFlip) {
    // Test vertical flip
    setupSimpleBG0();
    setupPalette();
    
    // Create a tile with different colors per row
    uint32_t tileAddr = 0x06000000 + 1 * 32;
    for (int row = 0; row < 4; row++) {
        uint8_t color = 1;  // First 4 rows: color 1
        for (int col = 0; col < 4; col++) {
            memory->write8(tileAddr + row * 4 + col, (color << 4) | color);
        }
    }
    for (int row = 4; row < 8; row++) {
        uint8_t color = 2;  // Last 4 rows: color 2
        for (int col = 0; col < 4; col++) {
            memory->write8(tileAddr + row * 4 + col, (color << 4) | color);
        }
    }
    
    // Place without flip at position 0
    setupTilemap(0, 0, 1, 0, false, false);
    
    // Place with vertical flip at position 1
    setupTilemap(1, 0, 1, 0, false, true);
    
    // Render scanline 0 (top of tiles)
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // First tile, row 0: color 1 (red)
    EXPECT_EQ(fb[0], 0x001F);
    
    // Second tile, row 0 (but flipped, so it's row 7 of the tile): color 2 (green)
    EXPECT_EQ(fb[8], 0x03E0);
    
    // Render scanline 7 (bottom of tiles)
    gpu->renderBGScanline(0, 7);
    
    // First tile, row 7: color 2 (green)
    EXPECT_EQ(fb[7 * 240 + 0], 0x03E0);
    
    // Second tile, row 7 (but flipped, so it's row 0): color 1 (red)
    EXPECT_EQ(fb[7 * 240 + 8], 0x001F);
}

TEST_F(BGRenderTest, RenderDifferentPalettes) {
    // Test 4bpp mode with different palettes
    setupSimpleBG0();
    
    // Setup multiple palettes
    // Palette 0, color 1: Red
    memory->write16(0x05000002, 0x001F);
    // Palette 1, color 1: Green
    memory->write16(0x05000022, 0x03E0);
    // Palette 2, color 1: Blue
    memory->write16(0x05000042, 0x7C00);
    
    // Create a solid tile with color index 1
    setupSimpleTile(1, 1);
    
    // Place same tile with different palettes
    setupTilemap(0, 0, 1, 0);  // Palette 0
    setupTilemap(1, 0, 1, 1);  // Palette 1
    setupTilemap(2, 0, 1, 2);  // Palette 2
    
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // First tile: red (palette 0)
    EXPECT_EQ(fb[0], 0x001F);
    
    // Second tile: green (palette 1)
    EXPECT_EQ(fb[8], 0x03E0);
    
    // Third tile: blue (palette 2)
    EXPECT_EQ(fb[16], 0x7C00);
}

TEST_F(BGRenderTest, RenderTransparency) {
    // Test that palette index 0 is transparent
    setupSimpleBG0();
    setupPalette();
    
    // Create a tile with some transparent pixels
    uint32_t tileAddr = 0x06000000 + 1 * 32;
    for (int row = 0; row < 8; row++) {
        if (row % 2 == 0) {
            // Even rows: color 1
            for (int col = 0; col < 4; col++) {
                memory->write8(tileAddr + row * 4 + col, 0x11);
            }
        } else {
            // Odd rows: color 0 (transparent)
            for (int col = 0; col < 4; col++) {
                memory->write8(tileAddr + row * 4 + col, 0x00);
            }
        }
    }
    
    setupTilemap(0, 0, 1);
    
    // Initialize framebuffer with a pattern
    uint16_t* fb = gpu->getFrameBuffer();
    for (int i = 0; i < 240; i++) {
        fb[i] = 0x7FFF;  // White background
    }
    
    // Render scanline 0 (even row, should be red)
    gpu->renderBGScanline(0, 0);
    
    // Pixels should be red
    EXPECT_EQ(fb[0], 0x001F);
    
    // Render scanline 1 (odd row, transparent)
    for (int i = 0; i < 240; i++) {
        fb[240 + i] = 0x7FFF;  // Reset to white
    }
    gpu->renderBGScanline(0, 1);
    
    // Pixels should still be white (transparent didn't overwrite)
    EXPECT_EQ(fb[240 + 0], 0x7FFF);
}

TEST_F(BGRenderTest, Render8bpp) {
    // Test 8bpp rendering - simpler version
    // Use the simple setup functions that work for 4bpp tests
    memory->write16(0x04000000, 0x0100);  // Mode 0, BG0 enabled
    // BG0CNT: bit 7 (0x0080) = 8bpp mode, bits 8-12 = screen base 30 (0x1E00)
    memory->write16(0x04000008, 0x1E80);  // 0x1E80 = screen base 30 + 8bpp mode
    memory->write16(0x04000010, 0);  // No scroll
    memory->write16(0x04000012, 0);  // No scroll
    
    // Setup 8bpp palette - all 256 colors share one palette starting at 0x05000000
    memory->write16(0x05000000, 0x0000);  // Color 0: Transparent
    memory->write16(0x05000002, 0x001F);  // Color 1: Red (5-bit red = 31)
    memory->write16(0x05000004, 0x03E0);  // Color 2: Green (5-bit green = 31)
    memory->write16(0x05000006, 0x7C00);  // Color 3: Blue (5-bit blue = 31)
    
    // Create a simple 8bpp tile (64 bytes) - all pixels are color 1 (red)
    uint32_t tileAddr = 0x06000000 + 64;  // Tile 1 at offset 64
    for (int i = 0; i < 64; i++) {
        memory->write8(tileAddr + i, 1);  // All red
    }
    
    // Place tile 1 at tilemap position (0,0)
    // Screen base 30 = 30 × 2KB = 0xF000
    uint32_t tilemapAddr = 0x06000000 + (30 * 0x800);  // Screen base 30
    memory->write16(tilemapAddr, 1);  // Tile number 1
    
    // Render scanline 0
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // All pixels in the tile should be red
    for (int x = 0; x < 8; x++) {
        EXPECT_EQ(fb[x], 0x001F) << "Pixel " << x << " should be red";
    }
}

TEST_F(BGRenderTest, RenderDisabledBG) {
    // Test that disabled backgrounds don't render
    setupSimpleBG0();
    setupPalette();
    setupSimpleTile(1, 1);
    setupTilemap(0, 0, 1);
    
    // Disable BG0
    memory->write16(0x04000000, 0x0000);  // DISPCNT: BG0 disabled
    
    // Initialize framebuffer
    uint16_t* fb = gpu->getFrameBuffer();
    for (int i = 0; i < 240; i++) {
        fb[i] = 0x7FFF;  // White
    }
    
    gpu->renderBGScanline(0, 0);
    
    // Should remain white (not rendered)
    EXPECT_EQ(fb[0], 0x7FFF);
}

TEST_F(BGRenderTest, RenderInvalidBG) {
    // Test that invalid BG numbers are handled
    setupSimpleBG0();
    
    uint16_t* fb = gpu->getFrameBuffer();
    for (int i = 0; i < 240; i++) {
        fb[i] = 0x7FFF;
    }
    
    // Should not crash
    gpu->renderBGScanline(-1, 0);
    gpu->renderBGScanline(4, 0);
    
    // Framebuffer should be unchanged
    EXPECT_EQ(fb[0], 0x7FFF);
}

TEST_F(BGRenderTest, RenderFullScanline) {
    // Test rendering a complete scanline with pattern
    setupSimpleBG0();
    setupPalette();
    
    // Create a repeating pattern across the screen
    setupSimpleTile(1, 1);  // Red
    setupSimpleTile(2, 2);  // Green
    setupSimpleTile(3, 3);  // Blue
    setupSimpleTile(4, 4);  // White
    
    // Fill first row with repeating pattern (30 tiles = 240 pixels)
    for (int i = 0; i < 30; i++) {
        setupTilemap(i, 0, (i % 4) + 1);
    }
    
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    uint16_t colors[] = {0x001F, 0x03E0, 0x7C00, 0x7FFF};  // Red, Green, Blue, White
    
    for (int i = 0; i < 240; i++) {
        int tileIndex = i / 8;
        int expectedColor = colors[tileIndex % 4];
        EXPECT_EQ(fb[i], expectedColor) << "Pixel " << i << " incorrect";
    }
}

TEST_F(BGRenderTest, RenderWithWrapping) {
    // Test that scrolling wraps correctly at screen boundaries
    memory->write16(0x04000000, 0x0100);  // Mode 0, BG0 enabled
    memory->write16(0x04000008, 0x0F00);  // 256x256 screen
    
    setupPalette();
    setupSimpleTile(1, 1);  // Red
    setupSimpleTile(2, 2);  // Green
    
    // Place red at (0,0) and green at (31,0) (rightmost tile)
    setupTilemap(0, 0, 1);
    setupTilemap(31, 0, 2);
    
    // Scroll right by 248 pixels (31 tiles)
    memory->write16(0x04000010, 248);
    
    gpu->renderBGScanline(0, 0);
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // First 8 pixels should be green (wrapped around from rightmost tile)
    EXPECT_EQ(fb[0], 0x03E0);
    
    // Pixels 8-15 should be red (wrapped to tile 0)
    EXPECT_EQ(fb[8], 0x001F);
}

TEST_F(BGRenderTest, RenderCheckerboard) {
    // Create a classic checkerboard pattern
    setupSimpleBG0();
    setupPalette();
    
    // Create black and white tiles
    setupSimpleTile(1, 1);  // Red (we'll use as black)
    setupSimpleTile(2, 4);  // White
    
    // Create checkerboard in 8x8 region
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int tileNum = ((x + y) % 2 == 0) ? 1 : 2;
            setupTilemap(x, y, tileNum);
        }
    }
    
    // Render several scanlines
    for (int line = 0; line < 64; line++) {
        gpu->renderBGScanline(0, line);
    }
    
    uint16_t* fb = gpu->getFrameBuffer();
    
    // Check a few key positions
    // Note: Each tile is 8x8 pixels
    EXPECT_EQ(fb[0], 0x001F);           // (0,0) - tile (0,0) = red
    EXPECT_EQ(fb[8], 0x7FFF);           // (8,0) - tile (1,0) = white
    EXPECT_EQ(fb[240], 0x001F);         // (0,1) - still tile (0,0) = red
    EXPECT_EQ(fb[240 + 8], 0x7FFF);     // (8,1) - still tile (1,0) = white
    EXPECT_EQ(fb[8 * 240], 0x7FFF);     // (0,8) - tile (0,1) = white
    EXPECT_EQ(fb[8 * 240 + 8], 0x001F); // (8,8) - tile (1,1) = red
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
