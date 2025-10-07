#include <gtest/gtest.h>
#include "../../include/gpu.h"
#include "../../include/memory.h"
#include "../../include/gba.h"

// ============================================================================
// Enhanced Alpha Blending Tests
// Tests for full alpha blending with proper second layer tracking
// ============================================================================

class EnhancedBlendTest : public ::testing::Test {
protected:
    std::unique_ptr<Memory> memory;
    std::unique_ptr<GPU> gpu;
    
    void SetUp() override {
        memory = std::make_unique<Memory>();
        gpu = std::make_unique<GPU>(*memory);
    }
    
    // Helper to set up a background with specific color
    void setupBG(int bgNum, uint16_t color, uint8_t priority) {
        // Enable BG in DISPCNT
        uint16_t dispcnt = memory->read16(0x04000000);
        memory->write16(0x04000000, dispcnt | (1 << (8 + bgNum)));
        
        // Configure BG: 4bpp, screen base varies per BG, character base 0
        // Use different screen bases to avoid overlap
        uint8_t screenBase = 2 + bgNum;  // Screen bases 2, 3, 4, 5
        memory->write16(0x04000008 + bgNum * 2, (screenBase << 8) | priority);
        
        // Write color to palette 0, color index (bgNum + 1)
        memory->write16(0x05000000 + (bgNum + 1) * 2, color);
        
        // Fill tilemap at screen base with tile (bgNum + 1)
        uint32_t tilemapAddr = 0x06000000 + (screenBase * 0x800);
        uint16_t tileEntry = (bgNum + 1);  // Tile number, palette 0
        for (int i = 0; i < 32 * 32; i++) {
            memory->write16(tilemapAddr + i * 2, tileEntry);
        }
        
        // Write tile data to character base 0
        // Tile (bgNum + 1): all pixels = color index (bgNum + 1)
        uint32_t tileAddr = 0x06000000 + ((bgNum + 1) * 32);  // 32 bytes per tile
        uint8_t colorIndex = bgNum + 1;
        uint8_t pixelPair = (colorIndex << 4) | colorIndex;  // Two 4-bit pixels
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, pixelPair);
        }
    }
};

// Test 1: Alpha blend between two overlapping backgrounds
TEST_F(EnhancedBlendTest, AlphaBlendBetweenTwoBackgrounds) {
    // Setup BG0 with red color (priority 0 - on top)
    setupBG(0, 0x001F, 0);  // Red: 0bBBBBBGGGGGRRRRR = 0x001F
    
    // Setup BG1 with blue color (priority 1 - behind)
    setupBG(1, 0x7C00, 1);  // Blue: 0x7C00
    
    // Enable alpha blending: BG0 as first target, BG1 as second target
    // BLDCNT: First targets (bit 0=BG0), Mode=1 (alpha), Second targets (bit 9=BG1)
    memory->write16(0x04000050, 0x0201 | (1 << 6));  // BG0 first, BG1 second, alpha mode
    
    // Set blend coefficients: EVA=8, EVB=8 (50/50 blend)
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    
    // Check pixel at (100, 0) - should be blended
    uint16_t pixel = framebuffer[100];
    
    // Expected: (red * 8 + blue * 8) / 16
    // Red:  R=31, G=0,  B=0
    // Blue: R=0,  G=0,  B=31
    // Result: R=15.5, G=0, B=15.5
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(15, r);  // ~15-16 due to rounding
    EXPECT_EQ(0, g);
    EXPECT_EQ(15, b);  // ~15-16 due to rounding
}

// Test 2: Alpha blend with different coefficients
TEST_F(EnhancedBlendTest, AlphaBlendWithDifferentCoefficients) {
    // Setup BG0 with red color (priority 0)
    setupBG(0, 0x001F, 0);  // Red
    
    // Setup BG1 with blue color (priority 1)
    setupBG(1, 0x7C00, 1);  // Blue
    
    // Enable alpha blending
    memory->write16(0x04000050, 0x0201 | (1 << 6));  // BG0 first, BG1 second, alpha mode
    
    // Set blend coefficients: EVA=12, EVB=4 (75% first, 25% second)
    memory->write16(0x04000052, 0x040C);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[100];
    
    // Expected: (red * 12 + blue * 4) / 16
    // Red:  R=31, G=0,  B=0
    // Blue: R=0,  G=0,  B=31
    // Result: R=23.25, G=0, B=7.75
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(23, r);  // 75% of 31
    EXPECT_EQ(0, g);
    EXPECT_EQ(7, b);   // 25% of 31
}

// Test 3: No blend if first layer not a first target
TEST_F(EnhancedBlendTest, NoBlendIfNotFirstTarget) {
    // Setup BG0 with red color (priority 0)
    setupBG(0, 0x001F, 0);  // Red
    
    // Setup BG1 with blue color (priority 1)
    setupBG(1, 0x7C00, 1);  // Blue
    
    // Enable alpha blending but don't mark BG0 as first target
    memory->write16(0x04000050, 0x0200 | (1 << 6));  // No first target, BG1 second, alpha mode
    
    // Set blend coefficients
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[100];
    
    // Should be pure red (no blending)
    EXPECT_EQ(0x001F, pixel);
}

// Test 4: No blend if second layer not a second target
TEST_F(EnhancedBlendTest, NoBlendIfNotSecondTarget) {
    // Setup BG0 with red color (priority 0)
    setupBG(0, 0x001F, 0);  // Red
    
    // Setup BG1 with blue color (priority 1)
    setupBG(1, 0x7C00, 1);  // Blue
    
    // Enable alpha blending but don't mark BG1 as second target
    memory->write16(0x04000050, 0x0001 | (1 << 6));  // BG0 first, no second target, alpha mode
    
    // Set blend coefficients
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[100];
    
    // Should be pure red (no blending)
    EXPECT_EQ(0x001F, pixel);
}

// Test 5: Alpha blend between background and backdrop
TEST_F(EnhancedBlendTest, AlphaBlendWithBackdrop) {
    // Set backdrop color to green
    memory->write16(0x05000000, 0x03E0);  // Green: 0bBBBBBGGGGGRRRRR = 0x03E0
    
    // Setup BG0 with red color (priority 0)
    setupBG(0, 0x001F, 0);  // Red
    
    // Enable alpha blending: BG0 as first target, backdrop as second target
    // Backdrop is bit 5 in targets
    memory->write16(0x04000050, 0x2001 | (1 << 6));  // BG0 first, backdrop second, alpha mode
    
    // Set blend coefficients: EVA=8, EVB=8
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[100];
    
    // Expected: (red * 8 + green * 8) / 16
    // Red:   R=31, G=0,  B=0
    // Green: R=0,  G=31, B=0
    // Result: R=15.5, G=15.5, B=0
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(15, r);
    EXPECT_EQ(15, g);
    EXPECT_EQ(0, b);
}

// Test 6: Three layers - blend only affects top two
TEST_F(EnhancedBlendTest, ThreeLayersBlendTopTwo) {
    // Setup BG0 with red (priority 0 - top)
    setupBG(0, 0x001F, 0);
    
    // Setup BG1 with green (priority 1 - middle)
    setupBG(1, 0x03E0, 1);
    
    // Setup BG2 with blue (priority 2 - bottom)
    setupBG(2, 0x7C00, 2);
    
    // Enable alpha blending: BG0 and BG1
    memory->write16(0x04000050, 0x0201 | (1 << 6));
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[100];
    
    // Should blend red (BG0) with green (BG1), ignoring blue (BG2)
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(15, r);  // From red
    EXPECT_EQ(15, g);  // From green
    EXPECT_EQ(0, b);   // No blue contribution
}

// Test 7: Alpha blend with maximum coefficients
TEST_F(EnhancedBlendTest, AlphaBlendMaximumCoefficients) {
    // Setup BG0 with red
    setupBG(0, 0x001F, 0);
    
    // Setup BG1 with blue
    setupBG(1, 0x7C00, 1);
    
    // Enable alpha blending
    memory->write16(0x04000050, 0x0201 | (1 << 6));
    
    // Set blend coefficients: EVA=16, EVB=16 (clamped to max)
    memory->write16(0x04000052, 0x1010);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[100];
    
    // Expected: (red * 16 + blue * 16) / 16 = red + blue (clamped to 31)
    // Result should be clamped to white
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(31, r);  // Clamped
    EXPECT_EQ(0, g);
    EXPECT_EQ(31, b);  // Clamped
}

// Test 8: Alpha blend priority - higher priority layer is first target
TEST_F(EnhancedBlendTest, AlphaBlendRespectsPriority) {
    // Setup BG0 with red (priority 1 - will be behind)
    setupBG(0, 0x001F, 1);
    
    // Setup BG1 with blue (priority 0 - will be on top)
    setupBG(1, 0x7C00, 0);
    
    // Enable alpha blending: BG1 as first (on top), BG0 as second
    memory->write16(0x04000050, 0x0102 | (1 << 6));  // BG1 first, BG0 second
    memory->write16(0x04000052, 0x0808);
    
    // Render scanline 0
    gpu->renderScanline(0);
    
    // Get framebuffer
    uint16_t* framebuffer = gpu->getTiledFramebuffer();
    uint16_t pixel = framebuffer[100];
    
    // Should blend blue (BG1 on top) with red (BG0 behind)
    uint8_t r = pixel & 0x1F;
    uint8_t g = (pixel >> 5) & 0x1F;
    uint8_t b = (pixel >> 10) & 0x1F;
    
    EXPECT_EQ(15, r);
    EXPECT_EQ(0, g);
    EXPECT_EQ(15, b);
}
