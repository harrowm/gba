#include <gtest/gtest.h>
#include "memory.h"
#include "dma.h"
#include "scheduler.h"
#include "interrupt.h"
#include <cstring>

// Test fixture for basic DMA tests
class DMABasicTest : public ::testing::Test {
protected:
    Memory* memory;
    Scheduler* scheduler;
    DMAController* dmaController;
    InterruptController* interruptController;

    void SetUp() override {
        // Create components
        memory = new Memory(false);  // Normal mode
        scheduler = new Scheduler();
        dmaController = new DMAController();
        interruptController = new InterruptController();

        // Wire them up
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

    // Helper: Fill memory region with pattern
    void fillMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            memory->write8(address + i, pattern + (i & 0xFF));
        }
    }

    // Helper: Verify memory region matches pattern
    bool verifyMemory(uint32_t address, uint32_t size, uint8_t pattern) {
        for (uint32_t i = 0; i < size; i++) {
            uint8_t expected = pattern + (i & 0xFF);
            uint8_t actual = memory->read8(address + i);
            if (actual != expected) {
                printf("Mismatch at 0x%08X: expected 0x%02X, got 0x%02X\n", 
                       address + i, expected, actual);
                return false;
            }
        }
        return true;
    }

    // Helper: Setup and trigger immediate DMA transfer
    void setupImmediateDMA(int channel, uint32_t src, uint32_t dest, uint16_t count, 
                          uint16_t control) {
        // Write DMA registers
        memory->write32(0x040000B0 + channel * 12, src);      // Source address
        memory->write32(0x040000B4 + channel * 12, dest);     // Dest address
        memory->write16(0x040000B8 + channel * 12, count);    // Word count
        memory->write16(0x040000BA + channel * 12, control);  // Control (triggers if enable bit set)
    }
};

// Test 1: Basic 16-bit immediate mode copy
TEST_F(DMABasicTest, ImmediateMode16BitCopy) {
    // Fill source with pattern
    uint32_t srcAddr = 0x02000000;  // EWRAM
    uint32_t destAddr = 0x02001000; // EWRAM
    fillMemory(srcAddr, 64, 0xAA);

    // Setup DMA0: copy 32 halfwords (64 bytes)
    // Control: Enable | Immediate | 16-bit | Src Inc | Dest Inc
    uint16_t control = 0x8000;  // Enable bit only, all other bits 0 (immediate, 16-bit, increment)
    
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify destination matches source
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0xAA));
}

// Test 2: Basic 32-bit immediate mode copy
TEST_F(DMABasicTest, ImmediateMode32BitCopy) {
    // Fill source with pattern
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    fillMemory(srcAddr, 128, 0x55);

    // Setup DMA0: copy 32 words (128 bytes)
    // Control: Enable | Immediate | 32-bit | Src Inc | Dest Inc
    uint16_t control = 0x8400;  // Enable + 32-bit transfer
    
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify destination matches source
    EXPECT_TRUE(verifyMemory(destAddr, 128, 0x55));
}

// Test 3: Source address increment mode
TEST_F(DMABasicTest, SourceAddressIncrement) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    // Write unique pattern to source
    for (int i = 0; i < 64; i += 2) {
        memory->write16(srcAddr + i, 0x1000 + i);
    }

    // Setup DMA: Src increment, Dest increment
    uint16_t control = 0x8000;  // Enable, all defaults (increment both)
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify each halfword copied correctly
    for (int i = 0; i < 64; i += 2) {
        uint16_t expected = 0x1000 + i;
        uint16_t actual = memory->read16(destAddr + i);
        EXPECT_EQ(expected, actual) << "Mismatch at offset " << i;
    }
}

// Test 4: Source address decrement mode
TEST_F(DMABasicTest, SourceAddressDecrement) {
    uint32_t srcAddr = 0x02000100;  // Start higher
    uint32_t destAddr = 0x02001000;
    
    // Write pattern to source
    for (int i = 0; i < 64; i += 2) {
        memory->write16(srcAddr - i, 0x2000 + i);
    }

    // Setup DMA: Src decrement (bits 7-8 = 01), Dest increment
    uint16_t control = 0x8080;  // Enable + Src decrement
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify data copied in reverse order
    for (int i = 0; i < 64; i += 2) {
        uint16_t expected = 0x2000 + i;
        uint16_t actual = memory->read16(destAddr + i);
        EXPECT_EQ(expected, actual) << "Mismatch at offset " << i;
    }
}

// Test 5: Source address fixed mode
TEST_F(DMABasicTest, SourceAddressFixed) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    // Write single value to source
    memory->write16(srcAddr, 0x1234);

    // Setup DMA: Src fixed (bits 7-8 = 10), Dest increment
    uint16_t control = 0x8100;  // Enable + Src fixed
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify all destination halfwords are the same
    for (int i = 0; i < 64; i += 2) {
        uint16_t actual = memory->read16(destAddr + i);
        EXPECT_EQ(0x1234, actual) << "Mismatch at offset " << i;
    }
}

// Test 6: Destination address increment mode (already tested, but explicit)
TEST_F(DMABasicTest, DestAddressIncrement) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    fillMemory(srcAddr, 64, 0x77);

    // Setup DMA: Dest increment (default = 00)
    uint16_t control = 0x8000;  // Enable
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify sequential write
    EXPECT_TRUE(verifyMemory(destAddr, 64, 0x77));
}

// Test 7: Destination address decrement mode
TEST_F(DMABasicTest, DestAddressDecrement) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001100;  // Start higher
    
    // Write pattern to source
    for (int i = 0; i < 64; i += 2) {
        memory->write16(srcAddr + i, 0x3000 + i);
    }

    // Setup DMA: Dest decrement (bits 5-6 = 01)
    uint16_t control = 0x8020;  // Enable + Dest decrement
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify data written in reverse
    for (int i = 0; i < 64; i += 2) {
        uint16_t expected = 0x3000 + i;
        uint16_t actual = memory->read16(destAddr - i);
        EXPECT_EQ(expected, actual) << "Mismatch at offset " << i;
    }
}

// Test 8: Destination address fixed mode
TEST_F(DMABasicTest, DestAddressFixed) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    // Write varying data to source
    for (int i = 0; i < 64; i += 2) {
        memory->write16(srcAddr + i, 0x4000 + i);
    }

    // Setup DMA: Dest fixed (bits 9-10 = 10)
    uint16_t control = 0x8400;  // Enable + Dest fixed... wait, that's 32-bit!
    // Let me fix: Dest fixed is 0x0400, but bit 10 is also 32-bit flag
    // Dest control is bits 5-6 (not 9-10!)
    control = 0x8040;  // Enable + Dest fixed (bit 6)
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Verify only last value written (all writes to same location)
    uint16_t actual = memory->read16(destAddr);
    // Last value written should be 0x4000 + 62 = 0x403E
    EXPECT_EQ(0x403E, actual);
}

// Test 9: Word count of zero means max transfer
TEST_F(DMABasicTest, ZeroWordCountMaxTransfer) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02010000;  // Different area
    
    // Write pattern to source
    fillMemory(srcAddr, 256, 0x11);

    // Setup DMA3 with word count = 0 (should mean 65536 for DMA3, but we'll test smaller area)
    // For DMA0-2, word count 0 = 16384
    // We can't test full 65536 transfers easily, so let's just verify count=0 is handled
    uint16_t control = 0x8000;  // Enable
    setupImmediateDMA(0, srcAddr, destAddr, 0, control);

    // Just verify some data was transferred (DMA0 with count=0 should do 16384 halfwords = 32KB)
    // Check first 256 bytes
    EXPECT_TRUE(verifyMemory(destAddr, 256, 0x11));
}

// Test 10: Multiple DMA channels (priority)
TEST_F(DMABasicTest, MultipleChannelsPriority) {
    uint32_t srcAddr0 = 0x02000000;
    uint32_t destAddr0 = 0x02010000;
    uint32_t srcAddr1 = 0x02000100;
    uint32_t destAddr1 = 0x02010100;
    
    fillMemory(srcAddr0, 64, 0xAA);
    fillMemory(srcAddr1, 64, 0xBB);

    // Setup both DMA0 and DMA1
    uint16_t control = 0x8000;
    setupImmediateDMA(0, srcAddr0, destAddr0, 32, control);
    setupImmediateDMA(1, srcAddr1, destAddr1, 32, control);

    // Both should complete (DMA0 has priority)
    EXPECT_TRUE(verifyMemory(destAddr0, 64, 0xAA));
    EXPECT_TRUE(verifyMemory(destAddr1, 64, 0xBB));
}

// Test 11: IRQ on completion
TEST_F(DMABasicTest, IRQOnCompletion) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    fillMemory(srcAddr, 64, 0xCC);

    // Clear IF register
    memory->write16(0x04000202, 0xFFFF);

    // Setup DMA0 with IRQ enable (bit 14)
    uint16_t control = 0xC000;  // Enable + IRQ enable
    setupImmediateDMA(0, srcAddr, destAddr, 32, control);

    // Check that DMA0 IRQ flag is set in IF (bit 8)
    uint16_t ifReg = memory->read16(0x04000202);
    EXPECT_TRUE(ifReg & 0x0100) << "DMA0 IRQ flag not set";
}

// Test 12: Transfer size validation (16-bit vs 32-bit)
TEST_F(DMABasicTest, TransferSizeValidation) {
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    // Write 32-bit values
    memory->write32(srcAddr + 0, 0x12345678);
    memory->write32(srcAddr + 4, 0x9ABCDEF0);

    // Test 32-bit transfer
    uint16_t control = 0x8400;  // Enable + 32-bit
    setupImmediateDMA(0, srcAddr, destAddr, 2, control);  // 2 words

    // Verify 32-bit values preserved
    EXPECT_EQ(0x12345678U, memory->read32(destAddr + 0));
    EXPECT_EQ(0x9ABCDEF0U, memory->read32(destAddr + 4));
}
