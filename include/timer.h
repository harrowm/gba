#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "timing.h"

// Timer control register bits
#define TIMER_ENABLE        (1 << 7)  // Timer enable
#define TIMER_IRQ_ENABLE    (1 << 6)  // IRQ enable
#define TIMER_COUNT_UP      (1 << 2)  // Count-up timing (not for Timer 0)
#define TIMER_PRESCALER_MASK 0x03     // Prescaler selection bits

// Timer register addresses
#define TM0CNT_L    0x04000100  // Timer 0 Counter/Reload
#define TM0CNT_H    0x04000102  // Timer 0 Control
#define TM1CNT_L    0x04000104  // Timer 1 Counter/Reload
#define TM1CNT_H    0x04000106  // Timer 1 Control
#define TM2CNT_L    0x04000108  // Timer 2 Counter/Reload
#define TM2CNT_H    0x0400010A  // Timer 2 Control
#define TM3CNT_L    0x0400010C  // Timer 3 Counter/Reload
#define TM3CNT_H    0x0400010E  // Timer 3 Control

// Timer state structure
typedef struct {
    uint16_t counter;           // Current counter value
    uint16_t reload;            // Reload value
    uint16_t control;           // Control register value
    uint32_t cycles_remaining;  // Cycles until next increment
    uint8_t prescaler;          // Prescaler setting (0-3)
    uint8_t enabled;            // Timer enabled flag
    uint8_t irq_enabled;        // IRQ enabled flag
    uint8_t count_up;           // Count-up mode flag
} Timer;

// Timer controller state
typedef struct {
    Timer timers[4];            // Four hardware timers
    uint32_t last_update_cycles; // Last timing update
} TimerController;

// Function prototypes
void timer_init(TimerController* controller);
void timer_update(TimerController* controller, TimingState* timing);
void timer_write_control(TimerController* controller, int timer_id, uint16_t value);
void timer_write_reload(TimerController* controller, int timer_id, uint16_t value);
uint16_t timer_read_counter(TimerController* controller, int timer_id);
uint16_t timer_read_control(TimerController* controller, int timer_id);

// Internal helper functions
static void timer_overflow(TimerController* controller, int timer_id);
static uint32_t timer_get_increment_cycles(uint8_t prescaler);
static void timer_reset(Timer* timer);

#endif // TIMER_H
