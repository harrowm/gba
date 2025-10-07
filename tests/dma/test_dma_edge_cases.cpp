#include <gtest/gtest.h>
#include "memory.h"
#include "dma.h"
#include "scheduler.h"
#include "interrupt.h"
#include "gba.h"

// Test fixture for DMA edge cases
class DMAEdgeCaseTest : public ::testing::Test {
protected:
    Memory* memory;
    Scheduler* scheduler;
    DMAController* dmaController;
    InterruptController* interruptController;

    void SetUp() override {
        memory = new Memory(false);  // Normal mode for full memory map
        scheduler = new Scheduler();
        dmaController = new DMAController();
        interruptController = new InterruptController();

        memory->setScheduler(scheduler);
        memory->setDMAController(dmaController);
        dmaController->setMemory(memory);
        dmaController->setScheduler(scheduler);
        dmaController->setInterruptController(interruptController);
        interruptController->setMemory(memory);
    }

    void TearDown() override {
        delete interruptController;
        delete dmaController;
        delete scheduler;
        delete memory;
    }

    void fillMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            memory->write8(address + i, pattern + (i & 0xFF));
        }
    }

    bool verifyMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            uint8_t expected = pattern + (i & 0xFF);
            uint8_t actual = memory->read8(address + i);
            if (actual != expected) return false;
        }
        return true;
    }

    void setupImmediateDMA(int channel, uint32_t src, uint32_t dest, uint16_t count, 
                          uint16_t control) {
        memory->write32(0x040000B0 + channel * 12, src);
        memory->write32(0x040000B4 + channel * 12, dest);
        memory->write16(0x040000B8 + channel * 12, count);
        memory->write16(0x040000BA + channel * 12, control);
    }
};

// Test 1: Overlapping memory - forward direction (dest > src)
TEST_F(DMAEdgeCaseTest, OverlappingMemoryForward) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02000020;  // 32 bytes ahead
    
    // Fill source with pattern
    for (int i = 0; i < 128; i++) {
        memory->write8(srcAddr + i, 0x10 + i);
    }
    
    // DMA 64 bytes - will overlap by 32 bytes
    uint16_t control = 0x8000;  // Enable, 16-bit, increment
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);
    
    // Verify first 32 bytes of dest came from src[0-31]
    for (int i = 0; i < 32; i++) {
        uint8_t expected = 0x10 + i;
        uint8_t actual = memory->read8(destAddr + i);
        EXPECT_EQ(expected, actual) << "Mismatch at offset " << i;
    }
}

// Test 2: Overlapping memory - backward direction (dest < src)
TEST_F(DMAEdgeCaseTest, OverlappingMemoryBackward) {
    uint32_t srcAddr = 0x02000040;
    uint32_t destAddr = 0x02000020;  // 32 bytes before source
    
    // Fill source with pattern
    for (int i = 0; i < 64; i++) {
        memory->write8(srcAddr + i, 0x20 + i);
    }
    
    // DMA 32 halfwords (64 bytes)
    uint16_t control = 0x8000;
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);
    
    // Verify dest got data from original source
    for (int i = 0; i < 64; i++) {
        uint8_t expected = 0x20 + i;
        uint8_t actual = memory->read8(destAddr + i);
        EXPECT_EQ(expected, actual) << "Mismatch at offset " << i;
    }
}

// Test 3: Same address for source and dest with fixed source
TEST_F(DMAEdgeCaseTest, SameAddressFixedSourceIncrementDest) {
    uint32_t addr = 0x02000000;
    
    // Write a single value at source
    memory->write16(addr, 0xABCD);
    
    // DMA with source=dest, source fixed, dest increment
    // Should fill area with repeated value
    uint16_t control = 0x8100;  // Enable + Source fixed
    setupImmediateDMA(0, addr, addr, 32, control);
    
    // First halfword should be 0xABCD, rest should also be 0xABCD
    for (int i = 0; i < 64; i += 2) {
        uint16_t actual = memory->read16(addr + i);
        EXPECT_EQ(0xABCD, actual) << "Mismatch at offset " << i;
    }
}

// Test 4: Transfer crossing memory region boundary (EWRAM → IWRAM)
TEST_F(DMAEdgeCaseTest, CrossRegionBoundary) {
    uint32_t srcAddr = 0x02000000;  // EWRAM
    uint32_t destAddr = 0x03000000; // IWRAM
    
    // Fill EWRAM
    fillMemory(srcAddr, 256, 0x30);
    
    // Transfer 128 halfwords (256 bytes)
    uint16_t control = 0x8000;
    setupImmediateDMA(0, srcAddr, destAddr, 128, control);
    
    // Verify in IWRAM
    EXPECT_TRUE(verifyMemory(destAddr, 256, 0x30));
}

// Test 5: Transfer to VRAM
TEST_F(DMAEdgeCaseTest, TransferToVRAM) {
    uint32_t srcAddr = 0x02000000;  // EWRAM
    uint32_t destAddr = 0x06000000; // VRAM
    
    fillMemory(srcAddr, 128, 0x40);
    
    uint16_t control = 0x8000;
    setupImmediateDMA(0, srcAddr, destAddr, 64, control);
    
    EXPECT_TRUE(verifyMemory(destAddr, 128, 0x40));
}

// Test 6: Transfer to Palette RAM
TEST_F(DMAEdgeCaseTest, TransferToPaletteRAM) {
    uint32_t srcAddr = 0x02000000;    // EWRAM
    uint32_t destAddr = 0x05000000;   // Palette RAM
    
    fillMemory(srcAddr, 64, 0x50);
    
    uint16_t control = 0x8000;
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);
    
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x50));
}

// Test 7: Transfer to OAM
TEST_F(DMAEdgeCaseTest, TransferToOAM) {
    uint32_t srcAddr = 0x02000000;    // EWRAM
    uint32_t destAddr = 0x07000000;   // OAM
    
    fillMemory(srcAddr, 128, 0x60);
    
    uint16_t control = 0x8000;
    setupImmediateDMA(0, srcAddr, destAddr, 64, control);
    
    EXPECT_TRUE(verifyMemory(destAddr, 128, 0x60));
}

// Test 8: Prohibited source address control (INCREMENT_RELOAD)
TEST_F(DMAEdgeCaseTest, ProhibitedSourceControl) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    fillMemory(srcAddr, 64, 0x70);
    
    // Source control = 11 (prohibited, should behave like increment)
    uint16_t control = 0x8180;  // Enable + Source control = 11
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);
    
    // Should still transfer (treating as increment)
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x70));
}

// Test 9: Disabled channel doesn't transfer
TEST_F(DMAEdgeCaseTest, DisabledChannelNoTransfer) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    fillMemory(srcAddr, 64, 0x80);
    
    // Control without enable bit
    uint16_t control = 0x0000;  // No enable
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);
    
    // Dest should still be empty (0x00 or 0xFF depending on memory init)
    uint8_t destByte = memory->read8(destAddr);
    EXPECT_NE(0x80, destByte) << "DMA transferred when disabled";
}

// Test 10: All 4 channels triggered simultaneously (V-Blank)
TEST_F(DMAEdgeCaseTest, AllChannelsSimultaneousVBlank) {
    // Setup 4 different source/dest pairs
    uint32_t srcAddrs[4] = { 0x02000000, 0x02000100, 0x02000200, 0x02000300 };
    uint32_t destAddrs[4] = { 0x02010000, 0x02010100, 0x02010200, 0x02010300 };
    
    // Fill each source
    for (int ch = 0; ch < 4; ch++) {
        fillMemory(srcAddrs[ch], 64, 0x90 + ch * 0x10);
    }
    
    // Setup all 4 channels with V-Blank timing
    uint16_t control = 0x9000;  // Enable + V-Blank
    for (int ch = 0; ch < 4; ch++) {
        memory->write32(0x040000B0 + ch * 12, srcAddrs[ch]);
        memory->write32(0x040000B4 + ch * 12, destAddrs[ch]);
        memory->write16(0x040000B8 + ch * 12, 32);
        memory->write16(0x040000BA + ch * 12, control);
    }
    
    // Trigger V-Blank - should run all in priority order
    dmaController->triggerVBlank();
    
    // Verify all transferred
    EXPECT_TRUE(verifyMemory(destAddrs[0], 64, 0x90));
    EXPECT_TRUE(verifyMemory(destAddrs[1], 64, 0xA0));
    EXPECT_TRUE(verifyMemory(destAddrs[2], 64, 0xB0));
    EXPECT_TRUE(verifyMemory(destAddrs[3], 64, 0xC0));
}

// Test 11: Higher priority interrupts lower priority
TEST_F(DMAEdgeCaseTest, PriorityOrdering) {
    uint32_t srcAddr0 = 0x02000000;
    uint32_t srcAddr1 = 0x02000100;
    uint32_t destAddr = 0x02010000;  // Same destination for both
    
    fillMemory(srcAddr0, 64, 0xD0);
    fillMemory(srcAddr1, 64, 0xE0);
    
    // Setup both DMA0 and DMA1 to same dest
    uint16_t control = 0x9000;  // V-Blank
    memory->write32(0x040000B0, srcAddr0);
    memory->write32(0x040000B4, destAddr);
    memory->write16(0x040000B8, 32);
    memory->write16(0x040000BA, control);
    
    memory->write32(0x040000BC, srcAddr1);
    memory->write32(0x040000C0, destAddr);
    memory->write16(0x040000C4, 32);
    memory->write16(0x040000C6, control);
    
    dmaController->triggerVBlank();
    
    // DMA0 runs first, then DMA1 overwrites
    // Final result should be DMA1's data
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0xE0));
}

// Test 12: Word count of 0 interpretation (skip actual max transfer)
TEST_F(DMAEdgeCaseTest, DMA3MaxWordCount) {
    // Note: Word count of 0 means 65536 for DMA3, but we can't actually
    // transfer that much in test environment. This test validates that
    // non-zero word counts work correctly with a large value.
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    // Fill source
    fillMemory(srcAddr, 1024, 0xF0);
    
    // DMA3 with large word count (512 halfwords = 1KB)
    uint16_t control = 0x8000;
    memory->write32(0x040000D4, srcAddr);  // DMA3 source
    memory->write32(0x040000D8, destAddr); // DMA3 dest
    memory->write16(0x040000DC, 512);      // 512 halfwords
    memory->write16(0x040000DE, control);  // Control
    
    // Verify transfer
    EXPECT_TRUE(verifyMemory(destAddr, 1024, 0xF0));
}

// Test 13: Rapid enable/disable cycles
TEST_F(DMAEdgeCaseTest, RapidEnableDisable) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    fillMemory(srcAddr, 64, 0xA5);
    
    // Enable
    uint16_t control = 0x9000;  // V-Blank, enabled
    memory->write32(0x040000B0, srcAddr);
    memory->write32(0x040000B4, destAddr);
    memory->write16(0x040000B8, 32);
    memory->write16(0x040000BA, control);
    
    // Immediately disable
    memory->write16(0x040000BA, 0x1000);  // Clear enable bit
    
    // Trigger V-Blank
    dmaController->triggerVBlank();
    
    // Should NOT have transferred
    uint8_t testByte = memory->read8(destAddr);
    EXPECT_NE(0xA5, testByte) << "DMA transferred after being disabled";
}

// Test 14: 32-bit transfer with odd word count (should handle correctly)
TEST_F(DMAEdgeCaseTest, OddWordCount32Bit) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    // Fill source
    for (int i = 0; i < 100; i += 4) {
        memory->write32(srcAddr + i, 0x12345678 + i);
    }
    
    // 32-bit transfer with count=25 (100 bytes)
    uint16_t control = 0x8400;  // Enable + 32-bit
    setupImmediateDMA(0, srcAddr, destAddr, 25, control);
    
    // Verify data transferred
    for (int i = 0; i < 100; i += 4) {
        uint32_t expected = 0x12345678 + i;
        uint32_t actual = memory->read32(destAddr + i);
        EXPECT_EQ(expected, actual) << "Mismatch at offset " << i;
    }
}

// Test 15: Transfer with source and dest both fixed (unusual but valid)
TEST_F(DMAEdgeCaseTest, BothFixedMode) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    memory->write16(srcAddr, 0x1234);
    memory->write16(destAddr, 0x0000);
    
    // Both source and dest fixed - should write same value repeatedly to same location
    uint16_t control = 0x8140;  // Enable + Src fixed + Dest fixed
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);
    
    // Dest should have the source value (written 32 times to same location)
    uint16_t actual = memory->read16(destAddr);
    EXPECT_EQ(0x1234, actual);
}
