# Testing Scheduler Integration

This document outlines strategies for testing that the scheduler integration works correctly with the existing GBA emulator components.

## Testing Strategy Overview

We'll use a **layered testing approach**:

1. **Unit Tests** - Test individual component integration
2. **Integration Tests** - Test components working together
3. **ROM Tests** - Test with actual GBA programs
4. **Validation Tests** - Compare against known correct behavior

---

## 1. Unit Tests for Component Integration

### Test 1: GPU Scheduler Integration

Create `tests/gpu/test_gpu_scheduler.cpp`:

```cpp
#include <gtest/gtest.h>
#include "scheduler.h"
#include "gpu.h"

class GPUSchedulerTest : public ::testing::Test {
protected:
    Scheduler scheduler;
    GPU gpu;
    
    void SetUp() override {
        scheduler.reset();
        gpu_init(&gpu, &scheduler);
    }
};

TEST_F(GPUSchedulerTest, HBlankEventsScheduled) {
    // Run for one scanline
    scheduler.runUntil(1232);
    
    // Check that HBlank occurred
    // (Requires adding test hooks to GPU)
    EXPECT_TRUE(gpu.hblank_occurred);
}

TEST_F(GPUSchedulerTest, VBlankAfter160Scanlines) {
    // Run for 160 scanlines
    scheduler.runUntil(160 * 1232);
    
    EXPECT_TRUE(gpu.in_vblank);
    EXPECT_EQ(gpu.vcount, 160);
}

TEST_F(GPUSchedulerTest, FrameCompletes228Scanlines) {
    int frame_count = 0;
    
    // Hook into frame completion
    gpu.on_frame_complete = [&]() { frame_count++; };
    
    // Run for one frame
    scheduler.runUntil(228 * 1232);
    
    EXPECT_EQ(frame_count, 1);
    EXPECT_EQ(gpu.vcount, 0);  // Should wrap back to 0
}
```

### Test 2: Timer Scheduler Integration

Create `tests/timer/test_timer_scheduler.cpp`:

```cpp
#include <gtest/gtest.h>
#include "scheduler.h"
#include "timer.h"

class TimerSchedulerTest : public ::testing::Test {
protected:
    Scheduler scheduler;
    Timer timer;
    int overflow_count = 0;
    
    void SetUp() override {
        scheduler.reset();
        timer_init(&timer, &scheduler);
        timer.on_overflow = [this]() { overflow_count++; };
        overflow_count = 0;
    }
};

TEST_F(TimerSchedulerTest, TimerOverflowAt65536Cycles) {
    // Start timer from 0 with 1:1 prescaler
    timer_start(&timer, 0x0000, PRESCALER_1);
    
    scheduler.runUntil(65536);
    
    EXPECT_EQ(overflow_count, 1);
}

TEST_F(TimerSchedulerTest, TimerReloadAfterOverflow) {
    // Start timer with reload value
    timer.reload_value = 0xFF00;
    timer_start(&timer, 0xFF00, PRESCALER_1);
    
    // Run until first overflow (256 cycles)
    scheduler.runUntil(256);
    EXPECT_EQ(overflow_count, 1);
    
    // Run until second overflow (another 256 cycles)
    scheduler.runUntil(512);
    EXPECT_EQ(overflow_count, 2);
}

TEST_F(TimerSchedulerTest, DisablingTimerCancelsEvents) {
    timer_start(&timer, 0x0000, PRESCALER_1);
    
    // Disable timer after 1000 cycles
    scheduler.runUntil(1000);
    timer_stop(&timer);
    
    // Run way past when overflow would have occurred
    scheduler.runUntil(100000);
    
    // No overflow should have occurred
    EXPECT_EQ(overflow_count, 0);
}

TEST_F(TimerSchedulerTest, PrescalerValues) {
    const uint32_t prescaler_cycles[] = {1, 64, 256, 1024};
    
    for (int i = 0; i < 4; i++) {
        scheduler.reset();
        overflow_count = 0;
        
        timer_start(&timer, 0x0000, i);
        
        uint32_t expected_cycles = 65536 * prescaler_cycles[i];
        scheduler.runUntil(expected_cycles);
        
        EXPECT_EQ(overflow_count, 1) 
            << "Failed for prescaler " << prescaler_cycles[i];
    }
}
```

---

## 2. Integration Tests

### Test 3: Multiple Components Together

Create `tests/integration/test_full_integration.cpp`:

```cpp
#include <gtest/gtest.h>
#include "gba.h"

class FullIntegrationTest : public ::testing::Test {
protected:
    GBA gba;
    
    void SetUp() override {
        gba_init(&gba);
    }
};

TEST_F(FullIntegrationTest, OneFrameExecution) {
    // Track events
    int hblank_count = 0;
    int vblank_count = 0;
    int timer_count = 0;
    
    gba.gpu.on_hblank = [&]() { hblank_count++; };
    gba.gpu.on_vblank = [&]() { vblank_count++; };
    gba.timers[0].on_overflow = [&]() { timer_count++; };
    
    // Start timer 0
    timer_start(&gba.timers[0], 0x0000, PRESCALER_1);
    
    // Run one frame
    gba_run_frame(&gba);
    
    // Verify expected events
    EXPECT_EQ(hblank_count, 160);  // One per visible scanline
    EXPECT_EQ(vblank_count, 1);
    EXPECT_GT(timer_count, 0);      // At least one timer overflow
}

TEST_F(FullIntegrationTest, VBlankTiming) {
    uint64_t vblank_cycle = 0;
    
    gba.gpu.on_vblank = [&]() {
        vblank_cycle = gba.scheduler.getCurrentCycle();
    };
    
    gba_run_frame(&gba);
    
    // VBlank should occur at exactly 160 * 1232 cycles
    EXPECT_EQ(vblank_cycle, 160 * 1232);
}

TEST_F(FullIntegrationTest, EventPriorityOrder) {
    std::vector<std::string> event_order;
    
    // Schedule events at same cycle with different priorities
    uint64_t test_cycle = 1000;
    
    gba.scheduler.scheduleAt(test_cycle, [&]() {
        event_order.push_back("Timer");
    }, EventType::TIMER_0_OVERFLOW, 2);
    
    gba.scheduler.scheduleAt(test_cycle, [&]() {
        event_order.push_back("HBlank");
    }, EventType::VIDEO_HBLANK, 1);
    
    gba.scheduler.scheduleAt(test_cycle, [&]() {
        event_order.push_back("VBlank");
    }, EventType::VIDEO_VBLANK, 0);
    
    gba.scheduler.runUntil(test_cycle + 1);
    
    ASSERT_EQ(event_order.size(), 3);
    EXPECT_EQ(event_order[0], "VBlank");   // Priority 0
    EXPECT_EQ(event_order[1], "HBlank");   // Priority 1
    EXPECT_EQ(event_order[2], "Timer");    // Priority 2
}
```

---

## 3. Simple Test ROMs

### Test ROM 1: VBlank Detection Test

Create a minimal GBA ROM that tests VBlank timing:

```c
// test_vblank.c
#include <stdint.h>

#define REG_DISPCNT  (*(volatile uint32_t*)0x04000000)
#define REG_DISPSTAT (*(volatile uint16_t*)0x04000004)
#define REG_VCOUNT   (*(volatile uint16_t*)0x04000006)
#define MEM_PALETTE  ((volatile uint16_t*)0x05000000)
#define MEM_VRAM     ((volatile uint16_t*)0x06000000)

#define DISPSTAT_VBLANK 0x0001
#define DISPSTAT_HBLANK 0x0002

int main() {
    // Set Mode 3 (240x160, 16-bit color bitmap)
    REG_DISPCNT = 0x0003;
    
    uint16_t color = 0x001F;  // Red
    
    while (1) {
        // Wait for VBlank
        while (!(REG_DISPSTAT & DISPSTAT_VBLANK));
        
        // Change background color during VBlank
        for (int i = 0; i < 240*160; i++) {
            MEM_VRAM[i] = color;
        }
        
        // Cycle through colors
        color += 0x0020;  // Increment green
        if (color > 0x7FFF) color = 0x001F;
        
        // Wait for VBlank to end
        while (REG_DISPSTAT & DISPSTAT_VBLANK);
    }
    
    return 0;
}
```

**Expected behavior**: Screen should smoothly change colors at 60 fps

**Test verification**:
```cpp
TEST(ROMTest, VBlankDetectionWorks) {
    GBA gba;
    gba_load_rom(&gba, "test_vblank.gba");
    
    uint16_t colors[60];
    
    // Run 60 frames and capture color changes
    for (int i = 0; i < 60; i++) {
        gba_run_frame(&gba);
        colors[i] = gba_read_vram(&gba, 0);
    }
    
    // Verify colors changed each frame
    for (int i = 1; i < 60; i++) {
        EXPECT_NE(colors[i], colors[i-1]);
    }
}
```

### Test ROM 2: Timer Test

```c
// test_timer.c
#define REG_TM0CNT_L (*(volatile uint16_t*)0x04000100)
#define REG_TM0CNT_H (*(volatile uint16_t*)0x04000102)
#define REG_DISPCNT  (*(volatile uint32_t*)0x04000000)
#define MEM_VRAM     ((volatile uint16_t*)0x06000000)

#define TIMER_ENABLE 0x0080
#define TIMER_PRESCALER_1 0x0000

int main() {
    REG_DISPCNT = 0x0003;  // Mode 3
    
    // Start timer from 0
    REG_TM0CNT_L = 0x0000;
    REG_TM0CNT_H = TIMER_ENABLE | TIMER_PRESCALER_1;
    
    uint16_t last_timer = 0;
    int pixel = 0;
    
    while (1) {
        uint16_t current_timer = REG_TM0CNT_L;
        
        // Draw a pixel when timer advances
        if (current_timer != last_timer) {
            MEM_VRAM[pixel++] = 0xFFFF;  // White pixel
            if (pixel >= 240*160) pixel = 0;
            last_timer = current_timer;
        }
    }
    
    return 0;
}
```

**Expected behavior**: White pixels should appear steadily (timer incrementing)

---

## 4. Validation Tests (Compare Against Reference)

### Test with Visual Comparison

Create a test that captures screen output and compares with expected:

```cpp
TEST(ValidationTest, CompareScreenOutput) {
    GBA gba;
    gba_load_rom(&gba, "test_vblank.gba");
    
    // Run for 10 frames
    for (int i = 0; i < 10; i++) {
        gba_run_frame(&gba);
    }
    
    // Capture framebuffer
    uint16_t* framebuffer = gba_get_framebuffer(&gba);
    
    // Load expected output
    uint16_t expected[240*160];
    load_expected_frame("test_vblank_frame10.raw", expected);
    
    // Compare
    int mismatches = 0;
    for (int i = 0; i < 240*160; i++) {
        if (framebuffer[i] != expected[i]) {
            mismatches++;
        }
    }
    
    EXPECT_EQ(mismatches, 0) << mismatches << " pixels differ";
}
```

### Test with Cycle Counting

```cpp
TEST(ValidationTest, CycleAccuracy) {
    GBA gba;
    gba_load_rom(&gba, "test_simple.gba");
    
    // Run exactly one frame
    uint64_t start_cycle = gba.scheduler.getCurrentCycle();
    gba_run_frame(&gba);
    uint64_t end_cycle = gba.scheduler.getCurrentCycle();
    
    // One frame should be exactly 280,896 cycles
    EXPECT_EQ(end_cycle - start_cycle, 280896);
}

TEST(ValidationTest, VBlankCycleAccuracy) {
    GBA gba;
    gba_init(&gba);
    
    std::vector<uint64_t> vblank_cycles;
    
    gba.gpu.on_vblank = [&]() {
        vblank_cycles.push_back(gba.scheduler.getCurrentCycle());
    };
    
    // Run 10 frames
    for (int i = 0; i < 10; i++) {
        gba_run_frame(&gba);
    }
    
    // Check VBlank occurs at exact intervals
    for (int i = 1; i < 10; i++) {
        uint64_t interval = vblank_cycles[i] - vblank_cycles[i-1];
        EXPECT_EQ(interval, 280896) << "Frame " << i << " interval incorrect";
    }
}
```

---

## 5. Debug Tools for Integration Testing

### Cycle Debugger

Add debug output to track scheduler state:

```cpp
void scheduler_debug_print() {
    printf("Scheduler State at cycle %llu:\n", scheduler.getCurrentCycle());
    printf("  Pending events: %zu\n", scheduler.getPendingEventCount());
    
    // Print next few events
    auto events = scheduler.getPendingEvents();  // Add this method
    for (int i = 0; i < std::min(5, (int)events.size()); i++) {
        printf("    [%d] Cycle %llu, Type %d, Priority %d\n",
               i, events[i].triggerCycle, 
               (int)events[i].type, events[i].priority);
    }
}
```

### Event Trace

Create an event logging system:

```cpp
struct EventTrace {
    uint64_t cycle;
    EventType type;
    std::string description;
};

std::vector<EventTrace> event_log;

void log_event(uint64_t cycle, EventType type, const char* desc) {
    event_log.push_back({cycle, type, desc});
}

// Use in tests
TEST(DebugTest, EventTracing) {
    event_log.clear();
    
    gba.gpu.on_hblank = [&]() {
        log_event(gba.scheduler.getCurrentCycle(), 
                  EventType::VIDEO_HBLANK, "HBlank");
    };
    
    gba_run_frame(&gba);
    
    // Analyze event log
    for (auto& evt : event_log) {
        printf("Cycle %8llu: %s\n", evt.cycle, evt.description.c_str());
    }
}
```

### Visual Timeline

Create a visualization of events:

```cpp
void print_event_timeline(uint64_t start, uint64_t end) {
    printf("Event Timeline (cycles %llu to %llu):\n", start, end);
    printf("Cycle    |Event\n");
    printf("---------|---------------------------------------------\n");
    
    for (auto& evt : event_log) {
        if (evt.cycle >= start && evt.cycle <= end) {
            printf("%8llu | %s\n", evt.cycle, evt.description.c_str());
        }
    }
}
```

---

## 6. Automated Test Suite

Create a comprehensive test runner:

```bash
#!/bin/bash
# run_integration_tests.sh

echo "Running Scheduler Integration Tests..."

# Unit tests
echo "=== GPU Integration Tests ==="
./test_gpu_scheduler

echo "=== Timer Integration Tests ==="
./test_timer_scheduler

# Integration tests
echo "=== Full Integration Tests ==="
./test_full_integration

# ROM tests
echo "=== Simple ROM Tests ==="
./test_rom test_vblank.gba
./test_rom test_timer.gba

# Validation tests
echo "=== Cycle Accuracy Tests ==="
./test_cycle_accuracy

echo ""
echo "All integration tests complete!"
```

---

## 7. Step-by-Step Integration Testing Plan

### Phase 1: Test GPU Integration Alone

1. **Integrate scheduler with GPU only**
2. Run `test_gpu_scheduler` 
3. Verify all HBlank/VBlank events occur at correct cycles
4. Check that `REG_VCOUNT` updates correctly
5. Test with a simple ROM that reads `REG_DISPSTAT`

### Phase 2: Test Timer Integration Alone

1. **Integrate scheduler with timers only**
2. Run `test_timer_scheduler`
3. Verify overflow events occur at correct intervals
4. Test prescaler values (1, 64, 256, 1024)
5. Test enabling/disabling timers mid-execution

### Phase 3: Test GPU + Timers Together

1. **Run both systems simultaneously**
2. Run `test_full_integration`
3. Verify events don't interfere with each other
4. Check priority ordering works correctly
5. Test with ROM that uses both VBlank and timers

### Phase 4: Add CPU Execution

1. **Integrate with main CPU loop**
2. Test that instructions execute with correct cycle counts
3. Verify interrupts trigger at correct times
4. Test with ROM that has interrupt handlers

### Phase 5: Full System Test

1. **Run complete emulator with scheduler**
2. Test with commercial ROMs
3. Compare frame rate (should be ~60 fps)
4. Check for timing glitches or stuttering
5. Verify save states work correctly

---

## 8. Debugging Integration Issues

### Common Issues and Solutions

#### Issue: Events Not Firing

**Debug steps**:
```cpp
// Add logging to event scheduling
scheduler.schedule(cycles, [&]() {
    printf("Event fired at cycle %llu\n", scheduler.getCurrentCycle());
    // ... actual handler
});

// Check pending events
printf("Pending events: %zu\n", scheduler.getPendingEventCount());
```

#### Issue: Timing Drift

**Debug steps**:
```cpp
// Track cycle accuracy
uint64_t expected_cycle = start + (frames * 280896);
uint64_t actual_cycle = scheduler.getCurrentCycle();
printf("Cycle drift: %lld\n", (int64_t)(actual_cycle - expected_cycle));
```

#### Issue: Events Out of Order

**Debug steps**:
```cpp
// Log all events with timestamps
scheduler.schedule(cycles, [cycle=cycles, type]() {
    printf("Executed %d at cycle %llu (expected %llu)\n", 
           type, scheduler.getCurrentCycle(), cycle);
});
```

---

## 9. Continuous Integration Tests

Add to your CI pipeline:

```yaml
# .github/workflows/integration-tests.yml
name: Integration Tests

on: [push, pull_request]

jobs:
  test-scheduler-integration:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build tests
        run: |
          cd tests/scheduler
          make
      - name: Run scheduler tests
        run: |
          cd tests/scheduler
          make test
      - name: Run integration tests
        run: |
          cd tests/integration
          make test
```

---

## Summary

The testing strategy covers:

1. ✅ **Unit tests** - Each component individually
2. ✅ **Integration tests** - Components working together  
3. ✅ **ROM tests** - Real GBA programs
4. ✅ **Validation tests** - Cycle-accurate verification
5. ✅ **Debug tools** - Event tracing and visualization

Start with **unit tests** for GPU and timers, then move to **integration tests**, and finally test with **real ROMs**. This incremental approach will help you catch issues early and verify each piece works correctly before combining them.
