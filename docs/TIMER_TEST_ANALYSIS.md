# Timer Test Suite Analysis

**Last updated:** 2026-02-13  
**Commit:** `efb6278` — Timer: use raw getCurrentCycle() for enable/disable; remove pendingCycles  
**Score:** 646 / 936 PASS (69%), 290 FAIL

## Test Suite Structure

Source: [mgba-emu/suite/src/timers.c](https://github.com/mgba-emu/suite/blob/master/src/timers.c)

Each test configures Timer 0 with a specific **prescaler** and **reload value**, then runs a polling loop in ARM assembly that:

1. Enables Timer 0 with IRQ, sets `irqCounter = ii`
2. Executes a delay loop of `dd` iterations
3. Writes the timer config via `STR [r4]` (32-bit write to 0x04000100)
4. Enters a **polling loop** that increments a counter and reads `LDR r2, [r4]` checking bit 23 (timer enable in the high halfword) — exits when timer is disabled
5. Records: **loop count** (xs = "times sum") and **timer counter value** (xv = "times value")

The IRQ handler (`testIrq`) decrements `irqCounter`; when it reaches 0, it writes `REG_TM0CNT_H = 0` to disable the timer.

### Test Parameters

- **Prescaler (Nb):** `0b`=1, `6b`=64, `8b`=256, `10b`=1024
- **Reload value:** e.g. `0x0005` → period = 0x10000 - 0x0005 = 65531 ticks
- **Loop type:** `1xs`/`1xv` = tight loop (1 ADD + LDR + TST + BNE), `16xs`/`16xv` = loose loop (16 ADDs + LDR + TST + BNE)
- **Delay (d):** `1d`/`2d`/`4d` — iterations before timer start
- **IRQ count (i):** `1i`/`2i`/`4i` — number of overflows before disable

### What Each Metric Tests

- **xs (sum):** How many loop iterations before the polling loop sees the timer disabled. Tests IRQ delivery timing relative to instruction boundaries.
- **xv (value):** The frozen timer counter value after disable. Tests the exact cycle at which the counter is captured during the STRH that clears the enable bit.

## What Was Fixed (556 → 646, +90 tests)

### Fix 1: Enable path — remove pendingCycles and +1 (556 → 646, but only with Fix 2)

**Before:** `lastReloadCycle = getCurrentCycle() + pendingCycles + 1`, then prescaler-align  
**After:** `lastReloadCycle = getCurrentCycle()`, then prescaler-align

mGBA does `lastEvent = mTimingCurrentTime() & ~tickMask`. Their `mTimingCurrentTime()` includes in-progress instruction costs (fetch + data access), but after prescaler rounding (`& ~tickMask`), the small fetch-cost difference is absorbed. Using raw `getCurrentCycle()` gives the same prescaler-aligned result.

**Tested variants:**
| Enable path formula | Score |
|---|---|
| `getCurrentCycle()` + prescaler align | **646** |
| `getCurrentCycle() + pendingCycles + 1` + prescaler align | 609 |
| `getCurrentCycle() + 1` + prescaler align | 605 |
| `getCurrentCycle() + pendingCycles` + prescaler align | 605 |
| `getCurrentCycle()` (no prescaler align) | 567 |

Prescaler alignment is critical. The `pendingCycles` and `+1` adjustments actively hurt.

### Fix 2: Disable path — remove pendingCycles and +1 (556 → 609)

**Before:** `currentCycle = getCurrentCycle() + pendingCycles + 1`  
**After:** `currentCycle = getCurrentCycle()`

mGBA's `GBATimerWriteTMCNT_HI` calls `GBATimerUpdateRegister(gba, timer, 0)` — cyclesLate=0. Their `mTimingCurrentTime()` already includes in-progress costs, while our `getCurrentCycle()` is at instruction-start. Empirically, raw `getCurrentCycle()` matches expected frozen counter values.

**Tested variants:**
| Disable path formula | Score |
|---|---|
| `getCurrentCycle()` (raw) | **609** (then 646 with enable fix) |
| `getCurrentCycle() + pendingCycles + 1` | 556 (baseline) |
| `getCurrentCycle() + 1` | 564 |

### ReadCounter adjustment — no effect on this suite

Tested `-2`, `-1`, and `0` — all produce identical results (646). The suite doesn't exercise mid-flight timer reads in a way that's sensitive to this offset. Kept at `-2` to match mGBA's `cyclesLate=2` for timer reads.

### IRQ latency — 7 cycles is critical

`IRQ_LATENCY_CYCLES = 7` → 646 PASS  
`IRQ_LATENCY_CYCLES = 0` → 419 PASS  

**Important:** The Makefile doesn't track header dependencies. Changing `interrupt.h` requires manually removing .o files: `rm -f build/interrupt.o build/gba.o build/cpu.o`

## Remaining 290 Failures — Three Categories

### Category 1: Prescaler=1 value tests — 141 failures

**Tests:** 72 `0b 1xv` + 69 `0b 16xv`  
**Symptom:** Frozen counter value after disable is wrong  
**Pattern:** Errors depend on reload value and `i` (IRQ iteration count):

| Reload | Period | Pass/Fail (of 9) | Pattern |
|--------|--------|-------------------|---------|
| 0x0001 | 65535 | 9P / 0F | All pass |
| 0x000C | 65524 | 6P / 3F | Fails at i=2 |
| 0x000D | 65523 | 0P / 9F | All fail |
| 0x0010 | 65520 | 3P / 6F | Fails at i=2,4 |
| 0x0014 | 65516 | 3P / 6F | " |
| 0x0015 | 65515 | 0P / 9F | All fail |
| 0x0020 | 65504 | 3P / 6F | Fails at i=2,4 |
| 0x0024 | 65500 | 3P / 6F | " |
| 0x0025 | 65499 | 0P / 9F | All fail |
| 0x0040 | 65472 | 6P / 3F | Fails at i=4 |
| 0x0080 | 65408 | 6P / 3F | " |
| 0x0800 | 63488 | 6P / 3F | " |
| 0x8000 | 32768 | 6P / 3F | " |

Observation: Periods ending in odd digits (`0x___5`, `0x___D`) fail most — modular arithmetic with the timer period amplifies small cycle errors. Higher `i` = more overflows = more accumulated error.

**Root cause:** Small cycle-count discrepancy in the path from timer overflow → IRQ vector → BIOS handler → libgba dispatch → `testIrq()` → STRH to TM0CNT_H. For prescaler=1, every CPU cycle = 1 timer tick, so a 1–2 cycle error in the total IRQ handling path becomes visible when taken mod(period).

**Approach:** Trace instruction-by-instruction cycle counts from the overflow event through the disable write, comparing against mGBA. The fix is in instruction-level cycle accuracy, not timer logic.

### Category 2: Higher prescaler iteration count tests — 131 failures

**Tests:** 
- `6b 1xs`: 32, `6b 16xs`: 13
- `8b 1xs`: 31, `8b 16xs`: 11  
- `10b 1xs`: 32, `10b 16xs`: 12
- `0b 1xs`: 12, `0b 16xs`: 6 (small overlap)

**Symptom:** Loop iteration count is wrong (historically off by +1). The timer **value** tests pass for 6b/8b/10b — the counter itself is correct.

**Root cause:** IRQ delivery alignment. The polling loop checks `tst r2, #0x800000` (bit 23 = timer enable). The IRQ handler disables the timer via STRH. The exact instruction boundary where the IRQ fires determines whether the current iteration's LDR sees the timer as enabled or disabled. If our IRQ fires 1 instruction later than mGBA's, the loop executes one extra iteration.

**Key insight:** The 6b/8b/10b xv tests pass, proving the timer counter logic is correct. Only the iteration count is wrong, meaning this is purely an IRQ-delivery-timing issue.

**Approach:** Compare the instruction at which our emulator takes the timer overflow IRQ vs mGBA. The 7-cycle deferred scheduling model may place the IRQ 1 instruction later than expected. Need to trace the polling loop execution and identify where the IRQ should fire.

**Failed experiment:** Changing to immediate IRQ delivery (`IRQ_LATENCY_CYCLES=0`) with 7+2 cycles charged in `handleInterrupt()` gave only 514 PASS — much worse. The deferred model is structurally better; the alignment needs fine-tuning.

### Category 3: Prescaler=1 sum overlap — 18 failures

**Tests:** 12 `0b 1xs` + 6 `0b 16xs`  
**Likely same root cause as Category 2**, compounded by prescaler=1 sensitivity. Expected to be fixed alongside Category 2.

## Failure Correlation with Parameters

| Dimension | Distribution | Observation |
|-----------|-------------|-------------|
| `i=4i` | 137 (47%) | More overflows = more accumulated error |
| `i=2i` | 97 (33%) | |
| `i=1i` | 56 (19%) | Fewest failures |
| `d=1d` | 100 (34%) | ~evenly distributed |
| `d=2d` | 89 (31%) | |
| `d=4d` | 101 (35%) | |

The `d` parameter (pre-start delay) has minimal effect — it only affects the initial cycle alignment and doesn't interact much with IRQ timing.

## Key mGBA Implementation Details

Source: `src/gba/timer.c` in mGBA

- **Timer read:** `GBATimerUpdateRegister(gba, timer, 2)` — subtracts `cyclesLate=2` from `mTimingCurrentTime()`
- **Timer enable:** `GBATimerUpdateRegister(gba, timer, 0)` then `lastEvent = mTimingCurrentTime() & ~tickMask`
- **Timer disable:** `GBATimerUpdateRegister(gba, timer, 0)` — updates counter in io[], `cyclesLate=0`
- **Timer overflow:** `cyclesLate=0`, reschedules with `mTimingScheduleAbsolute`
- **`mTimingCurrentTime()`:** Returns `now + instruction_costs_so_far` (fetch + data access costs accumulated during instruction execution)
- **IRQ latency:** `GBA_IRQ_DELAY = 7` cycles, scheduled as a deferred event

## Architecture Notes

- `scheduler->getCurrentCycle()` = cycle counter at the START of the current instruction (before any costs are committed)
- `memory->getPendingCycles()` = accumulated data-access wait cycles during the current instruction
- `scheduler->advanceCycles()` = increments counter without processing events
- `scheduler->runUntil()` = processes events up to the target cycle (called after each instruction in the main loop)
- Timer overflow events fire during `runUntil()` with `currentCycle` set to the event's trigger time
- IRQ trigger events fire during `runUntil()` with 7-cycle delay from when `requestInterrupt()` was called

## Build Notes

The Makefile does **not** track header dependencies. When changing headers, manually remove affected .o files:
```bash
# After changing interrupt.h:
rm -f build/interrupt.o build/gba.o build/cpu.o

# After changing memory.h:
rm -f build/memory.o

# After changing timer_controller.h:
rm -f build/timer_controller.o

# Nuclear option:
make clean && make -j4
```

## Test Command
```bash
timeout 30 ./gba_emulator --skip-bios --run-suite=timers assets/roms/suite.gba 2>&1 | grep "mGBA DEBUG" | grep -o "PASS\|FAIL" | sort | uniq -c
```
