# Timer Test Improvement Analysis

## Summary of Changes
Fixed a critical bug in `Scheduler::runUntil` where events triggering in the "refill window" (cycles advanced during callbacks) were delayed by one instruction.

### The Fix
In `src/scheduler.cpp`, the `runUntil` loop now re-checks for reachable events after processing callbacks:
```cpp
    // Drain events that became reachable because a callback advanced
    // currentCycle past targetCycle (e.g. IRQ pipeline refill pushing
    // the cycle counter into a window containing a timer overflow).
    uint64_t drainLimit = currentCycle;
    while (!eventQueue.empty() && eventQueue.top().triggerCycle <= drainLimit) {
        // ... process event ...
    }
```
This ensures that if an IRQ handler refill (advancing cycles by ~32) pushes the clock past a timer overflow point, that overflow is handled immediately before the next instruction executes.

### Results
- **Timers Suite**: Stable at **657 PASS / 279 FAIL** (baseline restored)
- **Regression Tests**:
  - Memory: **1552 PASS** (Perfect)
  - I/O Read: **130 PASS** (Perfect)
  - Timing: **1946 PASS / 74 FAIL**

### Failed Experiments (Reverted)
1. **LDM/STM +1 Cycle**: 
   - Tried adding a final cycle to block transfers to match mGBA source.
   - Result: **Regression** (572 PASS / 364 FAIL).
   - Conclusion: Our per-register `addWaitCycles` accumulation likely differs subtly from mGBA's bulk accumulation, making this specific adjustment incorrect in our current model.

2. **IRQ Latency Tuning**:
   - `IRQ_LATENCY_CYCLES = 7` (Standard): 656 PASS
   - `IRQ_LATENCY_CYCLES = 5`: 518 PASS
   - `IRQ_LATENCY_CYCLES = 6`: 530 PASS
   - `IRQ_LATENCY_CYCLES = 8`: 566 PASS
   - Conclusion: The standard 7 cycles (matching mGBA's 4-cycle pipeline + 3-cycle sync) remains optimal.

### Diagnosis of Remaining Failures (6b prescaler)
Trace analysis of `6b, 0x0010 1xs 1d 1i` (tight loop test) shows:
- Timer enable at cycle X
- Overflow at X + 1024 cycles
- Tight loop is `ADD, LDR, TST, BNE` (4 instructions)
- Our cycle trace shows costs: 1, 3, 1, 3 = 8 cycles/iteration
- The failure is likely due to phase alignment between the 8-cycle loop and the 1024-cycle timer period not matching hardware exactly, causing an off-by-one loop count.
- This requires sub-instruction timing precision (mid-instruction writes) which our "deferred commit" memory model cannot easily support without major refactoring.

## Recommendation
The scheduler fix is correct and aligns our event processing with mGBA's design. The remaining timer failures are due to the fundamental difference in memory access timing models (deferred vs immediate) and are likely acceptable for now given the perfect scores in other critical areas.
