/**
 * Test Suite: Blend and Window Integration Tests
 * 
 * Tests that blend and window features work correctly when integrated
 * into the actual rendering pipeline.
 */

#include <gtest/gtest.h>
#include "gba.h"
#include "memory.h"
#include "gpu.h"

class BlendWindowIntegrationTest : public ::testing::Test {
protected:
    Memory* memory;
    GPU* gpu;

    void SetUp() override {
        memory = new Memory();
        gpu = new GPU(*memory);
    }

    void TearDown() override {
        delete gpu;
        delete memory;
    }
    
    // Helper to set up a simple BG with one tile
    void setupSimpleBG(int bgNum, uint16_t color) {
        // Enable Mode 0 and the BG
        memory->write16(REG_DISPCNT, (1 << (8 + bgNum)));  // Enable BGx
        
        // Configure BG: Priority 0, charbase 0, screenbase (bgNum+1)*2KB, palette bgNum
        // Each BG gets its own screen base and palette to avoid conflicts
        uint16_t bgcnt = (bgNum + 1) << 8;  // Screen base block
        bgcnt |= (bgNum << 12);             // Palette bank (4bpp)
        memory->write16(REG_BG0CNT + (bgNum * 2), bgcnt);
        
        // Set scroll to 0
        memory->write16(REG_BG0HOFS + (bgNum * 4), 0);
        memory->write16(REG_BG0VOFS + (bgNum * 4), 0);
        
        // Set palette color 1 for this BG's palette bank
        uint32_t paletteAddr = 0x05000000 + (bgNum * 16 * 2) + 2;  // Palette bgNum, color 1
        memory->write16(paletteAddr, color);
        
        // Fill the entire 32x32 tilemap with tile 1
        uint32_t tilemapAddr = 0x06000800 + (bgNum * 0x800);  // Each BG gets 2KB tilemap
        for (int i = 0; i < 32 * 32; i++) {
            memory->write16(tilemapAddr + i * 2, 0x0001);  // Tile 1 (non-transparent)
        }
        
        // Fill tile 1 with color index 1 (tile 1 starts at offset 32 bytes)
        for (int i = 0; i < 32; i++) {
            memory->write8(0x06000000 + 32 + i, 0x11);  // 4bpp: two pixels of color 1
        }
    }
};

// ============================================================================
// Window Integration Tests
// ============================================================================

TEST_F(BlendWindowIntegrationTest, WindowMasksBG) {
    // Set up BG0 with red color
    setupSimpleBG(0, 0x001F);  // Pure red
    
    // Enable Window 0: only show pixels in x=[50,150], y=[30,100]
    memory->write16(REG_DISPCNT, DISPCNT_BG0_ENABLE | DISPCNT_WIN0_ENABLE);
    memory->write16(REG_WIN0H, 0x3296);  // left=50, right=150
    memory->write16(REG_WIN0V, 0x1E64);  // top=30, bottom=100
    memory->write16(REG_WININ, 0x0001);  // BG0 visible inside window
    memory->write16(REG_WINOUT, 0x0000); // Nothing visible outside
    
    // Render scanline 50 (inside window vertically)
    gpu->renderScanline(50);
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Check pixels
    EXPECT_NE(fb[50 * 240 + 25], 0x001F);  // Outside window (x=25) - should be backdrop
    EXPECT_EQ(fb[50 * 240 + 100], 0x001F); // Inside window (x=100) - should be red
    EXPECT_NE(fb[50 * 240 + 175], 0x001F); // Outside window (x=175) - should be backdrop
    
    // Render scanline 20 (outside window vertically)
    gpu->renderScanline(20);
    EXPECT_NE(fb[20 * 240 + 100], 0x001F); // Outside window (y=20) - should be backdrop
}

TEST_F(BlendWindowIntegrationTest, WindowShowsDifferentLayersInOut) {
    // Set up BG0 with red - use palette 0, screen base 1
    memory->write16(REG_BG0CNT, (1 << 8) | 0);  // Screen base 1, palette 0, priority 0
    memory->write16(0x05000002, 0x001F);  // Palette 0, color 1 = red
    for (int i = 0; i < 32 * 32; i++) {
        memory->write16(0x06000800 + i * 2, 0x0001);  // Fill tilemap with tile 1
    }
    for (int i = 0; i < 32; i++) {
        memory->write8(0x06000000 + 32 + i, 0x11);  // Fill tile 1 with color index 1
    }
    
    // Set up BG1 with blue - use palette 1, screen base 2
    memory->write16(REG_BG1CNT, (2 << 8) | 1);  // Screen base 2, priority 1
    memory->write16(0x05000022, 0x7C00);  // Palette 1, color 1 = blue
    for (int i = 0; i < 32 * 32; i++) {
        // Tilemap entry: tile 1, palette 1 (bits 12-15)
        memory->write16(0x06001000 + i * 2, 0x0001 | (1 << 12));  // Tile 1, palette 1
    }
    // Tile data already set up above (shared between BGs)
    
    // Enable both BGs and Window 0
    memory->write16(REG_DISPCNT, DISPCNT_BG0_ENABLE | DISPCNT_BG1_ENABLE | DISPCNT_WIN0_ENABLE);
    
    // Window: BG0 inside, BG1 outside
    memory->write16(REG_WIN0H, 0x4678);  // left=70, right=120
    memory->write16(REG_WIN0V, 0x0050);  // top=0, bottom=80
    memory->write16(REG_WININ, 0x0001);  // BG0 visible inside
    memory->write16(REG_WINOUT, 0x0002); // BG1 visible outside
    
    gpu->renderScanline(40);
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Inside window should show BG0 (red, higher priority)
    EXPECT_EQ(fb[40 * 240 + 90], 0x001F);
    
    // Outside window should show BG1 (blue)
    EXPECT_EQ(fb[40 * 240 + 50], 0x7C00);
    EXPECT_EQ(fb[40 * 240 + 150], 0x7C00);
}

// ============================================================================
// Blend Integration Tests
// ============================================================================

TEST_F(BlendWindowIntegrationTest, BrightnessIncreaseOnBG) {
    // Set up BG0 with dark gray
    setupSimpleBG(0, 0x4210);  // RGB(16,16,16)
    
    // Enable brighten mode: BG0 is first target, EVY=8 (50%)
    memory->write16(REG_BLDCNT, 0x0081);  // Bit 0: BG0 1st target, bits 6-7=2 (brighten)
    memory->write16(REG_BLDY, 0x0008);    // EVY=8
    
    gpu->renderScanline(0);
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Check that color has been brightened
    uint16_t result = fb[0];
    
    // Extract RGB components
    uint8_t r = result & 0x1F;
    uint8_t g = (result >> 5) & 0x1F;
    uint8_t b = (result >> 10) & 0x1F;
    
    // Original was RGB(16,16,16), brightened by 50%
    // Expected: 16 + (31-16)*8/16 = 16 + 7 = 23
    EXPECT_GT(r, 16);  // Should be brighter than original
    EXPECT_GT(g, 16);
    EXPECT_GT(b, 16);
    EXPECT_LE(r, 24);  // But not too bright
    EXPECT_LE(g, 24);
    EXPECT_LE(b, 24);
}

TEST_F(BlendWindowIntegrationTest, BrightnessDecreaseOnBG) {
    // Set up BG0 with bright gray
    setupSimpleBG(0, 0x7BDE);  // RGB(30,30,30)
    
    // Enable darken mode: BG0 is first target, EVY=8 (50%)
    memory->write16(REG_BLDCNT, 0x00C1);  // Bit 0: BG0 1st target, bits 6-7=3 (darken)
    memory->write16(REG_BLDY, 0x0008);    // EVY=8
    
    gpu->renderScanline(0);
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Check that color has been darkened
    uint16_t result = fb[0];
    
    uint8_t r = result & 0x1F;
    uint8_t g = (result >> 5) & 0x1F;
    uint8_t b = (result >> 10) & 0x1F;
    
    // Original was RGB(30,30,30), darkened by 50%
    // Expected: 30 - 30*8/16 = 30 - 15 = 15
    EXPECT_LT(r, 30);  // Should be darker than original
    EXPECT_LT(g, 30);
    EXPECT_LT(b, 30);
    EXPECT_GE(r, 14);  // But not too dark
    EXPECT_GE(g, 14);
    EXPECT_GE(b, 14);
}

TEST_F(BlendWindowIntegrationTest, BlendOnlyAffectsTargetLayers) {
    // Set up BG0 with red - use palette 0, screen base 1
    memory->write16(REG_DISPCNT, DISPCNT_BG0_ENABLE);
    memory->write16(REG_BG0CNT, (1 << 8) | 1);  // Screen base 1, palette 0, priority 1
    memory->write16(0x05000002, 0x001F);  // Palette 0, color 1 = red
    for (int i = 0; i < 32 * 32; i++) {
        memory->write16(0x06000800 + i * 2, 0x0001);  // Fill tilemap with tile 1
    }
    for (int i = 0; i < 32; i++) {
        memory->write8(0x06000000 + 32 + i, 0x11);  // Fill tile 1 with color index 1
    }
    
    // Set up BG1 with blue - use palette 1, screen base 2
    memory->write16(REG_BG1CNT, (2 << 8) | 0);  // Screen base 2, priority 0
    memory->write16(0x05000022, 0x7C00);  // Palette 1, color 1 = blue
    for (int i = 0; i < 32 * 32; i++) {
        // Tilemap entry: tile 1, palette 1 (bits 12-15)
        memory->write16(0x06001000 + i * 2, 0x0001 | (1 << 12));  // Tile 1, palette 1
    }
    // Tile data already set up above (shared between BGs)
    
    // Enable both BGs
    memory->write16(REG_DISPCNT, DISPCNT_BG0_ENABLE | DISPCNT_BG1_ENABLE);
    
    // Enable brighten mode: Only BG1 is first target
    memory->write16(REG_BLDCNT, 0x0082);  // Bit 1: BG1 1st target, mode=2 (brighten)
    memory->write16(REG_BLDY, 0x0010);    // EVY=16 (full white)
    
    gpu->renderScanline(0);
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // BG1 (blue) should be brightened to white
    uint16_t result = fb[0];
    
    // Should be very bright (close to white)
    uint8_t r = result & 0x1F;
    uint8_t g = (result >> 5) & 0x1F;
    uint8_t b = (result >> 10) & 0x1F;
    
    // With EVY=16, blue (0,0,31) becomes (31,31,31) - white
    EXPECT_EQ(r, 31);
    EXPECT_EQ(g, 31);
    EXPECT_EQ(b, 31);
}

TEST_F(BlendWindowIntegrationTest, BlendOffDoesNothing) {
    // Set up BG0 with red
    setupSimpleBG(0, 0x001F);
    
    // Set blend mode to OFF
    memory->write16(REG_BLDCNT, 0x0001);  // BG0 is target, but mode=0 (off)
    memory->write16(REG_BLDY, 0x0010);    // EVY=16 (should be ignored)
    
    gpu->renderScanline(0);
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Color should be unchanged
    EXPECT_EQ(fb[0], 0x001F);  // Still pure red
}

// ============================================================================
// Window + Blend Combination Tests
// ============================================================================

TEST_F(BlendWindowIntegrationTest, WindowAndBlendTogether) {
    // Set up BG0 with gray
    setupSimpleBG(0, 0x4210);  // RGB(16,16,16)
    
    // Enable window and blend
    memory->write16(REG_DISPCNT, DISPCNT_BG0_ENABLE | DISPCNT_WIN0_ENABLE);
    
    // Window: BG0 visible inside only
    memory->write16(REG_WIN0H, 0x5078);  // left=80, right=120
    memory->write16(REG_WIN0V, 0x0050);  // top=0, bottom=80
    memory->write16(REG_WININ, 0x0001);  // BG0 visible
    memory->write16(REG_WINOUT, 0x0000); // Nothing outside
    
    // Brighten BG0
    memory->write16(REG_BLDCNT, 0x0081);  // BG0 1st target, brighten mode
    memory->write16(REG_BLDY, 0x0008);    // EVY=8
    
    gpu->renderScanline(40);
    uint16_t* fb = gpu->getTiledFramebuffer();
    
    // Inside window: should see brightened BG0
    uint16_t insideColor = fb[40 * 240 + 100];
    uint8_t r = insideColor & 0x1F;
    EXPECT_GT(r, 16);  // Brightened
    
    // Outside window: should see backdrop (black)
    uint16_t outsideColor = fb[40 * 240 + 50];
    EXPECT_EQ(outsideColor, 0x0000);  // Backdrop
}

// Test counters
// This test suite adds 8 integration tests:
// - 2 window masking tests
// - 4 blend effect tests
// - 2 window + blend combination tests
// Total new tests: 8
