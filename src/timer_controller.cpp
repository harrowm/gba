#include "timer_controller.h"
#include "scheduler.h"
#include "interrupt.h"
#include "debug.h"

TimerController::TimerController() 
    : scheduler(nullptr), interruptController(nullptr) {
    reset();
}

void TimerController::reset() {
    for (int i = 0; i < 4; i++) {
        timers[i] = Timer();
    }
    DEBUG_INFO("TimerController reset");
}

void TimerController::writeReload(int timerID, uint16_t value) {
    if (timerID < 0 || timerID >= 4) return;
    
    Timer& timer = timers[timerID];
    timer.reload = value;
    
    // If timer is not enabled, also update counter
    if (!timer.enabled) {
        timer.counter = value;
    }
    
    DEBUG_INFO("Timer " + std::to_string(timerID) + " reload set to 0x" + 
               debug_to_hex_string(value, 4));
}

void TimerController::writeControl(int timerID, uint16_t value) {
    if (timerID < 0 || timerID >= 4) return;
    
    Timer& timer = timers[timerID];
    uint16_t oldControl = timer.control;
    timer.control = value;
    
    bool wasEnabled = (oldControl & TIMER_ENABLE) != 0;
    bool nowEnabled = (value & TIMER_ENABLE) != 0;
    
    DEBUG_INFO("Timer " + std::to_string(timerID) + " control = 0x" + 
               debug_to_hex_string(value, 4) + 
               " (prescaler=" + std::to_string(timer.getPrescaler()) + 
               ", enabled=" + std::to_string(nowEnabled) + 
               ", irq=" + std::to_string(timer.isIRQEnabled()) + 
               ", cascade=" + std::to_string(timer.isCountUpMode()) + ")");
    
    // Handle timer enable/disable transitions
    if (!wasEnabled && nowEnabled) {
        // Timer just enabled
        timer.enabled = true;
        timer.counter = timer.reload;
        
        // Schedule timer event (unless in cascade mode)
        if (!timer.isCountUpMode()) {
            scheduleTimer(timerID);
        }
        
        DEBUG_INFO("Timer " + std::to_string(timerID) + " started from 0x" + 
                   debug_to_hex_string(timer.reload, 4));
    } else if (wasEnabled && !nowEnabled) {
        // Timer just disabled
        timer.enabled = false;
        cancelTimer(timerID);
        
        DEBUG_INFO("Timer " + std::to_string(timerID) + " stopped");
    }
}

uint16_t TimerController::readCounter(int timerID) const {
    if (timerID < 0 || timerID >= 4) return 0;
    
    const Timer& timer = timers[timerID];
    
    if (!timer.enabled) {
        return timer.counter;
    }
    
    // For enabled timers in cascade mode, return current counter
    if (timer.isCountUpMode()) {
        return timer.counter;
    }
    
    // For enabled timers with prescaler, we need to calculate current value
    // based on elapsed cycles since last update
    // For now, just return the counter value (will be updated by scheduler events)
    return timer.counter;
}

uint16_t TimerController::readControl(int timerID) const {
    if (timerID < 0 || timerID >= 4) return 0;
    return timers[timerID].control;
}

void TimerController::scheduleTimer(int timerID) {
    if (!scheduler) return;
    if (timerID < 0 || timerID >= 4) return;
    
    Timer& timer = timers[timerID];
    
    // Calculate cycles until overflow
    // counter counts from reload value to 0xFFFF, then overflows
    uint32_t ticksUntilOverflow = 0x10000 - timer.counter;
    uint32_t prescalerValue = timer.getPrescalerValue();
    uint32_t cyclesUntilOverflow = ticksUntilOverflow * prescalerValue;
    
    timer.cyclesUntilOverflow = cyclesUntilOverflow;
    
    // Schedule overflow event
    EventType eventType = static_cast<EventType>(static_cast<int>(EventType::TIMER_0_OVERFLOW) + timerID);
    scheduler->schedule(cyclesUntilOverflow, 
                       [this, timerID]() { onTimerOverflow(timerID); },
                       eventType, 0);
    
    DEBUG_INFO("Timer " + std::to_string(timerID) + " scheduled: " + 
               std::to_string(ticksUntilOverflow) + " ticks * " + 
               std::to_string(prescalerValue) + " = " + 
               std::to_string(cyclesUntilOverflow) + " cycles");
}

void TimerController::cancelTimer(int timerID) {
    if (!scheduler) return;
    if (timerID < 0 || timerID >= 4) return;
    
    EventType eventType = static_cast<EventType>(static_cast<int>(EventType::TIMER_0_OVERFLOW) + timerID);
    scheduler->cancelEventsOfType(eventType);
    
    DEBUG_INFO("Timer " + std::to_string(timerID) + " cancelled");
}

void TimerController::onTimerOverflow(int timerID) {
    if (timerID < 0 || timerID >= 4) return;
    
    Timer& timer = timers[timerID];
    
    DEBUG_INFO("Timer " + std::to_string(timerID) + " overflow!");
    
    // Reload counter
    timer.counter = timer.reload;
    
    // Trigger interrupt if enabled
    if (timer.isIRQEnabled() && interruptController) {
        uint16_t irqFlag = IRQ_TIMER0 << timerID;
        interruptController->requestInterrupt(irqFlag);
        DEBUG_INFO("Timer " + std::to_string(timerID) + " triggered IRQ");
    }
    
    // Handle cascade mode: if next timer is in count-up mode, increment it
    if (timerID < 3) {
        Timer& nextTimer = timers[timerID + 1];
        if (nextTimer.enabled && nextTimer.isCountUpMode()) {
            nextTimer.counter++;
            if (nextTimer.counter == 0) {
                // Cascaded timer overflowed
                onTimerOverflow(timerID + 1);
            }
        }
    }
    
    // Reschedule this timer if it's still enabled
    if (timer.enabled && !timer.isCountUpMode()) {
        scheduleTimer(timerID);
    }
}

void TimerController::updateCounter(int timerID) {
    if (timerID < 0 || timerID >= 4) return;
    if (!scheduler) return;
    
    Timer& timer = timers[timerID];
    if (!timer.enabled || timer.isCountUpMode()) return;
    
    // This would calculate how much the counter has advanced since last update
    // For now, the scheduler event-driven approach handles this automatically
}
