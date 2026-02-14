# mgba-emu/suite — Full Test Results

**Last updated:** 2026-02-14 (commit `95c18b4`)

All 14 test suites run with `--skip-bios --run-suite=NAME assets/roms/suite.gba`.

---

## Summary Table

| # | Suite | Pass | Total | % | Status |
|---|-------|------|-------|---|--------|
| 0 | **memory** | 1552 | 1552 | **100%** | PERFECT |
| 1 | **io-read** | 130 | 130 | **100%** | PERFECT |
| 2 | **timing** | 1946 | 2020 | 96.3% | 74 failures — see TIMING_ISSUES_INVESTIGATED.md |
| 3 | **timers** | 646 | 938 | 68.9% | 292 failures — IRQ delivery alignment (see TIMER_TEST_ANALYSIS.md) |
| 4 | **timer-irq** | 44 | 90 | 48.9% | 46 failures — IRQ fires ~1 NOP late |
| 5 | **shifter** | 140 | 140 | **100%** | PERFECT |
| 6 | **carry** | 93 | 93 | **100%** | PERFECT (fixed: ADC/SBC/RSC flags) |
| 7 | **multiply-long** | 72 | 72 | **100%** | PERFECT (fixed: N/C flags) |
| 8 | **bios-math** | 615 | 615 | **100%** | PERFECT |
| 9 | **dma** | 1188 | 1256 | 94.6% | 68 failures — DMA data correctness |
| 10 | **sio-read** | 25 | 90 | 27.8% | 65 failures — SIO registers not implemented |
| 11 | **sio-timing** | 0 | 4 | 0% | 4 failures — SIO not implemented |
| 12 | **misc-edge** | 1 | 10 | 10% | 9 failures — DMA prefetch + H-blank timing |
| 13 | **video** | — | — | — | Visual-only (no text PASS/FAIL output) |

**Totals (text-based):** 6452 / 7010 = **92.0%**
**Perfect suites:** 6/12 (memory, io-read, shifter, carry, multiply-long, bios-math)

---

## Detailed Failure Analysis

### Suite 2: timing (74 failures) — PREVIOUSLY DOCUMENTED

See [TIMING_ISSUES_INVESTIGATED.md](TIMING_ISSUES_INVESTIGATED.md) for full analysis.
- 52 DMA ROM wait states (±1)
- 16 LDMIA OAM→ROM overflow (-1 to -4)
- 4 C loop (+1)
- 2 ldr pair (-1)

---

### Suite 3: timers (292 failures) — ACTIVE WORK AREA

See [TIMER_TEST_ANALYSIS.md](TIMER_TEST_ANALYSIS.md) for full analysis. Score: **646/938 (68.9%)**.

The suite was previously crashing (PC jumped to I/O region during IRQ). This was fixed by correcting the IRQ vector dispatch, IME address, and timer pending cycles handling.

**Failure breakdown by prescaler:**

| Prescaler | Fails | Notes |
|-----------|-------|-------|
| 0b (÷1) | 160 | Most sensitive — every CPU cycle = 1 timer tick |
| 6b (÷64) | 45 | Only `xs` (loop count) fails; `xv` (counter value) passes |
| 8b (÷256) | 43 | Same pattern as 6b |
| 10b (÷1024) | 44 | Same pattern as 6b |

**Root cause:** IRQ delivery fires 1 instruction boundary too late. The polling loop executes one extra iteration before seeing the timer disabled. Higher iteration counts (2i/4i) accumulate this error.

---

### Suite 4: timer-irq (46 failures) — IMPROVED

Tests set a timer reload value (FFFF, FFFE, ..., FFF7) and execute 0–9 NOP instructions, then check how many times the timer IRQ fired. Improved from 34→44 PASS (was 56 failures, now 46).

**Failure pattern:** Our IRQ fires ~1 NOP too late compared to hardware.
- FFFF/FFFE: all NOP counts 1-9 fail — period too short, timing must be exact
- FFF7: 0-6 NOPs fail — more margin but still off

**Root cause:** Same as timers suite — the 7-cycle deferred IRQ delivery model places the interrupt 1 instruction boundary later than hardware.

---

### Suite 6: carry — FIXED ✅ (was 23 failures, now 0)

All 93 tests pass. Fixed in commit `7d8d6cf` by correcting carry and overflow flag computation on ADC, SBC, RSC for both ARM and Thumb modes.

---

### Suite 7: multiply-long — FIXED ✅ (was 28 failures, now 0)

All 72 tests pass. Fixed in commit `80e9f56` by correcting multiply-long N flag (from RdHi bit 31) and C flag (from Booth multiplier carry).

---

### Suite 9: dma (68 failures)

These are DMA **data correctness** failures, different from the timing suite's DMA cycle-count failures.

**Pattern A — ROM source, unit count 3 (36 tests):**
- `1/2/3 Imm H =ROM/=IWRAM 3`: Got `CB0EBEEF` vs expected `CB0EDEAD` — halfword DMA from ROM gets wrong data for transfer #3
- `1/2/3 Imm W =ROM/=IWRAM 3`: Got `DEADBEEF` vs expected `DEADBEF2` — 32-bit DMA from ROM gets wrong data
- Same pattern for EWRAM destination

The 3rd transferred value is wrong, suggesting an addressing/increment bug in DMA when the source is ROM.

**Pattern B — SRAM source (14 tests):**  
- `0 Imm W =SRAM/=IWRAM 3`: Got `00000000` vs expected `00220000`
- SRAM reads return 0 instead of the expected value — SRAM/Flash read not implemented for DMA

**Pattern C — Decrementing ROM DMA (16 tests):**
- `0 Imm H -ROM/=IWRAM 3`: Decrementing DMA from ROM gets wrong data
- `0 Imm W -ROM/=IWRAM 3`: Same for 32-bit

**Pattern D — ROM offset DMA (8 tests):**
- `0 Imm W R+0x10/+IWRAM 3-6`: DMA with ROM base + offset, multiple repeat counts
- Got `0A090807` vs expected `06050403` — the stride/addressing is off

---

### Suite 10: sio-read (65 failures)

Serial I/O register reads return wrong values across all SIO modes:

| Mode prefix | Meaning | Fails |
|-------------|---------|-------|
| M: | Multiplayer mode | 11 |
| N8: | Normal 8-bit mode | 12 |
| N32: | Normal 32-bit mode | 9 |
| U: | UART mode | 13 |
| G: | General purpose mode | 10 |
| J: | JOY Bus mode | 10 |

**Root cause:** SIO registers (SIOCNT, RCNT, JOYCNT, SIOMULTI0-3, JOY_RECV, JOY_TRANS, etc.) at `0x04000120`–`0x0400015A` are not properly implemented. These registers have mode-dependent read behavior — the SIO controller has 6 modes and registers alias/change meaning depending on the active mode.

**Priority:** Low. SIO is only needed for link cable emulation. Most games work without it.

---

### Suite 11: sio-timing (4 failures)

All 4 tests fail: Normal8/256k, Normal8/2M, Normal32/256k, Normal32/2M.

**Root cause:** Same as sio-read — SIO transfer timing requires the SIO controller to be implemented.

---

### Suite 12: misc-edge (9 failures)

**DMA Prefetch (2 failures):**
- `DMA Prefetch Break`: Got `0x10002A64` vs `0x10000004` — DMA should break/invalidate the prefetch buffer  
- `DMA Prefetch Read`: Got `0xDEAD0000` vs `0x18181818` — DMA should be able to read from the prefetch buffer contents

**H-blank bit start (7 failures):**
- The H-blank flag in DISPSTAT (bit 1) transitions at the wrong cycle
- `Hblank`: Got `0x4D1` vs `0x4D0` — off by 1 dot cycle
- `Flip 1-6`: Various H-blank transition timing tests fail with varying deltas

**Root cause:** The GPU's H-blank flag is set 1 dot too late. The "Flip" tests check exactly which dot the H-blank bit transitions, and our timing is consistently off.

---

### Suite 13: video (visual-only)

This suite outputs test patterns on screen for visual comparison. No text PASS/FAIL output. Would need screenshot comparison against reference images to evaluate.

---

## Priority Ranking for Fixes

| Priority | Issue | Failures Fixed | Difficulty | Impact |
|----------|-------|----------------|------------|--------|
| ~~1~~ | ~~Carry flag (V) on SBC/RSC/ADC~~ | ~~23~~ | ~~Easy~~ | ✅ DONE |
| ~~2~~ | ~~Multiply-long flags (UMULLS/SMULLS)~~ | ~~28~~ | ~~Easy~~ | ✅ DONE |
| ~~3~~ | ~~Timer IRQ crash (timers suite)~~ | ~~Unknown~~ | ~~Medium~~ | ✅ DONE |
| **1** | Timer/IRQ delivery alignment | ~280 (timers+timer-irq) | Medium | Timer accuracy |
| **2** | DMA data correctness | 68 | Medium-Hard | DMA accuracy |
| **3** | H-blank flag timing | 7 | Medium | GPU accuracy |
| **4** | Timing suite remaining | 74 | Hard | Already documented |
| **5** | DMA prefetch interaction | 2 | Hard | Edge case |
| **6** | SIO registers | 69 | Large feature | Link cable only |
