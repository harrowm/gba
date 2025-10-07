#include <gtest/gtest.h>
#include "../../include/gpu.h"
#include "../../include/memory.h"
#include <cstring>

class SpriteRenderTest : public ::testing::Test {
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
        
        // Set DISPCNT to Mode 0 with OBJ enabled
        memory->write16(0x04000000, 0x1000);  // OBJ enable
    }

    void TearDown() override {
        delete gpu;
        delete memory;
    }
    
    void writeOBJ(int objNum, uint16_t attr0, uint16_t attr1, uint16_t attr2) {
        uint32_t oamAddr = 0x07000000 + (objNum * 8);
        memory->write16(oamAddr + 0, attr0);
        memory->write16(oamAddr + 2, attr1);
        memory->write16(oamAddr + 4, attr2);
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
        // 8bpp tiles are 64 bytes but tile numbers still advance by 32-byte blocks
        // So tile 0 is at +0, but uses 64 bytes (overlapping with tile 1's space)
        uint32_t tileAddr = 0x06010000 + (tileNum * 32);
        for (int i = 0; i < 64; i++) {
            memory->write8(tileAddr + i, colorIndex);
        }
    }
    
    void createCheckerTile4bpp(int tileNum, uint8_t color1, uint8_t color2) {
        // Create an 8×8 checkerboard tile (4bpp)
        uint32_t tileAddr = 0x06010000 + (tileNum * 32);
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x += 2) {
                uint8_t c1 = ((x/2 + y) % 2 == 0) ? color1 : color2;
                uint8_t c2 = ((x/2 + y) % 2 == 0) ? color2 : color1;
                uint8_t pixelPair = (c2 << 4) | c1;
                memory->write8(tileAddr + y * 4 + x / 2, pixelPair);
            }
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
};

// ============================================================================
// Test: Tile Address Calculation
// ============================================================================

TEST_F(SpriteRenderTest, TileAddress1D_4bpp) {
    // 16×16 sprite (2×2 tiles) in 1D mapping, 4bpp mode
    OBJAttributes obj;
    obj.tileNumber = 10;
    obj.width = 16;
    obj.height = 16;
    obj.paletteMode = false;  // 4bpp
    
    // In 1D mode, tiles are sequential
    EXPECT_EQ(0x06010140, gpu->getOBJTileAddress(obj, 0, 0, true));  // Tile 10
    EXPECT_EQ(0x06010160, gpu->getOBJTileAddress(obj, 1, 0, true));  // Tile 11
    EXPECT_EQ(0x06010180, gpu->getOBJTileAddress(obj, 0, 1, true));  // Tile 12
    EXPECT_EQ(0x060101A0, gpu->getOBJTileAddress(obj, 1, 1, true));  // Tile 13
}

TEST_F(SpriteRenderTest, TileAddress1D_8bpp) {
    // 16×16 sprite in 1D mapping, 8bpp mode
    OBJAttributes obj;
    obj.tileNumber = 10;
    obj.width = 16;
    obj.height = 16;
    obj.paletteMode = true;  // 8bpp
    
    // In 8bpp 1D mode, each tile takes 2 tile numbers
    EXPECT_EQ(0x06010140, gpu->getOBJTileAddress(obj, 0, 0, true));  // Tile 10
    EXPECT_EQ(0x06010180, gpu->getOBJTileAddress(obj, 1, 0, true));  // Tile 12 (10 + 2)
    EXPECT_EQ(0x060101C0, gpu->getOBJTileAddress(obj, 0, 1, true));  // Tile 14 (10 + 4)
    EXPECT_EQ(0x06010200, gpu->getOBJTileAddress(obj, 1, 1, true));  // Tile 16 (10 + 6)
}

TEST_F(SpriteRenderTest, TileAddress2D_4bpp) {
    // 16×16 sprite in 2D mapping, 4bpp mode
    OBJAttributes obj;
    obj.tileNumber = 10;
    obj.width = 16;
    obj.height = 16;
    obj.paletteMode = false;  // 4bpp
    
    // In 2D mode, tiles arranged in 32-tile-wide blocks
    EXPECT_EQ(0x06010140, gpu->getOBJTileAddress(obj, 0, 0, false));  // Tile 10
    EXPECT_EQ(0x06010160, gpu->getOBJTileAddress(obj, 1, 0, false));  // Tile 11
    EXPECT_EQ(0x06010540, gpu->getOBJTileAddress(obj, 0, 1, false));  // Tile 42 (10 + 32)
    EXPECT_EQ(0x06010560, gpu->getOBJTileAddress(obj, 1, 1, false));  // Tile 43 (10 + 33)
}

// ============================================================================
// Test: Sprite Scanline Intersection
// ============================================================================

TEST_F(SpriteRenderTest, SpriteOnScanline_Simple) {
    OBJAttributes obj;
    obj.y = 50;
    obj.height = 16;
    obj.visible = true;
    
    // Sprite at Y=50, height 16 covers scanlines 50-65
    EXPECT_FALSE(gpu->isSpriteOnScanline(obj, 49));
    EXPECT_TRUE(gpu->isSpriteOnScanline(obj, 50));
    EXPECT_TRUE(gpu->isSpriteOnScanline(obj, 55));
    EXPECT_TRUE(gpu->isSpriteOnScanline(obj, 65));
    EXPECT_FALSE(gpu->isSpriteOnScanline(obj, 66));
}

TEST_F(SpriteRenderTest, SpriteOnScanline_Wraparound) {
    OBJAttributes obj;
    obj.y = 250;
    obj.height = 16;
    obj.visible = true;
    
    // Sprite at Y=250, height 16 wraps around (250-255, then 0-9)
    EXPECT_TRUE(gpu->isSpriteOnScanline(obj, 250));
    EXPECT_TRUE(gpu->isSpriteOnScanline(obj, 255));
    EXPECT_TRUE(gpu->isSpriteOnScanline(obj, 0));
    EXPECT_TRUE(gpu->isSpriteOnScanline(obj, 9));
    EXPECT_FALSE(gpu->isSpriteOnScanline(obj, 10));
}

TEST_F(SpriteRenderTest, SpriteOnScanline_Invisible) {
    OBJAttributes obj;
    obj.y = 50;
    obj.height = 16;
    obj.visible = false;
    
    EXPECT_FALSE(gpu->isSpriteOnScanline(obj, 55));
}

// ============================================================================
// Test: Basic Sprite Rendering
// ============================================================================

TEST_F(SpriteRenderTest, RenderSingleSprite) {
    // Create a simple 8×8 sprite with red color
    createSimpleTile4bpp(0, 1);  // Tile 0, color index 1
    setupOBJPalette(0, 1, 0x001F);  // Palette 0, color 1 = red (RGB555)
    
    // Setup sprite at (50, 60)
    uint16_t attr0 = 60;                     // Y=60, shape=0 (square)
    uint16_t attr1 = 50;                     // X=50, size=0 (8×8)
    uint16_t attr2 = 0;                      // Tile 0, priority 0, palette 0
    writeOBJ(0, attr0, attr1, attr2);
    
    // Render frame
    renderFrame();
    
    // Check sprite pixels (should be red)
    EXPECT_EQ(0x001F, getPixel(50, 60));  // Top-left
    EXPECT_EQ(0x001F, getPixel(57, 67));  // Bottom-right
    EXPECT_EQ(0x001F, getPixel(54, 64));  // Middle
    
    // Check outside sprite (should be backdrop)
    EXPECT_EQ(0, getPixel(49, 60));       // Left of sprite
    EXPECT_EQ(0, getPixel(58, 60));       // Right of sprite
}

TEST_F(SpriteRenderTest, RenderMultipleSprites) {
    // Create two sprites with different colors
    createSimpleTile4bpp(0, 1);  // Tile 0, red
    createSimpleTile4bpp(1, 2);  // Tile 1, green
    setupOBJPalette(0, 1, 0x001F);  // Red
    setupOBJPalette(0, 2, 0x03E0);  // Green
    
    // Sprite 0 at (50, 60)
    writeOBJ(0, 60, 50, 0);  // Tile 0
    
    // Sprite 1 at (70, 60)
    writeOBJ(1, 60, 70, 1);  // Tile 1
    
    renderFrame();
    
    // Check both sprites
    EXPECT_EQ(0x001F, getPixel(50, 60));  // Sprite 0 (red)
    EXPECT_EQ(0x03E0, getPixel(70, 60));  // Sprite 1 (green)
}

TEST_F(SpriteRenderTest, RenderTransparency) {
    // Create a tile with transparent pixel at (0,0)
    // In 4bpp, byte contains two pixels: low nibble=pixel0, high nibble=pixel1
    uint32_t tileAddr = 0x06010000;
    memory->write8(tileAddr, 0x10);  // Pixel (0,0) = 0 (low nibble), (1,0) = 1 (high nibble)
    for (int i = 1; i < 32; i++) {
        memory->write8(tileAddr + i, 0x11);  // Rest are color 1
    }
    setupOBJPalette(0, 1, 0x001F);  // Red
    
    // Setup sprite
    writeOBJ(0, 60, 50, 0);
    renderFrame();
    
    // Transparent pixel should show backdrop (black)
    EXPECT_EQ(0, getPixel(50, 60));
    
    // Non-transparent pixels should show red
    EXPECT_EQ(0x001F, getPixel(51, 60));
}

// ============================================================================
// Test: Sprite Flipping
// ============================================================================

TEST_F(SpriteRenderTest, HorizontalFlip) {
    // Create a tile with gradient (colors 0-7 left-to-right in first row)
    uint32_t tileAddr = 0x06010000;
    for (int x = 0; x < 4; x++) {
        memory->write8(tileAddr + x, (x*2+1) << 4 | (x*2));
    }
    for (int i = 4; i < 32; i++) {
        memory->write8(tileAddr + i, 0);
    }
    
    // Setup palette (colors 0-7)
    for (int i = 0; i < 8; i++) {
        setupOBJPalette(0, i, i * 0x111);  // Grayscale gradient
    }
    
    // Normal sprite
    writeOBJ(0, 60, 50, 0);
    
    // H-flipped sprite (bit 12 of attr1)
    writeOBJ(1, 60, 70 | (1 << 12), 0);
    
    renderFrame();
    
    // Normal sprite: left pixel should be darker
    uint16_t normalLeft = getPixel(50, 60);
    uint16_t normalRight = getPixel(57, 60);
    
    // Flipped sprite: left pixel should be brighter (was right)
    uint16_t flippedLeft = getPixel(70, 60);
    uint16_t flippedRight = getPixel(77, 60);
    
    EXPECT_LT(normalLeft, normalRight);      // Normal: left < right
    EXPECT_GT(flippedLeft, flippedRight);    // Flipped: left > right
}

TEST_F(SpriteRenderTest, VerticalFlip) {
    // Create a tile with gradient (colors 0-7 top-to-bottom in first column)
    uint32_t tileAddr = 0x06010000;
    for (int y = 0; y < 8; y++) {
        memory->write8(tileAddr + y * 4, y);  // First pixel of each row
    }
    
    // Setup palette
    for (int i = 0; i < 8; i++) {
        setupOBJPalette(0, i, i * 0x111);
    }
    
    // Normal sprite
    writeOBJ(0, 60, 50, 0);
    
    // V-flipped sprite (bit 13 of attr1)
    writeOBJ(1, 60, 70 | (1 << 13), 0);
    
    renderFrame();
    
    // Normal sprite: top pixel should be darker
    uint16_t normalTop = getPixel(50, 60);
    uint16_t normalBottom = getPixel(50, 67);
    
    // Flipped sprite: top pixel should be brighter (was bottom)
    uint16_t flippedTop = getPixel(70, 60);
    uint16_t flippedBottom = getPixel(70, 67);
    
    EXPECT_LT(normalTop, normalBottom);      // Normal: top < bottom
    EXPECT_GT(flippedTop, flippedBottom);    // Flipped: top > bottom
}

// ============================================================================
// Test: 8bpp Mode
// ============================================================================

TEST_F(SpriteRenderTest, Render8bppSprite) {
    // Create an 8bpp tile with color index 100
    createSimpleTile8bpp(0, 100);
    
    // Setup 8bpp palette (single 256-color palette starting at 0x05000200)
    uint32_t paletteAddr = 0x05000200 + (100 * 2);
    memory->write16(paletteAddr, 0x7C00);  // Blue
    
    // Setup 8bpp sprite (bit 13 of attr0)
    uint16_t attr0 = 60 | 0x2000;  // Y=60, 8bpp mode
    uint16_t attr1 = 50;           // X=50
    uint16_t attr2 = 0;            // Tile 0
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Check sprite is blue
    EXPECT_EQ(0x7C00, getPixel(50, 60));
}

// ============================================================================
// Test: Different Sprite Sizes
// ============================================================================

TEST_F(SpriteRenderTest, Render16x16Sprite) {
    // Create 4 tiles (2×2) for a 16×16 sprite
    createSimpleTile4bpp(0, 1);
    createSimpleTile4bpp(1, 2);
    createSimpleTile4bpp(2, 3);
    createSimpleTile4bpp(3, 4);
    setupOBJPalette(0, 1, 0x001F);
    setupOBJPalette(0, 2, 0x03E0);
    setupOBJPalette(0, 3, 0x7C00);
    setupOBJPalette(0, 4, 0x7FFF);
    
    // 16×16 sprite (shape=0, size=1)
    uint16_t attr0 = 60;             // Y=60, shape=square
    uint16_t attr1 = 50 | (1 << 14); // X=50, size=1 (16×16)
    uint16_t attr2 = 0;              // Base tile 0
    writeOBJ(0, attr0, attr1, attr2);
    
    // Enable 1D mapping
    memory->write16(0x04000000, 0x1040);  // OBJ enable + 1D mapping
    
    renderFrame();
    
    // Check all 4 tile regions have different colors
    EXPECT_EQ(0x001F, getPixel(50, 60));   // Top-left (tile 0)
    EXPECT_EQ(0x03E0, getPixel(58, 60));   // Top-right (tile 1)
    EXPECT_EQ(0x7C00, getPixel(50, 68));   // Bottom-left (tile 2)
    EXPECT_EQ(0x7FFF, getPixel(58, 68));   // Bottom-right (tile 3)
}

TEST_F(SpriteRenderTest, Render32x32Sprite) {
    // 32×32 sprite needs 16 tiles (4×4)
    for (int i = 0; i < 16; i++) {
        createSimpleTile4bpp(i, i + 1);
        setupOBJPalette(0, i + 1, (i + 1) * 0x111);
    }
    
    // 32×32 sprite (shape=0, size=2)
    uint16_t attr0 = 60;             // Y=60
    uint16_t attr1 = 50 | (2 << 14); // X=50, size=2
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    memory->write16(0x04000000, 0x1040);  // 1D mapping
    renderFrame();
    
    // Check corners of the 32×32 sprite
    EXPECT_NE(0, getPixel(50, 60));   // Top-left
    EXPECT_NE(0, getPixel(81, 60));   // Top-right
    EXPECT_NE(0, getPixel(50, 91));   // Bottom-left
    EXPECT_NE(0, getPixel(81, 91));   // Bottom-right
}

TEST_F(SpriteRenderTest, RenderWideSprite) {
    // 32×8 sprite (shape=1, size=1)
    for (int i = 0; i < 4; i++) {
        createSimpleTile4bpp(i, i + 1);
        setupOBJPalette(0, i + 1, (i + 1) * 0x111);
    }
    
    uint16_t attr0 = 60 | (1 << 14);  // Y=60, shape=horizontal
    uint16_t attr1 = 50 | (1 << 14);  // X=50, size=1
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    memory->write16(0x04000000, 0x1040);  // 1D mapping
    renderFrame();
    
    // Check sprite is 32 pixels wide
    EXPECT_NE(0, getPixel(50, 60));
    EXPECT_NE(0, getPixel(81, 60));
    EXPECT_EQ(0, getPixel(82, 60));  // Outside sprite
}

TEST_F(SpriteRenderTest, RenderTallSprite) {
    // 8×32 sprite (shape=2, size=1)
    for (int i = 0; i < 4; i++) {
        createSimpleTile4bpp(i, i + 1);
        setupOBJPalette(0, i + 1, (i + 1) * 0x111);
    }
    
    uint16_t attr0 = 60 | (2 << 14);  // Y=60, shape=vertical
    uint16_t attr1 = 50 | (1 << 14);  // X=50, size=1
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    memory->write16(0x04000000, 0x1040);  // 1D mapping
    renderFrame();
    
    // Check sprite is 32 pixels tall
    EXPECT_NE(0, getPixel(50, 60));
    EXPECT_NE(0, getPixel(50, 91));
    EXPECT_EQ(0, getPixel(50, 92));  // Outside sprite
}

// ============================================================================
// Test: OAM Priority
// ============================================================================

TEST_F(SpriteRenderTest, OAMPriority) {
    // Create two overlapping sprites
    createSimpleTile4bpp(0, 1);  // Red
    createSimpleTile4bpp(1, 2);  // Green
    setupOBJPalette(0, 1, 0x001F);
    setupOBJPalette(0, 2, 0x03E0);
    
    // Sprite 0 at (50, 60)
    writeOBJ(0, 60, 50, 0);
    
    // Sprite 1 at (54, 60) - overlaps with sprite 0
    writeOBJ(1, 60, 54, 1);
    
    renderFrame();
    
    // OAM 0 has higher priority, should be on top
    EXPECT_EQ(0x001F, getPixel(54, 60));  // Overlap shows sprite 0 (red)
    EXPECT_EQ(0x001F, getPixel(50, 60));  // Left part of sprite 0
    EXPECT_EQ(0x03E0, getPixel(58, 60));  // Right part of sprite 1 (green)
}

// ============================================================================
// Test: Edge Cases
// ============================================================================

TEST_F(SpriteRenderTest, SpriteOffscreenLeft) {
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // Sprite at X=(-4) should be partially visible
    uint16_t attr0 = 60;
    uint16_t attr1 = 508;  // 512 - 4 = 508 (wraps to -4)
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Right 4 pixels should be visible at screen left edge
    EXPECT_EQ(0x001F, getPixel(0, 60));
    EXPECT_EQ(0x001F, getPixel(3, 60));
    EXPECT_EQ(0, getPixel(4, 60));  // Outside sprite
}

TEST_F(SpriteRenderTest, SpriteOffscreenRight) {
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // Sprite at X=236 should be partially visible
    writeOBJ(0, 60, 236, 0);
    renderFrame();
    
    // Left 4 pixels should be visible
    EXPECT_EQ(0x001F, getPixel(236, 60));
    EXPECT_EQ(0x001F, getPixel(239, 60));
}

TEST_F(SpriteRenderTest, DisabledSprite) {
    createSimpleTile4bpp(0, 1);
    setupOBJPalette(0, 1, 0x001F);
    
    // Sprite with disable bit set (bit 9 of attr0)
    uint16_t attr0 = 60 | 0x0200;  // Disabled
    uint16_t attr1 = 50;
    uint16_t attr2 = 0;
    writeOBJ(0, attr0, attr1, attr2);
    
    renderFrame();
    
    // Sprite should not be rendered
    EXPECT_EQ(0, getPixel(50, 60));
}
