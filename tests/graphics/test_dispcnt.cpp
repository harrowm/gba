#include <gtest/gtest.h>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

// Test fixture for DISPCNT register tests
class DISPCNTTest : public ::testing::Test {
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

    void writeDISPCNT(uint16_t value) {
        memory->write16(0x04000000, value);
    }
};

// Test 1: Parse video mode (bits 0-2)
TEST_F(DISPCNTTest, ParseVideoMode) {
    // Test all video modes 0-5
    for (int mode = 0; mode <= 5; mode++) {
        DisplayControl dc = gpu->parseDISPCNT(mode);
        EXPECT_EQ(mode, dc.videoMode) << "Video mode " << mode << " not parsed correctly";
    }
    
    // Mode 6-7 are invalid but should still parse
    DisplayControl dc6 = gpu->parseDISPCNT(6);
    EXPECT_EQ(6, dc6.videoMode);
    
    DisplayControl dc7 = gpu->parseDISPCNT(7);
    EXPECT_EQ(7, dc7.videoMode);
}

// Test 2: Parse BG enable flags (bits 8-11)
TEST_F(DISPCNTTest, ParseBGEnableFlags) {
    // Test each BG individually
    DisplayControl dc0 = gpu->parseDISPCNT(0x0100);  // BG0 enabled
    EXPECT_TRUE(dc0.bg0Enable);
    EXPECT_FALSE(dc0.bg1Enable);
    EXPECT_FALSE(dc0.bg2Enable);
    EXPECT_FALSE(dc0.bg3Enable);
    
    DisplayControl dc1 = gpu->parseDISPCNT(0x0200);  // BG1 enabled
    EXPECT_FALSE(dc1.bg0Enable);
    EXPECT_TRUE(dc1.bg1Enable);
    EXPECT_FALSE(dc1.bg2Enable);
    EXPECT_FALSE(dc1.bg3Enable);
    
    DisplayControl dc2 = gpu->parseDISPCNT(0x0400);  // BG2 enabled
    EXPECT_FALSE(dc2.bg0Enable);
    EXPECT_FALSE(dc2.bg1Enable);
    EXPECT_TRUE(dc2.bg2Enable);
    EXPECT_FALSE(dc2.bg3Enable);
    
    DisplayControl dc3 = gpu->parseDISPCNT(0x0800);  // BG3 enabled
    EXPECT_FALSE(dc3.bg0Enable);
    EXPECT_FALSE(dc3.bg1Enable);
    EXPECT_FALSE(dc3.bg2Enable);
    EXPECT_TRUE(dc3.bg3Enable);
    
    // Test all BGs enabled
    DisplayControl dcAll = gpu->parseDISPCNT(0x0F00);
    EXPECT_TRUE(dcAll.bg0Enable);
    EXPECT_TRUE(dcAll.bg1Enable);
    EXPECT_TRUE(dcAll.bg2Enable);
    EXPECT_TRUE(dcAll.bg3Enable);
}

// Test 3: Parse OBJ enable (bit 12)
TEST_F(DISPCNTTest, ParseOBJEnable) {
    DisplayControl dcOff = gpu->parseDISPCNT(0x0000);
    EXPECT_FALSE(dcOff.objEnable);
    
    DisplayControl dcOn = gpu->parseDISPCNT(0x1000);
    EXPECT_TRUE(dcOn.objEnable);
}

// Test 4: Parse forced blank (bit 7)
TEST_F(DISPCNTTest, ParseForcedBlank) {
    DisplayControl dcOff = gpu->parseDISPCNT(0x0000);
    EXPECT_FALSE(dcOff.forcedBlank);
    
    DisplayControl dcOn = gpu->parseDISPCNT(0x0080);
    EXPECT_TRUE(dcOn.forcedBlank);
}

// Test 5: Parse frame select (bit 4)
TEST_F(DISPCNTTest, ParseFrameSelect) {
    DisplayControl dcFrame0 = gpu->parseDISPCNT(0x0000);
    EXPECT_FALSE(dcFrame0.frameSelect);
    
    DisplayControl dcFrame1 = gpu->parseDISPCNT(0x0010);
    EXPECT_TRUE(dcFrame1.frameSelect);
}

// Test 6: Parse OBJ 1D mapping (bit 6)
TEST_F(DISPCNTTest, ParseOBJ1DMapping) {
    DisplayControl dc2D = gpu->parseDISPCNT(0x0000);
    EXPECT_FALSE(dc2D.obj1DMapping);
    
    DisplayControl dc1D = gpu->parseDISPCNT(0x0040);
    EXPECT_TRUE(dc1D.obj1DMapping);
}

// Test 7: Parse OAM HBlank access (bit 5)
TEST_F(DISPCNTTest, ParseOAMHBlankAccess) {
    DisplayControl dcOff = gpu->parseDISPCNT(0x0000);
    EXPECT_FALSE(dcOff.oamHBlankAccess);
    
    DisplayControl dcOn = gpu->parseDISPCNT(0x0020);
    EXPECT_TRUE(dcOn.oamHBlankAccess);
}

// Test 8: Parse window enable flags (bits 13-15)
TEST_F(DISPCNTTest, ParseWindowEnableFlags) {
    DisplayControl dcWin0 = gpu->parseDISPCNT(0x2000);
    EXPECT_TRUE(dcWin0.win0Enable);
    EXPECT_FALSE(dcWin0.win1Enable);
    EXPECT_FALSE(dcWin0.winObjEnable);
    
    DisplayControl dcWin1 = gpu->parseDISPCNT(0x4000);
    EXPECT_FALSE(dcWin1.win0Enable);
    EXPECT_TRUE(dcWin1.win1Enable);
    EXPECT_FALSE(dcWin1.winObjEnable);
    
    DisplayControl dcWinObj = gpu->parseDISPCNT(0x8000);
    EXPECT_FALSE(dcWinObj.win0Enable);
    EXPECT_FALSE(dcWinObj.win1Enable);
    EXPECT_TRUE(dcWinObj.winObjEnable);
    
    // All windows enabled
    DisplayControl dcAllWin = gpu->parseDISPCNT(0xE000);
    EXPECT_TRUE(dcAllWin.win0Enable);
    EXPECT_TRUE(dcAllWin.win1Enable);
    EXPECT_TRUE(dcAllWin.winObjEnable);
}

// Test 9: Complex configuration (multiple bits set)
TEST_F(DISPCNTTest, ParseComplexConfiguration) {
    // Mode 0, BG0+BG1+OBJ enabled, no forced blank
    uint16_t config = 0x1300;  // OBJ(bit12) + BG1(bit9) + BG0(bit8) + Mode0
    DisplayControl dc = gpu->parseDISPCNT(config);
    
    EXPECT_EQ(0, dc.videoMode);
    EXPECT_TRUE(dc.bg0Enable);
    EXPECT_TRUE(dc.bg1Enable);
    EXPECT_FALSE(dc.bg2Enable);
    EXPECT_FALSE(dc.bg3Enable);
    EXPECT_TRUE(dc.objEnable);
    EXPECT_FALSE(dc.forcedBlank);
}

// Test 10: Read DISPCNT from memory
TEST_F(DISPCNTTest, ReadDISPCNTFromMemory) {
    // Write a value to DISPCNT register
    writeDISPCNT(0x1703);  // Mode 3, BG0+BG1+BG2+OBJ enabled
    
    DisplayControl dc = gpu->readDISPCNT();
    
    EXPECT_EQ(3, dc.videoMode);
    EXPECT_TRUE(dc.bg0Enable);
    EXPECT_TRUE(dc.bg1Enable);
    EXPECT_TRUE(dc.bg2Enable);
    EXPECT_FALSE(dc.bg3Enable);
    EXPECT_TRUE(dc.objEnable);
}

// Test 11: Helper function - isBGEnabled
TEST_F(DISPCNTTest, HelperIsBGEnabled) {
    // Enable BG0 and BG2
    writeDISPCNT(0x0500);  // BG2(bit10) + BG0(bit8)
    
    EXPECT_TRUE(gpu->isBGEnabled(0));
    EXPECT_FALSE(gpu->isBGEnabled(1));
    EXPECT_TRUE(gpu->isBGEnabled(2));
    EXPECT_FALSE(gpu->isBGEnabled(3));
    
    // Test invalid BG numbers
    EXPECT_FALSE(gpu->isBGEnabled(-1));
    EXPECT_FALSE(gpu->isBGEnabled(4));
}

// Test 12: Helper function - isOBJEnabled
TEST_F(DISPCNTTest, HelperIsOBJEnabled) {
    writeDISPCNT(0x0000);
    EXPECT_FALSE(gpu->isOBJEnabled());
    
    writeDISPCNT(0x1000);  // OBJ enabled
    EXPECT_TRUE(gpu->isOBJEnabled());
}

// Test 13: Helper function - isForcedBlank
TEST_F(DISPCNTTest, HelperIsForcedBlank) {
    writeDISPCNT(0x0000);
    EXPECT_FALSE(gpu->isForcedBlank());
    
    writeDISPCNT(0x0080);  // Forced blank
    EXPECT_TRUE(gpu->isForcedBlank());
}

// Test 14: Helper function - getVideoMode
TEST_F(DISPCNTTest, HelperGetVideoMode) {
    for (int mode = 0; mode <= 5; mode++) {
        writeDISPCNT(mode);
        EXPECT_EQ(mode, gpu->getVideoMode());
    }
}

// Test 15: All bits set
TEST_F(DISPCNTTest, AllBitsSet) {
    DisplayControl dc = gpu->parseDISPCNT(0xFFFF);
    
    EXPECT_EQ(7, dc.videoMode);  // Mode 7 (invalid but parsed)
    EXPECT_TRUE(dc.frameSelect);
    EXPECT_TRUE(dc.oamHBlankAccess);
    EXPECT_TRUE(dc.obj1DMapping);
    EXPECT_TRUE(dc.forcedBlank);
    EXPECT_TRUE(dc.bg0Enable);
    EXPECT_TRUE(dc.bg1Enable);
    EXPECT_TRUE(dc.bg2Enable);
    EXPECT_TRUE(dc.bg3Enable);
    EXPECT_TRUE(dc.objEnable);
    EXPECT_TRUE(dc.win0Enable);
    EXPECT_TRUE(dc.win1Enable);
    EXPECT_TRUE(dc.winObjEnable);
}

// Test 16: Mode 3 typical configuration (bitmap mode)
TEST_F(DISPCNTTest, Mode3TypicalConfig) {
    // Mode 3, BG2 enabled (required for bitmap modes)
    uint16_t mode3Config = 0x0403;  // BG2(bit10) + Mode3
    DisplayControl dc = gpu->parseDISPCNT(mode3Config);
    
    EXPECT_EQ(3, dc.videoMode);
    EXPECT_FALSE(dc.bg0Enable);
    EXPECT_FALSE(dc.bg1Enable);
    EXPECT_TRUE(dc.bg2Enable);
    EXPECT_FALSE(dc.bg3Enable);
    EXPECT_FALSE(dc.objEnable);
}

// Test 17: Mode 0 typical configuration (tiled mode)
TEST_F(DISPCNTTest, Mode0TypicalConfig) {
    // Mode 0, all BGs + OBJ enabled
    uint16_t mode0Config = 0x1F00;  // OBJ + BG3 + BG2 + BG1 + BG0 + Mode0
    DisplayControl dc = gpu->parseDISPCNT(mode0Config);
    
    EXPECT_EQ(0, dc.videoMode);
    EXPECT_TRUE(dc.bg0Enable);
    EXPECT_TRUE(dc.bg1Enable);
    EXPECT_TRUE(dc.bg2Enable);
    EXPECT_TRUE(dc.bg3Enable);
    EXPECT_TRUE(dc.objEnable);
}

// Test 18: Zero value - all features disabled
TEST_F(DISPCNTTest, AllFeaturesDisabled) {
    DisplayControl dc = gpu->parseDISPCNT(0x0000);
    
    EXPECT_EQ(0, dc.videoMode);
    EXPECT_FALSE(dc.frameSelect);
    EXPECT_FALSE(dc.oamHBlankAccess);
    EXPECT_FALSE(dc.obj1DMapping);
    EXPECT_FALSE(dc.forcedBlank);
    EXPECT_FALSE(dc.bg0Enable);
    EXPECT_FALSE(dc.bg1Enable);
    EXPECT_FALSE(dc.bg2Enable);
    EXPECT_FALSE(dc.bg3Enable);
    EXPECT_FALSE(dc.objEnable);
    EXPECT_FALSE(dc.win0Enable);
    EXPECT_FALSE(dc.win1Enable);
    EXPECT_FALSE(dc.winObjEnable);
}
