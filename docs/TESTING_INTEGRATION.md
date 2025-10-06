# Quick Testing Guide for Scheduler Integration

## ✅ Pre-Integration Tests (Run These Now!)

You can verify the scheduler works **before** doing any integration:

```bash
cd /Users/malcolm/gba/tests/scheduler
make clean
make test
```

This runs **36 tests total**:
- ✅ 15 scheduler core tests
- ✅ 10 timing integration tests  
- ✅ 11 basic integration tests (with mock GPU/timers)

**All 36 tests currently passing!** 🎉

---

## 🔧 During Integration Testing

### Step 1: Integrate GPU Only

After integrating scheduler with `gpu.cpp`:

```bash
# Option A: Run existing GPU tests with new scheduler
cd tests/gpu
make test

# Option B: Create new GPU scheduler test
# Add test_gpu_scheduler.cpp (see scheduler_integration_testing.md)
```

**What to verify**:
- [ ] HBlank events occur every 1232 cycles during visible scanlines (0-159)
- [ ] VBlank event occurs at cycle 197,120 (160 * 1232)
- [ ] VCOUNT register updates correctly (0-227)
- [ ] Frame completes after 228 scanlines (280,896 cycles)

**Quick debug check**:
```cpp
// Add to gpu.cpp after integration
printf("VBlank at cycle %llu (expected %d)\n", 
       scheduler->getCurrentCycle(), 160 * 1232);
```

### Step 2: Integrate Timers Only

After integrating scheduler with `timer.c`:

```bash
cd tests/timer
make test
```

**What to verify**:
- [ ] Timer 0-3 overflow at correct intervals
- [ ] Prescaler values work (1, 64, 256, 1024)
- [ ] Stopping timer cancels scheduled events
- [ ] Timer reload value works correctly

**Quick debug check**:
```c
// Add to timer_overflow callback
printf("Timer %d overflow at cycle %llu\n", 
       timer_id, scheduler->getCurrentCycle());
```

### Step 3: Test GPU + Timers Together

Run the basic integration test again after real integration:

```bash
cd tests/scheduler
./test_integration_basic
```

Should still pass! If not, check:
- Event priorities (VBlank=0, HBlank=1, Timer=2)
- Events not interfering with each other
- Cycle counting accurate

### Step 4: Test with Main Loop

After integrating with main emulation loop:

**Manual test**:
```bash
# Run emulator with a simple ROM
./gba_emulator assets/roms/simple_test.bin

# Should see:
# - Smooth 60 fps rendering
# - No timing stutters
# - Frame counter advancing steadily
```

**Automated test**:
```cpp
TEST(MainLoopTest, FrameRateAccurate) {
    GBA gba;
    gba_init(&gba);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run 600 frames (10 seconds at 60fps)
    for (int i = 0; i < 600; i++) {
        gba_run_frame(&gba);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should take ~10 seconds (allowing some variance)
    EXPECT_NEAR(duration.count(), 10000, 500);
}
```

---

## 🐛 Debugging Integration Issues

### Issue: Events not firing

**Check 1**: Is event being scheduled?
```cpp
printf("Scheduling event type %d at cycle %llu\n", 
       (int)event_type, scheduler->getCurrentCycle() + cycles);
```

**Check 2**: Is scheduler advancing?
```cpp
printf("Scheduler at cycle %llu, pending events: %zu\n",
       scheduler->getCurrentCycle(), 
       scheduler->getPendingEventCount());
```

**Check 3**: Run until next event explicitly
```cpp
uint64_t next = scheduler->getCyclesUntilNextEvent();
printf("Next event in %llu cycles\n", next);
scheduler->runUntil(scheduler->getCurrentCycle() + next);
```

### Issue: Timing drift

**Check**: Compare expected vs actual cycles
```cpp
uint64_t expected = start_cycle + (frames * 280896);
uint64_t actual = scheduler->getCurrentCycle();
int64_t drift = (int64_t)(actual - expected);
printf("Cycle drift: %lld cycles over %d frames\n", drift, frames);

if (abs(drift) > 100) {
    printf("WARNING: Significant timing drift!\n");
}
```

### Issue: Events out of order

**Check**: Log all event executions
```cpp
scheduler->schedule(cycles, [=]() {
    printf("[%llu] Event type %d priority %d\n",
           scheduler->getCurrentCycle(), (int)type, priority);
    // ... actual handler
}, type, priority);
```

### Issue: Performance problems

**Check**: Event queue size
```cpp
if (scheduler->getPendingEventCount() > 1000) {
    printf("WARNING: %zu pending events - potential leak!\n",
           scheduler->getPendingEventCount());
}
```

---

## 📊 Validation Metrics

Track these metrics to ensure integration is correct:

### Cycle Accuracy
```
✓ One frame = exactly 280,896 cycles
✓ One scanline = exactly 1,232 cycles  
✓ HBlank start = cycle 960 of scanline
✓ VBlank start = cycle 197,120 (scanline 160)
```

### Event Counts (per frame)
```
✓ Scanline events: 228
✓ HBlank events: 160
✓ VBlank events: 1
✓ Timer 0 overflows (1:1): ~4 per frame
```

### Performance
```
✓ Frame rate: ~60 fps (59.73 actual)
✓ Cycle rate: ~16.78 MHz
✓ Event overhead: < 1% CPU time
```

### Memory
```
✓ Scheduler memory: O(n) where n = pending events
✓ Typical pending events: < 100
✓ No memory leaks after 10,000 frames
```

---

## 🎯 Integration Checklist

Use this checklist as you integrate:

### Before Integration
- [x] All 36 scheduler tests passing
- [x] Documentation reviewed
- [x] Integration guide read

### GPU Integration
- [ ] Scheduler pointer added to GPU struct
- [ ] `gpu_init()` takes scheduler parameter
- [ ] Scanline events scheduled every 1232 cycles
- [ ] HBlank events scheduled at cycle 960
- [ ] VBlank event scheduled at scanline 160
- [ ] VCOUNT register updated in scanline callback
- [ ] HBlank/VBlank flags set correctly
- [ ] Events canceled when GPU disabled

### Timer Integration
- [ ] Scheduler pointer added to Timer struct
- [ ] `timer_init()` takes scheduler parameter
- [ ] Overflow events scheduled based on counter + prescaler
- [ ] Auto-reload works (reschedule on overflow)
- [ ] Events canceled when timer stopped
- [ ] All 4 timers work independently
- [ ] Prescaler values correct (1, 64, 256, 1024)

### Main Loop Integration
- [ ] Global scheduler created
- [ ] CPU advances scheduler by instruction cycles
- [ ] `runUntil()` called after each instruction
- [ ] Frame timing maintained (280,896 cycles)
- [ ] Interrupts checked at appropriate times
- [ ] Event-driven loop optional but recommended

### Memory Integration
- [ ] Memory access functions return cycle counts
- [ ] Wait states implemented per region
- [ ] Sequential vs non-sequential access distinguished
- [ ] Scheduler advanced by memory cycles

### Testing
- [ ] Unit tests pass for each component
- [ ] Integration tests pass
- [ ] Simple ROM boots and runs
- [ ] Frame rate is stable ~60 fps
- [ ] No timing glitches or stutters
- [ ] Save states work correctly

---

## 🚀 Quick Start After Integration

Once integrated, run this sequence:

```bash
# 1. Verify scheduler still works
cd tests/scheduler
make test

# 2. Test individual components
cd ../gpu && make test
cd ../timer && make test

# 3. Full system test
cd ../..
make && ./gba_emulator assets/roms/simple_test.bin

# 4. Run benchmarks
cd tests/arm_benchmark
make && ./arm_benchmark
```

If all pass: **Integration successful!** ✅

---

## 📝 What Success Looks Like

After successful integration:

1. **All tests pass** (unit + integration)
2. **Frame rate stable** at ~60 fps
3. **No visual glitches** or stuttering
4. **HBlank/VBlank** work correctly in test ROMs
5. **Timers** tick at correct intervals
6. **No memory leaks** after extended running
7. **Performance** acceptable (< 100% CPU for GBA emulation)

You'll know you're done when you can run a simple test ROM that:
- Changes color every VBlank → smooth color transitions
- Uses timer interrupts → consistent timing
- Runs at 60 fps → no frame drops

---

## 📚 Additional Resources

- `docs/scheduler_integration.md` - Detailed integration guide with code examples
- `docs/scheduler_integration_testing.md` - Comprehensive testing strategies
- `tests/scheduler/README.md` - Test suite documentation
- `.github/instructions` - Project coding standards

---

## ❓ Need Help?

If tests fail after integration:

1. **Check logs** - Enable debug printing in scheduler
2. **Isolate component** - Test GPU/timer separately
3. **Verify cycles** - Print cycle counts at key points
4. **Check priorities** - Ensure events execute in correct order
5. **Review code** - Compare against integration guide examples

Most issues are:
- Missing scheduler pointer initialization
- Incorrect cycle calculations  
- Wrong event priorities
- Not calling `runUntil()` to advance scheduler
- Events not being rescheduled (for recurring events)
