#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

// Test fixture for BGxCNT register tests
class BGCNTTest : public ::testing::Test {
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

    void writeBGCNT(int bgNum, uint16_t value) {
        uint32_t addr = 0x04000008 + (bgNum * 2);
        memory->write16(addr, value);
    }
};

// Test 1: Parse priority (bits 0-1)
TEST_F(BGCNTTest, ParsePriority) {
    // Test all priority levels 0-3
    for (int priority = 0; priority <= 3; priority++) {
        BGConfig config = gpu->parseBGCNT(priority);
        EXPECT_EQ(priority, config.priority) << "Priority " << priority << " not parsed correctly";
    }
}

// Test 2: Parse character base block (bits 2-3)
TEST_F(BGCNTTest, ParseCharacterBaseBlock) {
    // Test all character base blocks 0-3
    for (int block = 0; block <= 3; block++) {
        uint16_t bgcnt = block << 2;
        BGConfig config = gpu->parseBGCNT(bgcnt);
        
        EXPECT_EQ(block, config.charBaseBlock);
        
        // Verify computed address (block * 16KB)
        uint32_t expectedAddr = 0x06000000 + (block * 0x4000);
        EXPECT_EQ(expectedAddr, config.charBaseAddr);
    }
}

// Test 3: Parse mosaic enable (bit 6)
TEST_F(BGCNTTest, ParseMosaicEnable) {
    BGConfig configOff = gpu->parseBGCNT(0x0000);
    EXPECT_FALSE(configOff.mosaicEnable);
    
    BGConfig configOn = gpu->parseBGCNT(0x0040);
    EXPECT_TRUE(configOn.mosaicEnable);
}

// Test 4: Parse palette mode (bit 7)
TEST_F(BGCNTTest, ParsePaletteMode) {
    // 0 = 16 palettes of 16 colors (4bpp)
    BGConfig config4bpp = gpu->parseBGCNT(0x0000);
    EXPECT_FALSE(config4bpp.paletteMode);
    
    // 1 = 1 palette of 256 colors (8bpp)
    BGConfig config8bpp = gpu->parseBGCNT(0x0080);
    EXPECT_TRUE(config8bpp.paletteMode);
}

// Test 5: Parse screen base block (bits 8-12)
TEST_F(BGCNTTest, ParseScreenBaseBlock) {
    // Test several screen base blocks
    for (int block = 0; block <= 31; block += 7) {
        uint16_t bgcnt = block << 8;
        BGConfig config = gpu->parseBGCNT(bgcnt);
        
        EXPECT_EQ(block, config.screenBaseBlock);
        
        // Verify computed address (block * 2KB)
        uint32_t expectedAddr = 0x06000000 + (block * 0x800);
        EXPECT_EQ(expectedAddr, config.screenBaseAddr);
    }
}

// Test 6: Parse screen size (bits 14-15)
TEST_F(BGCNTTest, ParseScreenSize) {
    // Size 0: 256x256 (32x32 tiles)
    BGConfig config0 = gpu->parseBGCNT(0x0000);
    EXPECT_EQ(0, config0.screenSize);
    EXPECT_EQ(32, config0.screenWidthTiles);
    EXPECT_EQ(32, config0.screenHeightTiles);
    EXPECT_EQ(256, config0.screenWidthPixels);
    EXPECT_EQ(256, config0.screenHeightPixels);
    
    // Size 1: 512x256 (64x32 tiles)
    BGConfig config1 = gpu->parseBGCNT(0x4000);
    EXPECT_EQ(1, config1.screenSize);
    EXPECT_EQ(64, config1.screenWidthTiles);
    EXPECT_EQ(32, config1.screenHeightTiles);
    EXPECT_EQ(512, config1.screenWidthPixels);
    EXPECT_EQ(256, config1.screenHeightPixels);
    
    // Size 2: 256x512 (32x64 tiles)
    BGConfig config2 = gpu->parseBGCNT(0x8000);
    EXPECT_EQ(2, config2.screenSize);
    EXPECT_EQ(32, config2.screenWidthTiles);
    EXPECT_EQ(64, config2.screenHeightTiles);
    EXPECT_EQ(256, config2.screenWidthPixels);
    EXPECT_EQ(512, config2.screenHeightPixels);
    
    // Size 3: 512x512 (64x64 tiles)
    BGConfig config3 = gpu->parseBGCNT(0xC000);
    EXPECT_EQ(3, config3.screenSize);
    EXPECT_EQ(64, config3.screenWidthTiles);
    EXPECT_EQ(64, config3.screenHeightTiles);
    EXPECT_EQ(512, config3.screenWidthPixels);
    EXPECT_EQ(512, config3.screenHeightPixels);
}

// Test 7: Complex configuration
TEST_F(BGCNTTest, ComplexConfiguration) {
    // Priority 2, char base 1, 8bpp, screen base 10, size 512x256
    uint16_t bgcnt = 0x4A86;  // 0100 1010 1000 0110
    // Bits: 01 00 1 010 1 000 0 1 10
    // Size=1, ScreenBase=10, Palette=1, Mosaic=0, CharBase=1, Priority=2
    
    BGConfig config = gpu->parseBGCNT(bgcnt);
    
    EXPECT_EQ(2, config.priority);
    EXPECT_EQ(1, config.charBaseBlock);
    EXPECT_FALSE(config.mosaicEnable);
    EXPECT_TRUE(config.paletteMode);
    EXPECT_EQ(10, config.screenBaseBlock);
    EXPECT_EQ(1, config.screenSize);
    
    // Verify addresses
    EXPECT_EQ(0x06000000 + 0x4000, config.charBaseAddr);    // Block 1
    EXPECT_EQ(0x06000000 + 0x5000, config.screenBaseAddr);  // Block 10 (10 * 0x800)
    
    // Verify dimensions
    EXPECT_EQ(64, config.screenWidthTiles);
    EXPECT_EQ(32, config.screenHeightTiles);
}

// Test 8: Read BG0CNT from memory
TEST_F(BGCNTTest, ReadBG0CNT) {
    // Priority 3, char base 0, screen base 28, size 512x512
    // Bits: 11 11100 0 0 0000 0 11
    // Size (14-15): 11 = 0xC000
    // Screen base (8-12): 11100 = 28 = 0x1C00
    // Priority (0-1): 11 = 3 = 0x0003
    writeBGCNT(0, 0xDC03);  // 0xC000 | 0x1C00 | 0x0003
    
    BGConfig config = gpu->readBGCNT(0);
    
    EXPECT_EQ(3, config.priority);
    EXPECT_EQ(0, config.charBaseBlock);
    EXPECT_EQ(28, config.screenBaseBlock);
    EXPECT_EQ(3, config.screenSize);
}

// Test 9: Read all BG registers
TEST_F(BGCNTTest, ReadAllBGCNT) {
    // Configure all 4 backgrounds differently
    writeBGCNT(0, 0x0800);  // BG0: screen base 8
    writeBGCNT(1, 0x1004);  // BG1: screen base 16, char base 1
    writeBGCNT(2, 0x1808);  // BG2: screen base 24, char base 2
    writeBGCNT(3, 0x1F0C);  // BG3: screen base 31 (max), char base 3
    
    BGConfig config0 = gpu->readBGCNT(0);
    BGConfig config1 = gpu->readBGCNT(1);
    BGConfig config2 = gpu->readBGCNT(2);
    BGConfig config3 = gpu->readBGCNT(3);
    
    EXPECT_EQ(8, config0.screenBaseBlock);
    EXPECT_EQ(16, config1.screenBaseBlock);
    EXPECT_EQ(24, config2.screenBaseBlock);
    EXPECT_EQ(31, config3.screenBaseBlock);  // Max is 31, not 32
    
    EXPECT_EQ(0, config0.charBaseBlock);
    EXPECT_EQ(1, config1.charBaseBlock);
    EXPECT_EQ(2, config2.charBaseBlock);
    EXPECT_EQ(3, config3.charBaseBlock);
}

// Test 10: Invalid BG number
TEST_F(BGCNTTest, InvalidBGNumber) {
    // Should return default config for invalid BG numbers
    BGConfig configNeg = gpu->readBGCNT(-1);
    BGConfig config4 = gpu->readBGCNT(4);
    
    // Both should return default (all zeros)
    EXPECT_EQ(0, configNeg.priority);
    EXPECT_EQ(0, config4.priority);
}

// Test 11: Typical Mode 0 configuration
TEST_F(BGCNTTest, TypicalMode0Config) {
    // Typical Mode 0 setup:
    // BG0: Priority 0, 4bpp, screen base 31, 256x256
    writeBGCNT(0, 0x1F00);
    
    BGConfig config = gpu->readBGCNT(0);
    
    EXPECT_EQ(0, config.priority);
    EXPECT_FALSE(config.paletteMode);  // 4bpp
    EXPECT_EQ(31, config.screenBaseBlock);
    EXPECT_EQ(0, config.screenSize);
    EXPECT_EQ(32, config.screenWidthTiles);
    EXPECT_EQ(32, config.screenHeightTiles);
}

// Test 12: Character base address boundaries
TEST_F(BGCNTTest, CharacterBaseAddresses) {
    // Block 0: 0x06000000
    BGConfig config0 = gpu->parseBGCNT(0x0000);
    EXPECT_EQ(0x06000000, config0.charBaseAddr);
    
    // Block 1: 0x06004000
    BGConfig config1 = gpu->parseBGCNT(0x0004);
    EXPECT_EQ(0x06004000, config1.charBaseAddr);
    
    // Block 2: 0x06008000
    BGConfig config2 = gpu->parseBGCNT(0x0008);
    EXPECT_EQ(0x06008000, config2.charBaseAddr);
    
    // Block 3: 0x0600C000
    BGConfig config3 = gpu->parseBGCNT(0x000C);
    EXPECT_EQ(0x0600C000, config3.charBaseAddr);
}

// Test 13: Screen base address boundaries
TEST_F(BGCNTTest, ScreenBaseAddresses) {
    // Block 0: 0x06000000
    BGConfig config0 = gpu->parseBGCNT(0x0000);
    EXPECT_EQ(0x06000000, config0.screenBaseAddr);
    
    // Block 8: 0x06004000
    BGConfig config8 = gpu->parseBGCNT(0x0800);
    EXPECT_EQ(0x06004000, config8.screenBaseAddr);
    
    // Block 16: 0x06008000
    BGConfig config16 = gpu->parseBGCNT(0x1000);
    EXPECT_EQ(0x06008000, config16.screenBaseAddr);
    
    // Block 31: 0x0600F800
    BGConfig config31 = gpu->parseBGCNT(0x1F00);
    EXPECT_EQ(0x0600F800, config31.screenBaseAddr);
}

// Test 14: All bits zero
TEST_F(BGCNTTest, AllBitsZero) {
    BGConfig config = gpu->parseBGCNT(0x0000);
    
    EXPECT_EQ(0, config.priority);
    EXPECT_EQ(0, config.charBaseBlock);
    EXPECT_FALSE(config.mosaicEnable);
    EXPECT_FALSE(config.paletteMode);
    EXPECT_EQ(0, config.screenBaseBlock);
    EXPECT_EQ(0, config.screenSize);
    EXPECT_EQ(0x06000000, config.charBaseAddr);
    EXPECT_EQ(0x06000000, config.screenBaseAddr);
    EXPECT_EQ(32, config.screenWidthTiles);
    EXPECT_EQ(32, config.screenHeightTiles);
}

// Test 15: All relevant bits set
TEST_F(BGCNTTest, AllRelevantBitsSet) {
    // 0xFFFF: all bits set
    BGConfig config = gpu->parseBGCNT(0xFFFF);
    
    EXPECT_EQ(3, config.priority);          // Bits 0-1: max
    EXPECT_EQ(3, config.charBaseBlock);     // Bits 2-3: max
    EXPECT_TRUE(config.mosaicEnable);       // Bit 6
    EXPECT_TRUE(config.paletteMode);        // Bit 7
    EXPECT_EQ(31, config.screenBaseBlock);  // Bits 8-12: max
    EXPECT_EQ(3, config.screenSize);        // Bits 14-15: max
}

// Test 16: Priority levels for layering
TEST_F(BGCNTTest, PriorityLevels) {
    // Configure BGs with different priorities
    writeBGCNT(0, 0x0000);  // Priority 0 (highest)
    writeBGCNT(1, 0x0001);  // Priority 1
    writeBGCNT(2, 0x0002);  // Priority 2
    writeBGCNT(3, 0x0003);  // Priority 3 (lowest)
    
    BGConfig config0 = gpu->readBGCNT(0);
    BGConfig config1 = gpu->readBGCNT(1);
    BGConfig config2 = gpu->readBGCNT(2);
    BGConfig config3 = gpu->readBGCNT(3);
    
    EXPECT_EQ(0, config0.priority);  // Draws on top
    EXPECT_EQ(1, config1.priority);
    EXPECT_EQ(2, config2.priority);
    EXPECT_EQ(3, config3.priority);  // Draws on bottom
}

// Test 17: Large screen configuration (512x512)
TEST_F(BGCNTTest, LargeScreenConfig) {
    // 512x512 screen with 8bpp, char base 2, screen base 20
    uint16_t bgcnt = 0xC280 | 0x1408;  // Size 3, screen 20, palette 8bpp, char 2
    
    BGConfig config = gpu->parseBGCNT(bgcnt);
    
    EXPECT_EQ(3, config.screenSize);
    EXPECT_EQ(64, config.screenWidthTiles);
    EXPECT_EQ(64, config.screenHeightTiles);
    EXPECT_EQ(512, config.screenWidthPixels);
    EXPECT_EQ(512, config.screenHeightPixels);
    
    // 64x64 tiles = 4096 tiles total
    // Each screen entry is 2 bytes
    // Total screen data: 4096 * 2 = 8192 bytes = 8KB
}

// Test 18: Screen dimensions helper function
TEST_F(BGCNTTest, ScreenDimensionsHelper) {
    int width, height;
    
    gpu->getScreenDimensions(0, width, height);
    EXPECT_EQ(32, width);
    EXPECT_EQ(32, height);
    
    gpu->getScreenDimensions(1, width, height);
    EXPECT_EQ(64, width);
    EXPECT_EQ(32, height);
    
    gpu->getScreenDimensions(2, width, height);
    EXPECT_EQ(32, width);
    EXPECT_EQ(64, height);
    
    gpu->getScreenDimensions(3, width, height);
    EXPECT_EQ(64, width);
    EXPECT_EQ(64, height);
}
