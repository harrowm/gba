#include <gtest/gtest.h>
#include "../../include/gpu.h"
#include "../../include/memory.h"

class OAMTest : public ::testing::Test {
protected:
    Memory* memory;
    GPU* gpu;

    void SetUp() override {
        memory = new Memory();
        gpu = new GPU(*memory);
        
        // Clear OAM
        for (uint32_t addr = 0x07000000; addr < 0x07000400; addr++) {
            memory->write8(addr, 0);
        }
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
};

// ============================================================================
// Test: OBJ Dimensions
// ============================================================================

TEST_F(OAMTest, SquareSpriteDimensions) {
    int width, height;
    
    // Square sprites: shape=0, size=0-3
    gpu->getOBJDimensions(0, 0, width, height);
    EXPECT_EQ(8, width);
    EXPECT_EQ(8, height);
    
    gpu->getOBJDimensions(0, 1, width, height);
    EXPECT_EQ(16, width);
    EXPECT_EQ(16, height);
    
    gpu->getOBJDimensions(0, 2, width, height);
    EXPECT_EQ(32, width);
    EXPECT_EQ(32, height);
    
    gpu->getOBJDimensions(0, 3, width, height);
    EXPECT_EQ(64, width);
    EXPECT_EQ(64, height);
}

TEST_F(OAMTest, HorizontalSpriteDimensions) {
    int width, height;
    
    // Horizontal (wide) sprites: shape=1, size=0-3
    gpu->getOBJDimensions(1, 0, width, height);
    EXPECT_EQ(16, width);
    EXPECT_EQ(8, height);
    
    gpu->getOBJDimensions(1, 1, width, height);
    EXPECT_EQ(32, width);
    EXPECT_EQ(8, height);
    
    gpu->getOBJDimensions(1, 2, width, height);
    EXPECT_EQ(32, width);
    EXPECT_EQ(16, height);
    
    gpu->getOBJDimensions(1, 3, width, height);
    EXPECT_EQ(64, width);
    EXPECT_EQ(32, height);
}

TEST_F(OAMTest, VerticalSpriteDimensions) {
    int width, height;
    
    // Vertical (tall) sprites: shape=2, size=0-3
    gpu->getOBJDimensions(2, 0, width, height);
    EXPECT_EQ(8, width);
    EXPECT_EQ(16, height);
    
    gpu->getOBJDimensions(2, 1, width, height);
    EXPECT_EQ(8, width);
    EXPECT_EQ(32, height);
    
    gpu->getOBJDimensions(2, 2, width, height);
    EXPECT_EQ(16, width);
    EXPECT_EQ(32, height);
    
    gpu->getOBJDimensions(2, 3, width, height);
    EXPECT_EQ(32, width);
    EXPECT_EQ(64, height);
}

TEST_F(OAMTest, ProhibitedShapeDimensions) {
    int width, height;
    
    // Prohibited shape (shape=3)
    gpu->getOBJDimensions(3, 0, width, height);
    EXPECT_EQ(0, width);
    EXPECT_EQ(0, height);
}

// ============================================================================
// Test: Basic OAM Parsing
// ============================================================================

TEST_F(OAMTest, ParseBasicSprite) {
    // Simple 16×16 sprite at (50, 30)
    // attr0: Y=30, shape=0 (square)
    // attr1: X=50, size=1 (16×16)
    // attr2: tile=10, priority=2, palette=3 (4bpp)
    
    uint16_t attr0 = 30 | (0 << 14);  // Y=30, shape=square
    uint16_t attr1 = 50 | (1 << 14);  // X=50, size=1
    uint16_t attr2 = 10 | (2 << 10) | (3 << 12);  // tile=10, priority=2, palette=3
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(30, obj.y);
    EXPECT_EQ(50, obj.x);
    EXPECT_EQ(16, obj.width);
    EXPECT_EQ(16, obj.height);
    EXPECT_EQ(10, obj.tileNumber);
    EXPECT_EQ(2, obj.priority);
    EXPECT_EQ(3, obj.paletteNum);
    EXPECT_FALSE(obj.paletteMode);  // 4bpp
    EXPECT_FALSE(obj.rotScaleFlag);
    EXPECT_FALSE(obj.hFlip);
    EXPECT_FALSE(obj.vFlip);
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, Parse8bppSprite) {
    // 32×32 sprite with 8bpp mode
    uint16_t attr0 = 100 | (0x2000) | (0 << 14);  // Y=100, 8bpp mode, square
    uint16_t attr1 = 120 | (2 << 14);              // X=120, size=2 (32×32)
    uint16_t attr2 = 50;                           // tile=50, priority=0, palette ignored
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(100, obj.y);
    EXPECT_EQ(120, obj.x);
    EXPECT_EQ(32, obj.width);
    EXPECT_EQ(32, obj.height);
    EXPECT_EQ(50, obj.tileNumber);
    EXPECT_TRUE(obj.paletteMode);  // 8bpp
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, ParseFlippedSprite) {
    // Sprite with horizontal and vertical flip
    uint16_t attr0 = 50;                           // Y=50
    uint16_t attr1 = 80 | 0x1000 | 0x2000;         // X=80, H-flip, V-flip
    uint16_t attr2 = 20;                           // tile=20
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(50, obj.y);
    EXPECT_EQ(80, obj.x);
    EXPECT_TRUE(obj.hFlip);
    EXPECT_TRUE(obj.vFlip);
    EXPECT_FALSE(obj.rotScaleFlag);
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, ParseDisabledSprite) {
    // Sprite with disable bit set (bit 9 of attr0, when rotScaleFlag=false)
    uint16_t attr0 = 50 | 0x0200;  // Y=50, OBJ disable bit
    uint16_t attr1 = 80;
    uint16_t attr2 = 20;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_FALSE(obj.visible);
    EXPECT_EQ(0, obj.width);
    EXPECT_EQ(0, obj.height);
}

TEST_F(OAMTest, ParseRotScaleSprite) {
    // Sprite with rotation/scaling enabled
    uint16_t attr0 = 60 | 0x0100;                  // Y=60, rot/scale flag
    uint16_t attr1 = 90 | (5 << 9);                // X=90, rot/scale param=5
    uint16_t attr2 = 30;                           // tile=30
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(60, obj.y);
    EXPECT_EQ(90, obj.x);
    EXPECT_TRUE(obj.rotScaleFlag);
    EXPECT_EQ(5, obj.rotScaleParam);
    EXPECT_FALSE(obj.hFlip);  // Flip flags ignored when rotating
    EXPECT_FALSE(obj.vFlip);
    EXPECT_FALSE(obj.doubleSize);
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, ParseDoubleSizeSprite) {
    // Sprite with rotation and double-size enabled
    uint16_t attr0 = 70 | 0x0100 | 0x0200;  // Y=70, rot/scale, double-size
    uint16_t attr1 = 100;
    uint16_t attr2 = 40;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_TRUE(obj.rotScaleFlag);
    EXPECT_TRUE(obj.doubleSize);
    EXPECT_TRUE(obj.visible);
}

// ============================================================================
// Test: All Sprite Sizes
// ============================================================================

TEST_F(OAMTest, AllSquareSizes) {
    // Test all 4 square sizes
    uint16_t attr0 = 50;  // Y=50, shape=0 (square)
    uint16_t attr2 = 0;
    
    for (int size = 0; size < 4; size++) {
        uint16_t attr1 = 80 | (size << 14);
        OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
        
        int expected[] = {8, 16, 32, 64};
        EXPECT_EQ(expected[size], obj.width) << "Size " << size;
        EXPECT_EQ(expected[size], obj.height) << "Size " << size;
        EXPECT_TRUE(obj.visible);
    }
}

TEST_F(OAMTest, AllHorizontalSizes) {
    // Test all 4 horizontal (wide) sizes
    uint16_t attr0 = 50 | (1 << 14);  // Y=50, shape=1 (horizontal)
    uint16_t attr2 = 0;
    
    int expectedWidth[] = {16, 32, 32, 64};
    int expectedHeight[] = {8, 8, 16, 32};
    
    for (int size = 0; size < 4; size++) {
        uint16_t attr1 = 80 | (size << 14);
        OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
        
        EXPECT_EQ(expectedWidth[size], obj.width) << "Size " << size;
        EXPECT_EQ(expectedHeight[size], obj.height) << "Size " << size;
        EXPECT_TRUE(obj.visible);
    }
}

TEST_F(OAMTest, AllVerticalSizes) {
    // Test all 4 vertical (tall) sizes
    uint16_t attr0 = 50 | (2 << 14);  // Y=50, shape=2 (vertical)
    uint16_t attr2 = 0;
    
    int expectedWidth[] = {8, 8, 16, 32};
    int expectedHeight[] = {16, 32, 32, 64};
    
    for (int size = 0; size < 4; size++) {
        uint16_t attr1 = 80 | (size << 14);
        OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
        
        EXPECT_EQ(expectedWidth[size], obj.width) << "Size " << size;
        EXPECT_EQ(expectedHeight[size], obj.height) << "Size " << size;
        EXPECT_TRUE(obj.visible);
    }
}

// ============================================================================
// Test: Priority Levels
// ============================================================================

TEST_F(OAMTest, AllPriorityLevels) {
    // Test all 4 priority levels
    uint16_t attr0 = 50;
    uint16_t attr1 = 80;
    
    for (int priority = 0; priority < 4; priority++) {
        uint16_t attr2 = 10 | (priority << 10);
        OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
        
        EXPECT_EQ(priority, obj.priority) << "Priority " << priority;
    }
}

// ============================================================================
// Test: Reading from OAM Memory
// ============================================================================

TEST_F(OAMTest, ReadOBJFromMemory) {
    // Write sprite attributes to OAM and read them back
    writeOBJ(0, 30 | (0 << 14), 50 | (1 << 14), 10 | (2 << 10) | (3 << 12));
    
    OBJAttributes obj = gpu->readOBJAttributes(0);
    
    EXPECT_EQ(30, obj.y);
    EXPECT_EQ(50, obj.x);
    EXPECT_EQ(16, obj.width);
    EXPECT_EQ(16, obj.height);
    EXPECT_EQ(10, obj.tileNumber);
    EXPECT_EQ(2, obj.priority);
    EXPECT_EQ(3, obj.paletteNum);
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, ReadMultipleOBJs) {
    // Write several sprites to OAM
    writeOBJ(0, 10, 20, 30);
    writeOBJ(5, 40, 50, 60);
    writeOBJ(127, 70, 80, 90);
    
    OBJAttributes obj0 = gpu->readOBJAttributes(0);
    OBJAttributes obj5 = gpu->readOBJAttributes(5);
    OBJAttributes obj127 = gpu->readOBJAttributes(127);
    
    EXPECT_EQ(10, obj0.y);
    EXPECT_EQ(40, obj5.y);
    EXPECT_EQ(70, obj127.y);
}

TEST_F(OAMTest, ReadInvalidOBJIndex) {
    // Test bounds checking
    OBJAttributes objNeg = gpu->readOBJAttributes(-1);
    OBJAttributes obj128 = gpu->readOBJAttributes(128);
    
    EXPECT_FALSE(objNeg.visible);
    EXPECT_FALSE(obj128.visible);
}

// ============================================================================
// Test: OBJ Modes
// ============================================================================

TEST_F(OAMTest, NormalMode) {
    uint16_t attr0 = 50 | (0 << 10);  // Y=50, mode=0 (normal)
    uint16_t attr1 = 80;
    uint16_t attr2 = 20;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(0, obj.objMode);
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, SemiTransparentMode) {
    uint16_t attr0 = 50 | (1 << 10);  // Y=50, mode=1 (semi-transparent)
    uint16_t attr1 = 80;
    uint16_t attr2 = 20;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(1, obj.objMode);
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, OBJWindowMode) {
    uint16_t attr0 = 50 | (2 << 10);  // Y=50, mode=2 (OBJ window)
    uint16_t attr1 = 80;
    uint16_t attr2 = 20;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(2, obj.objMode);
    EXPECT_TRUE(obj.visible);
}

TEST_F(OAMTest, ProhibitedMode) {
    uint16_t attr0 = 50 | (3 << 10);  // Y=50, mode=3 (prohibited)
    uint16_t attr1 = 80;
    uint16_t attr2 = 20;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(3, obj.objMode);
    EXPECT_FALSE(obj.visible);  // Prohibited mode = not visible
}

// ============================================================================
// Test: Edge Cases
// ============================================================================

TEST_F(OAMTest, MaxCoordinates) {
    // Test maximum Y (255) and X (511)
    uint16_t attr0 = 255;
    uint16_t attr1 = 511;
    uint16_t attr2 = 0;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(255, obj.y);
    EXPECT_EQ(511, obj.x);
}

TEST_F(OAMTest, MaxTileNumber) {
    // Test maximum tile number (1023)
    uint16_t attr0 = 50;
    uint16_t attr1 = 80;
    uint16_t attr2 = 1023;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(1023, obj.tileNumber);
}

TEST_F(OAMTest, MaxPaletteNumber) {
    // Test maximum palette number (15 in 4bpp mode)
    uint16_t attr0 = 50;
    uint16_t attr1 = 80;
    uint16_t attr2 = 10 | (15 << 12);
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(15, obj.paletteNum);
}

TEST_F(OAMTest, MosaicEnable) {
    // Test mosaic enable bit
    uint16_t attr0 = 50 | 0x1000;  // Y=50, mosaic enabled
    uint16_t attr1 = 80;
    uint16_t attr2 = 20;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_TRUE(obj.mosaicEnable);
}

TEST_F(OAMTest, ProhibitedShapeNotVisible) {
    // Sprite with prohibited shape should not be visible
    uint16_t attr0 = 50 | (3 << 14);  // Y=50, shape=3 (prohibited)
    uint16_t attr1 = 80;
    uint16_t attr2 = 20;
    
    OBJAttributes obj = gpu->parseOBJAttributes(attr0, attr1, attr2);
    
    EXPECT_EQ(3, obj.shape);
    EXPECT_FALSE(obj.visible);
}
