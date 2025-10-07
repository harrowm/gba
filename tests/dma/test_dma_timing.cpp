#include <gtest/gtest.h>
#include "memory.h"
#include "dma.h"
#include "scheduler.h"
#include "interrupt.h"
#include "gpu.h"
#include "gba.h"

// Test fixture for timing-based DMA tests
class DMATimingTest : public ::testing::Test {
protected:
    GBA* gba;

    void SetUp() override {
        gba = new GBA(true);  // Test mode
    }

    void TearDown() override {
        delete gba;
    }

    // Helper: Fill memory region with pattern
    void fillMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            gba->getMemory().write8(address + i, pattern + (i & 0xFF));
        }
    }

    // Helper: Verify memory region matches pattern
    bool verifyMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            uint8_t expected = pattern + (i & 0xFF);
            uint8_t actual = gba->getMemory().read8(address + i);
            if (actual != expected) {
                return false;
            }
        }
        return true;
    }

    // Helper: Setup DMA transfer (doesn't trigger until timing event)
    void setupDMA(int channel, uint32_t src, uint32_t dest, uint16_t count, 
                  uint16_t control) {
        gba->getMemory().write32(0x040000B0 + channel * 12, src);
        gba->getMemory().write32(0x040000B4 + channel * 12, dest);
        gba->getMemory().write16(0x040000B8 + channel * 12, count);
        gba->getMemory().write16(0x040000BA + channel * 12, control);
    }
};

// Test 1: V-Blank triggered DMA
TEST_F(DMATimingTest, VBlankTriggeredDMA) {
    uint32_t srcAddr = 0x00001000;  // Use test_ram range (0x00000000-0x00008000)
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0xAA);

    // Setup DMA1 with V-Blank timing (bits 12-13 = 01)
    uint16_t control = 0x9000;  // Enable + V-Blank timing
    setupDMA(1, srcAddr, destAddr, 32, control);

    // Verify transfer hasn't happened yet
    uint8_t testByte = gba->getMemory().read8(destAddr);
    EXPECT_NE(0xAA, testByte) << "DMA transferred before V-Blank";

    // Trigger V-Blank manually
    gba->getDMAController().triggerVBlank();

    // Verify transfer completed
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0xAA));
}

// Test 2: H-Blank triggered DMA
TEST_F(DMATimingTest, HBlankTriggeredDMA) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0xBB);

    // Setup DMA2 with H-Blank timing (bits 12-13 = 10)
    uint16_t control = 0xA000;  // Enable + H-Blank timing
    setupDMA(2, srcAddr, destAddr, 32, control);

    // Verify transfer hasn't happened yet
    uint8_t testByte = gba->getMemory().read8(destAddr);
    EXPECT_NE(0xBB, testByte) << "DMA transferred before H-Blank";

    // Trigger H-Blank manually
    gba->getDMAController().triggerHBlank();

    // Verify transfer completed
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0xBB));
}

// Test 3: Repeat mode - DMA repeats on next trigger
TEST_F(DMATimingTest, RepeatMode) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    
    // Fill source with first pattern
    fillMemory(srcAddr, 64, 0xCC);

    // Setup DMA1 with V-Blank timing and repeat mode (bit 9)
    uint16_t control = 0x9200;  // Enable + V-Blank + Repeat
    setupDMA(1, srcAddr, destAddr, 32, control);

    // First V-Blank - should transfer
    gba->getDMAController().triggerVBlank();
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0xCC));

    // Change source data
    fillMemory(srcAddr, 64, 0xDD);

    // Second V-Blank - should transfer again due to repeat
    gba->getDMAController().triggerVBlank();
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0xDD));
}

// Test 4: Priority handling - DMA0 has highest priority
TEST_F(DMATimingTest, PriorityHandling) {
    uint32_t srcAddr0 = 0x00001000;
    uint32_t destAddr = 0x00003000;  // Same destination!
    uint32_t srcAddr1 = 0x00001100;
    
    // Different patterns
    fillMemory(srcAddr0, 64, 0xAA);
    fillMemory(srcAddr1, 64, 0xBB);

    // Setup both DMA0 and DMA1 to same destination, V-Blank triggered
    uint16_t control = 0x9000;  // Enable + V-Blank
    setupDMA(0, srcAddr0, destAddr, 32, control);
    setupDMA(1, srcAddr1, destAddr, 32, control);

    // Trigger V-Blank
    gba->getDMAController().triggerVBlank();

    // DMA0 should run first, then DMA1 overwrites
    // So final result should be DMA1's pattern
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0xBB));
}

// Test 5: IRQ on completion with timing mode
TEST_F(DMATimingTest, IRQWithTimingMode) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0xEE);

    // Clear IF register
    gba->getMemory().write16(0x04000202, 0xFFFF);

    // Setup DMA1 with V-Blank timing and IRQ (bit 14)
    uint16_t control = 0xD000;  // Enable + IRQ + V-Blank
    setupDMA(1, srcAddr, destAddr, 32, control);

    // Trigger V-Blank
    gba->getDMAController().triggerVBlank();

    // Check IF register for DMA1 IRQ (bit 9)
    uint16_t ifReg = gba->getMemory().read16(0x04000202);
    EXPECT_TRUE(ifReg & 0x0200) << "DMA1 IRQ flag not set";
}

// Test 6: Destination reload mode (increment/reload)
TEST_F(DMATimingTest, DestinationReloadMode) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    
    // First transfer pattern
    fillMemory(srcAddr, 64, 0x11);

    // Setup DMA1 with V-Blank, repeat, and dest reload (bits 5-6 = 11)
    uint16_t control = 0x9260;  // Enable + V-Blank + Repeat + Dest Reload
    setupDMA(1, srcAddr, destAddr, 32, control);

    // First V-Blank
    gba->getDMAController().triggerVBlank();
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x11));

    // Change source
    fillMemory(srcAddr, 64, 0x22);

    // Second V-Blank - destination should reload to original address
    gba->getDMAController().triggerVBlank();
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x22));
}

// Test 7: Disable DMA mid-way (clear enable bit)
TEST_F(DMATimingTest, DisableDMAMidTransfer) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0x33);

    // Setup DMA1 with V-Blank timing
    uint16_t control = 0x9000;
    setupDMA(1, srcAddr, destAddr, 32, control);

    // Before trigger, disable it
    gba->getMemory().write16(0x040000BA + 1 * 12, 0x1000);  // Clear enable bit

    // Trigger V-Blank
    gba->getDMAController().triggerVBlank();

    // Should NOT have transferred
    uint8_t testByte = gba->getMemory().read8(destAddr);
    EXPECT_NE(0x33, testByte) << "DMA transferred when disabled";
}

// Test 8: All 4 channels with different priorities
TEST_F(DMATimingTest, AllChannelsPriority) {
    uint32_t srcAddr0 = 0x00001000;
    uint32_t srcAddr1 = 0x00001100;
    uint32_t srcAddr2 = 0x00001200;
    uint32_t srcAddr3 = 0x00001300;
    uint32_t destAddr0 = 0x00003000;
    uint32_t destAddr1 = 0x00003100;
    uint32_t destAddr2 = 0x00003200;
    uint32_t destAddr3 = 0x00003300;

    fillMemory(srcAddr0, 64, 0x00);
    fillMemory(srcAddr1, 64, 0x11);
    fillMemory(srcAddr2, 64, 0x22);
    fillMemory(srcAddr3, 64, 0x33);

    // Setup all 4 DMAs with V-Blank
    uint16_t control = 0x9000;
    setupDMA(0, srcAddr0, destAddr0, 32, control);
    setupDMA(1, srcAddr1, destAddr1, 32, control);
    setupDMA(2, srcAddr2, destAddr2, 32, control);
    setupDMA(3, srcAddr3, destAddr3, 32, control);

    // Trigger V-Blank
    gba->getDMAController().triggerVBlank();

    // All should complete (in priority order)
    EXPECT_TRUE(verifyMemory(destAddr0, 64, 0x00));
    EXPECT_TRUE(verifyMemory(destAddr1, 64, 0x11));
    EXPECT_TRUE(verifyMemory(destAddr2, 64, 0x22));
    EXPECT_TRUE(verifyMemory(destAddr3, 64, 0x33));
}

// Test 9: Verify cycle counting
TEST_F(DMATimingTest, CycleAccounting) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0x44);

    // Get initial cycle count (after setup, before DMA)
    uint64_t initialCycles = gba->getScheduler().getCurrentCycle();

    // Setup DMA0 immediate mode: 32 halfwords = 64 cycles (2 per transfer)
    uint16_t control = 0x8000;  // Immediate mode
    setupDMA(0, srcAddr, destAddr, 32, control);

    // Get final cycle count
    uint64_t finalCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = finalCycles - initialCycles;

    // Should be at least 64 cycles (32 transfers × 2 cycles each)
    // May be more due to wait cycles from I/O register writes
    EXPECT_GE(cyclesTaken, 64ULL) << "DMA should take at least 64 cycles";
    EXPECT_LE(cyclesTaken, 200ULL) << "DMA should not take more than 200 cycles";
}

// Test 10: Word count boundary (maximum transfer size)
TEST_F(DMATimingTest, MaximumTransferSize) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00004000;
    
    // Write test pattern
    for (int i = 0; i < 512; i++) {
        gba->getMemory().write8(srcAddr + i, i & 0xFF);
    }

    // DMA0 with word count = 0 means 16384 halfwords
    // But we'll test with smaller actual transfer
    // Just verify count=0 is handled specially
    uint16_t control = 0x8000;
    setupDMA(0, srcAddr, destAddr, 0, control);

    // Verify some data transferred (at least first 512 bytes of the 32KB transfer)
    for (int i = 0; i < 512; i++) {
        uint8_t expected = i & 0xFF;
        uint8_t actual = gba->getMemory().read8(destAddr + i);
        EXPECT_EQ(expected, actual) << "Mismatch at offset " << i;
    }
}
