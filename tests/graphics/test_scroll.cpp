#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

// Test fixture for scrolling tests
class ScrollTest : public ::testing::Test {
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

    void writeBGScroll(int bgNum, uint16_t hofs, uint16_t vofs) {
        uint32_t hofsAddr = 0x04000010 + (bgNum * 4);
        uint32_t vofsAddr = 0x04000012 + (bgNum * 4);
        memory->write16(hofsAddr, hofs);
        memory->write16(vofsAddr, vofs);
    }
    
    BGConfig createTestBGConfig(uint8_t screenSize = 0) {
        // Create a simple config
        uint16_t bgcnt = (8 << 8) | (screenSize << 14);
        return gpu->parseBGCNT(bgcnt);
    }
};

// Test 1: Read BG0 scroll registers
TEST_F(ScrollTest, ReadBG0Scroll) {
    writeBGScroll(0, 0, 0);
    BGScroll scroll = gpu->readBGScroll(0);
    EXPECT_EQ(0, scroll.hofs);
    EXPECT_EQ(0, scroll.vofs);
    
    writeBGScroll(0, 10, 20);
    scroll = gpu->readBGScroll(0);
    EXPECT_EQ(10, scroll.hofs);
    EXPECT_EQ(20, scroll.vofs);
}

// Test 2: Read all BG scroll registers
TEST_F(ScrollTest, ReadAllBGScrolls) {
    writeBGScroll(0, 10, 20);
    writeBGScroll(1, 30, 40);
    writeBGScroll(2, 50, 60);
    writeBGScroll(3, 70, 80);
    
    BGScroll scroll0 = gpu->readBGScroll(0);
    BGScroll scroll1 = gpu->readBGScroll(1);
    BGScroll scroll2 = gpu->readBGScroll(2);
    BGScroll scroll3 = gpu->readBGScroll(3);
    
    EXPECT_EQ(10, scroll0.hofs);
    EXPECT_EQ(20, scroll0.vofs);
    EXPECT_EQ(30, scroll1.hofs);
    EXPECT_EQ(40, scroll1.vofs);
    EXPECT_EQ(50, scroll2.hofs);
    EXPECT_EQ(60, scroll2.vofs);
    EXPECT_EQ(70, scroll3.hofs);
    EXPECT_EQ(80, scroll3.vofs);
}

// Test 3: Scroll values are masked to 9 bits (0-511)
TEST_F(ScrollTest, ScrollMasking) {
    // Write values > 511, should be masked to 9 bits
    writeBGScroll(0, 0x03FF, 0x03FF);  // 511 (max valid)
    BGScroll scroll1 = gpu->readBGScroll(0);
    EXPECT_EQ(511, scroll1.hofs);
    EXPECT_EQ(511, scroll1.vofs);
    
    writeBGScroll(0, 0x0400, 0x0500);  // Should mask to 0 and 256
    BGScroll scroll2 = gpu->readBGScroll(0);
    EXPECT_EQ(0, scroll2.hofs);
    EXPECT_EQ(256, scroll2.vofs);
    
    writeBGScroll(0, 0xFFFF, 0xFFFF);  // Should mask to 511
    BGScroll scroll3 = gpu->readBGScroll(0);
    EXPECT_EQ(511, scroll3.hofs);
    EXPECT_EQ(511, scroll3.vofs);
}

// Test 4: Invalid BG number returns zero
TEST_F(ScrollTest, InvalidBGNumber) {
    BGScroll scrollNeg = gpu->readBGScroll(-1);
    BGScroll scroll4 = gpu->readBGScroll(4);
    
    EXPECT_EQ(0, scrollNeg.hofs);
    EXPECT_EQ(0, scrollNeg.vofs);
    EXPECT_EQ(0, scroll4.hofs);
    EXPECT_EQ(0, scroll4.vofs);
}

// Test 5: Apply scroll with no offset (256x256 screen)
TEST_F(ScrollTest, ApplyScrollNoOffset) {
    BGConfig config = createTestBGConfig(0);  // 256x256
    BGScroll scroll;
    scroll.hofs = 0;
    scroll.vofs = 0;
    
    int bgX, bgY;
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(0, bgX);
    EXPECT_EQ(0, bgY);
    
    gpu->applyScroll(config, scroll, 120, 80, bgX, bgY);
    EXPECT_EQ(120, bgX);
    EXPECT_EQ(80, bgY);
}

// Test 6: Apply simple scroll offset
TEST_F(ScrollTest, ApplyScrollSimpleOffset) {
    BGConfig config = createTestBGConfig(0);  // 256x256
    BGScroll scroll;
    scroll.hofs = 10;
    scroll.vofs = 20;
    
    int bgX, bgY;
    
    // Screen (0,0) should map to BG (10,20)
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(10, bgX);
    EXPECT_EQ(20, bgY);
    
    // Screen (50,60) should map to BG (60,80)
    gpu->applyScroll(config, scroll, 50, 60, bgX, bgY);
    EXPECT_EQ(60, bgX);
    EXPECT_EQ(80, bgY);
}

// Test 7: Apply scroll with wrapping (256x256 screen)
TEST_F(ScrollTest, ApplyScrollWrapping256) {
    BGConfig config = createTestBGConfig(0);  // 256x256
    BGScroll scroll;
    scroll.hofs = 250;
    scroll.vofs = 250;
    
    int bgX, bgY;
    
    // Screen (0,0) → BG (250,250)
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(250, bgX);
    EXPECT_EQ(250, bgY);
    
    // Screen (10,10) → BG (260,260) → wraps to (4,4)
    gpu->applyScroll(config, scroll, 10, 10, bgX, bgY);
    EXPECT_EQ(4, bgX);
    EXPECT_EQ(4, bgY);
    
    // Screen (239,159) → BG (489,409) → wraps to (233,153)
    gpu->applyScroll(config, scroll, 239, 159, bgX, bgY);
    EXPECT_EQ(233, bgX);
    EXPECT_EQ(153, bgY);
}

// Test 8: Apply scroll with 512x256 screen
TEST_F(ScrollTest, ApplyScrollWrapping512x256) {
    BGConfig config = createTestBGConfig(1);  // 512x256
    BGScroll scroll;
    scroll.hofs = 500;
    scroll.vofs = 250;
    
    int bgX, bgY;
    
    // Screen (0,0) → BG (500,250)
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(500, bgX);
    EXPECT_EQ(250, bgY);
    
    // Screen (20,10) → BG (520,260) → wraps to (8,4)
    gpu->applyScroll(config, scroll, 20, 10, bgX, bgY);
    EXPECT_EQ(8, bgX);
    EXPECT_EQ(4, bgY);
}

// Test 9: Apply scroll with 512x512 screen
TEST_F(ScrollTest, ApplyScrollWrapping512x512) {
    BGConfig config = createTestBGConfig(3);  // 512x512
    BGScroll scroll;
    scroll.hofs = 500;
    scroll.vofs = 500;
    
    int bgX, bgY;
    
    // Screen (0,0) → BG (500,500)
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(500, bgX);
    EXPECT_EQ(500, bgY);
    
    // Screen (20,20) → BG (520,520) → wraps to (8,8)
    gpu->applyScroll(config, scroll, 20, 20, bgX, bgY);
    EXPECT_EQ(8, bgX);
    EXPECT_EQ(8, bgY);
}

// Test 10: Convert pixel coords to tile coords
TEST_F(ScrollTest, GetTileCoords) {
    int tileX, tileY, pixX, pixY;
    
    // Top-left of tile (0,0)
    gpu->getTileCoords(0, 0, tileX, tileY, pixX, pixY);
    EXPECT_EQ(0, tileX);
    EXPECT_EQ(0, tileY);
    EXPECT_EQ(0, pixX);
    EXPECT_EQ(0, pixY);
    
    // Middle of tile (0,0)
    gpu->getTileCoords(4, 4, tileX, tileY, pixX, pixY);
    EXPECT_EQ(0, tileX);
    EXPECT_EQ(0, tileY);
    EXPECT_EQ(4, pixX);
    EXPECT_EQ(4, pixY);
    
    // Bottom-right of tile (0,0)
    gpu->getTileCoords(7, 7, tileX, tileY, pixX, pixY);
    EXPECT_EQ(0, tileX);
    EXPECT_EQ(0, tileY);
    EXPECT_EQ(7, pixX);
    EXPECT_EQ(7, pixY);
    
    // Top-left of tile (1,0)
    gpu->getTileCoords(8, 0, tileX, tileY, pixX, pixY);
    EXPECT_EQ(1, tileX);
    EXPECT_EQ(0, tileY);
    EXPECT_EQ(0, pixX);
    EXPECT_EQ(0, pixY);
    
    // Top-left of tile (0,1)
    gpu->getTileCoords(0, 8, tileX, tileY, pixX, pixY);
    EXPECT_EQ(0, tileX);
    EXPECT_EQ(1, tileY);
    EXPECT_EQ(0, pixX);
    EXPECT_EQ(0, pixY);
}

// Test 11: Tile coords for various positions
TEST_F(ScrollTest, GetTileCoordsVariousPositions) {
    int tileX, tileY, pixX, pixY;
    
    // Pixel (50, 50) → Tile (6, 6), Pixel (2, 2)
    gpu->getTileCoords(50, 50, tileX, tileY, pixX, pixY);
    EXPECT_EQ(6, tileX);
    EXPECT_EQ(6, tileY);
    EXPECT_EQ(2, pixX);
    EXPECT_EQ(2, pixY);
    
    // Pixel (120, 80) → Tile (15, 10), Pixel (0, 0)
    gpu->getTileCoords(120, 80, tileX, tileY, pixX, pixY);
    EXPECT_EQ(15, tileX);
    EXPECT_EQ(10, tileY);
    EXPECT_EQ(0, pixX);
    EXPECT_EQ(0, pixY);
    
    // Pixel (255, 159) → Tile (31, 19), Pixel (7, 7)
    gpu->getTileCoords(255, 159, tileX, tileY, pixX, pixY);
    EXPECT_EQ(31, tileX);
    EXPECT_EQ(19, tileY);
    EXPECT_EQ(7, pixX);
    EXPECT_EQ(7, pixY);
}

// Test 12: Integration - scroll and tile coords
TEST_F(ScrollTest, IntegrationScrollAndTileCoords) {
    BGConfig config = createTestBGConfig(0);  // 256x256
    BGScroll scroll;
    scroll.hofs = 16;  // Scroll by 2 tiles horizontally
    scroll.vofs = 8;   // Scroll by 1 tile vertically
    
    int bgX, bgY;
    int tileX, tileY, pixX, pixY;
    
    // Screen pixel (0,0) with scroll
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(16, bgX);
    EXPECT_EQ(8, bgY);
    
    // Convert to tile coords
    gpu->getTileCoords(bgX, bgY, tileX, tileY, pixX, pixY);
    EXPECT_EQ(2, tileX);   // Tile 2 (16/8)
    EXPECT_EQ(1, tileY);   // Tile 1 (8/8)
    EXPECT_EQ(0, pixX);
    EXPECT_EQ(0, pixY);
}

// Test 13: Scroll with maximum values
TEST_F(ScrollTest, MaximumScrollValues) {
    BGConfig config = createTestBGConfig(0);  // 256x256
    BGScroll scroll;
    scroll.hofs = 511;  // Max 9-bit value
    scroll.vofs = 511;
    
    int bgX, bgY;
    
    // This should wrap around multiple times
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(255, bgX);  // 511 % 256
    EXPECT_EQ(255, bgY);
    
    gpu->applyScroll(config, scroll, 1, 1, bgX, bgY);
    EXPECT_EQ(0, bgX);   // (1 + 511) % 256 = 512 % 256 = 0
    EXPECT_EQ(0, bgY);
}

// Test 14: Large pixel coordinates
TEST_F(ScrollTest, LargePixelCoordinates) {
    int tileX, tileY, pixX, pixY;
    
    // Pixel (512, 512) → far outside screen
    gpu->getTileCoords(512, 512, tileX, tileY, pixX, pixY);
    EXPECT_EQ(64, tileX);
    EXPECT_EQ(64, tileY);
    EXPECT_EQ(0, pixX);
    EXPECT_EQ(0, pixY);
    
    // Pixel (1000, 1000)
    gpu->getTileCoords(1000, 1000, tileX, tileY, pixX, pixY);
    EXPECT_EQ(125, tileX);
    EXPECT_EQ(125, tileY);
    EXPECT_EQ(0, pixX);
    EXPECT_EQ(0, pixY);
}

// Test 15: Scrolling full screen
TEST_F(ScrollTest, ScrollFullScreen) {
    BGConfig config = createTestBGConfig(0);  // 256x256
    
    // Scroll by exactly one screen (256 pixels)
    BGScroll scroll;
    scroll.hofs = 256 % 512;  // Should be 256
    scroll.vofs = 256 % 512;
    
    int bgX, bgY;
    
    // Should wrap back to start
    gpu->applyScroll(config, scroll, 0, 0, bgX, bgY);
    EXPECT_EQ(0, bgX);  // 256 % 256 = 0
    EXPECT_EQ(0, bgY);
}

// Test 16: Edge cases - boundary tiles
TEST_F(ScrollTest, BoundaryTiles) {
    int tileX, tileY, pixX, pixY;
    
    // Last pixel of screen (239, 159) for GBA
    gpu->getTileCoords(239, 159, tileX, tileY, pixX, pixY);
    EXPECT_EQ(29, tileX);  // 239 / 8 = 29
    EXPECT_EQ(19, tileY);  // 159 / 8 = 19
    EXPECT_EQ(7, pixX);    // 239 % 8 = 7
    EXPECT_EQ(7, pixY);    // 159 % 8 = 7
}

// Test 17: Scroll interaction with different screen sizes
TEST_F(ScrollTest, ScrollDifferentScreenSizes) {
    BGScroll scroll;
    scroll.hofs = 100;
    scroll.vofs = 100;
    int bgX, bgY;
    
    // 256x256
    BGConfig config256 = createTestBGConfig(0);
    gpu->applyScroll(config256, scroll, 50, 50, bgX, bgY);
    EXPECT_EQ(150, bgX);
    EXPECT_EQ(150, bgY);
    
    // 512x256
    BGConfig config512x256 = createTestBGConfig(1);
    gpu->applyScroll(config512x256, scroll, 50, 50, bgX, bgY);
    EXPECT_EQ(150, bgX);
    EXPECT_EQ(150, bgY);
    
    // 512x512
    BGConfig config512 = createTestBGConfig(3);
    gpu->applyScroll(config512, scroll, 50, 50, bgX, bgY);
    EXPECT_EQ(150, bgX);
    EXPECT_EQ(150, bgY);
}
