#include "timer.h"
#include <string.h>

// Forward declarations for static functions
static void timer_overflow(TimerController* controller, int timer_id);
static uint32_t timer_get_increment_cycles(uint8_t prescaler);
static void timer_reset(Timer* timer);

// Initialize timer controller
void timer_init(TimerController* controller) {
    if (!controller) return;
    
    memset(controller, 0, sizeof(TimerController));
    
    // Initialize all timers to default state
    for (int i = 0; i < 4; i++) {
        timer_reset(&controller->timers[i]);
    }
    
    controller->last_update_cycles = 0;
}

// Update all timers based on elapsed cycles
void timer_update(TimerController* controller, TimingState* timing) {
    if (!controller || !timing) return;
    
    uint32_t elapsed_cycles = (uint32_t)(timing->total_cycles - controller->last_update_cycles);
    controller->last_update_cycles = timing->total_cycles;
    
    // Update each timer
    for (int i = 0; i < 4; i++) {
        Timer* timer = &controller->timers[i];
        
        if (!timer->enabled) continue;
        
        if (timer->count_up && i > 0) {
            // Count-up mode: timer increments when previous timer overflows
            // This is handled in timer_overflow() function
            continue;
        }
        
        // Normal timing mode
        uint32_t cycles_to_process = elapsed_cycles;
        
        while (cycles_to_process > 0) {
            if (timer->cycles_remaining <= cycles_to_process) {
                // Timer will increment
                cycles_to_process -= timer->cycles_remaining;
                timer->counter++;
                
                // Check for overflow
                if (timer->counter == 0) {
                    timer_overflow(controller, i);
                }
                
                // Reset cycles for next increment
                timer->cycles_remaining = timer_get_increment_cycles(timer->prescaler);
            } else {
                // Not enough cycles for increment
                timer->cycles_remaining -= cycles_to_process;
                cycles_to_process = 0;
            }
        }
    }
}

// Write to timer control register
void timer_write_control(TimerController* controller, int timer_id, uint16_t value) {
    if (!controller || timer_id < 0 || timer_id > 3) return;
    
    Timer* timer = &controller->timers[timer_id];
    uint16_t old_control = timer->control;
    
    timer->control = value;
    timer->prescaler = value & TIMER_PRESCALER_MASK;
    timer->irq_enabled = (value & TIMER_IRQ_ENABLE) != 0;
    timer->count_up = (value & TIMER_COUNT_UP) != 0 && timer_id > 0; // Count-up not available for Timer 0
    
    uint8_t new_enabled = (value & TIMER_ENABLE) != 0;
    uint8_t was_enabled = timer->enabled;
    timer->enabled = new_enabled;
    
    // Handle enable state changes
    if (new_enabled && !was_enabled) {
        // Timer being enabled
        timer->counter = timer->reload;
        timer->cycles_remaining = timer_get_increment_cycles(timer->prescaler);
    } else if (!new_enabled && was_enabled) {
        // Timer being disabled
        timer->cycles_remaining = 0;
    }
    
    // Handle reload value change when timer start bit changes from 0 to 1
    if ((value & TIMER_ENABLE) && !(old_control & TIMER_ENABLE)) {
        timer->counter = timer->reload;
    }
}

// Write to timer reload register
void timer_write_reload(TimerController* controller, int timer_id, uint16_t value) {
    if (!controller || timer_id < 0 || timer_id > 3) return;
    
    Timer* timer = &controller->timers[timer_id];
    timer->reload = value;
    
    // If timer is being started (writing reload while disabled), 
    // the reload value becomes the current counter
    if (!timer->enabled) {
        timer->counter = timer->reload;
    }
}

// Read timer counter value
uint16_t timer_read_counter(TimerController* controller, int timer_id) {
    if (!controller || timer_id < 0 || timer_id > 3) return 0;
    
    return controller->timers[timer_id].counter;
}

// Read timer control register
uint16_t timer_read_control(TimerController* controller, int timer_id) {
    if (!controller || timer_id < 0 || timer_id > 3) return 0;
    
    return controller->timers[timer_id].control;
}

// Internal: Handle timer overflow
static void timer_overflow(TimerController* controller, int timer_id) {
    Timer* timer = &controller->timers[timer_id];
    
    // Reload counter
    timer->counter = timer->reload;
    
    // Trigger IRQ if enabled
    if (timer->irq_enabled) {
        // In a complete implementation, this would trigger the appropriate IRQ
        // For now, we'll just note that an IRQ should be triggered
        // interrupt_request(IRQ_TIMER0 + timer_id);
    }
    
    // Handle count-up for next timer
    if (timer_id < 3) {
        Timer* next_timer = &controller->timers[timer_id + 1];
        if (next_timer->enabled && next_timer->count_up) {
            next_timer->counter++;
            if (next_timer->counter == 0) {
                timer_overflow(controller, timer_id + 1);
            }
        }
    }
}

// Internal: Get cycles needed for timer increment based on prescaler
static uint32_t timer_get_increment_cycles(uint8_t prescaler) {
    switch (prescaler) {
        case 0: return 1;      // F/1
        case 1: return 64;     // F/64
        case 2: return 256;    // F/256
        case 3: return 1024;   // F/1024
        default: return 1;
    }
}

// Internal: Reset timer to default state
static void timer_reset(Timer* timer) {
    timer->counter = 0;
    timer->reload = 0;
    timer->control = 0;
    timer->cycles_remaining = 0;
    timer->prescaler = 0;
    timer->enabled = 0;
    timer->irq_enabled = 0;
    timer->count_up = 0;
}

// Utility functions for specific timer operations

// Check if timer would overflow in next N cycles
int timer_will_overflow(TimerController* controller, int timer_id, uint32_t cycles) {
    if (!controller || timer_id < 0 || timer_id > 3) return 0;
    
    Timer* timer = &controller->timers[timer_id];
    if (!timer->enabled || timer->count_up) return 0;
    
    uint32_t remaining_counts = 0x10000 - timer->counter;
    uint32_t cycles_per_count = timer_get_increment_cycles(timer->prescaler);
    uint32_t cycles_to_overflow = remaining_counts * cycles_per_count - 
                                  (cycles_per_count - timer->cycles_remaining);
    
    return cycles >= cycles_to_overflow;
}

// Get cycles until next timer overflow
uint32_t timer_cycles_until_overflow(TimerController* controller, int timer_id) {
    if (!controller || timer_id < 0 || timer_id > 3) return 0;
    
    Timer* timer = &controller->timers[timer_id];
    if (!timer->enabled || timer->count_up) return 0;
    
    uint32_t remaining_counts = 0x10000 - timer->counter;
    uint32_t cycles_per_count = timer_get_increment_cycles(timer->prescaler);
    
    return remaining_counts * cycles_per_count - (cycles_per_count - timer->cycles_remaining);
}

// Force timer overflow (useful for testing or specific timing requirements)
void timer_force_overflow(TimerController* controller, int timer_id) {
    if (!controller || timer_id < 0 || timer_id > 3) return;
    
    Timer* timer = &controller->timers[timer_id];
    if (!timer->enabled) return;
    
    timer->counter = 0;
    timer_overflow(controller, timer_id);
}
