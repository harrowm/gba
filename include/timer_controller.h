#ifndef TIMER_CONTROLLER_H
#define TIMER_CONTROLLER_H

#include <cstdint>
#include <functional>

// Forward declarations
class Scheduler;
class InterruptController;
class Memory;

// Timer control register bits
constexpr uint16_t TIMER_ENABLE = (1 << 7);      // Timer enable
constexpr uint16_t TIMER_IRQ_ENABLE = (1 << 6);  // IRQ enable on overflow
constexpr uint16_t TIMER_COUNT_UP = (1 << 2);    // Count-up timing (cascade mode)
constexpr uint16_t TIMER_PRESCALER_MASK = 0x03;  // Prescaler selection bits (0-3)

// Timer register addresses
constexpr uint32_t TM0CNT_L = 0x04000100;  // Timer 0 Counter/Reload
constexpr uint32_t TM0CNT_H = 0x04000102;  // Timer 0 Control
constexpr uint32_t TM1CNT_L = 0x04000104;  // Timer 1 Counter/Reload
constexpr uint32_t TM1CNT_H = 0x04000106;  // Timer 1 Control
constexpr uint32_t TM2CNT_L = 0x04000108;  // Timer 2 Counter/Reload
constexpr uint32_t TM2CNT_H = 0x0400010A;  // Timer 2 Control
constexpr uint32_t TM3CNT_L = 0x0400010C;  // Timer 3 Counter/Reload
constexpr uint32_t TM3CNT_H = 0x0400010E;  // Timer 3 Control

// Prescaler values: how many CPU cycles per timer tick
constexpr uint32_t PRESCALER_VALUES[4] = {1, 64, 256, 1024};

class Timer {
public:
    Timer() : counter(0), reload(0), control(0), 
              cyclesUntilOverflow(0), lastReloadCycle(0), enabled(false) {}
    
    uint16_t counter;              // Current counter value
    uint16_t reload;               // Reload value (written to TM_CNT_L)
    uint16_t control;              // Control register (TM_CNT_H)
    uint32_t cyclesUntilOverflow;  // Cycles remaining until overflow
    uint64_t lastReloadCycle;      // CPU cycle when counter was last set to reload
    bool enabled;                  // Is timer currently running
    
    // Helper methods
    uint8_t getPrescaler() const { return control & TIMER_PRESCALER_MASK; }
    bool isIRQEnabled() const { return (control & TIMER_IRQ_ENABLE) != 0; }
    bool isCountUpMode() const { return (control & TIMER_COUNT_UP) != 0; }
    uint32_t getPrescalerValue() const { return PRESCALER_VALUES[getPrescaler()]; }
};

class TimerController {
public:
    TimerController();
    
    // Setup dependencies
    void setScheduler(Scheduler* sched) { scheduler = sched; }
    void setInterruptController(InterruptController* ic) { interruptController = ic; }
    void setMemory(Memory* mem) { memory = mem; }
    
    // Timer control
    void writeControl(int timerID, uint16_t value);
    void writeReload(int timerID, uint16_t value);
    uint16_t readCounter(int timerID) const;
    uint16_t readControl(int timerID) const;
    
    // Timer event callbacks (called by scheduler)
    void onTimerOverflow(int timerID);
    
    // Set external overflow callback (for APU FIFO timing)
    void setTimerOverflowCallback(std::function<void(int)> callback) {
        timerOverflowCallback = callback;
    }
    
    // Reset all timers
    void reset();
    
private:
    Timer timers[4];
    Scheduler* scheduler;
    InterruptController* interruptController;
    Memory* memory;
    std::function<void(int)> timerOverflowCallback;
    
    // Helper methods
    void scheduleTimer(int timerID);
    void cancelTimer(int timerID);
    void updateCounter(int timerID);  // Sync counter value with elapsed cycles
};

#endif // TIMER_CONTROLLER_H
