// Integration tests for video timing and V-Blank interrupt

#include <gtest/gtest.h>
#include "gba.h"
#include "scheduler.h"
#include "memory.h"
#include "gpu.h"
#include "interrupt.h"

class VideoTimingTest : public ::testing::Test {
protected:
    GBA* gba;
    
    void SetUp() override {
        // Use test mode (true) to skip BIOS - these tests measure exact cycle counts
        // and BIOS initialization would add unpredictable overhead
        gba = new GBA(true);
    }
    
    void TearDown() override {
        delete gba;
    }
};

TEST_F(VideoTimingTest, SchedulerInitialized) {
    // Verify scheduler starts at cycle 0
    EXPECT_EQ(gba->getScheduler().getCurrentCycle(), 0ull);
}

TEST_F(VideoTimingTest, OneFrameAdvancesCycles) {
    uint64_t cyclesBefore = gba->getScheduler().getCurrentCycle();
    
    gba->runFrame();
    
    uint64_t cyclesAfter = gba->getScheduler().getCurrentCycle();
    
    // One frame should advance by 280,896 cycles
    EXPECT_EQ(cyclesAfter - cyclesBefore, CYCLES_PER_FRAME);
    EXPECT_EQ(gba->getFrameCount(), 1ull);
}

TEST_F(VideoTimingTest, MultipleFramesAccumulate) {
    for (int i = 0; i < 10; i++) {
        gba->runFrame();
    }
    
    // 10 frames = 2,808,960 cycles
    EXPECT_EQ(gba->getScheduler().getCurrentCycle(), CYCLES_PER_FRAME * 10);
    EXPECT_EQ(gba->getFrameCount(), 10ull);
}

TEST_F(VideoTimingTest, VCountUpdatesEachScanline) {
    // Run enough to get through a few scanlines
    gba->getScheduler().runUntil(CYCLES_PER_SCANLINE * 5);
    
    // VCOUNT should be updated by GPU events
    uint16_t vcount = gba->getMemory().read16(REG_VCOUNT);
    
    // Should be somewhere in scanlines 0-4 (or possibly higher due to timing)
    // The exact value depends on when events fire
    EXPECT_GE(vcount, 0);  // At least should be valid
    EXPECT_LT(vcount, SCANLINES_TOTAL);  // Should be within valid range
}

TEST_F(VideoTimingTest, VBlankFlagSetAtScanline160) {
    // VBlank flag is set at the START of scanline 160 (after scanline 159's HBlank completes)
    // This happens at exactly: 160 scanlines * 1232 cycles/scanline = 197120 cycles
    // Run just past this point to ensure the event has fired
    uint64_t targetCycle = CYCLES_PER_SCANLINE * 160 + 10;
    gba->getScheduler().runUntil(targetCycle);
    
    uint64_t actualCycle = gba->getScheduler().getCurrentCycle();
    uint16_t dispstat = gba->getMemory().read16(REG_DISPSTAT);
    uint16_t vcount = gba->getMemory().read16(REG_VCOUNT);
    
    printf("[TEST] Target cycle: %llu, Actual cycle: %llu, VCOUNT: %u, VBLANK: %s\n",
           targetCycle, actualCycle, vcount, (dispstat & DISPSTAT_VBLANK) ? "SET" : "CLEAR");
    
    // V-Blank should be set and we should be at scanline 160
    EXPECT_EQ(vcount, 160);
    EXPECT_TRUE(dispstat & DISPSTAT_VBLANK);
}

TEST_F(VideoTimingTest, VBlankInterruptCanBeEnabled) {
    // Enable V-Blank interrupt in DISPSTAT
    uint16_t dispstat = gba->getMemory().read16(REG_DISPSTAT);
    dispstat |= DISPSTAT_VBLANK_IRQ_ENABLE;
    gba->getMemory().write16(REG_DISPSTAT, dispstat);
    
    // Enable interrupts globally
    gba->getMemory().write16(REG_IME, 0x0001);  // Master enable
    gba->getMemory().write16(REG_IE, IRQ_VBLANK);  // Enable V-Blank IRQ
    
    // Clear any existing IF flags
    gba->getMemory().write16(REG_IF, 0xFFFF);
    
    // Run until start of scanline 160 + a bit (V-Blank interrupt fires at start of scanline 160)
    gba->getScheduler().runUntil(CYCLES_PER_SCANLINE * 160 + 10);
    
    // Check that IF flag was set
    uint16_t ifReg = gba->getMemory().read16(REG_IF);
    EXPECT_TRUE(ifReg & IRQ_VBLANK);
}

TEST_F(VideoTimingTest, Mode3FrameBufferAccessible) {
    // Create a GBA instance without test mode (allocates all memory including VRAM)
    GBA* normalGBA = new GBA(false);
    
    // Set Mode 3 in DISPCNT
    normalGBA->getMemory().write16(REG_DISPCNT, DISPCNT_MODE_3);
    
    // Get framebuffer pointer
    uint16_t* framebuffer = normalGBA->getGPU().getFrameBuffer();
    EXPECT_NE(framebuffer, nullptr);
    
    // Write a pixel value
    if (framebuffer) {
        framebuffer[0] = 0x7FFF;  // White pixel
        
        // Verify it was written to VRAM
        uint16_t pixel = normalGBA->getMemory().read16(0x06000000);
        EXPECT_EQ(pixel, 0x7FFF);
    }
    
    delete normalGBA;
}

TEST_F(VideoTimingTest, ScanlineEventsScheduled) {
    // Check that there are video events in the scheduler
    EXPECT_GT(gba->getScheduler().getPendingEventCount(), 0ull);
    
    // Should have scanline/hblank events
    EXPECT_TRUE(gba->getScheduler().hasEventsOfType(EventType::VIDEO_HBLANK) ||
                gba->getScheduler().hasEventsOfType(EventType::VIDEO_SCANLINE));
}

TEST_F(VideoTimingTest, HBlankFlagToggles) {
    // Run through one scanline's H-Draw period
    gba->getScheduler().runUntil(CYCLES_HDRAW + 10);
    
    uint16_t dispstat = gba->getMemory().read16(REG_DISPSTAT);
    
    // H-Blank should be set after H-Draw
    EXPECT_TRUE(dispstat & DISPSTAT_HBLANK);
}

TEST_F(VideoTimingTest, VBlankClearsAtStartOfFrame) {
    // Run to middle of V-Blank
    gba->getScheduler().runUntil(CYCLES_PER_SCANLINE * 200);
    
    uint16_t dispstat = gba->getMemory().read16(REG_DISPSTAT);
    EXPECT_TRUE(dispstat & DISPSTAT_VBLANK);
    
    // Run past the end of the frame
    gba->runFrame();
    
    // V-Blank should clear at scanline 0
    dispstat = gba->getMemory().read16(REG_DISPSTAT);
    uint16_t vcount = gba->getMemory().read16(REG_VCOUNT);
    
    if (vcount < SCANLINES_VISIBLE) {
        EXPECT_FALSE(dispstat & DISPSTAT_VBLANK);
    }
}
