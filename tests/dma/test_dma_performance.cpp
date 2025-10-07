#include <gtest/gtest.h>
#include "gba.h"
#include "memory.h"
#include "dma.h"
#include "scheduler.h"
#include "interrupt.h"

// Test fixture for DMA performance and timing tests
class DMAPerformanceTest : public ::testing::Test {
protected:
    GBA* gba;

    void SetUp() override {
        gba = new GBA(true);  // Test mode
    }

    void TearDown() override {
        delete gba;
    }

    void fillMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            gba->getMemory().write8(address + i, pattern + (i & 0xFF));
        }
    }

    bool verifyMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            uint8_t expected = pattern + (i & 0xFF);
            uint8_t actual = gba->getMemory().read8(address + i);
            if (actual != expected) return false;
        }
        return true;
    }

    void setupDMA(int channel, uint32_t src, uint32_t dest, uint16_t count, 
                  uint16_t control) {
        gba->getMemory().write32(0x040000B0 + channel * 12, src);
        gba->getMemory().write32(0x040000B4 + channel * 12, dest);
        gba->getMemory().write16(0x040000B8 + channel * 12, count);
        gba->getMemory().write16(0x040000BA + channel * 12, control);
    }
};

// Test 1: 16-bit transfer timing (2 cycles per transfer)
TEST_F(DMAPerformanceTest, Transfer16BitTiming) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 128, 0x10);

    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Transfer 64 halfwords (128 bytes) = 128 cycles minimum
    uint16_t control = 0x8000;  // 16-bit, immediate
    setupDMA(0, srcAddr, destAddr, 64, control);
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // Should be at least 128 cycles (64 transfers × 2 cycles)
    EXPECT_GE(cyclesTaken, 128ULL) << "16-bit DMA should take at least 2 cycles per transfer";
    EXPECT_LE(cyclesTaken, 300ULL) << "Cycle count seems too high";
}

// Test 2: 32-bit transfer timing (2 cycles per transfer)
TEST_F(DMAPerformanceTest, Transfer32BitTiming) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 256, 0x20);

    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Transfer 64 words (256 bytes) = 128 cycles minimum
    uint16_t control = 0x8400;  // 32-bit, immediate
    setupDMA(0, srcAddr, destAddr, 64, control);
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // Should be at least 128 cycles (64 transfers × 2 cycles)
    EXPECT_GE(cyclesTaken, 128ULL) << "32-bit DMA should take at least 2 cycles per transfer";
    EXPECT_LE(cyclesTaken, 300ULL) << "Cycle count seems too high";
}

// Test 3: Large transfer timing (1000+ words)
TEST_F(DMAPerformanceTest, LargeTransferTiming) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00003000;
    
    // Fill 2048 bytes
    for (int i = 0; i < 2048; i++) {
        gba->getMemory().write8(srcAddr + i, i & 0xFF);
    }

    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Transfer 1024 halfwords (2048 bytes)
    uint16_t control = 0x8000;
    setupDMA(0, srcAddr, destAddr, 1024, control);
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // Should be at least 2048 cycles (1024 transfers × 2)
    EXPECT_GE(cyclesTaken, 2048ULL) << "Large DMA timing incorrect";
    
    // Verify data
    for (int i = 0; i < 2048; i++) {
        uint8_t expected = i & 0xFF;
        uint8_t actual = gba->getMemory().read8(destAddr + i);
        EXPECT_EQ(expected, actual) << "Data mismatch at offset " << i;
    }
}

// Test 4: Multiple small transfers vs one large transfer
TEST_F(DMAPerformanceTest, SmallVsLargeTransfers) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr1 = 0x00003000;
    uint32_t destAddr2 = 0x00004000;
    
    fillMemory(srcAddr, 512, 0x30);

    // Measure 16 small transfers of 16 halfwords each
    uint64_t start1 = gba->getScheduler().getCurrentCycle();
    for (int i = 0; i < 16; i++) {
        uint16_t control = 0x8000;
        setupDMA(0, srcAddr + i * 32, destAddr1 + i * 32, 16, control);
    }
    uint64_t end1 = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesSmall = end1 - start1;

    // Measure 1 large transfer of 256 halfwords
    uint64_t start2 = gba->getScheduler().getCurrentCycle();
    uint16_t control = 0x8000;
    setupDMA(0, srcAddr, destAddr2, 256, control);
    uint64_t end2 = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesLarge = end2 - start2;

    // Multiple small transfers should take more cycles due to setup overhead
    EXPECT_GT(cyclesSmall, cyclesLarge) << "Small transfers should be less efficient";
    
    // Both should have transferred the same data
    EXPECT_TRUE(verifyMemory(destAddr1, 512, 0x30));
    EXPECT_TRUE(verifyMemory(destAddr2, 512, 0x30));
}

// Test 5: V-Blank timing constraints (ensure DMA completes within V-Blank)
TEST_F(DMAPerformanceTest, VBlankTimingConstraints) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 512, 0x40);

    // Setup V-Blank DMA
    uint16_t control = 0x9000;  // V-Blank timing
    setupDMA(1, srcAddr, destAddr, 256, control);
    
    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Trigger V-Blank
    gba->getDMAController().triggerVBlank();
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // 256 halfword transfer = 512 cycles
    EXPECT_GE(cyclesTaken, 512ULL);
    
    // V-Blank period is 68 scanlines × 1232 cycles = 83,776 cycles
    // Our DMA should be well within that
    EXPECT_LE(cyclesTaken, 10000ULL) << "DMA too slow for V-Blank";
    
    EXPECT_TRUE(verifyMemory(destAddr, 512, 0x40));
}

// Test 6: H-Blank timing constraints (ensure DMA completes within H-Blank)
TEST_F(DMAPerformanceTest, HBlankTimingConstraints) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0x50);

    // Setup H-Blank DMA (smaller transfer to fit in H-Blank)
    uint16_t control = 0xA000;  // H-Blank timing
    setupDMA(2, srcAddr, destAddr, 32, control);
    
    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Trigger H-Blank
    gba->getDMAController().triggerHBlank();
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // 32 halfword transfer = 64 cycles
    EXPECT_GE(cyclesTaken, 64ULL);
    
    // H-Blank period is 272 cycles
    EXPECT_LE(cyclesTaken, 300ULL) << "DMA too slow for H-Blank";
    
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x50));
}

// Test 7: Maximum concurrent DMAs (all 4 channels)
TEST_F(DMAPerformanceTest, MaxConcurrentDMAs) {
    uint32_t srcAddrs[4] = { 0x00001000, 0x00001100, 0x00001200, 0x00001300 };
    uint32_t destAddrs[4] = { 0x00003000, 0x00003100, 0x00003200, 0x00003300 };
    
    // Fill sources
    for (int i = 0; i < 4; i++) {
        fillMemory(srcAddrs[i], 64, 0x60 + i * 0x10);
    }
    
    // Setup all 4 with V-Blank timing
    for (int i = 0; i < 4; i++) {
        uint16_t control = 0x9000;
        setupDMA(i, srcAddrs[i], destAddrs[i], 32, control);
    }
    
    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Trigger V-Blank - all 4 should run in sequence
    gba->getDMAController().triggerVBlank();
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // 4 channels × 32 halfwords × 2 cycles = 256 cycles minimum
    EXPECT_GE(cyclesTaken, 256ULL);
    
    // Verify all transferred
    for (int i = 0; i < 4; i++) {
        EXPECT_TRUE(verifyMemory(destAddrs[i], 64, 0x60 + i * 0x10));
    }
}

// Test 8: Rapid enable/disable cycles (stress test)
TEST_F(DMAPerformanceTest, RapidEnableDisableCycles) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0x70);

    // Rapidly enable and disable 100 times
    for (int i = 0; i < 100; i++) {
        // Enable
        uint16_t control = 0x8000;
        setupDMA(0, srcAddr, destAddr, 32, control);
        
        // Disable
        gba->getMemory().write16(0x040000BA, 0x0000);
    }
    
    // Final enable and verify
    uint16_t control = 0x8000;
    setupDMA(0, srcAddr, destAddr, 32, control);
    
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x70));
}

// Test 9: Large transfer for DMA3 (test with realistic size)
TEST_F(DMAPerformanceTest, LargeTransferSizeDMA3) {
    // Test a large (but reasonable) transfer for DMA3
    // Note: Word count = 0 would mean 65536, but we test large practical transfers instead
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00004000;
    
    // Fill 4KB
    fillMemory(srcAddr, 4096, 0x80);

    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // DMA3: Transfer 2048 halfwords (4KB)
    uint16_t control = 0x8000;
    gba->getMemory().write32(0x040000D4, srcAddr);  // DMA3 source
    gba->getMemory().write32(0x040000D8, destAddr); // DMA3 dest
    gba->getMemory().write16(0x040000DC, 2048);     // 2048 halfwords = 4KB
    gba->getMemory().write16(0x040000DE, control);
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // 2048 halfwords × 2 cycles = 4096 cycles minimum
    EXPECT_GE(cyclesTaken, 4096ULL) << "Large transfer should take at least 2 cycles per transfer";
    EXPECT_LE(cyclesTaken, 10000ULL) << "Cycle count seems too high";
    
    // Verify data transferred
    EXPECT_TRUE(verifyMemory(destAddr, 4096, 0x80));
}

// Test 10: Repeat mode performance (multiple triggers)
TEST_F(DMAPerformanceTest, RepeatModePerformance) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    
    fillMemory(srcAddr, 64, 0x90);

    // Setup repeat mode
    uint16_t control = 0x9200;  // V-Blank + Repeat
    setupDMA(1, srcAddr, destAddr, 32, control);
    
    uint64_t totalCycles = 0;
    
    // Trigger 10 times and measure
    for (int i = 0; i < 10; i++) {
        uint64_t start = gba->getScheduler().getCurrentCycle();
        gba->getDMAController().triggerVBlank();
        uint64_t end = gba->getScheduler().getCurrentCycle();
        totalCycles += (end - start);
    }
    
    // Each repeat should take ~64 cycles (32 halfwords × 2)
    // 10 repeats = ~640 cycles
    EXPECT_GE(totalCycles, 640ULL);
    EXPECT_LE(totalCycles, 2000ULL) << "Repeat mode seems inefficient";
    
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x90));
}

// Test 11: DMA priority performance (higher priority preempts)
TEST_F(DMAPerformanceTest, PriorityPerformance) {
    uint32_t srcAddr0 = 0x00001000;
    uint32_t srcAddr1 = 0x00001100;
    uint32_t destAddr0 = 0x00003000;
    uint32_t destAddr1 = 0x00003100;
    
    fillMemory(srcAddr0, 64, 0xA0);
    fillMemory(srcAddr1, 64, 0xB0);

    // Setup DMA0 (high priority) and DMA1 (lower)
    uint16_t control = 0x9000;  // V-Blank
    setupDMA(0, srcAddr0, destAddr0, 32, control);
    setupDMA(1, srcAddr1, destAddr1, 32, control);
    
    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Trigger - DMA0 should run first, then DMA1
    gba->getDMAController().triggerVBlank();
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // Both should complete: 2 × 32 × 2 = 128 cycles
    EXPECT_GE(cyclesTaken, 128ULL);
    
    EXPECT_TRUE(verifyMemory(destAddr0, 64, 0xA0));
    EXPECT_TRUE(verifyMemory(destAddr1, 64, 0xB0));
}

// Test 12: Back-to-back immediate DMAs
TEST_F(DMAPerformanceTest, BackToBackImmediateDMAs) {
    uint32_t srcAddr = 0x00001000;
    fillMemory(srcAddr, 256, 0xC0);

    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // Execute 4 immediate DMAs back-to-back
    uint16_t control = 0x8000;
    for (int i = 0; i < 4; i++) {
        uint32_t destAddr = 0x00003000 + i * 64;
        setupDMA(i, srcAddr + i * 64, destAddr, 32, control);
    }
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // 4 DMAs × 32 halfwords × 2 cycles = 256 cycles minimum
    EXPECT_GE(cyclesTaken, 256ULL);
    
    // Verify all (each uses different source offset, so pattern shifts)
    for (int i = 0; i < 4; i++) {
        uint32_t destAddr = 0x00003000 + i * 64;
        // Source was srcAddr + i * 64, so pattern base is 0xC0 + (i * 64) % 256
        uint8_t basePattern = (0xC0 + (i * 64)) & 0xFF;
        EXPECT_TRUE(verifyMemory(destAddr, 64, basePattern));
    }
}

// Test 13: IRQ overhead measurement
TEST_F(DMAPerformanceTest, IRQOverhead) {
    uint32_t srcAddr = 0x00001000;
    uint32_t destAddr = 0x00002000;
    fillMemory(srcAddr, 64, 0xD0);

    // Clear IF
    gba->getMemory().write16(0x04000202, 0xFFFF);

    uint64_t startCycles = gba->getScheduler().getCurrentCycle();
    
    // DMA with IRQ enabled
    uint16_t control = 0xC000;  // Enable + IRQ
    setupDMA(0, srcAddr, destAddr, 32, control);
    
    uint64_t endCycles = gba->getScheduler().getCurrentCycle();
    uint64_t cyclesTaken = endCycles - startCycles;
    
    // Should still be around 64 cycles (IRQ just sets flag)
    EXPECT_GE(cyclesTaken, 64ULL);
    EXPECT_LE(cyclesTaken, 200ULL);
    
    // Check IRQ was triggered
    uint16_t ifReg = gba->getMemory().read16(0x04000202);
    EXPECT_TRUE(ifReg & 0x0100) << "DMA0 IRQ not set";
}
