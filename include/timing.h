#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

// ============================================================================
// GBA timing constants (reference values)
// ============================================================================
// The live timing system uses Scheduler + Memory wait-state tables.
// These constants are retained as convenient reference definitions.

// GBA system clock frequency: 16.78MHz
#define GBA_CLOCK_FREQUENCY 16780000
#define CYCLES_PER_SECOND GBA_CLOCK_FREQUENCY

// Timer prescaler dividers
#define TIMER_FREQ_1    GBA_CLOCK_FREQUENCY         // F/1: 16.78MHz
#define TIMER_FREQ_64   (GBA_CLOCK_FREQUENCY / 64)  // F/64: 262.5kHz
#define TIMER_FREQ_256  (GBA_CLOCK_FREQUENCY / 256) // F/256: 65.625kHz
#define TIMER_FREQ_1024 (GBA_CLOCK_FREQUENCY / 1024)// F/1024: 16.384kHz

// Timer overflow periods (in cycles, from 0x0000 to 0x0000)
#define TIMER_OVERFLOW_CYCLES_1    65536
#define TIMER_OVERFLOW_CYCLES_64   (65536 * 64)
#define TIMER_OVERFLOW_CYCLES_256  (65536 * 256)
#define TIMER_OVERFLOW_CYCLES_1024 (65536 * 1024)

// Convert between different time units
#define CYCLES_TO_MICROSECONDS(cycles) ((cycles * 1000000) / GBA_CLOCK_FREQUENCY)
#define MICROSECONDS_TO_CYCLES(us) ((us * GBA_CLOCK_FREQUENCY) / 1000000)
#define CYCLES_TO_MILLISECONDS(cycles) ((cycles * 1000) / GBA_CLOCK_FREQUENCY)
#define MILLISECONDS_TO_CYCLES(ms) ((ms * GBA_CLOCK_FREQUENCY) / 1000)

#endif // TIMING_H
