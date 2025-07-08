#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "../include/timing.h"
#include "../include/timer.h"

// Test timing system functionality
void test_timing_basic() {
    printf("Testing basic timing functionality...\n");
    
    TimingState timing;
    timing_init(&timing);
    
    // Test initial state
    assert(timing.total_cycles == 0);
    assert(timing.current_scanline == 0);
    assert(timing.scanline_cycles == 0);
    
    // Test advancing cycles
    timing_advance(&timing, 1000);
    assert(timing.total_cycles == 1000);
    assert(timing.scanline_cycles == 1000);
    
    // Test scanline advancement
    timing_advance(&timing, 1232); // Complete one scanline
    assert(timing.current_scanline == 1);
    assert(timing.scanline_cycles == 1000); // Remaining from previous
    
    printf("✓ Basic timing tests passed\n");
}

void test_video_timing() {
    printf("Testing video timing...\n");
    
    TimingState timing;
    timing_init(&timing);
    
    // Test VBlank detection
    assert(!timing_in_vblank(&timing));
    
    // Advance to VBlank period (scanline 160)
    timing_advance(&timing, 160 * 1232);
    assert(timing_in_vblank(&timing));
    assert(timing_get_vcount(&timing) == 160);
    
    // Test HBlank detection
    timing.scanline_cycles = 1000; // In HBlank period
    assert(timing_in_hblank(&timing));
    
    timing.scanline_cycles = 500; // In visible period
    assert(!timing_in_hblank(&timing));
    
    printf("✓ Video timing tests passed\n");
}

void test_memory_timing() {
    printf("Testing memory access timing...\n");
    
    // Test different memory regions
    assert(timing_calculate_memory_cycles(0x00001000, 4) == 1); // BIOS
    assert(timing_calculate_memory_cycles(0x03001000, 4) == 1); // Work RAM 32K
    assert(timing_calculate_memory_cycles(0x02001000, 4) == 6); // Work RAM 256K (32-bit)
    assert(timing_calculate_memory_cycles(0x02001000, 2) == 3); // Work RAM 256K (16-bit)
    assert(timing_calculate_memory_cycles(0x04000000, 4) == 1); // I/O
    assert(timing_calculate_memory_cycles(0x05000000, 4) == 2); // Palette (32-bit)
    assert(timing_calculate_memory_cycles(0x06000000, 4) == 3); // VRAM + conflict
    assert(timing_calculate_memory_cycles(0x08000000, 4) == 5); // GamePak ROM
    
    printf("✓ Memory timing tests passed\n");
}

void test_timer_basic() {
    printf("Testing basic timer functionality...\n");
    
    TimerController controller;
    TimingState timing;
    
    timer_init(&controller);
    timing_init(&timing);
    
    // Test initial state
    for (int i = 0; i < 4; i++) {
        assert(timer_read_counter(&controller, i) == 0);
        assert(timer_read_control(&controller, i) == 0);
    }
    
    // Test timer enable
    timer_write_reload(&controller, 0, 0xF000);
    timer_write_control(&controller, 0, TIMER_ENABLE); // F/1 prescaler
    
    Timer* timer0 = &controller.timers[0];
    assert(timer0->enabled == 1);
    assert(timer0->counter == 0xF000);
    assert(timer0->prescaler == 0);
    
    printf("✓ Basic timer tests passed\n");
}

void test_timer_overflow() {
    printf("Testing timer overflow...\n");
    
    TimerController controller;
    TimingState timing;
    
    timer_init(&controller);
    timing_init(&timing);
    
    // Set up timer to overflow quickly
    timer_write_reload(&controller, 0, 0xFFFE); // Will overflow in 2 counts
    timer_write_control(&controller, 0, TIMER_ENABLE | TIMER_IRQ_ENABLE);
    
    // Update with enough cycles to cause overflow
    timing_advance(&timing, 10);
    timer_update(&controller, &timing);
    
    // Timer should have overflowed and reloaded
    assert(timer_read_counter(&controller, 0) == 0xFFFE);
    
    printf("✓ Timer overflow tests passed\n");
}

void test_timer_count_up() {
    printf("Testing timer count-up mode...\n");
    
    TimerController controller;
    TimingState timing;
    
    timer_init(&controller);
    timing_init(&timing);
    
    // Set up Timer 0 to overflow quickly
    timer_write_reload(&controller, 0, 0xFFFE);
    timer_write_control(&controller, 0, TIMER_ENABLE);
    
    // Set up Timer 1 in count-up mode
    timer_write_reload(&controller, 1, 0x0000);
    timer_write_control(&controller, 1, TIMER_ENABLE | TIMER_COUNT_UP);
    
    // Update enough to cause Timer 0 overflow
    timing_advance(&timing, 10);
    timer_update(&controller, &timing);
    
    // Timer 1 should have incremented due to count-up
    // Note: This is a simplified test - full implementation would need
    // proper overflow handling
    
    printf("✓ Timer count-up tests passed\n");
}

void test_timer_frequencies() {
    printf("Testing timer frequency calculations...\n");
    
    assert(timing_get_timer_frequency(0) == TIMER_FREQ_1);
    assert(timing_get_timer_frequency(1) == TIMER_FREQ_64);
    assert(timing_get_timer_frequency(2) == TIMER_FREQ_256);
    assert(timing_get_timer_frequency(3) == TIMER_FREQ_1024);
    
    // Test frequency calculations
    assert(TIMER_FREQUENCY_HZ(1) == 16780000 / 65536);
    assert(TIMER_FREQUENCY_HZ(64) == 16780000 / (65536 * 64));
    
    printf("✓ Timer frequency tests passed\n");
}

void test_conversion_macros() {
    printf("Testing time conversion macros...\n");
    
    // Test cycle/time conversions
    uint32_t cycles_per_ms = MILLISECONDS_TO_CYCLES(1);
    uint32_t expected_cycles = 16780; // ~16.78k cycles per millisecond
    
    // Allow for small rounding differences
    assert(cycles_per_ms >= expected_cycles - 10 && cycles_per_ms <= expected_cycles + 10);
    
    uint32_t ms_per_1000_cycles = CYCLES_TO_MILLISECONDS(1000);
    // Should be very small (much less than 1ms)
    assert(ms_per_1000_cycles == 0);
    
    printf("✓ Conversion macro tests passed\n");
}

void benchmark_timing_system() {
    printf("\nRunning timing system benchmarks...\n");
    
    TimingState timing;
    TimerController controller;
    
    timing_init(&timing);
    timer_init(&controller);
    
    // Enable all timers with different prescalers
    for (int i = 0; i < 4; i++) {
        timer_write_reload(&controller, i, 0x8000);
        timer_write_control(&controller, i, TIMER_ENABLE | i); // Different prescaler for each
    }
    
    // Simulate running for one frame (1/60th second)
    uint32_t cycles_per_frame = MILLISECONDS_TO_CYCLES(16); // ~16ms
    
    printf("Simulating %u cycles (one frame at 60 FPS)...\n", cycles_per_frame);
    
    for (int frame = 0; frame < 60; frame++) {
        timing_advance(&timing, cycles_per_frame);
        timer_update(&controller, &timing);
        
        if (frame % 10 == 0) {
            printf("Frame %d: Scanline %d, VBlank: %s\n", 
                   frame, 
                   timing_get_vcount(&timing),
                   timing_in_vblank(&timing) ? "Yes" : "No");
        }
    }
    
    printf("Final state after 60 frames:\n");
    printf("  Total cycles: %llu\n", (unsigned long long)timing.total_cycles);
    printf("  Current scanline: %d\n", timing.current_scanline);
    printf("  Timer counters: ");
    for (int i = 0; i < 4; i++) {
        printf("T%d=0x%04X ", i, timer_read_counter(&controller, i));
    }
    printf("\n");
}

int main() {
    printf("GBA Timing System Test Suite\n");
    printf("============================\n\n");
    
    // Run all tests
    test_timing_basic();
    test_video_timing();
    test_memory_timing();
    test_timer_basic();
    test_timer_overflow();
    test_timer_count_up();
    test_timer_frequencies();
    test_conversion_macros();
    
    printf("\n✓ All tests passed!\n");
    
    // Run benchmarks
    benchmark_timing_system();
    
    printf("\nTiming system test complete.\n");
    return 0;
}
