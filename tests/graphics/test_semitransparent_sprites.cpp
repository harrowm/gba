#include <gtest/gtest.h>
#include "../../include/gpu.h"
#include "../../include/memory.h"
#include "../../include/gba.h"

// ============================================================================
// Semi-Transparent Sprite Tests
// Tests for sprites with objMode == 1 (OBJ_MODE_SEMI_TRANSPARENT)
// ============================================================================

class SemiTransparentSpriteTest : public ::testing::Test {
protected:
    std::unique_ptr<Memory> memory;
    std::unique_ptr<GPU> gpu;
    
    void SetUp() override {
        memory = std::make_unique<Memory>();
        gpu = std::make_unique<GPU>(*memory);
        
        // Enable OBJ in DISPCNT (Mode 0)
        memory->write16(0x04000000, 0x1000);
    }
    
    // Helper to create a sprite with specific color and mode
    void createSprite(int objNum, int x, int y, uint16_t color, uint8_t objMode, uint8_t priority = 0) {
        uint32_t oamBase = 0x07000000 + objNum * 8;
        
        // Attribute 0: Y position, objMode, shape (square)
        uint16_t attr0 = (y & 0xFF) | (objMode << 10);
        memory->write16(oamBase, attr0);
        
        // Attribute 1: X position, size (8x8)
        uint16_t attr1 = (x & 0x1FF);
        memory->write16(oamBase + 2, attr1);
        
        // Attribute 2: tile number 1, priority, palette 0
        uint16_t attr2 = 1 | (priority << 10);
        memory->write16(oamBase + 4, attr2);
        
        // Write sprite tile data (8x8, 4bpp)
        // All pixels use color index 1
        uint32_t tileAddr = 0x06010000 + 32;  // Tile 1
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, 0x11);  // Color index 1 in both nibbles
        }
        
        // Write sprite palette (palette 0, color 1)
        memory->write16(0x05000200 + 2, color);
    }
    
    // Helper to set up a background
    void setupBG(int bgNum, uint16_t color, uint8_t priority) {
        // Enable BG in DISPCNT
        uint16_t dispcnt = memory->read16(0x04000000);
        memory->write16(0x04000000, dispcnt | (1 << (8 + bgNum)));
        
        // Configure BG: 4bpp, screen base varies per BG
        uint8_t screenBase = 2 + bgNum;
        memory->write16(0x04000008 + bgNum * 2, (screenBase << 8) | priority);
        
        // Write color to palette 0, color index (bgNum + 1)
        memory->write16(0x05000000 + (bgNum + 1) * 2, color);
        
        // Fill tilemap with tile (bgNum + 1)
        uint32_t tilemapAddr = 0x06000000 + (screenBase * 0x800);
        uint16_t tileEntry = (bgNum + 1);
        for (int i = 0; i < 32 * 32; i++) {
            memory->write16(tilemapAddr + i * 2, tileEntry);
        }
        
        // Write tile data
        uint32_t tileAddr = 0x06000000 + ((bgNum + 1) * 32);
        uint8_t colorIndex = bgNum + 1;
        uint8_t pixelPair = (colorIndex << 4) | colorIndex;
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, pixelPair);
        }
    }
};

// Test 1: Semi-transparent sprite blends with background
TEST_F(SemiTransparentSpriteTest, SemiTransparentSpriteBlendWithBackground) {
    // Setup BG0 with red color (priority 1 - behind sprite)
    setupBG(0, 0x001F, 1);  // Red
    
    // Create semi-transparent sprite with blue color at (100, 50)
    createSprite(0, 100, 50, 0x7C00, OBJ_MODE_SEMI_TRANSPARENT, 0);  // Blue, priority 0
    
    // Enable alpha blending: OBJ as first target, BG0 as second target
    // BLDCNT: First targets (bit 4=OBJ), Mode=1 (alpha), Second targets (bit 8=BG0)
    memory->write16(0x04000050, 0x0110 | (1 << 6));
    
    // Set blend coefficients: EVA=8, EVB=8 (50/50)
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 50 (where sprite is)
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    
    // Check pixel at (104, 50) - inside sprite
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Expected: (blue * 8 + red * 8) / 16
    // Blue: R=0,  G=0,  B=31
    // Red:  R=31, G=0,  B=0
    // Result: R=15.5, G=0, B=15.5
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(15, r);
    EXPECT_EQ(0, g);
    EXPECT_EQ(15, b);
}

// Test 2: Semi-transparent sprite with different blend ratios
TEST_F(SemiTransparentSpriteTest, DifferentBlendCoefficients) {
    // Setup BG0 with green
    setupBG(0, 0x03E0, 1);  // Green
    
    // Create semi-transparent sprite with red
    createSprite(0, 100, 50, 0x001F, OBJ_MODE_SEMI_TRANSPARENT, 0);  // Red
    
    // Enable alpha blending: OBJ first, BG0 second
    memory->write16(0x04000050, 0x0110 | (1 << 6));
    
    // Set blend coefficients: EVA=12, EVB=4 (75% sprite, 25% BG)
    memory->write16(0x04000052, 0x040C);
    
    // Render scanline 50
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Expected: (red * 12 + green * 4) / 16
    // Red:   R=31, G=0,  B=0
    // Green: R=0,  G=31, B=0
    // Result: R=23.25, G=7.75, B=0
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(23, r);
    EXPECT_EQ(7, g);
    EXPECT_EQ(0, b);
}

// Test 3: Semi-transparent sprite with backdrop
TEST_F(SemiTransparentSpriteTest, SemiTransparentSpriteWithBackdrop) {
    // Set backdrop to green
    memory->write16(0x05000000, 0x03E0);  // Green
    
    // Create semi-transparent sprite with red (no BG, so it blends with backdrop)
    createSprite(0, 100, 50, 0x001F, OBJ_MODE_SEMI_TRANSPARENT, 0);
    
    // Enable alpha blending: OBJ first, backdrop (bit 5) second
    memory->write16(0x04000050, 0x2010 | (1 << 6));
    
    // Set blend coefficients: EVA=8, EVB=8
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 50
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Expected: (red * 8 + green * 8) / 16
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(15, r);
    EXPECT_EQ(15, g);
    EXPECT_EQ(0, b);
}

// Test 4: Normal sprite (objMode=0) does NOT blend
TEST_F(SemiTransparentSpriteTest, NormalSpriteDoesNotBlend) {
    // Setup BG0 with red
    setupBG(0, 0x001F, 1);
    
    // Create NORMAL sprite (objMode=0) with blue
    createSprite(0, 100, 50, 0x7C00, OBJ_MODE_NORMAL, 0);
    
    // Enable alpha blending (but sprite is normal, so no blend)
    memory->write16(0x04000050, 0x0110 | (1 << 6));
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 50
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Should be pure blue (no blending)
    EXPECT_EQ(0x7C00, pixel);
}

// Test 5: Semi-transparent sprite without blend targets enabled
TEST_F(SemiTransparentSpriteTest, NoBlendIfTargetsNotSet) {
    // Setup BG0 with red
    setupBG(0, 0x001F, 1);
    
    // Create semi-transparent sprite with blue
    createSprite(0, 100, 50, 0x7C00, OBJ_MODE_SEMI_TRANSPARENT, 0);
    
    // Enable alpha blending but DON'T mark OBJ as first target
    memory->write16(0x04000050, 0x0100 | (1 << 6));  // BG0 first, BG0 second (not OBJ)
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 50
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Should be pure blue (no blending because OBJ not a first target)
    EXPECT_EQ(0x7C00, pixel);
}

// Test 6: Semi-transparent sprite blends with another sprite behind it
TEST_F(SemiTransparentSpriteTest, SemiTransparentSpriteOverAnotherSprite) {
    // Create normal sprite with red at (100, 50) priority 1 (behind) - palette 0
    createSprite(0, 100, 50, 0x001F, OBJ_MODE_NORMAL, 1);
    
    // Create semi-transparent sprite with blue at (100, 50) priority 0 (front) - palette 1
    // Need to use different tile and palette for second sprite
    {
        uint32_t oamBase = 0x07000000 + 1 * 8;
        uint16_t attr0 = (50 & 0xFF) | (OBJ_MODE_SEMI_TRANSPARENT << 10);
        memory->write16(oamBase, attr0);
        uint16_t attr1 = (100 & 0x1FF);
        memory->write16(oamBase + 2, attr1);
        uint16_t attr2 = 2 | (0 << 10) | (1 << 12);  // Tile 2, priority 0, palette 1
        memory->write16(oamBase + 4, attr2);
        
        // Write tile 2 data
        uint32_t tileAddr = 0x06010000 + 64;
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, 0x11);
        }
        
        // Write blue color to palette 1, color 1
        memory->write16(0x05000200 + 16 * 2 + 2, 0x7C00);  // Blue
    }
    
    // Enable alpha blending: OBJ first and second
    memory->write16(0x04000050, 0x1010 | (1 << 6));
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 50
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Expected: blend blue sprite with red sprite
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(15, r);  // 50% red
    EXPECT_EQ(0, g);
    EXPECT_EQ(15, b);  // 50% blue
}

// Test 7: Semi-transparent sprite respects EVA=0 (invisible)
TEST_F(SemiTransparentSpriteTest, EVAZeroMakesSpriteInvisible) {
    // Setup BG0 with red
    setupBG(0, 0x001F, 1);
    
    // Create semi-transparent sprite with blue
    createSprite(0, 100, 50, 0x7C00, OBJ_MODE_SEMI_TRANSPARENT, 0);
    
    // Enable alpha blending
    memory->write16(0x04000050, 0x0110 | (1 << 6));
    
    // Set EVA=0, EVB=16 (0% sprite, 100% background)
    memory->write16(0x04000052, 0x1000);
    
    // Render scanline 50
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Should be pure red (0% blue sprite)
    EXPECT_EQ(0x001F, pixel);
}

// Test 8: Multiple semi-transparent sprites at different priorities
TEST_F(SemiTransparentSpriteTest, MultipleSemiTransparentSprites) {
    // Setup BG0 with white
    setupBG(0, 0x7FFF, 2);  // White
    
    // Create semi-transparent sprite 1 with red at priority 1 - palette 0
    createSprite(0, 100, 50, 0x001F, OBJ_MODE_SEMI_TRANSPARENT, 1);
    
    // Create semi-transparent sprite 2 with blue at priority 0 (on top) - palette 1
    {
        uint32_t oamBase = 0x07000000 + 1 * 8;
        uint16_t attr0 = (50 & 0xFF) | (OBJ_MODE_SEMI_TRANSPARENT << 10);
        memory->write16(oamBase, attr0);
        uint16_t attr1 = (100 & 0x1FF);
        memory->write16(oamBase + 2, attr1);
        uint16_t attr2 = 2 | (0 << 10) | (1 << 12);  // Tile 2, priority 0, palette 1
        memory->write16(oamBase + 4, attr2);
        
        uint32_t tileAddr = 0x06010000 + 64;
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, 0x11);
        }
        memory->write16(0x05000200 + 16 * 2 + 2, 0x7C00);  // Palette 1, color 1: Blue
    }
    
    // Enable alpha blending: OBJ first and second
    memory->write16(0x04000050, 0x1010 | (1 << 6));
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 50
    gpu->renderScanline(50);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[50 * 240 + 104];
    
    // Blue sprite (priority 0) blends with red sprite (priority 1)
    // The red sprite itself is semi-transparent but that's its second blend
    // We should see blue blended with red
    uint8_t r = pixel & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    // Should have both red and blue components
    EXPECT_GT(r, 0);  // Has red component
    EXPECT_GT(b, 0);  // Has blue component
}
