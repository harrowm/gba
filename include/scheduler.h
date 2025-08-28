// Scheduler.h
#pragma once

#include <queue>
#include <functional>
#include <vector>
#include <cstdint>

struct ScheduledEvent {
    uint64_t triggerCycle;
    std::function<void()> callback;

    // For priority_queue: smallest triggerCycle first
    bool operator>(const ScheduledEvent& other) const {
        return triggerCycle > other.triggerCycle;
    }
};

class Scheduler {
public:
    Scheduler() : currentCycle(0) {}

    // Advance to the next event or up to targetCycle
    void runUntil(uint64_t targetCycle);

    // Schedule an event to occur at a future cycle
    void schedule(uint32_t cyclesFromNow, std::function<void()> callback);

    // Get current global cycle count
    uint64_t getCurrentCycle() const { return currentCycle; }

    // Reset the scheduler (e.g., on reset)
    void reset();

private:
    uint64_t currentCycle;
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>> eventQueue;
};