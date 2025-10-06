# Scheduler Integration Guide

This document explains how to integrate the event-driven scheduler with the existing GBA emulator components.

## Overview

The scheduler is now fully implemented and tested. This guide shows how to integrate it with:
1. Video controller (GPU)
2. Timer controller
3. Main emulation loop
4. Memory system

## 1. Video Controller Integration

### Current State
The video controller in `src/gpu.cpp` and `src/timing.c` has placeholder functions for HBlank/VBlank events.

### Integration Steps

#### Step 1: Add scheduler to GPU context

```cpp
// In include/gpu.h
#include "scheduler.h"

typedef struct {
    // ... existing fields ...
    Scheduler* scheduler;  // Add scheduler pointer
} GPU;
```

#### Step 2: Initialize GPU with scheduler

```cpp
// In src/gpu.cpp
void gpu_init(GPU* gpu, Scheduler* scheduler) {
    // ... existing initialization ...
    gpu->scheduler = scheduler;
    
    // Schedule first scanline event
    scheduler->schedule(CYCLES_PER_SCANLINE, 
        [gpu]() { gpu_scanline_callback(gpu); },
        EventType::VIDEO_SCANLINE, 1);
}
```

#### Step 3: Implement scanline callback

```cpp
void gpu_scanline_callback(GPU* gpu) {
    gpu->current_scanline++;
    
    if (gpu->current_scanline == 160) {
        // VBlank starts
        gpu->in_vblank = true;
        gpu->vcount = 160;
        
        // Trigger VBlank interrupt if enabled
        if (gpu->interrupt_enable & INT_VBLANK) {
            trigger_interrupt(INT_VBLANK);
        }
        
        // Schedule VBlank end
        gpu->scheduler->schedule(VBLANK_CYCLES,
            [gpu]() { gpu_vblank_end_callback(gpu); },
            EventType::VIDEO_VBLANK, 0);
            
    } else if (gpu->current_scanline == 228) {
        // Frame complete, restart
        gpu->current_scanline = 0;
        gpu->in_vblank = false;
    }
    
    // Check for HBlank
    if (gpu->current_scanline < 160) {
        // Schedule HBlank start
        gpu->scheduler->schedule(HBLANK_START_CYCLE,
            [gpu]() { gpu_hblank_callback(gpu); },
            EventType::VIDEO_HBLANK, 1);
    }
    
    // Schedule next scanline
    gpu->scheduler->schedule(CYCLES_PER_SCANLINE,
        [gpu]() { gpu_scanline_callback(gpu); },
        EventType::VIDEO_SCANLINE, 1);
}
```

#### Step 4: Implement HBlank callback

```cpp
void gpu_hblank_callback(GPU* gpu) {
    gpu->in_hblank = true;
    
    // Trigger HBlank interrupt if enabled
    if (gpu->interrupt_enable & INT_HBLANK) {
        trigger_interrupt(INT_HBLANK);
    }
    
    // Schedule HBlank end
    gpu->scheduler->schedule(HBLANK_DURATION,
        [gpu]() { gpu->in_hblank = false; },
        EventType::VIDEO_HBLANK, 1);
}
```

### Timing Constants

```c
#define CYCLES_PER_SCANLINE 1232
#define HBLANK_START_CYCLE 960
#define HBLANK_DURATION 272
#define VISIBLE_SCANLINES 160
#define TOTAL_SCANLINES 228
#define VBLANK_CYCLES (CYCLES_PER_SCANLINE * (TOTAL_SCANLINES - VISIBLE_SCANLINES))
```

## 2. Timer Controller Integration

### Current State
Timer functions exist in `src/timer.c` with basic overflow detection.

### Integration Steps

#### Step 1: Add scheduler to timer context

```cpp
// In include/timer.h
#include "scheduler.h"

typedef struct {
    // ... existing timer fields ...
    Scheduler* scheduler;
    bool scheduled;  // Track if overflow event is scheduled
} Timer;
```

#### Step 2: Update timer reload function

```cpp
void timer_reload(Timer* timer, int timer_id) {
    timer->counter = timer->reload_value;
    
    if (timer->enabled && timer->scheduler) {
        // Calculate cycles until overflow
        uint32_t cycles_until_overflow = 
            (0x10000 - timer->counter) * timer_get_prescaler(timer);
        
        // Schedule overflow event
        EventType event_type;
        switch (timer_id) {
            case 0: event_type = EventType::TIMER_0_OVERFLOW; break;
            case 1: event_type = EventType::TIMER_1_OVERFLOW; break;
            case 2: event_type = EventType::TIMER_2_OVERFLOW; break;
            case 3: event_type = EventType::TIMER_3_OVERFLOW; break;
        }
        
        timer->scheduler->schedule(cycles_until_overflow,
            [timer, timer_id]() { timer_overflow_callback(timer, timer_id); },
            event_type, 2);  // Lower priority than video events
            
        timer->scheduled = true;
    }
}
```

#### Step 3: Implement overflow callback

```cpp
void timer_overflow_callback(Timer* timer, int timer_id) {
    timer->scheduled = false;
    
    // Trigger overflow interrupt if enabled
    if (timer->interrupt_enable) {
        trigger_interrupt(INT_TIMER0 + timer_id);
    }
    
    // Reload and reschedule if still enabled
    if (timer->enabled) {
        timer_reload(timer, timer_id);
    }
}
```

#### Step 4: Cancel events when timer disabled

```cpp
void timer_disable(Timer* timer, int timer_id) {
    timer->enabled = false;
    
    if (timer->scheduled && timer->scheduler) {
        EventType event_type;
        switch (timer_id) {
            case 0: event_type = EventType::TIMER_0_OVERFLOW; break;
            case 1: event_type = EventType::TIMER_1_OVERFLOW; break;
            case 2: event_type = EventType::TIMER_2_OVERFLOW; break;
            case 3: event_type = EventType::TIMER_3_OVERFLOW; break;
        }
        
        timer->scheduler->cancelEventsOfType(event_type);
        timer->scheduled = false;
    }
}
```

## 3. Main Emulation Loop Integration

### Current State
The main loop in `src/main.cpp` runs CPU instructions in a simple loop.

### Integration Steps

#### Step 1: Create global scheduler

```cpp
// In src/main.cpp
#include "scheduler.h"

Scheduler g_scheduler;
```

#### Step 2: Update main loop

```cpp
void emulation_loop(GBA* gba) {
    const uint64_t CYCLES_PER_FRAME = 280896;
    
    while (gba->running) {
        // Run emulation for one frame
        uint64_t target_cycle = g_scheduler.getCurrentCycle() + CYCLES_PER_FRAME;
        
        while (g_scheduler.getCurrentCycle() < target_cycle) {
            // Execute one CPU instruction
            int cycles = cpu_execute_instruction(&gba->cpu);
            
            // Advance scheduler by cycles executed
            g_scheduler.runUntil(g_scheduler.getCurrentCycle() + cycles);
            
            // Check for interrupts
            if (g_scheduler.getCurrentCycle() % 1000 == 0) {
                cpu_check_interrupts(&gba->cpu);
            }
        }
        
        // Render frame
        gpu_render_frame(&gba->gpu);
        
        // Handle input
        input_update(&gba->input);
        
        // Maintain frame rate (~60 fps)
        frame_limiter();
    }
}
```

#### Step 3: Alternative approach - event-driven loop

For better accuracy, run until next event:

```cpp
void emulation_loop_event_driven(GBA* gba) {
    const uint64_t CYCLES_PER_FRAME = 280896;
    uint64_t next_frame = g_scheduler.getCurrentCycle() + CYCLES_PER_FRAME;
    
    while (gba->running) {
        // Get cycles until next event
        uint64_t cycles_until_event = g_scheduler.getCyclesUntilNextEvent();
        
        if (cycles_until_event == 0) {
            // Run pending events
            g_scheduler.runUntil(g_scheduler.getCurrentCycle());
        } else {
            // Execute CPU instructions until event or frame end
            uint64_t target = g_scheduler.getCurrentCycle() + cycles_until_event;
            
            while (g_scheduler.getCurrentCycle() < target && 
                   g_scheduler.getCurrentCycle() < next_frame) {
                int cycles = cpu_execute_instruction(&gba->cpu);
                g_scheduler.runUntil(g_scheduler.getCurrentCycle() + cycles);
            }
        }
        
        // Check if frame complete
        if (g_scheduler.getCurrentCycle() >= next_frame) {
            gpu_render_frame(&gba->gpu);
            input_update(&gba->input);
            frame_limiter();
            next_frame += CYCLES_PER_FRAME;
        }
    }
}
```

## 4. Memory System Integration

### Current State
Memory system returns values but doesn't account for wait states.

### Integration Steps

#### Step 1: Update memory read/write to return cycles

```cpp
// In include/memory.h
typedef struct {
    uint8_t* regions[16];  // Memory regions
    uint32_t wait_states[16];  // Wait state configuration
    Scheduler* scheduler;
} Memory;

// Update function signatures
uint32_t memory_read32(Memory* mem, uint32_t address, int* cycles_out);
uint16_t memory_read16(Memory* mem, uint32_t address, int* cycles_out);
uint8_t memory_read8(Memory* mem, uint32_t address, int* cycles_out);

void memory_write32(Memory* mem, uint32_t address, uint32_t value, int* cycles_out);
void memory_write16(Memory* mem, uint32_t address, uint16_t value, int* cycles_out);
void memory_write8(Memory* mem, uint32_t address, uint8_t value, int* cycles_out);
```

#### Step 2: Implement wait state calculation

```cpp
int memory_get_access_cycles(Memory* mem, uint32_t address, int access_size, bool sequential) {
    uint8_t region = (address >> 24) & 0x0F;
    
    switch (region) {
        case 0x00: // BIOS
            return 1;
            
        case 0x02: // EWRAM (256KB)
            return (access_size == 4) ? 6 : 3;
            
        case 0x03: // IWRAM (32KB)
            return 1;
            
        case 0x04: // I/O Registers
            return 1;
            
        case 0x05: // Palette RAM
            return (access_size == 4) ? 2 : 1;
            
        case 0x06: // VRAM
            return (access_size == 4) ? 2 : 1;
            
        case 0x07: // OAM
            return 1;
            
        case 0x08: // Game Pak ROM (Wait State 0)
        case 0x09:
            return sequential ? mem->wait_states[0] : mem->wait_states[1];
            
        case 0x0A: // Game Pak ROM (Wait State 1)
        case 0x0B:
            return sequential ? mem->wait_states[2] : mem->wait_states[3];
            
        case 0x0C: // Game Pak ROM (Wait State 2)
        case 0x0D:
            return sequential ? mem->wait_states[4] : mem->wait_states[5];
            
        case 0x0E: // Game Pak SRAM
        case 0x0F:
            return mem->wait_states[6];
            
        default:
            return 1;
    }
}
```

#### Step 3: Update memory access functions

```cpp
uint32_t memory_read32(Memory* mem, uint32_t address, int* cycles_out) {
    if (cycles_out) {
        *cycles_out = memory_get_access_cycles(mem, address, 4, false);
    }
    
    // ... existing read logic ...
}
```

## 5. Complete Integration Example

Here's how all pieces fit together:

```cpp
// In src/gba.cpp
void gba_init(GBA* gba) {
    // Initialize scheduler first
    g_scheduler.reset();
    
    // Initialize components with scheduler
    memory_init(&gba->memory, &g_scheduler);
    cpu_init(&gba->cpu, &gba->memory, &g_scheduler);
    gpu_init(&gba->gpu, &g_scheduler);
    timer_init(&gba->timers[0], &g_scheduler);
    timer_init(&gba->timers[1], &g_scheduler);
    timer_init(&gba->timers[2], &g_scheduler);
    timer_init(&gba->timers[3], &g_scheduler);
    
    // Start initial events
    gpu_start_frame(&gba->gpu);
}

void gba_run_frame(GBA* gba) {
    uint64_t frame_start = g_scheduler.getCurrentCycle();
    uint64_t frame_end = frame_start + CYCLES_PER_FRAME;
    
    while (g_scheduler.getCurrentCycle() < frame_end) {
        // Execute instruction
        int cpu_cycles;
        cpu_execute(&gba->cpu, &cpu_cycles);
        
        // Advance scheduler
        g_scheduler.runUntil(g_scheduler.getCurrentCycle() + cpu_cycles);
    }
}
```

## Testing the Integration

### Test 1: Verify HBlank Timing
```cpp
void test_hblank_timing() {
    GBA gba;
    gba_init(&gba);
    
    int hblank_count = 0;
    g_scheduler.scheduleAt(960, [&]() { 
        assert(gba.gpu.in_hblank);
        hblank_count++;
    });
    
    gba_run_frame(&gba);
    assert(hblank_count > 0);
}
```

### Test 2: Verify Timer Overflow
```cpp
void test_timer_overflow() {
    GBA gba;
    gba_init(&gba);
    
    bool overflow_triggered = false;
    timer_enable(&gba.timers[0], 0x0000, true, 
        [&]() { overflow_triggered = true; });
    
    g_scheduler.runUntil(65536);
    assert(overflow_triggered);
}
```

## Debugging Tips

1. **Enable scheduler logging**:
   ```cpp
   #define SCHEDULER_DEBUG 1
   ```

2. **Track event execution**:
   ```cpp
   scheduler.scheduleAt(cycle, [=]() {
       printf("Event executed at cycle %llu\n", scheduler.getCurrentCycle());
   });
   ```

3. **Verify event ordering**:
   ```cpp
   assert(scheduler.getPendingEventCount() == expected_count);
   ```

4. **Check cycle accuracy**:
   ```cpp
   uint64_t expected = frame_start + CYCLES_PER_FRAME;
   uint64_t actual = scheduler.getCurrentCycle();
   assert(actual == expected);
   ```

## Performance Considerations

- The scheduler overhead is minimal (O(log n) per event)
- Use `getCyclesUntilNextEvent()` to skip ahead efficiently
- Batch CPU execution between events when possible
- Cancel events when components are disabled to reduce queue size

## Next Steps

1. Integrate scheduler with GPU (highest priority)
2. Integrate scheduler with timers
3. Test with simple ROMs that use HBlank/VBlank
4. Implement memory wait states
5. Optimize main loop for performance
