#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"
#include "display.h"
#include <SDL2/SDL.h>

// Test fixture for visual tile rendering
class TileRenderTest : public ::testing::Test {
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

    // Helper to create a checkerboard tile pattern (4bpp)
    void createCheckerboardTile(uint32_t tileAddr) {
        // Checkerboard: alternating colors
        uint8_t tileData[32];
        for (int row = 0; row < 8; row++) {
            for (int byteInRow = 0; byteInRow < 4; byteInRow++) {
                // Alternate between palette indices 1 and 2
                if ((row + byteInRow) % 2 == 0) {
                    tileData[row * 4 + byteInRow] = 0x11;  // Both pixels = 1
                } else {
                    tileData[row * 4 + byteInRow] = 0x22;  // Both pixels = 2
                }
            }
        }
        
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, tileData[i]);
        }
    }

    // Helper to render a single tile to framebuffer
    void renderTileToFramebuffer(int screenX, int screenY, uint32_t tileAddr, 
                                   int paletteNum, bool is8bpp) {
        uint16_t* framebuffer = gpu->getFrameBuffer();
        
        for (int ty = 0; ty < 8; ty++) {
            for (int tx = 0; tx < 8; tx++) {
                int pixelX = screenX + tx;
                int pixelY = screenY + ty;
                
                if (pixelX >= 240 || pixelY >= 160) continue;
                
                // Get palette index for this pixel
                uint8_t paletteIndex;
                if (is8bpp) {
                    paletteIndex = gpu->getTilePixel8bpp(tileAddr, tx, ty);
                } else {
                    paletteIndex = gpu->getTilePixel4bpp(tileAddr, tx, ty);
                }
                
                // Get color from palette (skip transparent pixels)
                if (paletteIndex == 0) continue;
                
                uint32_t color = gpu->getBGColor(paletteNum, paletteIndex);
                
                // Convert ARGB8888 back to RGB555 for framebuffer
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t g = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;
                
                uint16_t rgb555 = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
                
                // Write to framebuffer
                framebuffer[pixelY * 240 + pixelX] = rgb555;
            }
        }
    }
};

// Test 1: Render a single colored tile (programmatic test)
TEST_F(TileRenderTest, RenderSingleTile) {
    // Create a simple tile in VRAM
    uint32_t tileAddr = 0x06000000;
    createCheckerboardTile(tileAddr);
    
    // Set up palette colors
    memory->write16(0x05000000 + 2, 0x001F);  // Palette 0, color 1 = Red
    memory->write16(0x05000000 + 4, 0x03E0);  // Palette 0, color 2 = Green
    
    // Render tile to framebuffer at position (10, 10)
    renderTileToFramebuffer(10, 10, tileAddr, 0, false);
    
    // Verify some pixels were written
    uint16_t* framebuffer = gpu->getFrameBuffer();
    
    // Check that pixels were rendered (non-zero)
    bool foundNonZero = false;
    for (int y = 10; y < 18; y++) {
        for (int x = 10; x < 18; x++) {
            if (framebuffer[y * 240 + x] != 0) {
                foundNonZero = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(foundNonZero) << "Tile should have rendered some pixels";
}

// Test 2: Render multiple tiles
TEST_F(TileRenderTest, RenderMultipleTiles) {
    // Create tile 0 (all palette index 1)
    uint32_t tile0Addr = 0x06000000;
    uint8_t tile0Data[32];
    for (int i = 0; i < 32; i++) tile0Data[i] = 0x11;
    for (int i = 0; i < 32; i++) {
        memory->write8(tile0Addr + i, tile0Data[i]);
    }
    
    // Create tile 1 (all palette index 2)
    uint32_t tile1Addr = 0x06000020;
    uint8_t tile1Data[32];
    for (int i = 0; i < 32; i++) tile1Data[i] = 0x22;
    for (int i = 0; i < 32; i++) {
        memory->write8(tile1Addr + i, tile1Data[i]);
    }
    
    // Set up palette
    memory->write16(0x05000000 + 2, 0x7FFF);  // Color 1 = White
    memory->write16(0x05000000 + 4, 0x001F);  // Color 2 = Red
    
    // Render both tiles
    renderTileToFramebuffer(0, 0, tile0Addr, 0, false);
    renderTileToFramebuffer(8, 0, tile1Addr, 0, false);
    
    // Verify framebuffer has data
    uint16_t* framebuffer = gpu->getFrameBuffer();
    
    // Tile 0 pixels (should be white = 0x7FFF)
    EXPECT_EQ(0x7FFF, framebuffer[0 * 240 + 0]);
    
    // Tile 1 pixels (should be red = 0x001F)
    EXPECT_EQ(0x001F, framebuffer[0 * 240 + 8]);
}

// Test 3: 8bpp tile rendering
TEST_F(TileRenderTest, Render8bppTile) {
    // Create an 8bpp tile with various palette indices
    uint32_t tileAddr = 0x06000000;
    uint8_t tileData[64];
    for (int i = 0; i < 64; i++) {
        tileData[i] = (i % 16) + 1;  // Palette indices 1-16 repeated
    }
    
    for (int i = 0; i < 64; i++) {
        memory->write8(tileAddr + i, tileData[i]);
    }
    
    // Set up palette (16 colors)
    for (int i = 1; i < 16; i++) {
        uint16_t color = (i << 10) | (i << 5) | i;  // Gradient
        memory->write16(0x05000000 + i * 2, color);
    }
    
    // Render tile
    renderTileToFramebuffer(20, 20, tileAddr, 0, true);
    
    // Verify some pixels were written
    uint16_t* framebuffer = gpu->getFrameBuffer();
    bool foundNonZero = false;
    for (int y = 20; y < 28; y++) {
        for (int x = 20; x < 28; x++) {
            if (framebuffer[y * 240 + x] != 0) {
                foundNonZero = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(foundNonZero) << "8bpp tile should have rendered";
}

// Test 4: Transparent pixels (color 0) not rendered
TEST_F(TileRenderTest, TransparentPixels) {
    // Simple test: just verify that getTilePixel4bpp correctly identifies
    // transparent (0) and non-transparent pixels
    uint32_t tileAddr = 0x06000000;
    
    // Write tile data: alternating pattern
    // Byte 0x01 means: pixel 0 = 1 (white), pixel 1 = 0 (transparent)
    for (int i = 0; i < 32; i++) {
        memory->write8(tileAddr + i, 0x01);
    }
    
    // Set up palette
    memory->write16(0x05000000 + 2, 0x7FFF);  // Color 1 = White (RGB555)
    
    // Verify pixel extraction
    uint8_t pixel0 = gpu->getTilePixel4bpp(tileAddr, 0, 0);  // Should be 1
    uint8_t pixel1 = gpu->getTilePixel4bpp(tileAddr, 1, 0);  // Should be 0
    
    EXPECT_EQ(1, pixel0) << "Pixel 0 should have palette index 1";
    EXPECT_EQ(0, pixel1) << "Pixel 1 should have palette index 0 (transparent)";
    
    // Verify color lookup
    uint32_t color1 = gpu->getBGColor(0, 1);  // Should be white in ARGB8888
    // White RGB555 (0x7FFF = 31,31,31) converts to (255,255,255) in 8-bit
    // Our conversion: (val5 << 3) | (val5 >> 2) gives 255 for 31
    EXPECT_EQ(0xFFu, (color1 >> 24) & 0xFF) << "Alpha should be 255";
    EXPECT_EQ(0xFFu, (color1 >> 16) & 0xFF) << "Red should be 255";
    EXPECT_EQ(0xFFu, (color1 >> 8) & 0xFF) << "Green should be 255";
    EXPECT_EQ(0xFFu, color1 & 0xFF) << "Blue should be 255";
}

// Test 5: Integration test - tile + palette + rendering
TEST_F(TileRenderTest, IntegrationTest) {
    // Create a simple smiley face tile (4bpp)
    uint32_t tileAddr = 0x06000000;
    uint8_t smileyData[32] = {
        0x00, 0x00, 0x00, 0x00,  // Row 0: blank
        0x11, 0x00, 0x00, 0x11,  // Row 1: eyes
        0x11, 0x00, 0x00, 0x11,  // Row 2: eyes
        0x00, 0x00, 0x00, 0x00,  // Row 3: blank
        0x00, 0x00, 0x00, 0x00,  // Row 4: blank
        0x11, 0x00, 0x00, 0x11,  // Row 5: smile
        0x01, 0x11, 0x11, 0x10,  // Row 6: smile
        0x00, 0x11, 0x11, 0x00,  // Row 7: smile
    };
    
    for (int i = 0; i < 32; i++) {
        memory->write8(tileAddr + i, smileyData[i]);
    }
    
    // Set up palette
    memory->write16(0x05000000 + 0, 0x0000);  // Color 0 = Black (transparent)
    memory->write16(0x05000000 + 2, 0x001F);  // Color 1 = Red
    
    // Render tile
    renderTileToFramebuffer(100, 80, tileAddr, 0, false);
    
    // Just verify it doesn't crash and renders something
    uint16_t* framebuffer = gpu->getFrameBuffer();
    bool foundRed = false;
    for (int y = 80; y < 88; y++) {
        for (int x = 100; x < 108; x++) {
            if (framebuffer[y * 240 + x] == 0x001F) {
                foundRed = true;
                break;
            }
        }
    }
    
    EXPECT_TRUE(foundRed) << "Should find red pixels in smiley face";
}
