#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

// Test fixture for tile map tests
class TileMapTest : public ::testing::Test {
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

    // Helper to write a screen entry to VRAM
    void writeScreenEntry(uint32_t addr, uint16_t entry) {
        memory->write16(addr, entry);
    }
    
    // Helper to create a BGConfig for testing
    BGConfig createTestBGConfig(uint8_t screenSize = 0) {
        // Create a simple config: char base 0, screen base 8, specified size
        uint16_t bgcnt = (8 << 8) | (screenSize << 14);
        return gpu->parseBGCNT(bgcnt);
    }
};

// Test 1: Parse screen entry - tile number (bits 0-9)
TEST_F(TileMapTest, ParseTileNumber) {
    // Test various tile numbers
    ScreenEntry se0 = gpu->parseScreenEntry(0x0000);
    EXPECT_EQ(0, se0.tileNumber);
    
    ScreenEntry se1 = gpu->parseScreenEntry(0x0001);
    EXPECT_EQ(1, se1.tileNumber);
    
    ScreenEntry se42 = gpu->parseScreenEntry(0x002A);  // 42 in hex
    EXPECT_EQ(42, se42.tileNumber);
    
    ScreenEntry se1023 = gpu->parseScreenEntry(0x03FF);  // Max tile number
    EXPECT_EQ(1023, se1023.tileNumber);
}

// Test 2: Parse horizontal flip (bit 10)
TEST_F(TileMapTest, ParseHorizontalFlip) {
    ScreenEntry seNoFlip = gpu->parseScreenEntry(0x0000);
    EXPECT_FALSE(seNoFlip.hFlip);
    
    ScreenEntry seHFlip = gpu->parseScreenEntry(0x0400);
    EXPECT_TRUE(seHFlip.hFlip);
}

// Test 3: Parse vertical flip (bit 11)
TEST_F(TileMapTest, ParseVerticalFlip) {
    ScreenEntry seNoFlip = gpu->parseScreenEntry(0x0000);
    EXPECT_FALSE(seNoFlip.vFlip);
    
    ScreenEntry seVFlip = gpu->parseScreenEntry(0x0800);
    EXPECT_TRUE(seVFlip.vFlip);
}

// Test 4: Parse palette number (bits 12-15)
TEST_F(TileMapTest, ParsePaletteNumber) {
    // Test all palette numbers 0-15
    for (int pal = 0; pal <= 15; pal++) {
        uint16_t entry = pal << 12;
        ScreenEntry se = gpu->parseScreenEntry(entry);
        EXPECT_EQ(pal, se.paletteNum) << "Palette " << pal << " not parsed correctly";
    }
}

// Test 5: Parse complex screen entry
TEST_F(TileMapTest, ParseComplexEntry) {
    // Tile 123, hFlip, vFlip, palette 5
    // 0101 1100 0111 1011
    // Pal=5, VFlip=1, HFlip=1, Tile=123
    uint16_t entry = 0x5C7B;
    
    ScreenEntry se = gpu->parseScreenEntry(entry);
    
    EXPECT_EQ(123, se.tileNumber);
    EXPECT_TRUE(se.hFlip);
    EXPECT_TRUE(se.vFlip);
    EXPECT_EQ(5, se.paletteNum);
}

// Test 6: Read screen entry from VRAM (32x32 screen)
TEST_F(TileMapTest, ReadScreenEntry32x32) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    
    // Write screen entries to VRAM
    // Screen base is at 0x06004000 (block 8)
    writeScreenEntry(0x06004000, 0x0001);  // Tile (0,0) = tile 1
    writeScreenEntry(0x06004002, 0x0002);  // Tile (1,0) = tile 2
    writeScreenEntry(0x06004040, 0x0003);  // Tile (0,1) = tile 3 (next row, 32*2 = 0x40)
    
    ScreenEntry se00 = gpu->readScreenEntry(config, 0, 0);
    ScreenEntry se10 = gpu->readScreenEntry(config, 1, 0);
    ScreenEntry se01 = gpu->readScreenEntry(config, 0, 1);
    
    EXPECT_EQ(1, se00.tileNumber);
    EXPECT_EQ(2, se10.tileNumber);
    EXPECT_EQ(3, se01.tileNumber);
}

// Test 7: Read from different positions in 32x32 screen
TEST_F(TileMapTest, ReadVariousPositions32x32) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    
    // Top-left corner
    writeScreenEntry(0x06004000, 0x0010);
    
    // Top-right corner (x=31, y=0)
    // Offset = (0 * 32 + 31) * 2 = 62 bytes
    writeScreenEntry(0x0600403E, 0x0020);
    
    // Bottom-left corner (x=0, y=31)
    // Offset = (31 * 32 + 0) * 2 = 1984 bytes = 0x7C0
    writeScreenEntry(0x060047C0, 0x0030);
    
    // Bottom-right corner (x=31, y=31)
    // Offset = (31 * 32 + 31) * 2 = 2046 bytes = 0x7FE
    writeScreenEntry(0x060047FE, 0x0040);
    
    EXPECT_EQ(0x10, gpu->readScreenEntry(config, 0, 0).tileNumber);
    EXPECT_EQ(0x20, gpu->readScreenEntry(config, 31, 0).tileNumber);
    EXPECT_EQ(0x30, gpu->readScreenEntry(config, 0, 31).tileNumber);
    EXPECT_EQ(0x40, gpu->readScreenEntry(config, 31, 31).tileNumber);
}

// Test 8: Read from 512x256 screen (64x32 tiles)
TEST_F(TileMapTest, ReadScreenEntry512x256) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_512x256);
    
    // Left half (first 32x32 block at offset 0)
    writeScreenEntry(0x06004000, 0x0001);  // Tile (0,0)
    
    // Right half (second 32x32 block at offset 0x800)
    // Tile (32,0) is in the second block at local position (0,0)
    writeScreenEntry(0x06004800, 0x0002);  // Tile (32,0)
    
    // Tile (63,0) is in the second block at local position (31,0)
    writeScreenEntry(0x0600483E, 0x0003);  // Tile (63,0)
    
    EXPECT_EQ(1, gpu->readScreenEntry(config, 0, 0).tileNumber);
    EXPECT_EQ(2, gpu->readScreenEntry(config, 32, 0).tileNumber);
    EXPECT_EQ(3, gpu->readScreenEntry(config, 63, 0).tileNumber);
}

// Test 9: Read from 256x512 screen (32x64 tiles)
TEST_F(TileMapTest, ReadScreenEntry256x512) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x512);
    
    // Top half (first 32x32 block at offset 0)
    writeScreenEntry(0x06004000, 0x0001);  // Tile (0,0)
    
    // Bottom half (second 32x32 block at offset 0x800)
    // Tile (0,32) is in the second block at local position (0,0)
    writeScreenEntry(0x06004800, 0x0002);  // Tile (0,32)
    
    // Tile (0,63) is in the second block at local position (0,31)
    writeScreenEntry(0x06004FC0, 0x0003);  // Tile (0,63)
    
    EXPECT_EQ(1, gpu->readScreenEntry(config, 0, 0).tileNumber);
    EXPECT_EQ(2, gpu->readScreenEntry(config, 0, 32).tileNumber);
    EXPECT_EQ(3, gpu->readScreenEntry(config, 0, 63).tileNumber);
}

// Test 10: Read from 512x512 screen (64x64 tiles)
TEST_F(TileMapTest, ReadScreenEntry512x512) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_512x512);
    
    // Block layout: [0][1]
    //               [2][3]
    
    // Block 0: Top-left (0,0)
    writeScreenEntry(0x06004000, 0x0001);
    
    // Block 1: Top-right (32,0)
    writeScreenEntry(0x06004800, 0x0002);
    
    // Block 2: Bottom-left (0,32)
    writeScreenEntry(0x06005000, 0x0003);
    
    // Block 3: Bottom-right (32,32)
    writeScreenEntry(0x06005800, 0x0004);
    
    EXPECT_EQ(1, gpu->readScreenEntry(config, 0, 0).tileNumber);
    EXPECT_EQ(2, gpu->readScreenEntry(config, 32, 0).tileNumber);
    EXPECT_EQ(3, gpu->readScreenEntry(config, 0, 32).tileNumber);
    EXPECT_EQ(4, gpu->readScreenEntry(config, 32, 32).tileNumber);
}

// Test 11: Bounds checking
TEST_F(TileMapTest, BoundsChecking) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    
    // Out of bounds should return tile 0
    ScreenEntry seNegX = gpu->readScreenEntry(config, -1, 0);
    ScreenEntry seNegY = gpu->readScreenEntry(config, 0, -1);
    ScreenEntry seTooLargeX = gpu->readScreenEntry(config, 32, 0);
    ScreenEntry seTooLargeY = gpu->readScreenEntry(config, 0, 32);
    
    EXPECT_EQ(0, seNegX.tileNumber);
    EXPECT_EQ(0, seNegY.tileNumber);
    EXPECT_EQ(0, seTooLargeX.tileNumber);
    EXPECT_EQ(0, seTooLargeY.tileNumber);
}

// Test 12: Get tile address from screen entry (4bpp)
TEST_F(TileMapTest, GetTileAddress4bpp) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    config.paletteMode = false;  // 4bpp
    config.charBaseAddr = 0x06000000;
    
    ScreenEntry se;
    se.tileNumber = 0;
    EXPECT_EQ(0x06000000u, gpu->getTileAddress(config, se));
    
    se.tileNumber = 1;
    EXPECT_EQ(0x06000020u, gpu->getTileAddress(config, se));  // Tile 1 at +32 bytes
    
    se.tileNumber = 10;
    EXPECT_EQ(0x06000140u, gpu->getTileAddress(config, se));  // Tile 10 at +320 bytes
}

// Test 13: Get tile address from screen entry (8bpp)
TEST_F(TileMapTest, GetTileAddress8bpp) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    config.paletteMode = true;  // 8bpp
    config.charBaseAddr = 0x06000000;
    
    ScreenEntry se;
    se.tileNumber = 0;
    EXPECT_EQ(0x06000000u, gpu->getTileAddress(config, se));
    
    se.tileNumber = 1;
    EXPECT_EQ(0x06000040u, gpu->getTileAddress(config, se));  // Tile 1 at +64 bytes
    
    se.tileNumber = 10;
    EXPECT_EQ(0x06000280u, gpu->getTileAddress(config, se));  // Tile 10 at +640 bytes
}

// Test 14: Screen block offset calculation
TEST_F(TileMapTest, ScreenBlockOffset) {
    BGConfig config256 = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    BGConfig config512x256 = createTestBGConfig(BG_SCREEN_SIZE_512x256);
    BGConfig config256x512 = createTestBGConfig(BG_SCREEN_SIZE_256x512);
    BGConfig config512x512 = createTestBGConfig(BG_SCREEN_SIZE_512x512);
    
    // 256x256: no offset needed
    EXPECT_EQ(0u, gpu->getScreenBlockOffset(config256, 0, 0));
    EXPECT_EQ(0u, gpu->getScreenBlockOffset(config256, 31, 31));
    
    // 512x256: offset 0x800 for x >= 32
    EXPECT_EQ(0u, gpu->getScreenBlockOffset(config512x256, 0, 0));
    EXPECT_EQ(0u, gpu->getScreenBlockOffset(config512x256, 31, 0));
    EXPECT_EQ(0x800u, gpu->getScreenBlockOffset(config512x256, 32, 0));
    EXPECT_EQ(0x800u, gpu->getScreenBlockOffset(config512x256, 63, 0));
    
    // 256x512: offset 0x800 for y >= 32
    EXPECT_EQ(0u, gpu->getScreenBlockOffset(config256x512, 0, 0));
    EXPECT_EQ(0u, gpu->getScreenBlockOffset(config256x512, 0, 31));
    EXPECT_EQ(0x800u, gpu->getScreenBlockOffset(config256x512, 0, 32));
    EXPECT_EQ(0x800u, gpu->getScreenBlockOffset(config256x512, 0, 63));
    
    // 512x512: [0][1]
    //          [2][3]
    EXPECT_EQ(0x0000u, gpu->getScreenBlockOffset(config512x512, 0, 0));      // Block 0
    EXPECT_EQ(0x0800u, gpu->getScreenBlockOffset(config512x512, 32, 0));     // Block 1
    EXPECT_EQ(0x1000u, gpu->getScreenBlockOffset(config512x512, 0, 32));     // Block 2
    EXPECT_EQ(0x1800u, gpu->getScreenBlockOffset(config512x512, 32, 32));    // Block 3
}

// Test 15: Read raw screen entry
TEST_F(TileMapTest, ReadScreenEntryRaw) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    
    // Write a complex entry
    writeScreenEntry(0x06004000, 0xABCD);
    
    uint16_t raw = gpu->readScreenEntryRaw(config, 0, 0);
    EXPECT_EQ(0xABCD, raw);
}

// Test 16: Integration test - tile map with flips and palettes
TEST_F(TileMapTest, IntegrationTileMapWithFlips) {
    BGConfig config = createTestBGConfig(BG_SCREEN_SIZE_256x256);
    
    // Create entries with different attributes
    // Entry 0: Tile 5, no flips, palette 0
    writeScreenEntry(0x06004000, 0x0005);
    
    // Entry 1: Tile 10, hFlip, palette 3
    writeScreenEntry(0x06004002, 0x3400 | 0x000A);  // Pal 3, HFlip, Tile 10
    
    // Entry 2: Tile 15, vFlip, palette 7
    writeScreenEntry(0x06004004, 0x7800 | 0x000F);  // Pal 7, VFlip, Tile 15
    
    // Entry 3: Tile 20, both flips, palette 15
    writeScreenEntry(0x06004006, 0xFC00 | 0x0014);  // Pal 15, HFlip, VFlip, Tile 20
    
    ScreenEntry se0 = gpu->readScreenEntry(config, 0, 0);
    ScreenEntry se1 = gpu->readScreenEntry(config, 1, 0);
    ScreenEntry se2 = gpu->readScreenEntry(config, 2, 0);
    ScreenEntry se3 = gpu->readScreenEntry(config, 3, 0);
    
    // Verify tile 0
    EXPECT_EQ(5, se0.tileNumber);
    EXPECT_FALSE(se0.hFlip);
    EXPECT_FALSE(se0.vFlip);
    EXPECT_EQ(0, se0.paletteNum);
    
    // Verify tile 1
    EXPECT_EQ(10, se1.tileNumber);
    EXPECT_TRUE(se1.hFlip);
    EXPECT_FALSE(se1.vFlip);
    EXPECT_EQ(3, se1.paletteNum);
    
    // Verify tile 2
    EXPECT_EQ(15, se2.tileNumber);
    EXPECT_FALSE(se2.hFlip);
    EXPECT_TRUE(se2.vFlip);
    EXPECT_EQ(7, se2.paletteNum);
    
    // Verify tile 3
    EXPECT_EQ(20, se3.tileNumber);
    EXPECT_TRUE(se3.hFlip);
    EXPECT_TRUE(se3.vFlip);
    EXPECT_EQ(15, se3.paletteNum);
}

// Test 17: Large tile numbers
TEST_F(TileMapTest, LargeTileNumbers) {
    // Test tile numbers near the max (1023)
    ScreenEntry se1020 = gpu->parseScreenEntry(0x03FC);  // 1020
    ScreenEntry se1023 = gpu->parseScreenEntry(0x03FF);  // 1023 (max)
    
    EXPECT_EQ(1020, se1020.tileNumber);
    EXPECT_EQ(1023, se1023.tileNumber);
}

// Test 18: All bits zero vs all bits set
TEST_F(TileMapTest, ExtremeBitPatterns) {
    // All zeros
    ScreenEntry seZero = gpu->parseScreenEntry(0x0000);
    EXPECT_EQ(0, seZero.tileNumber);
    EXPECT_FALSE(seZero.hFlip);
    EXPECT_FALSE(seZero.vFlip);
    EXPECT_EQ(0, seZero.paletteNum);
    
    // All ones
    ScreenEntry seMax = gpu->parseScreenEntry(0xFFFF);
    EXPECT_EQ(1023, seMax.tileNumber);
    EXPECT_TRUE(seMax.hFlip);
    EXPECT_TRUE(seMax.vFlip);
    EXPECT_EQ(15, seMax.paletteNum);
}
