// Scheduler.cpp
#include "scheduler.h"
#include <vector>
#include <algorithm>

void Scheduler::runUntil(uint64_t targetCycle) {
    while (!eventQueue.empty() && eventQueue.top().triggerCycle <= targetCycle) {
        auto event = eventQueue.top();
        eventQueue.pop();

        // Advance current cycle to the event's trigger time
        currentCycle = event.triggerCycle;

        // Execute the callback
        if (event.callback) {
            event.callback();
        }
        
        eventsProcessed++;
    }

    // Finally, advance current cycle to target (if no earlier event)
    if (currentCycle < targetCycle) {
        currentCycle = targetCycle;
    }
}

void Scheduler::schedule(uint32_t cyclesFromNow, std::function<void()> callback, 
                        EventType type, int priority) {
    uint64_t triggerCycle = currentCycle + cyclesFromNow;
    eventQueue.push(ScheduledEvent{
        triggerCycle,
        type,
        std::move(callback),
        priority
    });
}

void Scheduler::scheduleAt(uint64_t absoluteCycle, std::function<void()> callback,
                           EventType type, int priority) {
    eventQueue.push(ScheduledEvent{
        absoluteCycle,
        type,
        std::move(callback),
        priority
    });
}

void Scheduler::cancelEventsOfType(EventType type) {
    // Create temporary queue to rebuild without cancelled events
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>> newQueue;
    
    while (!eventQueue.empty()) {
        auto event = eventQueue.top();
        eventQueue.pop();
        
        // Keep events that don't match the type
        if (event.type != type) {
            newQueue.push(event);
        }
    }
    
    // Replace the old queue with the filtered one
    eventQueue = std::move(newQueue);
}

bool Scheduler::hasEventsOfType(EventType type) const {
    // We need to iterate through the queue to check
    // This is inefficient but priority_queue doesn't provide direct access
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>> tempQueue = eventQueue;
    
    while (!tempQueue.empty()) {
        if (tempQueue.top().type == type) {
            return true;
        }
        tempQueue.pop();
    }
    
    return false;
}

uint64_t Scheduler::getCyclesUntilNextEvent() const {
    if (eventQueue.empty()) {
        return UINT64_MAX;
    }
    
    uint64_t nextEventCycle = eventQueue.top().triggerCycle;
    if (nextEventCycle <= currentCycle) {
        return 0;
    }
    
    return nextEventCycle - currentCycle;
}

uint64_t Scheduler::getCyclesUntilEvent(EventType type) const {
    uint64_t minCycles = UINT64_MAX;
    
    // Create a temporary copy to search
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>> tempQueue = eventQueue;
    
    while (!tempQueue.empty()) {
        auto event = tempQueue.top();
        tempQueue.pop();
        
        if (event.type == type) {
            uint64_t cyclesUntil = (event.triggerCycle <= currentCycle) ? 0 : (event.triggerCycle - currentCycle);
            minCycles = std::min(minCycles, cyclesUntil);
        }
    }
    
    return minCycles;
}

void Scheduler::reset() {
    currentCycle = 0;
    eventsProcessed = 0;
    while (!eventQueue.empty()) {
        eventQueue.pop();
    }
}