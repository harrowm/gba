#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

// Test fixture for Palette tests
class PaletteTest : public ::testing::Test {
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

    // Helper to write a color to BG palette
    void writeBGPalette(int paletteNum, int colorIndex, uint16_t rgb555) {
        uint32_t addr = 0x05000000 + (paletteNum * 16 + colorIndex) * 2;
        memory->write16(addr, rgb555);
    }

    // Helper to write a color to OBJ palette
    void writeOBJPalette(int paletteNum, int colorIndex, uint16_t rgb555) {
        uint32_t addr = 0x05000200 + (paletteNum * 16 + colorIndex) * 2;
        memory->write16(addr, rgb555);
    }
};

// Test 1: Read BG palette (raw RGB555)
TEST_F(PaletteTest, ReadBGPaletteRaw) {
    // Write some test colors
    writeBGPalette(0, 0, 0x7FFF);  // White: 11111 11111 11111
    writeBGPalette(0, 1, 0x001F);  // Red:   00000 00000 11111
    writeBGPalette(0, 2, 0x03E0);  // Green: 00000 11111 00000
    writeBGPalette(0, 3, 0x7C00);  // Blue:  11111 00000 00000

    EXPECT_EQ(0x7FFF, gpu->readBGPaletteRaw(0, 0));
    EXPECT_EQ(0x001F, gpu->readBGPaletteRaw(0, 1));
    EXPECT_EQ(0x03E0, gpu->readBGPaletteRaw(0, 2));
    EXPECT_EQ(0x7C00, gpu->readBGPaletteRaw(0, 3));
}

// Test 2: Read OBJ palette (raw RGB555)
TEST_F(PaletteTest, ReadOBJPaletteRaw) {
    // Write some test colors
    writeOBJPalette(0, 0, 0x7FFF);  // White
    writeOBJPalette(0, 1, 0x001F);  // Red
    writeOBJPalette(0, 2, 0x03E0);  // Green
    writeOBJPalette(0, 3, 0x7C00);  // Blue

    EXPECT_EQ(0x7FFF, gpu->readOBJPaletteRaw(0, 0));
    EXPECT_EQ(0x001F, gpu->readOBJPaletteRaw(0, 1));
    EXPECT_EQ(0x03E0, gpu->readOBJPaletteRaw(0, 2));
    EXPECT_EQ(0x7C00, gpu->readOBJPaletteRaw(0, 3));
}

// Test 3: Multiple BG palettes
TEST_F(PaletteTest, MultipleBGPalettes) {
    // Write colors to different palettes
    writeBGPalette(0, 5, 0x1234);
    writeBGPalette(5, 10, 0x5678);
    writeBGPalette(15, 15, 0xABCD);

    EXPECT_EQ(0x1234, gpu->readBGPaletteRaw(0, 5));
    EXPECT_EQ(0x5678, gpu->readBGPaletteRaw(5, 10));
    EXPECT_EQ(0xABCD, gpu->readBGPaletteRaw(15, 15));
}

// Test 4: Multiple OBJ palettes
TEST_F(PaletteTest, MultipleOBJPalettes) {
    // Write colors to different palettes
    writeOBJPalette(0, 5, 0x1234);
    writeOBJPalette(5, 10, 0x5678);
    writeOBJPalette(15, 15, 0xABCD);

    EXPECT_EQ(0x1234, gpu->readOBJPaletteRaw(0, 5));
    EXPECT_EQ(0x5678, gpu->readOBJPaletteRaw(5, 10));
    EXPECT_EQ(0xABCD, gpu->readOBJPaletteRaw(15, 15));
}

// Test 5: RGB555 to ARGB8888 conversion - White
TEST_F(PaletteTest, RGB555ConversionWhite) {
    // White: RGB555 = 0x7FFF (11111 11111 11111)
    // Expected ARGB8888: 0xFFFFFFFF (255, 255, 255)
    uint32_t result = gpu->convertRGB555toARGB8888(0x7FFF);
    EXPECT_EQ(0xFFFFFFFF, result);
}

// Test 6: RGB555 to ARGB8888 conversion - Red
TEST_F(PaletteTest, RGB555ConversionRed) {
    // Red: RGB555 = 0x001F (00000 00000 11111)
    // R5=31 → R8=248 (31<<3 | 31>>2 = 248+7 = 255... actually 248|7=255)
    uint32_t result = gpu->convertRGB555toARGB8888(0x001F);
    
    // Extract components
    uint8_t a = (result >> 24) & 0xFF;
    uint8_t r = (result >> 16) & 0xFF;
    uint8_t g = (result >> 8) & 0xFF;
    uint8_t b = result & 0xFF;
    
    EXPECT_EQ(0xFF, a);  // Alpha should be 255
    EXPECT_GE(r, 248);   // Red should be close to 255
    EXPECT_EQ(0, g);     // Green should be 0
    EXPECT_EQ(0, b);     // Blue should be 0
}

// Test 7: RGB555 to ARGB8888 conversion - Green
TEST_F(PaletteTest, RGB555ConversionGreen) {
    // Green: RGB555 = 0x03E0 (00000 11111 00000)
    uint32_t result = gpu->convertRGB555toARGB8888(0x03E0);
    
    uint8_t a = (result >> 24) & 0xFF;
    uint8_t r = (result >> 16) & 0xFF;
    uint8_t g = (result >> 8) & 0xFF;
    uint8_t b = result & 0xFF;
    
    EXPECT_EQ(0xFF, a);
    EXPECT_EQ(0, r);
    EXPECT_GE(g, 248);   // Green should be close to 255
    EXPECT_EQ(0, b);
}

// Test 8: RGB555 to ARGB8888 conversion - Blue
TEST_F(PaletteTest, RGB555ConversionBlue) {
    // Blue: RGB555 = 0x7C00 (11111 00000 00000)
    uint32_t result = gpu->convertRGB555toARGB8888(0x7C00);
    
    uint8_t a = (result >> 24) & 0xFF;
    uint8_t r = (result >> 16) & 0xFF;
    uint8_t g = (result >> 8) & 0xFF;
    uint8_t b = result & 0xFF;
    
    EXPECT_EQ(0xFF, a);
    EXPECT_EQ(0, r);
    EXPECT_EQ(0, g);
    EXPECT_GE(b, 248);   // Blue should be close to 255
}

// Test 9: RGB555 to ARGB8888 conversion - Black
TEST_F(PaletteTest, RGB555ConversionBlack) {
    // Black: RGB555 = 0x0000
    uint32_t result = gpu->convertRGB555toARGB8888(0x0000);
    EXPECT_EQ(0xFF000000, result);  // Alpha=255, RGB=0
}

// Test 10: RGB555 to ARGB8888 conversion - Gray
TEST_F(PaletteTest, RGB555ConversionGray) {
    // Medium gray: R=G=B=16 (in 5-bit)
    // RGB555 = 16 | (16<<5) | (16<<10) = 16 | 512 | 16384 = 0x4210
    // 16 in 5-bit → 132 in 8-bit (16<<3 | 16>>2 = 128 + 4 = 132)
    uint32_t result = gpu->convertRGB555toARGB8888(0x4210);
    
    uint8_t a = (result >> 24) & 0xFF;
    uint8_t r = (result >> 16) & 0xFF;
    uint8_t g = (result >> 8) & 0xFF;
    uint8_t b = result & 0xFF;
    
    EXPECT_EQ(0xFF, a);
    EXPECT_NEAR(132, r, 2);  // 16 in 5-bit → 132 in 8-bit
    EXPECT_NEAR(132, g, 2);
    EXPECT_NEAR(132, b, 2);
}

// Test 11: getBGColor (full pipeline)
TEST_F(PaletteTest, GetBGColorFullPipeline) {
    // Write white to palette 0, color 1
    writeBGPalette(0, 1, 0x7FFF);
    
    uint32_t color = gpu->getBGColor(0, 1);
    EXPECT_EQ(0xFFFFFFFF, color);
}

// Test 12: getOBJColor (full pipeline)
TEST_F(PaletteTest, GetOBJColorFullPipeline) {
    // Write red to palette 0, color 2
    writeOBJPalette(0, 2, 0x001F);
    
    uint32_t color = gpu->getOBJColor(0, 2);
    
    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    EXPECT_EQ(0xFF, a);
    EXPECT_GE(r, 248);
    EXPECT_EQ(0, g);
    EXPECT_EQ(0, b);
}

// Test 13: Bounds checking (out of range palette number)
TEST_F(PaletteTest, BoundsCheckingInvalidPalette) {
    uint16_t result = gpu->readBGPaletteRaw(16, 0);  // Palette 16 is out of range
    EXPECT_EQ(0, result);
    
    result = gpu->readBGPaletteRaw(-1, 0);  // Negative palette
    EXPECT_EQ(0, result);
}

// Test 14: Bounds checking (out of range color index)
TEST_F(PaletteTest, BoundsCheckingInvalidColor) {
    uint16_t result = gpu->readBGPaletteRaw(0, 16);  // Color 16 is out of range
    EXPECT_EQ(0, result);
    
    result = gpu->readBGPaletteRaw(0, -1);  // Negative color
    EXPECT_EQ(0, result);
}

// Test 15: BG and OBJ palettes are separate
TEST_F(PaletteTest, BGAndOBJPalettesAreSeparate) {
    // Write different colors to same palette/color index
    writeBGPalette(5, 10, 0x1111);
    writeOBJPalette(5, 10, 0x2222);
    
    EXPECT_EQ(0x1111, gpu->readBGPaletteRaw(5, 10));
    EXPECT_EQ(0x2222, gpu->readOBJPaletteRaw(5, 10));
}

// Test 16: All 256 BG palette entries accessible
TEST_F(PaletteTest, All256BGPaletteEntries) {
    // Write pattern to all 256 entries (16 palettes × 16 colors)
    for (int pal = 0; pal < 16; pal++) {
        for (int col = 0; col < 16; col++) {
            uint16_t value = (pal << 8) | col;
            writeBGPalette(pal, col, value);
        }
    }
    
    // Verify all entries
    for (int pal = 0; pal < 16; pal++) {
        for (int col = 0; col < 16; col++) {
            uint16_t expected = (pal << 8) | col;
            EXPECT_EQ(expected, gpu->readBGPaletteRaw(pal, col))
                << "Failed at palette " << pal << ", color " << col;
        }
    }
}

// Test 17: All 256 OBJ palette entries accessible
TEST_F(PaletteTest, All256OBJPaletteEntries) {
    // Write pattern to all 256 entries
    for (int pal = 0; pal < 16; pal++) {
        for (int col = 0; col < 16; col++) {
            uint16_t value = (pal << 8) | col | 0x8000;  // Different pattern
            writeOBJPalette(pal, col, value);
        }
    }
    
    // Verify all entries
    for (int pal = 0; pal < 16; pal++) {
        for (int col = 0; col < 16; col++) {
            uint16_t expected = (pal << 8) | col | 0x8000;
            EXPECT_EQ(expected, gpu->readOBJPaletteRaw(pal, col))
                << "Failed at palette " << pal << ", color " << col;
        }
    }
}

// Test 18: Palette via DMA transfer
TEST_F(PaletteTest, PaletteViaDMATransfer) {
    // Create color data in EWRAM
    uint32_t ewramAddr = 0x02000000;
    
    // Write some test colors (4 colors = 8 bytes)
    memory->write16(ewramAddr + 0, 0x7FFF);  // White
    memory->write16(ewramAddr + 2, 0x001F);  // Red
    memory->write16(ewramAddr + 4, 0x03E0);  // Green
    memory->write16(ewramAddr + 6, 0x7C00);  // Blue
    
    // DMA3: Transfer to BG palette
    uint32_t paletteAddr = 0x05000000;
    uint16_t control = 0x8400;  // Enable + 32-bit + immediate
    
    memory->write32(0x040000D4, ewramAddr);   // DMA3 source
    memory->write32(0x040000D8, paletteAddr); // DMA3 dest
    memory->write16(0x040000DC, 2);           // 2 words = 8 bytes = 4 colors
    memory->write16(0x040000DE, control);     // Start transfer
    
    // Verify colors were transferred
    EXPECT_EQ(0x7FFF, gpu->readBGPaletteRaw(0, 0));
    EXPECT_EQ(0x001F, gpu->readBGPaletteRaw(0, 1));
    EXPECT_EQ(0x03E0, gpu->readBGPaletteRaw(0, 2));
    EXPECT_EQ(0x7C00, gpu->readBGPaletteRaw(0, 3));
}
