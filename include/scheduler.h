// Scheduler.h
#pragma once

#include <queue>
#include <functional>
#include <vector>
#include <cstdint>
#include <string>

// Event types for different system components
enum class EventType {
    NONE = 0,
    TIMER_0_OVERFLOW,
    TIMER_1_OVERFLOW,
    TIMER_2_OVERFLOW,
    TIMER_3_OVERFLOW,
    VIDEO_HBLANK,
    VIDEO_VBLANK,
    VIDEO_SCANLINE,
    DMA_TRANSFER,
    AUDIO_SAMPLE,
    CUSTOM
};

struct ScheduledEvent {
    uint64_t triggerCycle;
    EventType type;
    std::function<void()> callback;
    int priority;  // Lower number = higher priority (for same cycle)

    // For priority_queue: smallest triggerCycle first, then highest priority
    bool operator>(const ScheduledEvent& other) const {
        if (triggerCycle != other.triggerCycle) {
            return triggerCycle > other.triggerCycle;
        }
        // For same cycle, higher priority value = lower priority
        return priority > other.priority;
    }
};

class Scheduler {
public:
    Scheduler() : currentCycle(0), eventsProcessed(0) {}

    // Advance to the next event or up to targetCycle
    void runUntil(uint64_t targetCycle);

    // Schedule an event to occur at a future cycle
    void schedule(uint32_t cyclesFromNow, std::function<void()> callback, 
                  EventType type = EventType::CUSTOM, int priority = 0);
    
    // Schedule an event at an absolute cycle time
    void scheduleAt(uint64_t absoluteCycle, std::function<void()> callback,
                    EventType type = EventType::CUSTOM, int priority = 0);

    // Cancel all events of a specific type
    void cancelEventsOfType(EventType type);

    // Check if there are any events of a specific type
    bool hasEventsOfType(EventType type) const;

    // Get cycles until next event of any type
    uint64_t getCyclesUntilNextEvent() const;
    
    // Get cycles until next event of specific type
    uint64_t getCyclesUntilEvent(EventType type) const;

    // Get current global cycle count
    uint64_t getCurrentCycle() const { return currentCycle; }
    
    // Get total events processed (for debugging/stats)
    uint64_t getEventsProcessed() const { return eventsProcessed; }

    // Check if event queue is empty
    bool isEmpty() const { return eventQueue.empty(); }
    
    // Get number of pending events
    size_t getPendingEventCount() const { return eventQueue.size(); }

    // Reset the scheduler (e.g., on reset)
    void reset();

    // Advance cycle counter without processing events (use with caution)
    void advanceCycles(uint32_t cycles) { currentCycle += cycles; }
    
    // Set cycle counter to specific value (use for frame timing corrections)
    void setCurrentCycle(uint64_t cycle) { currentCycle = cycle; }

private:
    uint64_t currentCycle;
    uint64_t eventsProcessed;
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>> eventQueue;
};