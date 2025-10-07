/**
 * Test Suite: Blend and Window Effects (Day 7-8 Session 3)
 * 
 * Tests GBA blend effects and window masking:
 * - Alpha blending between layers
 * - Brightness increase/decrease
 * - Window boundaries and control
 * - Layer visibility masking
 */

#include <gtest/gtest.h>
#include "gba.h"
#include "memory.h"
#include "gpu.h"

class BlendWindowTest : public ::testing::Test {
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
    
    // Helper to set up palette colors
    void setupPalette() {
        // Color 0: Black (backdrop) - RGB(0,0,0)
        memory->write16(0x05000000, 0x0000);
        
        // Color 1: Pure Red - RGB(31,0,0)
        memory->write16(0x05000002, 0x001F);
        
        // Color 2: Pure Green - RGB(0,31,0)
        memory->write16(0x05000004, 0x03E0);
        
        // Color 3: Pure Blue - RGB(0,0,31)
        memory->write16(0x05000006, 0x7C00);
        
        // Color 4: White - RGB(31,31,31)
        memory->write16(0x05000008, 0x7FFF);
        
        // Color 5: Gray - RGB(16,16,16)
        memory->write16(0x0500000A, 0x4210);
    }
};

// ============================================================================
// Blend Control Register Tests
// ============================================================================

TEST_F(BlendWindowTest, BlendControlParsing) {
    // Set BLDCNT: BG0 1st target, BG1 2nd target, alpha blend mode
    memory->write16(REG_BLDCNT, 0x0241);  // Bit 0=BG0, Bit 6-7=1 (alpha), Bit 9=BG1
    
    BlendControl blend = gpu->readBlendControl();
    
    EXPECT_EQ(blend.mode, BLEND_MODE_ALPHA);
    EXPECT_EQ(blend.firstTargets & BLDCNT_BG0_1ST, BLDCNT_BG0_1ST);
    EXPECT_EQ(blend.secondTargets & (BLDCNT_BG1_2ND >> 8), (BLDCNT_BG1_2ND >> 8));
}

TEST_F(BlendWindowTest, AlphaCoefficients) {
    // Set BLDALPHA: EVA=8, EVB=8 (50/50 blend)
    memory->write16(REG_BLDALPHA, 0x0808);
    
    BlendControl blend = gpu->readBlendControl();
    
    EXPECT_EQ(blend.eva, 8);
    EXPECT_EQ(blend.evb, 8);
}

TEST_F(BlendWindowTest, AlphaCoefficientClamping) {
    // Set BLDALPHA: EVA=31 (overflow), EVB=20 (overflow)
    memory->write16(REG_BLDALPHA, 0x141F);
    
    BlendControl blend = gpu->readBlendControl();
    
    EXPECT_EQ(blend.eva, 16);  // Clamped to max
    EXPECT_EQ(blend.evb, 16);  // Clamped to max
}

TEST_F(BlendWindowTest, BrightnessCoefficient) {
    // Set BLDY: EVY=8 (50% brightness change)
    memory->write16(REG_BLDY, 0x0008);
    
    BlendControl blend = gpu->readBlendControl();
    
    EXPECT_EQ(blend.evy, 8);
}

// ============================================================================
// Brightness Effects Tests
// ============================================================================

TEST_F(BlendWindowTest, BrightnessIncrease) {
    setupPalette();
    
    // Test brightening pure red RGB(31,0,0) by 50% (evy=8)
    uint16_t red = 0x001F;  // RGB(31,0,0)
    uint16_t result = gpu->applyBrightnessIncrease(red, 8);
    
    // Expected: R = 31 + (31-31)*8/16 = 31, G = 0 + (31-0)*8/16 = 15, B = same
    uint8_t r = result & 0x1F;
    uint8_t g = (result >> 5) & 0x1F;
    uint8_t b = (result >> 10) & 0x1F;
    
    EXPECT_EQ(r, 31);  // Already max, stays max
    EXPECT_EQ(g, 15);  // 0 + 31*8/16 = 15
    EXPECT_EQ(b, 15);  // 0 + 31*8/16 = 15
}

TEST_F(BlendWindowTest, BrightnessIncreaseToWhite) {
    setupPalette();
    
    // Test brightening black RGB(0,0,0) to white with full intensity (evy=16)
    uint16_t black = 0x0000;
    uint16_t result = gpu->applyBrightnessIncrease(black, 16);
    
    // Expected: All components = 0 + (31-0)*16/16 = 31
    EXPECT_EQ(result, 0x7FFF);  // White
}

TEST_F(BlendWindowTest, BrightnessDecrease) {
    setupPalette();
    
    // Test darkening white RGB(31,31,31) by 50% (evy=8)
    uint16_t white = 0x7FFF;
    uint16_t result = gpu->applyBrightnessDecrease(white, 8);
    
    // Expected: R = 31 - (31*8)/16 = 31 - 15 = 16, same for G and B
    uint8_t r = result & 0x1F;
    uint8_t g = (result >> 5) & 0x1F;
    uint8_t b = (result >> 10) & 0x1F;
    
    EXPECT_EQ(r, 16);
    EXPECT_EQ(g, 16);
    EXPECT_EQ(b, 16);
}

TEST_F(BlendWindowTest, BrightnessDecreaseToBlack) {
    setupPalette();
    
    // Test darkening any color to black with full intensity (evy=16)
    uint16_t red = 0x001F;
    uint16_t result = gpu->applyBrightnessDecrease(red, 16);
    
    // Expected: All components = color - color*16/16 = 0
    EXPECT_EQ(result, 0x0000);  // Black
}

// ============================================================================
// Alpha Blending Tests
// ============================================================================

TEST_F(BlendWindowTest, AlphaBlend50_50) {
    setupPalette();
    
    // Set up 50/50 blend: EVA=8, EVB=8
    memory->write16(REG_BLDCNT, 0x0241);   // BG0 1st, BG1 2nd, alpha mode
    memory->write16(REG_BLDALPHA, 0x0808); // 50/50
    
    BlendControl blend = gpu->readBlendControl();
    
    // Blend pure red RGB(31,0,0) with pure green RGB(0,31,0)
    uint16_t red = 0x001F;
    uint16_t green = 0x03E0;
    uint16_t result = gpu->applyBlend(red, green, blend, 0, 1);
    
    // Expected: R = 31*8/16 = 15, G = 31*8/16 = 15, B = 0
    uint8_t r = result & 0x1F;
    uint8_t g = (result >> 5) & 0x1F;
    uint8_t b = (result >> 10) & 0x1F;
    
    EXPECT_EQ(r, 15);
    EXPECT_EQ(g, 15);
    EXPECT_EQ(b, 0);
}

TEST_F(BlendWindowTest, AlphaBlend75_25) {
    setupPalette();
    
    // Set up 75/25 blend: EVA=12, EVB=4
    memory->write16(REG_BLDCNT, 0x0241);   // BG0 1st, BG1 2nd, alpha mode
    memory->write16(REG_BLDALPHA, 0x040C); // 75/25
    
    BlendControl blend = gpu->readBlendControl();
    
    // Blend pure red with pure blue
    uint16_t red = 0x001F;
    uint16_t blue = 0x7C00;
    uint16_t result = gpu->applyBlend(red, blue, blend, 0, 1);
    
    // Expected: R = 31*12/16 = 23, G = 0, B = 31*4/16 = 7
    uint8_t r = result & 0x1F;
    uint8_t g = (result >> 5) & 0x1F;
    uint8_t b = (result >> 10) & 0x1F;
    
    EXPECT_EQ(r, 23);
    EXPECT_EQ(g, 0);
    EXPECT_EQ(b, 7);
}

TEST_F(BlendWindowTest, BlendOnlyWhenTargeted) {
    setupPalette();
    
    // Set up blend but BG2 is NOT a first target
    memory->write16(REG_BLDCNT, 0x0241);   // Only BG0 is 1st target
    memory->write16(REG_BLDALPHA, 0x0808);
    
    BlendControl blend = gpu->readBlendControl();
    
    // Try to blend BG2 (layerType=2) - should be ignored
    uint16_t red = 0x001F;
    uint16_t green = 0x03E0;
    uint16_t result = gpu->applyBlend(red, green, blend, 2, 1);
    
    // Expected: No blending, return color1 unchanged
    EXPECT_EQ(result, red);
}

TEST_F(BlendWindowTest, BlendModeOff) {
    setupPalette();
    
    // Set blend mode to OFF
    memory->write16(REG_BLDCNT, 0x0001);   // BG0 1st target, mode=0 (off)
    memory->write16(REG_BLDALPHA, 0x0808);
    
    BlendControl blend = gpu->readBlendControl();
    EXPECT_EQ(blend.mode, BLEND_MODE_OFF);
    
    uint16_t red = 0x001F;
    uint16_t green = 0x03E0;
    uint16_t result = gpu->applyBlend(red, green, blend, 0, 1);
    
    // Expected: No blending
    EXPECT_EQ(result, red);
}

// ============================================================================
// Window Control Register Tests
// ============================================================================

TEST_F(BlendWindowTest, WindowControlParsing) {
    // Enable WIN0 in DISPCNT
    memory->write16(REG_DISPCNT, DISPCNT_WIN0_ENABLE);
    
    // Set WIN0 dimensions: left=50, right=150, top=30, bottom=100
    memory->write16(REG_WIN0H, 0x3296);  // (50 << 8) | 150
    memory->write16(REG_WIN0V, 0x1E64);  // (30 << 8) | 100
    
    // Set WIN0 control: Enable BG0 and BG1
    memory->write16(REG_WININ, 0x0003);  // Bits 0-1
    
    WindowControl winCtrl = gpu->readWindowControl();
    
    EXPECT_TRUE(winCtrl.win0.enabled);
    EXPECT_EQ(winCtrl.win0.left, 50);
    EXPECT_EQ(winCtrl.win0.right, 150);
    EXPECT_EQ(winCtrl.win0.top, 30);
    EXPECT_EQ(winCtrl.win0.bottom, 100);
    EXPECT_EQ(winCtrl.win0.control, 0x03);  // BG0 and BG1 enabled
}

TEST_F(BlendWindowTest, WindowNotEnabledInDISPCNT) {
    // Don't enable WIN0 in DISPCNT
    memory->write16(REG_DISPCNT, 0x0000);
    memory->write16(REG_WIN0H, 0x3296);
    memory->write16(REG_WIN0V, 0x1E64);
    memory->write16(REG_WININ, 0x0003);
    
    WindowControl winCtrl = gpu->readWindowControl();
    
    EXPECT_FALSE(winCtrl.win0.enabled);
}

TEST_F(BlendWindowTest, Window1Configuration) {
    // Enable WIN1
    memory->write16(REG_DISPCNT, DISPCNT_WIN1_ENABLE);
    
    // Set WIN1 dimensions
    memory->write16(REG_WIN1H, 0x5078);  // left=80, right=120
    memory->write16(REG_WIN1V, 0x3C50);  // top=60, bottom=80
    
    // Set WIN1 control: Enable BG2 and OBJ
    memory->write16(REG_WININ, 0x1400);  // Bits 10 and 12 (WIN1 is bits 8-13)
    
    WindowControl winCtrl = gpu->readWindowControl();
    
    EXPECT_TRUE(winCtrl.win1.enabled);
    EXPECT_EQ(winCtrl.win1.left, 80);
    EXPECT_EQ(winCtrl.win1.right, 120);
    EXPECT_EQ(winCtrl.win1.top, 60);
    EXPECT_EQ(winCtrl.win1.bottom, 80);
    EXPECT_EQ(winCtrl.win1.control, 0x14);  // BG2 and OBJ
}

TEST_F(BlendWindowTest, OutsideWindowControl) {
    // Set WINOUT: Enable BG3 outside windows
    memory->write16(REG_WINOUT, 0x0008);  // Bit 3 = BG3
    
    WindowControl winCtrl = gpu->readWindowControl();
    
    EXPECT_EQ(winCtrl.winOut, 0x08);
}

// ============================================================================
// Window Boundary Tests
// ============================================================================

TEST_F(BlendWindowTest, PixelInsideWindow) {
    // Enable WIN0: left=50, right=150, top=30, bottom=100
    memory->write16(REG_DISPCNT, DISPCNT_WIN0_ENABLE);
    memory->write16(REG_WIN0H, 0x3296);
    memory->write16(REG_WIN0V, 0x1E64);
    memory->write16(REG_WININ, 0x0001);  // BG0 enabled
    
    WindowControl winCtrl = gpu->readWindowControl();
    
    // Test pixel inside window
    EXPECT_TRUE(gpu->isPixelInWindow(100, 50, winCtrl.win0));
    
    // Test pixel at edges (inclusive left/top, exclusive right/bottom)
    EXPECT_TRUE(gpu->isPixelInWindow(50, 30, winCtrl.win0));   // Top-left corner
    EXPECT_FALSE(gpu->isPixelInWindow(150, 100, winCtrl.win0)); // Bottom-right corner
    
    // Test pixels outside window
    EXPECT_FALSE(gpu->isPixelInWindow(40, 50, winCtrl.win0));   // Left of window
    EXPECT_FALSE(gpu->isPixelInWindow(160, 50, winCtrl.win0));  // Right of window
    EXPECT_FALSE(gpu->isPixelInWindow(100, 20, winCtrl.win0));  // Above window
    EXPECT_FALSE(gpu->isPixelInWindow(100, 110, winCtrl.win0)); // Below window
}

TEST_F(BlendWindowTest, WindowWraparoundHorizontal) {
    // Window with wraparound: left=200, right=50 (wraps at edge)
    // This means x is in range [200,239] OR [0,49]
    memory->write16(REG_DISPCNT, DISPCNT_WIN0_ENABLE);
    memory->write16(REG_WIN0H, 0xC832);  // (200 << 8) | 50
    memory->write16(REG_WIN0V, 0x00A0);  // top=0, bottom=160 (full height)
    memory->write16(REG_WININ, 0x0001);
    
    WindowControl winCtrl = gpu->readWindowControl();
    
    // Test wraparound region
    EXPECT_TRUE(gpu->isPixelInWindow(220, 50, winCtrl.win0));  // In [200,239]
    EXPECT_TRUE(gpu->isPixelInWindow(30, 50, winCtrl.win0));   // In [0,49]
    EXPECT_FALSE(gpu->isPixelInWindow(100, 50, winCtrl.win0)); // In gap [50,199]
}

TEST_F(BlendWindowTest, Window0HasPriorityOverWindow1) {
    // Enable both windows with overlapping regions
    memory->write16(REG_DISPCNT, DISPCNT_WIN0_ENABLE | DISPCNT_WIN1_ENABLE);
    
    // WIN0: 50-150, BG0 enabled
    memory->write16(REG_WIN0H, 0x3296);
    memory->write16(REG_WIN0V, 0x0050);
    memory->write16(REG_WININ, 0x0001);  // WIN0: BG0
    
    // WIN1: 100-200, BG1 enabled
    memory->write16(REG_WIN1H, 0x64C8);
    memory->write16(REG_WIN1V, 0x0050);
    memory->write16(REG_WININ, 0x0201);  // WIN0: BG0, WIN1: BG1
    
    // WINOUT: BG2 enabled
    memory->write16(REG_WINOUT, 0x0004);
    
    WindowControl winCtrl = gpu->readWindowControl();
    
    // Pixel at x=75 (in WIN0 only)
    uint8_t ctrl = gpu->getWindowControlForPixel(75, 10, winCtrl);
    EXPECT_EQ(ctrl, 0x01);  // BG0
    
    // Pixel at x=125 (in both WIN0 and WIN1 - WIN0 takes priority)
    ctrl = gpu->getWindowControlForPixel(125, 10, winCtrl);
    EXPECT_EQ(ctrl, 0x01);  // BG0, not BG1
    
    // Pixel at x=175 (in WIN1 only)
    ctrl = gpu->getWindowControlForPixel(175, 10, winCtrl);
    EXPECT_EQ(ctrl, 0x02);  // BG1
    
    // Pixel at x=220 (outside all windows)
    ctrl = gpu->getWindowControlForPixel(220, 10, winCtrl);
    EXPECT_EQ(ctrl, 0x04);  // BG2 (WINOUT)
}

// ============================================================================
// Layer Visibility Tests
// ============================================================================

TEST_F(BlendWindowTest, LayerVisibleWithoutWindows) {
    // No windows enabled - all layers visible
    memory->write16(REG_DISPCNT, 0x0000);
    
    // All layer types should be visible
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(0, 100, 50));  // BG0
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(1, 100, 50));  // BG1
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(2, 100, 50));  // BG2
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(3, 100, 50));  // BG3
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(4, 100, 50));  // OBJ
}

TEST_F(BlendWindowTest, LayerMaskedByWindow) {
    // Enable WIN0: only BG0 visible inside
    memory->write16(REG_DISPCNT, DISPCNT_WIN0_ENABLE);
    memory->write16(REG_WIN0H, 0x3296);  // 50-150
    memory->write16(REG_WIN0V, 0x1E64);  // 30-100
    memory->write16(REG_WININ, 0x0001);  // Only BG0
    memory->write16(REG_WINOUT, 0x003F); // All layers outside
    
    // Inside window (x=100, y=50)
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(0, 100, 50));   // BG0 visible
    EXPECT_FALSE(gpu->isLayerVisibleAtPixel(1, 100, 50));  // BG1 hidden
    EXPECT_FALSE(gpu->isLayerVisibleAtPixel(2, 100, 50));  // BG2 hidden
    EXPECT_FALSE(gpu->isLayerVisibleAtPixel(3, 100, 50));  // BG3 hidden
    EXPECT_FALSE(gpu->isLayerVisibleAtPixel(4, 100, 50));  // OBJ hidden
    
    // Outside window (x=200, y=50)
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(0, 200, 50));  // All visible
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(1, 200, 50));
    EXPECT_TRUE(gpu->isLayerVisibleAtPixel(4, 200, 50));
}

// Test counters
// This test suite adds comprehensive coverage of:
// - Blend register parsing (4 tests)
// - Brightness effects (4 tests)
// - Alpha blending (4 tests)
// - Window configuration (4 tests)
// - Window boundaries (3 tests)
// - Layer visibility (2 tests)
// Total: 21 new tests for Session 3
