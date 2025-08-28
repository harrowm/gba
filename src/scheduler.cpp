// Scheduler.cpp
#include "Scheduler.h"

void Scheduler::runUntil(uint64_t targetCycle) {
    while (!eventQueue.empty() && eventQueue.top().triggerCycle <= targetCycle) {
        auto event = eventQueue.top();
        eventQueue.pop();

        // Advance current cycle to the event's trigger time
        currentCycle = event.triggerCycle;

        // Execute the callback
        event.callback();
    }

    // Finally, advance current cycle to target (if no earlier event)
    if (currentCycle < targetCycle) {
        currentCycle = targetCycle;
    }
}

void Scheduler::schedule(uint32_t cyclesFromNow, std::function<void()> callback) {
    uint64_t triggerCycle = currentCycle + cyclesFromNow;
    eventQueue.emplace(ScheduledEvent{triggerCycle, std::move(callback)});
}

void Scheduler::reset() {
    currentCycle = 0;
    while (!eventQueue.empty()) {
        eventQueue.pop();
    }
}