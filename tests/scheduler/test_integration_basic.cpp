// test_integration_basic.cpp
// A minimal integration test you can run immediately to verify scheduler works
// with emulator components before doing full integration

#include <gtest/gtest.h>
#include "scheduler.h"
#include <vector>
#include <string>
#include <cstdio>

// Mock GPU structure to test with
struct MockGPU {
    Scheduler* scheduler;
    int vcount = 0;
    bool in_hblank = false;
    bool in_vblank = false;
    std::vector<std::string> events;
    
    void init(Scheduler* sched) {
        scheduler = sched;
        vcount = 0;
        in_hblank = false;
        in_vblank = false;
        scheduleNextScanline();
    }
    
    void scheduleNextScanline() {
        scheduler->schedule(1232, [this]() { onScanline(); }, 
                           EventType::VIDEO_SCANLINE, 1);
    }
    
    void onScanline() {
        events.push_back("Scanline " + std::to_string(vcount));
        
        if (vcount < 160) {
            // Schedule HBlank during visible scanlines
            scheduler->schedule(960, [this]() { onHBlank(); },
                               EventType::VIDEO_HBLANK, 1);
        }
        
        vcount++;
        
        if (vcount == 160) {
            // VBlank starts
            in_vblank = true;
            events.push_back("VBlank Start");
        } else if (vcount >= 228) {
            // Frame complete, restart
            vcount = 0;
            in_vblank = false;
            events.push_back("Frame Complete");
        }
        
        // Schedule next scanline
        scheduleNextScanline();
    }
    
    void onHBlank() {
        in_hblank = true;
        events.push_back("HBlank " + std::to_string(vcount));
        
        // Schedule HBlank end
        scheduler->schedule(272, [this]() { 
            in_hblank = false; 
        }, EventType::VIDEO_HBLANK, 1);
    }
};

// Mock Timer structure
struct MockTimer {
    Scheduler* scheduler;
    uint16_t counter = 0;
    uint16_t reload_value = 0;
    bool enabled = false;
    int overflow_count = 0;
    EventType event_type;
    
    void init(Scheduler* sched, EventType type) {
        scheduler = sched;
        event_type = type;
    }
    
    void start(uint16_t reload, int prescaler) {
        counter = reload;
        reload_value = reload;
        enabled = true;
        
        uint32_t cycles = (0x10000 - counter) * prescaler;
        scheduler->schedule(cycles, [this]() { onOverflow(); },
                           event_type, 2);
    }
    
    void stop() {
        enabled = false;
        scheduler->cancelEventsOfType(event_type);
    }
    
    void onOverflow() {
        overflow_count++;
        counter = reload_value;
        
        if (enabled) {
            // Reschedule next overflow
            scheduler->schedule(0x10000, [this]() { onOverflow(); },
                               event_type, 2);
        }
    }
};

// ============================================================================
// Basic Integration Tests
// ============================================================================

class BasicIntegrationTest : public ::testing::Test {
protected:
    Scheduler scheduler;
    
    void SetUp() override {
        scheduler.reset();
    }
};

TEST_F(BasicIntegrationTest, GPUSimulation_OneFrame) {
    MockGPU gpu;
    gpu.init(&scheduler);
    
    // Run one complete frame (228 scanlines)
    scheduler.runUntil(228 * 1232);
    
    // Check that frame completed
    EXPECT_EQ(gpu.vcount, 0);  // Should wrap back to 0
    
    // Count events
    int scanline_events = 0;
    int hblank_events = 0;
    int vblank_events = 0;
    
    for (const auto& event : gpu.events) {
        if (event.find("Scanline") == 0) scanline_events++;
        if (event.find("HBlank") == 0) hblank_events++;
        if (event.find("VBlank") == 0) vblank_events++;
    }
    
    EXPECT_EQ(scanline_events, 228);  // All scanlines
    EXPECT_EQ(hblank_events, 160);    // Only during visible scanlines
    EXPECT_EQ(vblank_events, 1);      // One VBlank
    
    printf("✓ GPU simulation: %d scanlines, %d HBlanks, %d VBlanks\n",
           scanline_events, hblank_events, vblank_events);
}

TEST_F(BasicIntegrationTest, GPUSimulation_VBlankAtCorrectCycle) {
    MockGPU gpu;
    gpu.init(&scheduler);
    
    uint64_t vblank_cycle = 0;
    
    // Run until VBlank
    while (!gpu.in_vblank && scheduler.getCurrentCycle() < 300000) {
        scheduler.runUntil(scheduler.getCurrentCycle() + 1232);
    }
    
    vblank_cycle = scheduler.getCurrentCycle();
    
    // VBlank should occur at exactly 160 * 1232 cycles
    EXPECT_EQ(vblank_cycle, 160ULL * 1232);
    
    printf("✓ VBlank occurred at cycle %llu (expected %llu)\n", 
           vblank_cycle, 160ULL * 1232);
}

TEST_F(BasicIntegrationTest, TimerSimulation_BasicOverflow) {
    MockTimer timer;
    timer.init(&scheduler, EventType::TIMER_0_OVERFLOW);
    
    // Start timer from 0 with prescaler 1
    timer.start(0x0000, 1);
    
    // Run for exactly one overflow period
    scheduler.runUntil(65536);
    
    EXPECT_EQ(timer.overflow_count, 1);
    
    printf("✓ Timer overflowed %d time(s) after 65536 cycles\n", 
           timer.overflow_count);
}

TEST_F(BasicIntegrationTest, TimerSimulation_MultipleOverflows) {
    MockTimer timer;
    timer.init(&scheduler, EventType::TIMER_0_OVERFLOW);
    
    timer.start(0x0000, 1);
    
    // Run for 5 overflow periods
    scheduler.runUntil(65536 * 5);
    
    EXPECT_EQ(timer.overflow_count, 5);
    
    printf("✓ Timer overflowed %d times in 327,680 cycles\n", 
           timer.overflow_count);
}

TEST_F(BasicIntegrationTest, TimerSimulation_StopCancelsEvents) {
    MockTimer timer;
    timer.init(&scheduler, EventType::TIMER_0_OVERFLOW);
    
    timer.start(0x0000, 1);
    
    // Run partway through overflow period
    scheduler.runUntil(30000);
    
    // Stop timer
    timer.stop();
    
    // Run way past overflow point
    scheduler.runUntil(100000);
    
    // Should not have overflowed
    EXPECT_EQ(timer.overflow_count, 0);
    
    printf("✓ Timer stopped and no overflow occurred\n");
}

TEST_F(BasicIntegrationTest, GPUAndTimer_Together) {
    MockGPU gpu;
    MockTimer timer;
    
    gpu.init(&scheduler);
    timer.init(&scheduler, EventType::TIMER_0_OVERFLOW);
    
    // Start timer
    timer.start(0x0000, 1);
    
    // Run one frame
    scheduler.runUntil(228 * 1232);
    
    // Check both systems worked
    EXPECT_EQ(gpu.vcount, 0);  // Frame completed
    EXPECT_GT(timer.overflow_count, 0);  // Timer overflowed at least once
    
    printf("✓ GPU + Timer: Frame complete with %d timer overflows\n",
           timer.overflow_count);
}

TEST_F(BasicIntegrationTest, MultipleTimers_DifferentRates) {
    MockTimer timer0, timer1, timer2;
    
    timer0.init(&scheduler, EventType::TIMER_0_OVERFLOW);
    timer1.init(&scheduler, EventType::TIMER_1_OVERFLOW);
    timer2.init(&scheduler, EventType::TIMER_2_OVERFLOW);
    
    // Different prescalers
    timer0.start(0x0000, 1);    // Fast
    timer1.start(0x0000, 64);   // Medium
    timer2.start(0x0000, 256);  // Slow
    
    // Run for a while
    scheduler.runUntil(65536 * 256);
    
    // Timer 0 should overflow most
    EXPECT_GT(timer0.overflow_count, timer1.overflow_count);
    EXPECT_GT(timer1.overflow_count, timer2.overflow_count);
    
    printf("✓ Multiple timers: T0=%d, T1=%d, T2=%d overflows\n",
           timer0.overflow_count, timer1.overflow_count, timer2.overflow_count);
}

TEST_F(BasicIntegrationTest, EventPriority_SameCycle) {
    std::vector<std::string> execution_order;
    
    uint64_t target_cycle = 1000;
    
    // Schedule events at same cycle with different priorities
    scheduler.scheduleAt(target_cycle, [&]() {
        execution_order.push_back("Timer");
    }, EventType::TIMER_0_OVERFLOW, 2);
    
    scheduler.scheduleAt(target_cycle, [&]() {
        execution_order.push_back("HBlank");
    }, EventType::VIDEO_HBLANK, 1);
    
    scheduler.scheduleAt(target_cycle, [&]() {
        execution_order.push_back("VBlank");
    }, EventType::VIDEO_VBLANK, 0);
    
    scheduler.runUntil(target_cycle + 1);
    
    ASSERT_EQ(execution_order.size(), 3UL);
    EXPECT_EQ(execution_order[0], "VBlank");   // Priority 0 (highest)
    EXPECT_EQ(execution_order[1], "HBlank");   // Priority 1
    EXPECT_EQ(execution_order[2], "Timer");    // Priority 2
    
    printf("✓ Event priority: %s -> %s -> %s\n",
           execution_order[0].c_str(), execution_order[1].c_str(), 
           execution_order[2].c_str());
}

TEST_F(BasicIntegrationTest, FullSystem_MultipleFrames) {
    MockGPU gpu;
    MockTimer timer0, timer1;
    
    gpu.init(&scheduler);
    timer0.init(&scheduler, EventType::TIMER_0_OVERFLOW);
    timer1.init(&scheduler, EventType::TIMER_1_OVERFLOW);
    
    timer0.start(0x0000, 1);
    timer1.start(0xF000, 64);  // Longer period
    
    // Run 10 frames
    for (int frame = 0; frame < 10; frame++) {
        uint64_t frame_start = scheduler.getCurrentCycle();
        scheduler.runUntil(frame_start + 228 * 1232);
        
        EXPECT_EQ(gpu.vcount, 0) << "Frame " << frame << " didn't complete";
    }
    
    printf("✓ Ran 10 frames successfully\n");
    printf("  - Cycles: %llu\n", scheduler.getCurrentCycle());
    printf("  - Timer 0 overflows: %d\n", timer0.overflow_count);
    printf("  - Timer 1 overflows: %d\n", timer1.overflow_count);
}

TEST_F(BasicIntegrationTest, CycleAccuracy_OneFrame) {
    MockGPU gpu;
    gpu.init(&scheduler);
    
    uint64_t start_cycle = scheduler.getCurrentCycle();
    
    // Run one frame
    scheduler.runUntil(start_cycle + 228 * 1232);
    
    uint64_t end_cycle = scheduler.getCurrentCycle();
    uint64_t elapsed = end_cycle - start_cycle;
    
    // Should be exactly 280,896 cycles (228 scanlines * 1232 cycles)
    EXPECT_EQ(elapsed, 280896ULL);
    
    printf("✓ Frame timing: %llu cycles (expected 280,896)\n", elapsed);
}

TEST_F(BasicIntegrationTest, StressTest_ManyEvents) {
    const int NUM_TIMERS = 100;
    std::vector<MockTimer> timers(NUM_TIMERS);
    
    // Start many timers with different periods
    for (int i = 0; i < NUM_TIMERS; i++) {
        timers[i].init(&scheduler, EventType::TIMER_0_OVERFLOW);
        timers[i].start(i * 100, 1);  // Different starting points
    }
    
    // Run for a while
    scheduler.runUntil(100000);
    
    // Count total overflows
    int total_overflows = 0;
    for (auto& timer : timers) {
        total_overflows += timer.overflow_count;
    }
    
    EXPECT_GT(total_overflows, 0);
    
    printf("✓ Stress test: %d timers, %d total overflows\n",
           NUM_TIMERS, total_overflows);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    printf("\n");
    printf("========================================\n");
    printf("  Basic Integration Tests\n");
    printf("========================================\n\n");
    
    int result = RUN_ALL_TESTS();
    
    printf("\n");
    if (result == 0) {
        printf("✅ All integration tests passed!\n");
        printf("\nThe scheduler is ready to integrate with:\n");
        printf("  • GPU (video controller)\n");
        printf("  • Timers\n");
        printf("  • Main emulation loop\n\n");
    } else {
        printf("❌ Some tests failed - check implementation\n\n");
    }
    
    return result;
}
