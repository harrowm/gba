# GBA Emulator — Test Suite Status

**Last updated:** 2026-02-14
**Commit:** `95c18b4` — Fix BIOS boot freeze: add write-to-clear for IF register in write8

---

## mgba-emu/suite Results Summary

| # | Suite | Pass | Total | % | Change vs Baseline | Status |
|---|-------|------|-------|---|--------------------|--------|
| 0 | **memory** | 1552 | 1552 | **100%** | — | PERFECT |
| 1 | **io-read** | 130 | 130 | **100%** | — | PERFECT |
| 2 | **timing** | 1946 | 2020 | 96.3% | — | 74 failures (see TIMING_ISSUES_INVESTIGATED.md) |
| 3 | **timers** | 646 | 938 | 68.9% | — | Active work area (see TIMER_TEST_ANALYSIS.md) |
| 4 | **timer-irq** | 44 | 90 | 48.9% | +10 vs old | IRQ delivery timing off by ~1 NOP |
| 5 | **shifter** | 140 | 140 | **100%** | — | PERFECT |
| 6 | **carry** | 93 | 93 | **100%** | +23 vs old | PERFECT (was 75.3%) |
| 7 | **multiply-long** | 72 | 72 | **100%** | +28 vs old | PERFECT (was 61.1%) |
| 8 | **bios-math** | 615 | 615 | **100%** | — | PERFECT |
| 9 | **dma** | 1188 | 1256 | 94.6% | — | 68 failures — DMA data correctness |
| 10 | **sio-read** | 25 | 90 | 27.8% | — | SIO registers not implemented |
| 11 | **sio-timing** | 0 | 4 | 0% | — | SIO not implemented |
| 12 | **misc-edge** | 1 | 10 | 10% | — | DMA prefetch + H-blank edge cases |
| 13 | **video** | — | — | — | — | Visual-only (no text output) |

**Totals (text-based):** 6452 / 7010 = **92.0%**
**Perfect suites:** 6/12 (memory, io-read, shifter, carry, multiply-long, bios-math)

---

## Recent Improvements

### Commit `95c18b4` — BIOS Boot Fix
- **Bug:** `write8` to IF register (0x04000202) had no write-to-clear semantics
- **Impact:** BIOS IRQ handler uses `STRB` to acknowledge VBlank interrupts. Without write-to-clear, the VBlank flag was never cleared, trapping the CPU in an infinite IRQ loop. Result: no Nintendo logo animation, games freeze when booting through BIOS
- **Fix:** Added `base[offset] = base[offset] & ~value` for write8 to 0x04000202/0x04000203

### Commit `7d8d6cf` — Carry/Overflow Flags (carry: 70→93, multiply-long: 44→72)
- Fixed carry and overflow flag computation on ADC, SBC, RSC for ARM and Thumb
- Fixed multiply-long N flag (from RdHi) and C flag (from Booth carry)

### Commit `efb6278` — Timer Enable/Disable Path (timers: 556→646)
- Use raw `getCurrentCycle()` for timer enable/disable instead of adding `pendingCycles + 1`
- Prescaler alignment on enable (`lastReloadCycle & ~tickMask`)

### Commit `fc1e6ed` — Pipeline Refill on PC Load (timers: ~495→526)
- Added +2 cycle pipeline refill for LDR/LDM/POP that modify PC

---

## Failure Analysis by Suite

### timers (292 failures) — Primary Work Area

**Test format:** `Nb, 0xRRRR Mxs/Mxv Dd Di`
- `Nb` = prescaler (0=1, 6=64, 8=256, 10=1024)
- `0xRRRR` = reload value (period = 0x10000 - reload)
- `xs`/`xv` = loop iteration count / timer counter value
- `Dd` = delay iterations before timer start
- `Di` = number of overflows before disable

**Breakdown by prescaler:**

| Prescaler | Fails | Total | % Pass |
|-----------|-------|-------|--------|
| 0b (÷1) | 160 | ~468 | 65.8% |
| 6b (÷64) | 45 | ~156 | 71.2% |
| 8b (÷256) | 43 | ~156 | 72.4% |
| 10b (÷1024) | 44 | ~156 | 71.8% |

**Breakdown by iteration count:**

| Iterations | Fails | % of failures | Observation |
|------------|-------|---------------|-------------|
| 1i | 57 | 20% | Single overflow — base timing error |
| 2i | 98 | 34% | Error doubles |
| 4i | 137 | 47% | Error quadruples — cumulative drift |

**Root cause categories:**
1. **Category 1 (57 tests, 1i):** Single-overflow cycle count error in IRQ handler → timer disable path. Small reload values (0x0005–0x0015) are most sensitive because the error mod period produces a wrong counter value.
2. **Category 2 (235 tests, 2i/4i):** Same per-overflow error accumulated across multiple overflows. The per-overflow error is consistent (~1 cycle), meaning the fundamental IRQ-to-disable path timing is off by 1 instruction boundary.
3. **Category 3 (0b overlap, ~18 tests):** Prescaler=1 makes every CPU cycle visible as a timer tick, amplifying instruction-level timing errors hidden by ÷64/÷256/÷1024 prescalers.

**Key insight:** The 6b/8b/10b `xv` (counter value) tests mostly **pass** — the timer counter itself is correct. Only the `xs` (loop iteration count) tests fail. This proves the timer is counting cycles correctly but IRQ delivery fires 1 instruction boundary too late, causing the polling loop to execute one extra iteration.

### timer-irq (46 failures)

Tests configure timer with reload FFFF–FFF7, execute 0–9 NOP instructions, then check how many times the timer IRQ fired. Pattern: our IRQ fires ~1 NOP too late. Same root cause as timers Category 2 — the 7-cycle deferred IRQ delivery model places the interrupt 1 instruction later than hardware.

**Failing pattern:**
- FFFF (period=1): all 1–9 NOPs fail
- FFFE (period=2): all 1–9 NOPs fail
- FFFD (period=3): 0 NOPs fail
- ...down to FFF7 (period=9): 0–6 NOPs fail

### timing (74 failures)

Fully analyzed in [TIMING_ISSUES_INVESTIGATED.md](TIMING_ISSUES_INVESTIGATED.md):
- 52 DMA ROM wait states (±1 per-unit)
- 16 LDMIA OAM→ROM overflow (-1 to -4 with prefetch)
- 4 C loop (+1 from LDR→STR credit waste)
- 2 ldr pair (-1 prefetch buffer state)

### dma (68 failures)

DMA data transfer correctness — not yet deeply investigated. Likely related to region crossing, alignment, or DMA mode handling.

### sio-read (65 failures) / sio-timing (4 failures)

Serial I/O completely unimplemented. These will all fail until SIO register stubs are added.

### misc-edge (9 failures)

DMA prefetch interactions and H-blank timing edge cases. Low priority.

---

## What to Fix Next — Priority Recommendations

### Priority 1: Timer/IRQ Delivery Alignment (est. +200 tests across timers + timer-irq)
**Impact:** Would fix timers Categories 2+3 (~235 tests) and timer-irq (~46 tests)
**Approach:** The consistent +1 iteration count across all prescalers and configurations points to a single root cause: the 7-cycle deferred IRQ delivery fires at the wrong instruction boundary. Need to trace the exact cycle where mGBA delivers the timer overflow IRQ vs where we deliver it. Possible fixes:
- Adjust `cyclesLate` compensation when scheduling the timer overflow IRQ event
- Change when in the instruction cycle the IRQ check happens (before vs after advance)
- Fine-tune the interaction between timer overflow event and IRQ scheduling

### Priority 2: Save Support (game compatibility)
**Impact:** Most commercial games can't save — EEPROM, Flash, SRAM persistence all missing
**Approach:** Implement SRAM file persistence first (easiest), then EEPROM serial protocol, then Flash command protocol

### Priority 3: VCount Match IRQ (game compatibility)
**Impact:** Breaks raster effects and VCount-based timing in many games
**Approach:** Compare DISPSTAT VCount setting against current scanline each HBlank, fire IRQ on match

### Priority 4: PSG Audio Channels 1–4 (polish)
**Impact:** Nearly every game has missing music/SFX
**Approach:** Implement square wave (ch1/2), wave table (ch3), noise (ch4), frame sequencer

---

## Build & Test Commands

```bash
# Build
make -j4

# Run individual suite
timeout 30 ./gba_emulator --skip-bios --run-suite=timers assets/roms/suite.gba 2>&1 \
  | grep "mGBA DEBUG" | grep -o "PASS\|FAIL" | sort | uniq -c

# Run with BIOS boot (Nintendo logo animation)
./gba_emulator assets/roms/sonic.gba

# Run with skip-bios
./gba_emulator --skip-bios assets/roms/sonic.gba

# Header dependency note: Makefile doesn't track headers
rm -f build/memory.o   # after changing memory.h
rm -f build/interrupt.o build/gba.o build/cpu.o  # after changing interrupt.h
```

---

## Historical Milestones

| Date | Commit | Event | Score Change |
|------|--------|-------|--------------|
| 2025-10 | various | Initial timing validation — loop timing matches mGBA | — |
| 2025-10 | `caf7292` | Sequential wait states for LDM/STM | timing: 1882→1946 |
| 2025-10 | various | DMA 3-cycle startup delay | timing: +64 tests |
| 2026-02 | `b750ea0` | Timer fixes: pending cycles, prescaler alignment | timers: 0→469 |
| 2026-02 | `fc1e6ed` | Pipeline refill for PC loads | timers: 495→526 |
| 2026-02 | `efb6278` | Timer enable/disable raw cycle | timers: 556→646 |
| 2026-02 | `7d8d6cf` | Carry/overflow flag fixes | carry: 70→93, mul-long: 44→72 |
| 2026-02 | `95c18b4` | BIOS boot fix (IF write-to-clear in write8) | BIOS boot works |
