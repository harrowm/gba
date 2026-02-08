// test_timing_integration.cpp - Integration tests for scheduler with timing system
#include <gtest/gtest.h>
#include "scheduler.h"
#include <vector>

class TimingIntegrationTest : public ::testing::Test {
protected:
    Scheduler scheduler;
    std::vector<std::string> events;
    
    void SetUp() override {
        scheduler.reset();
        events.clear();
    }
};

// Test 1: Video timing with HBlank events
TEST_F(TimingIntegrationTest, HBlankEvents) {
    const uint32_t CYCLES_PER_SCANLINE = 1232;
    const uint32_t HBLANK_START = 960;
    
    int hblankCount = 0;
    
    // Schedule HBlank events for first 5 scanlines
    for (int scanline = 0; scanline < 5; scanline++) {
        uint64_t hblankCycle = scanline * CYCLES_PER_SCANLINE + HBLANK_START;
        scheduler.scheduleAt(hblankCycle, [&hblankCount, scanline, this]() {
            hblankCount++;
            events.push_back("HBlank " + std::to_string(scanline));
        }, EventType::VIDEO_HBLANK, 1);
    }
    
    // Run for 5 complete scanlines
    scheduler.runUntil(5 * CYCLES_PER_SCANLINE);
    
    EXPECT_EQ(hblankCount, 5);
    EXPECT_EQ(events.size(), 5UL);
}

// Test 2: VBlank timing
TEST_F(TimingIntegrationTest, VBlankTiming) {
    const uint32_t CYCLES_PER_SCANLINE = 1232;
    const uint32_t VISIBLE_SCANLINES = 160;
    const uint32_t VBLANK_CYCLE = VISIBLE_SCANLINES * CYCLES_PER_SCANLINE;
    
    bool vblankTriggered = false;
    uint64_t vblankCycle = 0;
    
    scheduler.scheduleAt(VBLANK_CYCLE, [&vblankTriggered, &vblankCycle, this]() {
        vblankTriggered = true;
        vblankCycle = scheduler.getCurrentCycle();
        events.push_back("VBlank");
    }, EventType::VIDEO_VBLANK, 0);
    
    // Run past VBlank
    scheduler.runUntil(VBLANK_CYCLE + 1000);
    
    EXPECT_TRUE(vblankTriggered);
    EXPECT_EQ(vblankCycle, VBLANK_CYCLE);
}

// Test 3: Timer overflow simulation
TEST_F(TimingIntegrationTest, TimerOverflow) {
    const uint32_t TIMER_CYCLES = 65536;  // 16-bit timer at 1:1 prescaler
    
    int overflowCount = 0;
    std::vector<uint64_t> overflowCycles;
    
    // Schedule recurring timer overflow
    std::function<void()> timerOverflow;
    timerOverflow = [&, this]() {
        overflowCount++;
        overflowCycles.push_back(scheduler.getCurrentCycle());
        
        // Reschedule next overflow
        if (overflowCount < 5) {
            scheduler.schedule(TIMER_CYCLES, timerOverflow, EventType::TIMER_0_OVERFLOW);
        }
    };
    
    scheduler.schedule(TIMER_CYCLES, timerOverflow, EventType::TIMER_0_OVERFLOW);
    
    // Run for 5 timer periods
    scheduler.runUntil(TIMER_CYCLES * 5 + 100);
    
    EXPECT_EQ(overflowCount, 5);
    EXPECT_EQ(overflowCycles.size(), 5UL);
    
    // Verify timing is correct
    for (size_t i = 0; i < overflowCycles.size(); i++) {
        EXPECT_EQ(overflowCycles[i], static_cast<uint64_t>(TIMER_CYCLES) * (i + 1));
    }
}

// Test 4: Mixed video and timer events
TEST_F(TimingIntegrationTest, MixedVideoAndTimerEvents) {
    const uint32_t CYCLES_PER_SCANLINE = 1232;
    const uint32_t TIMER_CYCLES = 65536;
    
    // Schedule video events
    for (int i = 0; i < 160; i++) {
        uint64_t cycle = i * CYCLES_PER_SCANLINE + 960;
        scheduler.scheduleAt(cycle, [this, i]() {
            events.push_back("HBlank" + std::to_string(i));
        }, EventType::VIDEO_HBLANK, 1);
    }
    
    // Schedule VBlank
    scheduler.scheduleAt(160 * CYCLES_PER_SCANLINE, [this]() {
        events.push_back("VBlank");
    }, EventType::VIDEO_VBLANK, 0);
    
    // Schedule timer overflows
    std::function<void()> timerEvent;
    int timerCount = 0;
    timerEvent = [&, this]() {
        events.push_back("Timer" + std::to_string(timerCount++));
        if (timerCount < 5) {
            scheduler.schedule(TIMER_CYCLES, timerEvent, EventType::TIMER_0_OVERFLOW);
        }
    };
    scheduler.schedule(TIMER_CYCLES, timerEvent, EventType::TIMER_0_OVERFLOW);
    
    // Run one full frame plus a bit
    scheduler.runUntil(228 * CYCLES_PER_SCANLINE);
    
    // Should have 160 HBlanks, 1 VBlank, and multiple timer events
    int hblankCount = 0;
    int vblankCount = 0;
    int timerEventCount = 0;
    
    for (const auto& event : events) {
        if (event.find("HBlank") == 0) hblankCount++;
        else if (event.find("VBlank") == 0) vblankCount++;
        else if (event.find("Timer") == 0) timerEventCount++;
    }
    
    EXPECT_EQ(hblankCount, 160);
    EXPECT_EQ(vblankCount, 1);
    EXPECT_GT(timerEventCount, 0);
}

// Test 5: Event priority at same cycle
TEST_F(TimingIntegrationTest, EventPriorityAtSameCycle) {
    const uint32_t TARGET_CYCLE = 1000;
    
    // Schedule events at same cycle with different priorities
    // Priority: VBlank (0) > HBlank (1) > Timer (2)
    scheduler.scheduleAt(TARGET_CYCLE, [this]() {
        events.push_back("Timer");
    }, EventType::TIMER_0_OVERFLOW, 2);
    
    scheduler.scheduleAt(TARGET_CYCLE, [this]() {
        events.push_back("HBlank");
    }, EventType::VIDEO_HBLANK, 1);
    
    scheduler.scheduleAt(TARGET_CYCLE, [this]() {
        events.push_back("VBlank");
    }, EventType::VIDEO_VBLANK, 0);
    
    scheduler.runUntil(TARGET_CYCLE + 1);
    
    EXPECT_EQ(events.size(), 3UL);
    EXPECT_EQ(events[0], "VBlank");   // Priority 0 (highest)
    EXPECT_EQ(events[1], "HBlank");   // Priority 1
    EXPECT_EQ(events[2], "Timer");    // Priority 2
}

// Test 6: Cycle-accurate timing advancement
TEST_F(TimingIntegrationTest, CycleAccurateAdvancement) {
    std::vector<uint64_t> cycleSnapshots;
    
    // Schedule events at specific intervals
    for (int i = 1; i <= 10; i++) {
        scheduler.schedule(i * 100, [&cycleSnapshots, this]() {
            cycleSnapshots.push_back(scheduler.getCurrentCycle());
        });
    }
    
    scheduler.runUntil(1000);
    
    EXPECT_EQ(cycleSnapshots.size(), 10UL);
    
    // Verify each event executed at exact cycle
    for (size_t i = 0; i < cycleSnapshots.size(); i++) {
        EXPECT_EQ(cycleSnapshots[i], (i + 1) * 100ULL);
    }
}

// Test 7: Canceling video events (mode change scenario)
TEST_F(TimingIntegrationTest, CancelVideoEventsOnModeChange) {
    // Schedule some video events
    for (int i = 0; i < 10; i++) {
        scheduler.schedule(i * 100, [this, i]() {
            events.push_back("Video" + std::to_string(i));
        }, EventType::VIDEO_HBLANK);
    }
    
    // Schedule some timer events
    for (int i = 0; i < 10; i++) {
        scheduler.schedule(i * 150 + 50, [this, i]() {
            events.push_back("Timer" + std::to_string(i));
        }, EventType::TIMER_0_OVERFLOW);
    }
    
    // Simulate mode change at cycle 400 - cancel all video events
    scheduler.schedule(400, [this]() {
        scheduler.cancelEventsOfType(EventType::VIDEO_HBLANK);
        events.push_back("ModeChange");
    });
    
    scheduler.runUntil(2000);
    
    // Count event types
    int videoEvents = 0;
    int timerEvents = 0;
    
    for (const auto& event : events) {
        if (event.find("Video") == 0) videoEvents++;
        else if (event.find("Timer") == 0) timerEvents++;
    }
    
    // Should have executed video events before 400, but not after
    EXPECT_LT(videoEvents, 10);  // Not all 10 video events
    EXPECT_EQ(timerEvents, 10);  // All timer events still execute
}

// Test 8: Query cycles until next event type
TEST_F(TimingIntegrationTest, QueryCyclesUntilEventType) {
    scheduler.schedule(100, []() {}, EventType::TIMER_0_OVERFLOW);
    scheduler.schedule(200, []() {}, EventType::VIDEO_HBLANK);
    scheduler.schedule(300, []() {}, EventType::VIDEO_VBLANK);
    
    // Initially at cycle 0
    EXPECT_EQ(scheduler.getCyclesUntilEvent(EventType::TIMER_0_OVERFLOW), 100ULL);
    EXPECT_EQ(scheduler.getCyclesUntilEvent(EventType::VIDEO_HBLANK), 200ULL);
    EXPECT_EQ(scheduler.getCyclesUntilEvent(EventType::VIDEO_VBLANK), 300ULL);
    
    // Advance to cycle 150
    scheduler.runUntil(150);
    
    EXPECT_EQ(scheduler.getCyclesUntilEvent(EventType::VIDEO_HBLANK), 50ULL);  // 200 - 150
    EXPECT_EQ(scheduler.getCyclesUntilEvent(EventType::VIDEO_VBLANK), 150ULL); // 300 - 150
}

// Test 9: Frame timing simulation (one complete frame)
TEST_F(TimingIntegrationTest, CompleteFrameSimulation) {
    const uint32_t CYCLES_PER_SCANLINE = 1232;
    const uint32_t TOTAL_SCANLINES = 228;
    const uint32_t CYCLES_PER_FRAME = CYCLES_PER_SCANLINE * TOTAL_SCANLINES;
    
    int scanlineCount = 0;
    bool frameComplete = false;
    
    // Schedule scanline events
    for (uint32_t scanline = 0; scanline < TOTAL_SCANLINES; scanline++) {
        uint64_t cycle = scanline * CYCLES_PER_SCANLINE;
        scheduler.scheduleAt(cycle, [&scanlineCount]() {
            scanlineCount++;
        }, EventType::VIDEO_SCANLINE);
    }
    
    // Schedule frame complete event
    scheduler.scheduleAt(CYCLES_PER_FRAME, [&frameComplete]() {
        frameComplete = true;
    });
    
    scheduler.runUntil(CYCLES_PER_FRAME + 1);
    
    EXPECT_EQ(scanlineCount, static_cast<int>(TOTAL_SCANLINES));
    EXPECT_TRUE(frameComplete);
    EXPECT_EQ(scheduler.getCurrentCycle(), CYCLES_PER_FRAME + 1);
}

// Test 10: Stress test - many events
TEST_F(TimingIntegrationTest, StressTestManyEvents) {
    const size_t NUM_EVENTS = 10000;
    int executedEvents = 0;
    
    // Schedule many events
    for (size_t i = 0; i < NUM_EVENTS; i++) {
        scheduler.schedule(i + 1, [&executedEvents]() {
            executedEvents++;
        });
    }
    
    EXPECT_EQ(scheduler.getPendingEventCount(), NUM_EVENTS);
    
    scheduler.runUntil(NUM_EVENTS + 100);
    
    EXPECT_EQ(executedEvents, static_cast<int>(NUM_EVENTS));
    EXPECT_TRUE(scheduler.isEmpty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
