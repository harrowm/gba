#include "timing.h"
#include <stddef.h>

// Initialize timing state
void timing_init(TimingState* timing) {
    if (!timing) return;
    
    timing->total_cycles = 0;
    timing->current_scanline = 0;
    timing->scanline_cycles = 0;
    
    // Initialize timer cycles
    for (int i = 0; i < 4; i++) {
        timing->timer_cycles[i] = 0;
    }
}

// Advance timing by specified number of cycles
void timing_advance(TimingState* timing, uint32_t cycles) {
    if (!timing) return;
    
    timing->total_cycles += cycles;
    timing->scanline_cycles += cycles;
    
    // Update video timing
    timing_update_video(timing);
    
    // Update timer cycles (simplified - actual implementation would 
    // need to check timer control registers)
    for (int i = 0; i < 4; i++) {
        timing->timer_cycles[i] += cycles;
    }
}

// Get timer frequency based on prescaler setting
uint32_t timing_get_timer_frequency(uint8_t prescaler) {
    switch (prescaler) {
        case 0: return TIMER_FREQ_1;
        case 1: return TIMER_FREQ_64;
        case 2: return TIMER_FREQ_256;
        case 3: return TIMER_FREQ_1024;
        default: return TIMER_FREQ_1;
    }
}

// Calculate memory access cycles based on address
uint32_t timing_calculate_memory_cycles(uint32_t address, uint8_t access_size) {
    // Determine memory region based on address
    if (address < 0x00004000) {
        // BIOS ROM (0x00000000-0x00003FFF)
        return CYCLES_BIOS_32;
    }
    else if (address >= 0x02000000 && address < 0x02040000) {
        // Main RAM 256K (0x02000000-0x0203FFFF)
        switch (access_size) {
            case 1: return 3; // 8-bit: 3 cycles
            case 2: return 3; // 16-bit: 3 cycles
            case 4: return 6; // 32-bit: 6 cycles
            default: return 6;
        }
    }
    else if (address >= 0x03000000 && address < 0x03008000) {
        // Work RAM 32K (0x03000000-0x03007FFF)
        return CYCLES_WRAM_32K_32;
    }
    else if (address >= 0x04000000 && address < 0x04000400) {
        // I/O Registers (0x04000000-0x040003FE)
        return CYCLES_IO_32;
    }
    else if (address >= 0x05000000 && address < 0x05000400) {
        // Palette RAM (0x05000000-0x050003FF)
        switch (access_size) {
            case 1: return 1; // 8-bit: 1 cycle
            case 2: return 1; // 16-bit: 1 cycle
            case 4: return 2; // 32-bit: 2 cycles
            default: return 2;
        }
    }
    else if (address >= 0x06000000 && address < 0x06018000) {
        // VRAM (0x06000000-0x06017FFF)
        uint32_t base_cycles;
        switch (access_size) {
            case 1: base_cycles = 1; break; // 8-bit: 1 cycle
            case 2: base_cycles = 1; break; // 16-bit: 1 cycle
            case 4: base_cycles = 2; break; // 32-bit: 2 cycles
            default: base_cycles = 2; break;
        }
        
        // Add potential video controller conflict
        return base_cycles + CYCLES_VIDEO_CONFLICT;
    }
    else if (address >= 0x07000000 && address < 0x07000400) {
        // OAM (0x07000000-0x070003FF)
        return CYCLES_OAM_32;
    }
    else if (address >= 0x08000000 && address < 0x0E000000) {
        // GamePak ROM (0x08000000-0x0DFFFFFF)
        // This would need to check wait state configuration
        // For now, return default timing
        return CYCLES_GAMEPAK_ROM_DEFAULT_N;
    }
    else if (address >= 0x0E000000 && address < 0x0E010000) {
        // GamePak SRAM (0x0E000000-0x0E00FFFF)
        return CYCLES_GAMEPAK_SRAM;
    }
    
    // Unknown region - assume slow access
    return CYCLES_GAMEPAK_ROM_DEFAULT_N;
}

// Update video timing state
void timing_update_video(TimingState* timing) {
    // GBA video timing:
    // 308 dots per scanline (240 visible + 68 blanking)
    // 4 cycles per dot = 1232 cycles per scanline
    // 228 scanlines total (160 visible + 68 vblank)
    
    const uint32_t CYCLES_PER_SCANLINE = 1232;
    const uint32_t TOTAL_SCANLINES = 228;
    
    // Handle multiple scanlines if necessary
    while (timing->scanline_cycles >= CYCLES_PER_SCANLINE) {
        // Move to next scanline
        timing->current_scanline++;
        timing->scanline_cycles -= CYCLES_PER_SCANLINE;
        
        // Wrap around after last scanline
        if (timing->current_scanline >= TOTAL_SCANLINES) {
            timing->current_scanline = 0;
        }
    }
}

// Video timing helper functions
uint32_t timing_get_vcount(TimingState* timing) {
    if (!timing) return 0;
    return timing->current_scanline;
}

int timing_in_vblank(TimingState* timing) {
    if (!timing) return 0;
    return timing->current_scanline >= 160;
}

int timing_in_hblank(TimingState* timing) {
    if (!timing) return 0;
    // HBlank occurs after 960 cycles of visible area
    return timing->scanline_cycles >= 960;
}

// Calculate cycles until next VBlank
uint32_t timing_cycles_until_vblank(const TimingState* timing) {
    if (timing->current_scanline >= 160) {
        // Already in VBlank, calculate cycles until next VBlank
        uint32_t remaining_scanlines = 228 - timing->current_scanline + 160;
        uint32_t cycles = remaining_scanlines * 1232 - timing->scanline_cycles;
        return cycles;
    } else {
        // Calculate cycles until VBlank starts
        uint32_t remaining_scanlines = 160 - timing->current_scanline;
        uint32_t cycles = remaining_scanlines * 1232 - timing->scanline_cycles;
        return cycles;
    }
}

// Calculate cycles until next HBlank
uint32_t timing_cycles_until_hblank(const TimingState* timing) {
    const uint32_t VISIBLE_CYCLES = 960;
    
    if (timing->scanline_cycles >= VISIBLE_CYCLES) {
        // Already in HBlank, calculate cycles until next HBlank
        return 1232 - timing->scanline_cycles + VISIBLE_CYCLES;
    } else {
        // Calculate cycles until HBlank starts
        return VISIBLE_CYCLES - timing->scanline_cycles;
    }
}

// Calculate cycles until the next timer event
uint32_t timing_cycles_until_next_timer_event(TimingState* timing) {
    if (!timing) return UINT32_MAX;
    
    uint32_t min_cycles = UINT32_MAX;
    
    // Check each timer for next overflow event
    // This is simplified - actual implementation would check timer control registers
    for (int i = 0; i < 4; i++) {
        // Assume timer is enabled with prescaler 0 (1:1) for this example
        // Real implementation would read timer control registers
        uint32_t cycles_per_tick = 1; // This would be based on prescaler
        uint32_t ticks_until_overflow = 65536; // This would be calculated from current timer value
        uint32_t cycles_until_overflow = ticks_until_overflow * cycles_per_tick;
        
        if (cycles_until_overflow < min_cycles) {
            min_cycles = cycles_until_overflow;
        }
    }
    
    return min_cycles;
}

// Calculate cycles until the next video event (HBlank or VBlank)
uint32_t timing_cycles_until_next_video_event(TimingState* timing) {
    if (!timing) return UINT32_MAX;
    
    uint32_t cycles_to_hblank = timing_cycles_until_hblank(timing);
    uint32_t cycles_to_vblank = timing_cycles_until_vblank(timing);
    
    return (cycles_to_hblank < cycles_to_vblank) ? cycles_to_hblank : cycles_to_vblank;
}

// Calculate cycles until the next significant timing event
uint32_t timing_cycles_until_next_event(TimingState* timing) {
    if (!timing) return UINT32_MAX;
    
    uint32_t timer_cycles = timing_cycles_until_next_timer_event(timing);
    uint32_t video_cycles = timing_cycles_until_next_video_event(timing);
    
    uint32_t min_cycles = (timer_cycles < video_cycles) ? timer_cycles : video_cycles;
    
    // Ensure we don't return 0 - always allow at least 1 cycle
    return (min_cycles == 0) ? 1 : min_cycles;
}

// Process timer events that should occur at current timing
void timing_process_timer_events(TimingState* timing) {
    if (!timing) return;
    
    // This function would check timer overflows and trigger interrupts
    // For now, it's a placeholder for the actual timer controller integration
    
    for (int i = 0; i < 4; i++) {
        // Check if timer has overflowed and needs to trigger interrupt
        // This would integrate with the timer controller and interrupt system
    }
}

// Process video events that should occur at current timing  
void timing_process_video_events(TimingState* timing) {
    if (!timing) return;
    
    // This function would trigger HBlank/VBlank interrupts and GPU updates
    // For now, it's a placeholder for the actual video controller integration
    
    // Check for HBlank entry
    if (timing->scanline_cycles >= 960 && timing->scanline_cycles < 960 + 1) {
        // Trigger HBlank interrupt if enabled
    }
    
    // Check for VBlank entry
    if (timing->current_scanline == 160) {
        // Trigger VBlank interrupt if enabled
    }
}
