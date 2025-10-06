// test_scheduler.cpp - Comprehensive tests for the Scheduler class
#include <gtest/gtest.h>
#include "scheduler.h"
#include <vector>
#include <functional>

class SchedulerTest : public ::testing::Test {
protected:
    Scheduler scheduler;
    std::vector<int> executionOrder;
    
    void SetUp() override {
        scheduler.reset();
        executionOrder.clear();
    }
};

// Test 1: Basic event execution
TEST_F(SchedulerTest, BasicEventExecution) {
    int counter = 0;
    
    scheduler.schedule(100, [&counter]() {
        counter++;
    });
    
    EXPECT_EQ(counter, 0);
    EXPECT_EQ(scheduler.getCurrentCycle(), 0ULL);
    
    // Run until the event triggers
    scheduler.runUntil(100);
    
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(scheduler.getCurrentCycle(), 100ULL);
}

// Test 2: Multiple events execute in correct order
TEST_F(SchedulerTest, MultipleEventsInOrder) {
    std::vector<int> results;
    
    scheduler.schedule(10, [&results]() { results.push_back(1); });
    scheduler.schedule(20, [&results]() { results.push_back(2); });
    scheduler.schedule(30, [&results]() { results.push_back(3); });
    
    scheduler.runUntil(35);
    
    EXPECT_EQ(results.size(), 3UL);
    EXPECT_EQ(results[0], 1);
    EXPECT_EQ(results[1], 2);
    EXPECT_EQ(results[2], 3);
}

// Test 3: Events scheduled out of order execute in correct order
TEST_F(SchedulerTest, OutOfOrderScheduling) {
    scheduler.schedule(30, [this]() { executionOrder.push_back(3); });
    scheduler.schedule(10, [this]() { executionOrder.push_back(1); });
    scheduler.schedule(20, [this]() { executionOrder.push_back(2); });
    
    scheduler.runUntil(40);
    
    EXPECT_EQ(executionOrder.size(), 3UL);
    EXPECT_EQ(executionOrder[0], 1);
    EXPECT_EQ(executionOrder[1], 2);
    EXPECT_EQ(executionOrder[2], 3);
}

// Test 4: Priority handling for events at same cycle
TEST_F(SchedulerTest, PriorityHandling) {
    // Lower priority value = higher priority
    scheduler.schedule(100, [this]() { executionOrder.push_back(2); }, EventType::CUSTOM, 2);
    scheduler.schedule(100, [this]() { executionOrder.push_back(1); }, EventType::CUSTOM, 1);
    scheduler.schedule(100, [this]() { executionOrder.push_back(3); }, EventType::CUSTOM, 3);
    
    scheduler.runUntil(100);
    
    EXPECT_EQ(executionOrder.size(), 3UL);
    EXPECT_EQ(executionOrder[0], 1);  // Priority 1 (highest)
    EXPECT_EQ(executionOrder[1], 2);  // Priority 2
    EXPECT_EQ(executionOrder[2], 3);  // Priority 3
}

// Test 5: Cycles until next event
TEST_F(SchedulerTest, CyclesUntilNextEvent) {
    scheduler.schedule(50, []() {});
    scheduler.schedule(100, []() {});
    scheduler.schedule(150, []() {});
    
    EXPECT_EQ(scheduler.getCyclesUntilNextEvent(), 50ULL);
    
    scheduler.runUntil(60);
    EXPECT_EQ(scheduler.getCyclesUntilNextEvent(), 40ULL);  // 100 - 60
    
    scheduler.runUntil(110);
    EXPECT_EQ(scheduler.getCyclesUntilNextEvent(), 40ULL);  // 150 - 110
    
    scheduler.runUntil(200);
    EXPECT_EQ(scheduler.getCyclesUntilNextEvent(), UINT64_MAX);  // No more events
}

// Test 6: Event type filtering
TEST_F(SchedulerTest, EventTypeFiltering) {
    scheduler.schedule(10, [this]() { executionOrder.push_back(1); }, EventType::TIMER_0_OVERFLOW);
    scheduler.schedule(20, [this]() { executionOrder.push_back(2); }, EventType::VIDEO_HBLANK);
    scheduler.schedule(30, [this]() { executionOrder.push_back(3); }, EventType::TIMER_0_OVERFLOW);
    
    EXPECT_TRUE(scheduler.hasEventsOfType(EventType::TIMER_0_OVERFLOW));
    EXPECT_TRUE(scheduler.hasEventsOfType(EventType::VIDEO_HBLANK));
    EXPECT_FALSE(scheduler.hasEventsOfType(EventType::VIDEO_VBLANK));
    
    uint64_t cyclesUntilTimer = scheduler.getCyclesUntilEvent(EventType::TIMER_0_OVERFLOW);
    EXPECT_EQ(cyclesUntilTimer, 10ULL);
    
    uint64_t cyclesUntilVideo = scheduler.getCyclesUntilEvent(EventType::VIDEO_HBLANK);
    EXPECT_EQ(cyclesUntilVideo, 20ULL);
}

// Test 7: Canceling events by type
TEST_F(SchedulerTest, CancelEventsByType) {
    scheduler.schedule(10, [this]() { executionOrder.push_back(1); }, EventType::TIMER_0_OVERFLOW);
    scheduler.schedule(20, [this]() { executionOrder.push_back(2); }, EventType::VIDEO_HBLANK);
    scheduler.schedule(30, [this]() { executionOrder.push_back(3); }, EventType::TIMER_0_OVERFLOW);
    scheduler.schedule(40, [this]() { executionOrder.push_back(4); }, EventType::VIDEO_HBLANK);
    
    EXPECT_EQ(scheduler.getPendingEventCount(), 4UL);
    
    // Cancel all timer events
    scheduler.cancelEventsOfType(EventType::TIMER_0_OVERFLOW);
    
    EXPECT_EQ(scheduler.getPendingEventCount(), 2UL);
    EXPECT_FALSE(scheduler.hasEventsOfType(EventType::TIMER_0_OVERFLOW));
    EXPECT_TRUE(scheduler.hasEventsOfType(EventType::VIDEO_HBLANK));
    
    scheduler.runUntil(50);
    
    EXPECT_EQ(executionOrder.size(), 2UL);
    EXPECT_EQ(executionOrder[0], 2);  // Only video events remain
    EXPECT_EQ(executionOrder[1], 4);
}

// Test 8: Rescheduling from within event callback
TEST_F(SchedulerTest, ReschedulingFromCallback) {
    int counter = 0;
    
    std::function<void()> recurringEvent;
    recurringEvent = [&counter, &recurringEvent, this]() {
        counter++;
        if (counter < 5) {
            scheduler.schedule(10, recurringEvent);
        }
    };
    
    scheduler.schedule(10, recurringEvent);
    scheduler.runUntil(100);
    
    EXPECT_EQ(counter, 5);
}

// Test 9: Absolute vs relative cycle scheduling
TEST_F(SchedulerTest, AbsoluteCycleScheduling) {
    // Advance time to cycle 50
    scheduler.runUntil(50);
    
    // Schedule event at absolute cycle 100
    scheduler.scheduleAt(100, [this]() { executionOrder.push_back(1); });
    
    // Schedule event 30 cycles from now (should be cycle 80)
    scheduler.schedule(30, [this]() { executionOrder.push_back(2); });
    
    scheduler.runUntil(150);
    
    EXPECT_EQ(executionOrder.size(), 2UL);
    EXPECT_EQ(executionOrder[0], 2);  // Cycle 80
    EXPECT_EQ(executionOrder[1], 1);  // Cycle 100
}

// Test 10: Reset functionality
TEST_F(SchedulerTest, ResetFunctionality) {
    scheduler.schedule(10, [this]() { executionOrder.push_back(1); });
    scheduler.schedule(20, [this]() { executionOrder.push_back(2); });
    scheduler.schedule(30, [this]() { executionOrder.push_back(3); });
    
    scheduler.runUntil(15);
    EXPECT_EQ(executionOrder.size(), 1UL);
    EXPECT_EQ(scheduler.getCurrentCycle(), 15ULL);
    
    // Reset should clear everything
    scheduler.reset();
    
    EXPECT_EQ(scheduler.getCurrentCycle(), 0ULL);
    EXPECT_EQ(scheduler.getPendingEventCount(), 0UL);
    EXPECT_TRUE(scheduler.isEmpty());
    
    // Run to where remaining events would have been
    scheduler.runUntil(40);
    EXPECT_EQ(executionOrder.size(), 1UL);  // No new events executed
}

// Test 11: Video timing simulation
TEST_F(SchedulerTest, VideoTimingSimulation) {
    const uint32_t CYCLES_PER_SCANLINE = 1232;
    const uint32_t VISIBLE_SCANLINES = 160;
    
    int hblankCount = 0;
    int vblankCount = 0;
    
    // Schedule HBlank events for visible scanlines
    for (uint32_t scanline = 0; scanline < VISIBLE_SCANLINES; scanline++) {
        uint64_t hblankCycle = scanline * CYCLES_PER_SCANLINE + 960;
        scheduler.scheduleAt(hblankCycle, [&hblankCount]() {
            hblankCount++;
        }, EventType::VIDEO_HBLANK, 1);
    }
    
    // Schedule VBlank
    scheduler.scheduleAt(VISIBLE_SCANLINES * CYCLES_PER_SCANLINE, [&vblankCount]() {
        vblankCount++;
    }, EventType::VIDEO_VBLANK, 0);
    
    // Run one frame
    scheduler.runUntil(228 * CYCLES_PER_SCANLINE);
    
    EXPECT_EQ(hblankCount, 160);
    EXPECT_EQ(vblankCount, 1);
}

// Test 12: Timer simulation
TEST_F(SchedulerTest, TimerSimulation) {
    const uint32_t TIMER_OVERFLOW_CYCLES = 65536;
    
    int timerOverflows = 0;
    
    // Schedule recurring timer overflow
    std::function<void()> timerEvent;
    timerEvent = [&timerOverflows, &timerEvent, this]() {
        timerOverflows++;
        // Reschedule for next overflow
        scheduler.schedule(TIMER_OVERFLOW_CYCLES, timerEvent, EventType::TIMER_0_OVERFLOW);
    };
    
    // Initial scheduling
    scheduler.schedule(TIMER_OVERFLOW_CYCLES, timerEvent, EventType::TIMER_0_OVERFLOW);
    
    // Run for 5 overflow periods
    scheduler.runUntil(TIMER_OVERFLOW_CYCLES * 5);
    
    EXPECT_EQ(timerOverflows, 5);
}

// Test 13: Event statistics
TEST_F(SchedulerTest, EventStatistics) {
    scheduler.schedule(10, []() {});
    scheduler.schedule(20, []() {});
    scheduler.schedule(30, []() {});
    
    EXPECT_EQ(scheduler.getPendingEventCount(), 3UL);
    EXPECT_FALSE(scheduler.isEmpty());
    
    scheduler.runUntil(25);
    EXPECT_EQ(scheduler.getPendingEventCount(), 1UL);
    
    scheduler.runUntil(50);
    EXPECT_EQ(scheduler.getPendingEventCount(), 0UL);
    EXPECT_TRUE(scheduler.isEmpty());
}

// Test 14: Large cycle counts
TEST_F(SchedulerTest, LargeCycleCounts) {
    uint64_t largeCycle = 1000000000000ULL;  // 1 trillion
    
    scheduler.runUntil(largeCycle);
    
    bool eventExecuted = false;
    scheduler.schedule(1000, [&eventExecuted]() {
        eventExecuted = true;
    });
    
    scheduler.runUntil(largeCycle + 2000);
    EXPECT_TRUE(eventExecuted);
    EXPECT_EQ(scheduler.getCurrentCycle(), largeCycle + 2000);
}

// Test 15: Empty queue behavior
TEST_F(SchedulerTest, EmptyQueueBehavior) {
    EXPECT_TRUE(scheduler.isEmpty());
    EXPECT_EQ(scheduler.getPendingEventCount(), 0UL);
    EXPECT_EQ(scheduler.getCyclesUntilNextEvent(), UINT64_MAX);
    
    // Should handle running with no events
    scheduler.runUntil(1000);
    EXPECT_EQ(scheduler.getCurrentCycle(), 1000ULL);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
