#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

// GBA system clock frequency: 16.78MHz
#define GBA_CLOCK_FREQUENCY 16780000
#define CYCLES_PER_SECOND GBA_CLOCK_FREQUENCY

// Timer frequencies based on prescaler settings
#define TIMER_FREQ_1    GBA_CLOCK_FREQUENCY         // F/1: 16.78MHz
#define TIMER_FREQ_64   (GBA_CLOCK_FREQUENCY / 64)  // F/64: 262.5kHz
#define TIMER_FREQ_256  (GBA_CLOCK_FREQUENCY / 256) // F/256: 65.625kHz
#define TIMER_FREQ_1024 (GBA_CLOCK_FREQUENCY / 1024)// F/1024: 16.384kHz

// Memory access timing constants (in CPU cycles)
// Based on GBA Technical Data from the documentation

// Internal memory (fast, 32-bit bus)
#define CYCLES_BIOS_32      1  // BIOS ROM access
#define CYCLES_WRAM_32K_32  1  // Work RAM 32K (on-chip)
#define CYCLES_IO_32        1  // I/O registers
#define CYCLES_OAM_32       1  // OAM access
#define CYCLES_PALETTE_32   1  // Palette RAM

// External memory (slower, 16-bit bus)
#define CYCLES_WRAM_256K_32 6  // Work RAM 256K (on-board) - 3/3/6 cycles for 8/16/32bit
#define CYCLES_VRAM_32      2  // VRAM access - 1/1/2 cycles + potential video controller conflicts

// GamePak memory (configurable wait states)
#define CYCLES_GAMEPAK_ROM_DEFAULT_N    5  // Non-sequential (4+1 wait states default)
#define CYCLES_GAMEPAK_ROM_DEFAULT_S    3  // Sequential (2+1 wait states default)
#define CYCLES_GAMEPAK_SRAM             5  // SRAM access (8-bit only)

// Video memory additional wait states
#define CYCLES_VIDEO_CONFLICT           1  // Additional cycle when video controller accesses simultaneously

// DMA transfer timing
#define CYCLES_DMA_SETUP               2  // DMA setup overhead
#define CYCLES_DMA_INTERNAL            2  // Internal processing per transfer

// CPU instruction timing base
#define CYCLES_ARM_FETCH               1  // ARM instruction fetch
#define CYCLES_THUMB_FETCH             1  // Thumb instruction fetch
#define CYCLES_BRANCH_TAKEN            3  // Branch instruction timing
#define CYCLES_MULTIPLY_BASE           1  // Base multiply cycles
#define CYCLES_MULTIPLY_PER_BYTE       1  // Additional cycles per significant byte

// Timer overflow periods (in cycles)
// Time until 16-bit timer overflows from 0x0000 to 0x0000
#define TIMER_OVERFLOW_CYCLES_1    65536
#define TIMER_OVERFLOW_CYCLES_64   (65536 * 64)
#define TIMER_OVERFLOW_CYCLES_256  (65536 * 256)
#define TIMER_OVERFLOW_CYCLES_1024 (65536 * 1024)

// Convert between different time units
#define CYCLES_TO_MICROSECONDS(cycles) ((cycles * 1000000) / GBA_CLOCK_FREQUENCY)
#define MICROSECONDS_TO_CYCLES(us) ((us * GBA_CLOCK_FREQUENCY) / 1000000)
#define CYCLES_TO_MILLISECONDS(cycles) ((cycles * 1000) / GBA_CLOCK_FREQUENCY)
#define MILLISECONDS_TO_CYCLES(ms) ((ms * GBA_CLOCK_FREQUENCY) / 1000)

// Common timing calculations
#define TIMER_PERIOD_MICROSECONDS(prescaler) CYCLES_TO_MICROSECONDS(65536 * prescaler)
#define TIMER_FREQUENCY_HZ(prescaler) (GBA_CLOCK_FREQUENCY / (65536 * prescaler))

// Struct to track timing information
typedef struct {
    uint64_t total_cycles;      // Total CPU cycles executed
    uint32_t current_scanline;  // Current video scanline (0-227)
    uint32_t scanline_cycles;   // Cycles within current scanline
    uint32_t timer_cycles[4];   // Cycle counters for each timer
} TimingState;

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes
void timing_init(TimingState* timing);
void timing_advance(TimingState* timing, uint32_t cycles);
uint32_t timing_get_timer_frequency(uint8_t prescaler);
uint32_t timing_calculate_memory_cycles(uint32_t address, uint8_t access_size);
void timing_update_video(TimingState* timing);

// Video timing functions
uint32_t timing_get_vcount(TimingState* timing);
int timing_in_vblank(TimingState* timing);
int timing_in_hblank(TimingState* timing);

// New functions for cycle-driven execution
uint32_t timing_cycles_until_next_timer_event(TimingState* timing);
uint32_t timing_cycles_until_next_video_event(TimingState* timing);
uint32_t timing_cycles_until_next_event(TimingState* timing);
void timing_process_timer_events(TimingState* timing);
void timing_process_video_events(TimingState* timing);

#ifdef __cplusplus
}
#endif

#endif // TIMING_H
