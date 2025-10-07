/**
 * Test suite for GBA affine sprite rendering (rotation/scaling)
 * 
 * Affine sprites use a 2x2 transformation matrix stored in OAM to apply
 * rotation, scaling, and shearing effects. The GBA supports up to 32
 * simultaneous affine parameter sets.
 */

#include <gtest/gtest.h>
#include "../../include/gpu.h"
#include "../../include/memory.h"
#include <cstring>
#include <cmath>

class AffineSpriteTest : public ::testing::Test {
protected:
    Memory* memory;
    GPU* gpu;

    void SetUp() override {
        memory = new Memory();
        gpu = new GPU(*memory);
        
        // Clear OAM and VRAM
        for (uint32_t addr = 0x07000000; addr < 0x07000400; addr++) {
            memory->write8(addr, 0);
        }
        for (uint32_t addr = 0x06000000; addr < 0x06018000; addr++) {
            memory->write8(addr, 0);
        }
        
        // Set DISPCNT to Mode 0 with OBJ enabled, 1D mapping
        memory->write16(0x04000000, 0x1000 | 0x0040);  // OBJ enable + 1D mapping
    }

    void TearDown() override {
        delete gpu;
        delete memory;
    }
    
    void writeAffineParams(uint8_t paramIndex, int16_t pa, int16_t pb, int16_t pc, int16_t pd) {
        uint32_t baseAddr = OAM_BASE + (paramIndex * 32);
        memory->write16(baseAddr + 0x06, pa);  // PA
        memory->write16(baseAddr + 0x0E, pb);  // PB
        memory->write16(baseAddr + 0x16, pc);  // PC
        memory->write16(baseAddr + 0x1E, pd);  // PD
    }
    
    void writeOBJ(int objNum, uint16_t attr0, uint16_t attr1, uint16_t attr2) {
        uint32_t addr = OAM_BASE + (objNum * 8);
        memory->write16(addr + 0, attr0);
        memory->write16(addr + 2, attr1);
        memory->write16(addr + 4, attr2);
    }
    
    void createSimpleTile4bpp(int tileNum, uint8_t colorIndex) {
        // Create a solid-color 8×8 tile (4bpp)
        uint32_t tileAddr = 0x06010000 + (tileNum * 32);
        uint8_t pixelPair = (colorIndex << 4) | colorIndex;
        for (int i = 0; i < 32; i++) {
            memory->write8(tileAddr + i, pixelPair);
        }
    }
    
    void createSimpleTile8bpp(int tileNum, uint8_t colorIndex) {
        // Create a solid-color 8×8 tile (8bpp)
        // 8bpp tiles are 64 bytes but tile numbers still count by 32-byte blocks
        uint32_t tileAddr = 0x06010000 + (tileNum * 32);  // Tile addressing in 32-byte units
        for (int i = 0; i < 64; i++) {
            memory->write8(tileAddr + i, colorIndex);
        }
    }
    
    void setupOBJPalette(int paletteNum, int colorIndex, uint16_t rgb555) {
        uint32_t paletteAddr = 0x05000200 + (paletteNum * 32) + (colorIndex * 2);
        memory->write16(paletteAddr, rgb555);
    }
    
    void renderFrame() {
        for (uint16_t scanline = 0; scanline < 160; scanline++) {
            gpu->setCurrentScanline(scanline);
            gpu->clearScanlineToBackdrop(scanline);
            gpu->renderSpriteScanline(scanline);
        }
    }
    
    uint16_t getPixel(int x, int y) {
        uint16_t* fb = gpu->getTiledFramebuffer();
        return fb[y * 240 + x];
    }
    
    // Helper: Convert float to fixed-point 8.8
    int16_t floatToFixed8_8(float val) {
        return (int16_t)(val * 256.0f);
    }
    
    // Helper: Create rotation matrix for angle in degrees
    void makeRotationMatrix(float degrees, int16_t& pa, int16_t& pb, int16_t& pc, int16_t& pd) {
        float radians = degrees * M_PI / 180.0f;
        float cosTheta = cosf(radians);
        float sinTheta = sinf(radians);
        
        pa = floatToFixed8_8(cosTheta);
        pb = floatToFixed8_8(-sinTheta);
        pc = floatToFixed8_8(sinTheta);
        pd = floatToFixed8_8(cosTheta);
    }
};

// ============================================================================
// Test 1: Parse affine parameters from OAM
// ============================================================================
TEST_F(AffineSpriteTest, ParseAffineParams) {
    // Write identity matrix to affine param set 0
    // PA = 0x0100 (1.0), PB = 0x0000, PC = 0x0000, PD = 0x0100 (1.0)
    writeAffineParams(0, 0x0100, 0x0000, 0x0000, 0x0100);
    
    // Write 2x scale matrix to affine param set 1
    // PA = 0x0080 (0.5), PB = 0x0000, PC = 0x0000, PD = 0x0080 (0.5)
    writeAffineParams(1, 0x0080, 0x0000, 0x0000, 0x0080);
    
    // Test reading param set 0
    auto params0 = gpu->readAffineParams(0);
    EXPECT_EQ(0x0100, params0.pa);
    EXPECT_EQ(0x0000, params0.pb);
    EXPECT_EQ(0x0000, params0.pc);
    EXPECT_EQ(0x0100, params0.pd);
    
    // Test reading param set 1
    auto params1 = gpu->readAffineParams(1);
    EXPECT_EQ(0x0080, params1.pa);
    EXPECT_EQ(0x0000, params1.pb);
    EXPECT_EQ(0x0000, params1.pc);
    EXPECT_EQ(0x0080, params1.pd);
}

// ============================================================================
// Test 2: Identity transformation
// ============================================================================
TEST_F(AffineSpriteTest, IdentityTransform) {
    // Create 8x8 solid sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);  // Red
    
    // Set identity matrix
    writeAffineParams(0, 0x0100, 0x0000, 0x0000, 0x0100);
    
    // Create affine sprite at (50, 50), 8x8, using affine param 0
    // Attr0: Y=50, rot/scale=1, double-size=0, shape=square
    // Attr1: X=50, affine param=0, size=0 (8x8)
    // Attr2: tile=0, priority=0, palette=0
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9);  // Affine param 0
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    // Render
    renderFrame();
    
    // Verify pixels are visible (identity should render like normal sprite)
    for (int x = 50; x < 58; x++) {
        EXPECT_NE(0, getPixel(x, 50));
    }
}

// ============================================================================
// Test 3: 2x Scaling
// ============================================================================
TEST_F(AffineSpriteTest, ScaleUp2x) {
    // Create 8x8 sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);  // Red
    
    // 2x scale matrix (inverse = 0.5)
    writeAffineParams(0, 0x0080, 0x0000, 0x0000, 0x0080);
    
    // Create affine sprite
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // 2x scaled 8x8 sprite should have visible pixels
    int visiblePixels = 0;
    for (int x = 50; x < 66; x++) {  // Scaled width
        if (getPixel(x, 54) != 0) {  // Middle row
            visiblePixels++;
        }
    }
    EXPECT_GE(visiblePixels, 8);  // Should be at least as wide as original
}

// ============================================================================
// Test 4: 0.5x Scaling (shrinking)
// ============================================================================
TEST_F(AffineSpriteTest, ScaleDown) {
    // Create 16x16 sprite (4 tiles)
    for (int i = 0; i < 4; i++) {
        createSimpleTile4bpp(i, 1);
    }
    setupOBJPalette(0, 1, 0x001F);  // Red
    
    // 0.5x scale matrix (inverse = 2.0)
    writeAffineParams(0, 0x0200, 0x0000, 0x0000, 0x0200);
    
    // Create 16x16 affine sprite
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9) | (1 << 14);  // size=1 for 16x16 square
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // With 0.5x scaling, the 16x16 sprite appears as 8x8
    int visiblePixels = 0;
    for (int x = 50; x < 58; x++) {
        if (getPixel(x, 54) != 0) {
            visiblePixels++;
        }
    }
    EXPECT_GE(visiblePixels, 4);  // Should see some pixels
}

// ============================================================================
// Test 5: 90 degree rotation
// ============================================================================
TEST_F(AffineSpriteTest, Rotation90Degrees) {
    // Create 8x8 solid sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);  // Red
    
    // 90° rotation matrix
    int16_t pa, pb, pc, pd;
    makeRotationMatrix(90.0f, pa, pb, pc, pd);
    writeAffineParams(0, pa, pb, pc, pd);
    
    // Create sprite
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Should see some rotated pixels
    int visiblePixels = 0;
    for (int y = 50; y < 58; y++) {
        for (int x = 50; x < 58; x++) {
            if (getPixel(x, y) != 0) {
                visiblePixels++;
            }
        }
    }
    EXPECT_GT(visiblePixels, 10);  // Should see rotated sprite
}

// ============================================================================
// Test 6: 180 degree rotation
// ============================================================================
TEST_F(AffineSpriteTest, Rotation180Degrees) {
    // Create 8x8 solid sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // 180° rotation matrix
    int16_t pa, pb, pc, pd;
    makeRotationMatrix(180.0f, pa, pb, pc, pd);
    writeAffineParams(0, pa, pb, pc, pd);
    
    // Create sprite
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Should see pixels
    int visiblePixels = 0;
    for (int y = 50; y < 58; y++) {
        for (int x = 50; x < 58; x++) {
            if (getPixel(x, y) != 0) {
                visiblePixels++;
            }
        }
    }
    EXPECT_GT(visiblePixels, 10);
}

// ============================================================================
// Test 7: 45 degree rotation
// ============================================================================
TEST_F(AffineSpriteTest, Rotation45Degrees) {
    // Create 8x8 solid sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // 45° rotation
    int16_t pa, pb, pc, pd;
    makeRotationMatrix(45.0f, pa, pb, pc, pd);
    writeAffineParams(0, pa, pb, pc, pd);
    
    // Create sprite
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Should see some rotated pixels
    int visiblePixels = 0;
    for (int y = 50; y < 58; y++) {
        for (int x = 50; x < 58; x++) {
            if (getPixel(x, y) != 0) {
                visiblePixels++;
            }
        }
    }
    EXPECT_GT(visiblePixels, 5);
}

// ============================================================================
// Test 8: Combined rotation and scaling
// ============================================================================
TEST_F(AffineSpriteTest, RotationAndScaling) {
    // Create 8x8 solid sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // Combined 45° rotation + 1.5x scale
    float scale = 1.0f / 1.5f;
    float radians = 45.0f * M_PI / 180.0f;
    int16_t pa = floatToFixed8_8(cosf(radians) * scale);
    int16_t pb = floatToFixed8_8(-sinf(radians) * scale);
    int16_t pc = floatToFixed8_8(sinf(radians) * scale);
    int16_t pd = floatToFixed8_8(cosf(radians) * scale);
    writeAffineParams(0, pa, pb, pc, pd);
    
    // Create sprite
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Should see pixels (rotated and scaled)
    int visiblePixels = 0;
    for (int y = 46; y < 62; y++) {  // Wider range
        for (int x = 46; x < 62; x++) {
            if (getPixel(x, y) != 0) {
                visiblePixels++;
            }
        }
    }
    EXPECT_GT(visiblePixels, 10);
}

// ============================================================================
// Test 9: Double-size mode
// ============================================================================
TEST_F(AffineSpriteTest, DoubleSize) {
    // Create 8x8 sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // Identity transform with double-size
    writeAffineParams(0, 0x0100, 0x0000, 0x0000, 0x0100);
    
    // Create sprite with double-size flag
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG | OBJ_ATTR0_DOUBLE_SIZE;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    // Parse attributes to verify double-size is detected
    auto attrs = gpu->parseOBJAttributes(attr0, attr1, attr2);
    EXPECT_TRUE(attrs.doubleSize);
    
    // With double-size, effective sprite area is 16x16
    EXPECT_TRUE(gpu->isSpriteOnScanline(attrs, 50));   // Top edge
    EXPECT_TRUE(gpu->isSpriteOnScanline(attrs, 65));   // Bottom edge (50 + 16 - 1)
    EXPECT_FALSE(gpu->isSpriteOnScanline(attrs, 66));  // Beyond bottom
}

// ============================================================================
// Test 10: Multiple affine parameters
// ============================================================================
TEST_F(AffineSpriteTest, MultipleAffineParams) {
    // Create sprites
    createSimpleTile4bpp(0, 1);
    createSimpleTile4bpp(1, 2);
    setupOBJPalette(0, 1, 0x001F);  // Red
    setupOBJPalette(0, 2, 0x03E0);  // Green
    
    // Param 0: Identity
    writeAffineParams(0, 0x0100, 0x0000, 0x0000, 0x0100);
    
    // Param 1: 2x scale
    writeAffineParams(1, 0x0080, 0x0000, 0x0000, 0x0080);
    
    // Sprite 0 uses param 0
    uint16_t attr0_0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1_0 = 50 | (0 << 9);
    uint16_t attr2_0 = 0;
    writeOBJ(0, attr0_0, attr1_0, attr2_0);
    
    // Sprite 1 uses param 1
    uint16_t attr0_1 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1_1 = 80 | (1 << 9);
    uint16_t attr2_1 = 1;
    writeOBJ(1, attr0_1, attr1_1, attr2_1);
    
    renderFrame();
    
    // Both sprites should be visible
    EXPECT_NE(0, getPixel(52, 52));  // Sprite 0
    EXPECT_NE(0, getPixel(84, 54));  // Sprite 1
}

// ============================================================================
// Test 11: Affine sprite with 8bpp
// ============================================================================
TEST_F(AffineSpriteTest, Affine8bpp) {
    // Create 8bpp sprite
    createSimpleTile8bpp(0, 5);
    setupOBJPalette(0, 5, 0x7C00);  // Blue
    
    // Identity
    writeAffineParams(0, 0x0100, 0x0000, 0x0000, 0x0100);
    
    // Create 8bpp affine sprite
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG | OBJ_ATTR0_PALETTE_MODE;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Verify pixels are visible
    EXPECT_NE(0, getPixel(50, 50));
    EXPECT_NE(0, getPixel(54, 54));
}

// ============================================================================
// Test 12: Out-of-bounds texture coordinates
// ============================================================================
TEST_F(AffineSpriteTest, OutOfBoundsHandling) {
    // Create small 8x8 sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // Large rotation with double-size
    int16_t pa, pb, pc, pd;
    makeRotationMatrix(45.0f, pa, pb, pc, pd);
    writeAffineParams(0, pa, pb, pc, pd);
    
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG | OBJ_ATTR0_DOUBLE_SIZE;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Should render without crashing - some pixels visible
    int visiblePixels = 0;
    for (int y = 50; y < 66; y++) {
        for (int x = 50; x < 66; x++) {
            if (getPixel(x, y) != 0) {
                visiblePixels++;
            }
        }
    }
    EXPECT_GT(visiblePixels, 0);
}

// ============================================================================
// Test 13: Negative affine parameters (mirroring)
// ============================================================================
TEST_F(AffineSpriteTest, NegativeAffineParams) {
    // Create sprite
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // Negative PA and PD for mirror effect
    writeAffineParams(0, -0x0100, 0x0000, 0x0000, -0x0100);
    
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (0 << 9);
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Should see mirrored pixels
    int visiblePixels = 0;
    for (int y = 50; y < 58; y++) {
        for (int x = 50; x < 58; x++) {
            if (getPixel(x, y) != 0) {
                visiblePixels++;
            }
        }
    }
    EXPECT_GT(visiblePixels, 0);
}

// ============================================================================
// Test 14: Mixed affine and normal sprites
// ============================================================================
TEST_F(AffineSpriteTest, MixedAffineAndNormal) {
    // Create tiles
    createSimpleTile4bpp(0, 1);
    createSimpleTile4bpp(1, 2);
    setupOBJPalette(0, 1, 0x001F);  // Red
    setupOBJPalette(0, 2, 0x03E0);  // Green
    
    // Affine param
    writeAffineParams(0, 0x0100, 0x0000, 0x0000, 0x0100);
    
    // Normal sprite at x=50
    uint16_t attr0_0 = 50;  // No rot/scale flag
    uint16_t attr1_0 = 50;
    uint16_t attr2_0 = 0;
    writeOBJ(0, attr0_0, attr1_0, attr2_0);
    
    // Affine sprite at x=80
    uint16_t attr0_1 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1_1 = 80 | (0 << 9);
    uint16_t attr2_1 = 1;
    writeOBJ(1, attr0_1, attr1_1, attr2_1);
    
    renderFrame();
    
    // Both should be visible
    EXPECT_NE(0, getPixel(52, 52));  // Normal
    EXPECT_NE(0, getPixel(82, 52));  // Affine
}

// ============================================================================
// Test 15: Affine parameter bounds (param 31)
// ============================================================================
TEST_F(AffineSpriteTest, AffineParamBounds) {
    // Write to param 31 (last valid param)
    writeAffineParams(31, 0x0100, 0x0000, 0x0000, 0x0100);
    
    // Read it back
    auto params = gpu->readAffineParams(31);
    EXPECT_EQ(0x0100, params.pa);
    EXPECT_EQ(0x0100, params.pd);
    
    // Create sprite using param 31
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    uint16_t attr0 = 50 | OBJ_ATTR0_ROT_SCALE_FLAG;
    uint16_t attr1 = 50 | (31 << 9);  // Param 31
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Should render successfully
    EXPECT_NE(0, getPixel(52, 52));
}
