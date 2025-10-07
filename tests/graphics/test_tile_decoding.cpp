#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"
#include <cstring>

// Test fixture for Tile Decoding tests
class TileDecodingTest : public ::testing::Test {
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

    // Helper to write tile data to VRAM
    void writeTileData(uint32_t addr, const uint8_t* data, size_t size) {
        for (size_t i = 0; i < size; i++) {
            memory->write8(addr + i, data[i]);
        }
    }
};

// Test 1: Decode 4bpp tile with simple pattern
TEST_F(TileDecodingTest, Decode4bppSimplePattern) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a simple pattern: alternating palette indices
    // Byte format: low nibble = first pixel, high nibble = second pixel
    uint8_t tileData[32];
    for (int i = 0; i < 32; i++) {
        tileData[i] = 0x12;  // Palette index 2 (low), 1 (high)
    }
    
    writeTileData(tileAddr, tileData, 32);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile4bpp(tileAddr, output);
    
    // Verify pattern: even indices should be 2, odd indices should be 1
    for (int i = 0; i < 64; i++) {
        if (i % 2 == 0) {
            EXPECT_EQ(2, output[i]) << "Even pixel " << i << " should be 2";
        } else {
            EXPECT_EQ(1, output[i]) << "Odd pixel " << i << " should be 1";
        }
    }
}

// Test 2: Decode 4bpp tile with all different values
TEST_F(TileDecodingTest, Decode4bppAllValues) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a tile with values 0-15
    uint8_t tileData[32];
    for (int i = 0; i < 16; i++) {
        // Pack two palette indices per byte
        tileData[i] = (i << 4) | i;  // Same value in both nibbles
    }
    for (int i = 16; i < 32; i++) {
        tileData[i] = 0x00;
    }
    
    writeTileData(tileAddr, tileData, 32);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile4bpp(tileAddr, output);
    
    // Verify first 32 pixels (16 bytes × 2 pixels/byte)
    for (int i = 0; i < 32; i++) {
        EXPECT_EQ(i / 2, output[i]) << "Pixel " << i << " incorrect";
    }
}

// Test 3: Decode 8bpp tile with simple pattern
TEST_F(TileDecodingTest, Decode8bppSimplePattern) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a simple pattern: sequential palette indices
    uint8_t tileData[64];
    for (int i = 0; i < 64; i++) {
        tileData[i] = i;
    }
    
    writeTileData(tileAddr, tileData, 64);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile8bpp(tileAddr, output);
    
    // Verify all pixels
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(i, output[i]) << "Pixel " << i << " should be " << i;
    }
}

// Test 4: Decode 8bpp tile with repeated value
TEST_F(TileDecodingTest, Decode8bppRepeatedValue) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a tile with all pixels set to 42
    uint8_t tileData[64];
    for (int i = 0; i < 64; i++) {
        tileData[i] = 42;
    }
    
    writeTileData(tileAddr, tileData, 64);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile8bpp(tileAddr, output);
    
    // Verify all pixels are 42
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(42, output[i]);
    }
}

// Test 5: Get single pixel from 4bpp tile
TEST_F(TileDecodingTest, GetSinglePixel4bpp) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a tile where each row has a different pattern
    uint8_t tileData[32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 4; col++) {  // 4 bytes per row (8 pixels)
            int byteIndex = row * 4 + col;
            tileData[byteIndex] = (row << 4) | col;
        }
    }
    
    writeTileData(tileAddr, tileData, 32);
    
    // Test getting specific pixels
    EXPECT_EQ(0, gpu->getTilePixel4bpp(tileAddr, 0, 0));  // First pixel of first byte
    EXPECT_EQ(0, gpu->getTilePixel4bpp(tileAddr, 1, 0));  // Second pixel of first byte
    EXPECT_EQ(1, gpu->getTilePixel4bpp(tileAddr, 2, 0));  // First pixel of second byte
    EXPECT_EQ(0, gpu->getTilePixel4bpp(tileAddr, 3, 0));  // Second pixel of second byte
    
    // Test a pixel from row 5, column 6
    EXPECT_EQ(5, gpu->getTilePixel4bpp(tileAddr, 7, 5));  // High nibble of last byte in row 5
}

// Test 6: Get single pixel from 8bpp tile
TEST_F(TileDecodingTest, GetSinglePixel8bpp) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a tile where pixel value = row * 8 + col
    uint8_t tileData[64];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            tileData[row * 8 + col] = row * 8 + col;
        }
    }
    
    writeTileData(tileAddr, tileData, 64);
    
    // Test getting specific pixels
    EXPECT_EQ(0, gpu->getTilePixel8bpp(tileAddr, 0, 0));    // Top-left
    EXPECT_EQ(7, gpu->getTilePixel8bpp(tileAddr, 7, 0));    // Top-right
    EXPECT_EQ(56, gpu->getTilePixel8bpp(tileAddr, 0, 7));   // Bottom-left
    EXPECT_EQ(63, gpu->getTilePixel8bpp(tileAddr, 7, 7));   // Bottom-right
    EXPECT_EQ(35, gpu->getTilePixel8bpp(tileAddr, 3, 4));   // Middle (4*8 + 3)
}

// Test 7: Bounds checking for getTilePixel4bpp
TEST_F(TileDecodingTest, BoundsChecking4bpp) {
    uint32_t tileAddr = 0x06000000;
    
    // Out of bounds should return 0
    EXPECT_EQ(0, gpu->getTilePixel4bpp(tileAddr, -1, 0));
    EXPECT_EQ(0, gpu->getTilePixel4bpp(tileAddr, 8, 0));
    EXPECT_EQ(0, gpu->getTilePixel4bpp(tileAddr, 0, -1));
    EXPECT_EQ(0, gpu->getTilePixel4bpp(tileAddr, 0, 8));
}

// Test 8: Bounds checking for getTilePixel8bpp
TEST_F(TileDecodingTest, BoundsChecking8bpp) {
    uint32_t tileAddr = 0x06000000;
    
    // Out of bounds should return 0
    EXPECT_EQ(0, gpu->getTilePixel8bpp(tileAddr, -1, 0));
    EXPECT_EQ(0, gpu->getTilePixel8bpp(tileAddr, 8, 0));
    EXPECT_EQ(0, gpu->getTilePixel8bpp(tileAddr, 0, -1));
    EXPECT_EQ(0, gpu->getTilePixel8bpp(tileAddr, 0, 8));
}

// Test 9: Multiple tiles at different addresses (4bpp)
TEST_F(TileDecodingTest, MultipleTiles4bpp) {
    // Tile 0 at 0x06000000 (32 bytes)
    uint32_t tile0Addr = 0x06000000;
    uint8_t tile0Data[32];
    for (int i = 0; i < 32; i++) tile0Data[i] = 0x11;
    writeTileData(tile0Addr, tile0Data, 32);
    
    // Tile 1 at 0x06000020 (next 32 bytes)
    uint32_t tile1Addr = 0x06000020;
    uint8_t tile1Data[32];
    for (int i = 0; i < 32; i++) tile1Data[i] = 0x22;
    writeTileData(tile1Addr, tile1Data, 32);
    
    // Decode both tiles
    uint8_t output0[64], output1[64];
    gpu->decodeTile4bpp(tile0Addr, output0);
    gpu->decodeTile4bpp(tile1Addr, output1);
    
    // Verify tile 0 has pattern 0x11
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(1, output0[i]) << "Tile 0, pixel " << i;
    }
    
    // Verify tile 1 has pattern 0x22
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(2, output1[i]) << "Tile 1, pixel " << i;
    }
}

// Test 10: Multiple tiles at different addresses (8bpp)
TEST_F(TileDecodingTest, MultipleTiles8bpp) {
    // Tile 0 at 0x06000000 (64 bytes)
    uint32_t tile0Addr = 0x06000000;
    uint8_t tile0Data[64];
    for (int i = 0; i < 64; i++) tile0Data[i] = 100;
    writeTileData(tile0Addr, tile0Data, 64);
    
    // Tile 1 at 0x06000040 (next 64 bytes)
    uint32_t tile1Addr = 0x06000040;
    uint8_t tile1Data[64];
    for (int i = 0; i < 64; i++) tile1Data[i] = 200;
    writeTileData(tile1Addr, tile1Data, 64);
    
    // Decode both tiles
    uint8_t output0[64], output1[64];
    gpu->decodeTile8bpp(tile0Addr, output0);
    gpu->decodeTile8bpp(tile1Addr, output1);
    
    // Verify tile 0 has value 100
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(100, output0[i]) << "Tile 0, pixel " << i;
    }
    
    // Verify tile 1 has value 200
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(200, output1[i]) << "Tile 1, pixel " << i;
    }
}

// Test 11: 4bpp tile row-by-row verification
TEST_F(TileDecodingTest, Tile4bppRowByRow) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a tile where each row is filled with the row number
    uint8_t tileData[32];
    for (int row = 0; row < 8; row++) {
        for (int byteInRow = 0; byteInRow < 4; byteInRow++) {  // 4 bytes per row
            tileData[row * 4 + byteInRow] = (row << 4) | row;
        }
    }
    
    writeTileData(tileAddr, tileData, 32);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile4bpp(tileAddr, output);
    
    // Verify each row
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            EXPECT_EQ(row, output[row * 8 + col]) 
                << "Row " << row << ", col " << col;
        }
    }
}

// Test 12: 8bpp tile row-by-row verification
TEST_F(TileDecodingTest, Tile8bppRowByRow) {
    uint32_t tileAddr = 0x06000000;
    
    // Create a tile where each row is filled with the row number
    uint8_t tileData[64];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            tileData[row * 8 + col] = row;
        }
    }
    
    writeTileData(tileAddr, tileData, 64);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile8bpp(tileAddr, output);
    
    // Verify each row
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            EXPECT_EQ(row, output[row * 8 + col])
                << "Row " << row << ", col " << col;
        }
    }
}

// Test 13: Tile data via DMA transfer (4bpp)
TEST_F(TileDecodingTest, TileDataViaDMA4bpp) {
    // Create tile data in EWRAM
    uint32_t ewramAddr = 0x02000000;
    uint8_t tileData[32];
    for (int i = 0; i < 32; i++) {
        tileData[i] = 0x34;  // Pattern: 4 (low), 3 (high)
    }
    
    for (int i = 0; i < 32; i++) {
        memory->write8(ewramAddr + i, tileData[i]);
    }
    
    // DMA3: Transfer to VRAM
    uint32_t vramAddr = 0x06000000;
    uint16_t control = 0x8400;  // Enable + 32-bit + immediate
    
    memory->write32(0x040000D4, ewramAddr);
    memory->write32(0x040000D8, vramAddr);
    memory->write16(0x040000DC, 8);  // 8 words = 32 bytes
    memory->write16(0x040000DE, control);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile4bpp(vramAddr, output);
    
    // Verify pattern
    for (int i = 0; i < 64; i++) {
        if (i % 2 == 0) {
            EXPECT_EQ(4, output[i]);
        } else {
            EXPECT_EQ(3, output[i]);
        }
    }
}

// Test 14: Tile data via DMA transfer (8bpp)
TEST_F(TileDecodingTest, TileDataViaDMA8bpp) {
    // Create tile data in EWRAM
    uint32_t ewramAddr = 0x02000000;
    uint8_t tileData[64];
    for (int i = 0; i < 64; i++) {
        tileData[i] = 77;
    }
    
    for (int i = 0; i < 64; i++) {
        memory->write8(ewramAddr + i, tileData[i]);
    }
    
    // DMA3: Transfer to VRAM
    uint32_t vramAddr = 0x06000000;
    uint16_t control = 0x8400;  // Enable + 32-bit + immediate
    
    memory->write32(0x040000D4, ewramAddr);
    memory->write32(0x040000D8, vramAddr);
    memory->write16(0x040000DC, 16);  // 16 words = 64 bytes
    memory->write16(0x040000DE, control);
    
    // Decode tile
    uint8_t output[64];
    gpu->decodeTile8bpp(vramAddr, output);
    
    // Verify all pixels are 77
    for (int i = 0; i < 64; i++) {
        EXPECT_EQ(77, output[i]);
    }
}

// Test 15: Null pointer safety for decode functions
TEST_F(TileDecodingTest, NullPointerSafety) {
    uint32_t tileAddr = 0x06000000;
    
    // These should not crash
    gpu->decodeTile4bpp(tileAddr, nullptr);
    gpu->decodeTile8bpp(tileAddr, nullptr);
    
    // If we get here, test passes
    SUCCEED();
}
