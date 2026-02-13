#include "timer_controller.h"
#include "scheduler.h"
#include "interrupt.h"
#include "memory.h"
#include "debug.h"

TimerController::TimerController() 
    : scheduler(nullptr), interruptController(nullptr), memory(nullptr) {
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
        
        // Match mGBA: lastEvent = mTimingCurrentTime() & ~tickMask
        // Use raw getCurrentCycle() (instruction-start time) with prescaler
        // alignment. mGBA's mTimingCurrentTime() includes in-progress costs,
        // but after prescaler rounding the result is equivalent.
        uint64_t effectiveCycle = scheduler ? scheduler->getCurrentCycle() : 0;
        uint32_t prescalerValue = timer.getPrescalerValue();
        if (prescalerValue > 1) {
            effectiveCycle = effectiveCycle - (effectiveCycle % prescalerValue);
        }
        timer.lastReloadCycle = effectiveCycle;
        
        if (!timer.isCountUpMode()) {
            scheduleTimer(timerID);
        }
        
        DEBUG_INFO("Timer " + std::to_string(timerID) + " started from 0x" + 
                   debug_to_hex_string(timer.reload, 4));
    } else if (wasEnabled && !nowEnabled) {
        // Timer just disabled — freeze counter at current interpolated value.
        // Without this, readCounter() on a disabled timer returns the stale
        // reload value from the last overflow, not the in-progress count.
        // IMPORTANT: Use OLD control word for prescaler/cascade since timer.control
        // was already updated to the new (disabled) value above.
        bool wasCountUp = (oldControl & TIMER_COUNT_UP) != 0;
        if (scheduler && !wasCountUp) {
            // Use raw getCurrentCycle() for the disable freeze point.
            uint64_t currentCycle = scheduler->getCurrentCycle();
            uint64_t elapsed = (currentCycle >= timer.lastReloadCycle)
                             ? (currentCycle - timer.lastReloadCycle) : 0;
            static constexpr uint32_t prescalerLUT[] = {1, 64, 256, 1024};
            uint32_t prescaler = prescalerLUT[oldControl & 0x3];
            uint64_t elapsedTicks = elapsed / prescaler;
            uint32_t periodTicks = 0x10000 - timer.reload;
            uint32_t ticksInPeriod = (periodTicks > 0)
                                   ? static_cast<uint32_t>(elapsedTicks % periodTicks)
                                   : 0;
            timer.counter = static_cast<uint16_t>(timer.reload + ticksInPeriod);
        }
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
    
    // Interpolate counter value based on elapsed cycles since last reload.
    // On real hardware the counter increments every (prescaler) CPU cycles.
    // Our scheduler only updates timer.counter on overflow events, so between
    // overflows we must compute the current value for any mid-frame reads.
    if (scheduler) {
        uint64_t currentCycle = scheduler->getCurrentCycle();
        
        // mGBA subtracts 2 from mTimingCurrentTime() when reading timers:
        //   GBATimerUpdateRegister(gba, timer, 2)
        // mTimingCurrentTime() includes instruction fetch progress (~1 cycle
        // for IWRAM/BIOS), while our getCurrentCycle() is at instruction start.
        // Net: mGBA reads at (base + fetchCost - 2) ≈ (base - 1) for fast mem.
        // We match with (base + 0 - 2) = (base - 2).
        if (currentCycle >= timer.lastReloadCycle + 2) {
            currentCycle -= 2;
        } else {
            currentCycle = timer.lastReloadCycle;
        }
        
        uint64_t elapsed = (currentCycle >= timer.lastReloadCycle)
                         ? (currentCycle - timer.lastReloadCycle) : 0;
        uint32_t prescaler = timer.getPrescalerValue();
        uint64_t elapsedTicks = elapsed / prescaler;
        
        // Counter = reload + elapsed ticks, with period wrapping
        uint32_t periodTicks = 0x10000 - timer.reload;
        uint32_t ticksInPeriod = (periodTicks > 0)
                               ? static_cast<uint32_t>(elapsedTicks % periodTicks)
                               : 0;
        
        return static_cast<uint16_t>(timer.reload + ticksInPeriod);
    }
    
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
    
    // Schedule overflow event at absolute time relative to lastReloadCycle.
    // This matches mGBA's GBATimerUpdateRegister which uses scheduleAbsolute:
    //   mTimingScheduleAbsolute(timing, event, currentTime + tickIncrement)
    // where currentTime = lastEvent (aligned). Using absolute scheduling from
    // lastReloadCycle ensures the overflow grid is consistent regardless of
    // when getCurrentCycle() gets advanced.
    EventType eventType = static_cast<EventType>(static_cast<int>(EventType::TIMER_0_OVERFLOW) + timerID);
    uint64_t overflowCycle = timer.lastReloadCycle + cyclesUntilOverflow;
    scheduler->scheduleAt(overflowCycle,
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
    timer.lastReloadCycle = scheduler ? scheduler->getCurrentCycle() : 0;
    
    // Trigger interrupt if enabled
    if (timer.isIRQEnabled() && interruptController) {
        uint16_t irqFlag = IRQ_TIMER0 << timerID;
        interruptController->requestInterrupt(irqFlag);
        DEBUG_INFO("Timer " + std::to_string(timerID) + " triggered IRQ");
    }
    
    // Call external overflow callback (for APU FIFO timing)
    if (timerOverflowCallback) {
        timerOverflowCallback(timerID);
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
