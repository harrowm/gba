# mgba-emu/suite — Full Test Results

All 14 test suites run with `--skip-bios --run-suite=NAME assets/roms/suite.gba`.

---

## Summary Table

| # | Suite | Pass | Total | % | Status |
|---|-------|------|-------|---|--------|
| 0 | **memory** | 1552 | 1552 | **100%** | PERFECT |
| 1 | **io-read** | 130 | 130 | **100%** | PERFECT |
| 2 | **timing** | 1946 | 2020 | 96.3% | 74 failures — see TIMING_ISSUES_INVESTIGATED.md |
| 3 | **timers** | 0 | ? | — | CRASH — PC jumped to I/O region during IRQ |
| 4 | **timer-irq** | 34 | 90 | 37.8% | 56 failures — timer overflow counting off-by-N |
| 5 | **shifter** | 140 | 140 | **100%** | PERFECT |
| 6 | **carry** | 70 | 93 | 75.3% | 23 failures — V flag wrong on sbcs/rscs/adcs |
| 7 | **multiply-long** | 44 | 72 | 61.1% | 28 failures — C flag wrong on umulls/smulls |
| 8 | **bios-math** | 615 | 615 | **100%** | PERFECT |
| 9 | **dma** | 1188 | 1256 | 94.6% | 68 failures — DMA data correctness |
| 10 | **sio-read** | 25 | 90 | 27.8% | 65 failures — SIO registers not implemented |
| 11 | **sio-timing** | 0 | 4 | 0% | 4 failures — SIO not implemented |
| 12 | **misc-edge** | 1 | 10 | 10% | 9 failures — DMA prefetch + H-blank timing |
| 13 | **video** | — | — | — | Visual-only (no text PASS/FAIL output) |

**Totals (text-based suites):** 5745 / 6062+ = ~94.8%

---

## Detailed Failure Analysis

### Suite 2: timing (74 failures) — PREVIOUSLY DOCUMENTED

See [TIMING_ISSUES_INVESTIGATED.md](TIMING_ISSUES_INVESTIGATED.md) for full analysis.
- 52 DMA ROM wait states (±1)
- 16 LDMIA OAM→ROM overflow (-1 to -4)
- 4 C loop (+1)
- 2 ldr pair (-1)

---

### Suite 3: timers — CRASH

The emulator crashes before producing any test output. The CPU enters IRQ mode (CPSR mode 0x12) and the PC jumps to `0x04000000` (I/O register space), then runs through `0xDEADBEEF` values.

**Root cause:** The timers suite installs its own IRQ handler and expects timer overflow IRQs to fire. The IRQ dispatch path has a bug — the return address in LR_irq or the jump target from the IRQ vector table is wrong, causing execution to land in I/O space instead of returning to the handler.

**Key crash trace evidence:**
- Mode 0x12 (IRQ), LR=0x00000138 (BIOS IRQ return point)
- R0=0x04000000 — the CPU tried to use this as a branch target
- The handler at 0x030004B0 looks valid (contains counter increment logic)
- Likely cause: IRQ vector/dispatch table not properly set up, or the BIOS IRQ handler trampoline at 0x128–0x138 isn't working correctly with the test's custom handler

---

### Suite 4: timer-irq (56 failures)

Tests set a timer reload value (FFFF, FFFE, ..., FFF7) and execute 0–9 NOP instructions, then check how many times the timer IRQ fired.

**Failure pattern:** Our IRQ fires ~1 NOP too late compared to hardware.
- `FFFF 0 nops` → PASS (0 fires, correct)
- `FFFF 1 nop` → FAIL: Got 0000, expected 0001 (IRQ should have fired after 1 NOP)
- `FFFF 4+ nops` → FAIL: Got 0000, expected 0051 (way off — likely IRQ never fires at all for some configs)

For reload values further from overflow (FFFD, FFFC), the "0 nops" case also fails with Got 0000 != FFFF, suggesting the timer counter read-back is wrong too.

**Root cause:** Timer IRQ latency or timer counter update timing is wrong. The timer overflow detection and IRQ assertion happen at the wrong cycle boundary relative to the CPU instruction stream.

---

### Suite 6: carry (23 failures)

All failures are **V (overflow) flag** errors on `sbcs`, `rscs`, and `adcs` instructions. The result value is always correct — only the flags differ.

| Pattern | Got CPSR | Expected CPSR | Flag diff |
|---------|----------|---------------|-----------|
| `0, 0x7FFFFFFF (.) sbcs` | 9 (NV) | 8 (N) | V set, should be clear |
| `0, 0x7FFFFFFF (C) adcs` | 8 (N) | 9 (NV) | V clear, should be set |
| `0, 0xFFFFFFFF (.) sbcs` | 6 (CV) | 4 (C) | V set, should be clear |
| `0, 0xFFFFFFFF (C) adcs` | 4 (C) | 6 (CV) | V clear, should be set |

CPSR flags encoding: bit 3=N, bit 2=Z, bit 1=C, bit 0=V (as printed by the test).

**Root cause:** The V flag calculation for SBC/RSC/ADC with carry is getting the overflow detection wrong for edge cases involving 0x7FFFFFFF, 0x80000000, and 0xFFFFFFFF. The carry-in from CPSR affects the overflow differently than our implementation computes.

Specifically: `SBC` is `Rn - Op2 - !C`. The overflow should be computed on the full subtraction including the borrow, but our code may compute it on the intermediate result or miss the carry-in contribution.

---

### Suite 7: multiply-long (28 failures)

All failures are on `umulls` (unsigned multiply long + S flag) and `smulls` (signed multiply long + S flag). Results are always correct — only CPSR flags differ.

**Pattern:** We're setting the N flag based on bit 63 of the result, but the **C flag** behavior is wrong. On ARM7TDMI hardware, the C flag after MULL+S is "UNPREDICTABLE" per the ARM ARM, but real GBA hardware has deterministic behavior that the test expects.

| Example | Got CPSR | Expected CPSR | Issue |
|---------|----------|---------------|-------|
| `-1 * 1 umulls` → result `00000000:FFFFFFFF` | 8 (N) | 0 (none) | N flag wrong |
| `-1 * -1 umulls` → result `FFFFFFFE:00000001` | 8 (N) | A (NZ) | N and Z flag both wrong |

**Root cause:** Our `updateFlagsMultiply()` is likely computing N from the wrong half of the 64-bit result, or the C/V flag clearing behavior doesn't match hardware. The ARM7TDMI sets N from bit 31 of RdHi, Z from the full 64-bit result being zero, and C/V are "meaningless" but have specific hardware behavior.

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
| **1** | Carry flag (V) on SBC/RSC/ADC | 23 | Easy | Correctness |
| **2** | Multiply-long flags (UMULLS/SMULLS) | 28 | Easy | Correctness |
| **3** | Timer IRQ crash (timers suite) | Unknown | Medium | Unblocks a whole suite |
| **4** | Timer IRQ latency | 56 | Medium | Timer accuracy |
| **5** | H-blank flag timing | 7 | Medium | GPU accuracy |
| **6** | DMA data correctness | 68 | Medium-Hard | DMA accuracy |
| **7** | DMA prefetch interaction | 2 | Hard | Edge case |
| **8** | Timing suite remaining | 74 | Hard | Already documented |
| **9** | SIO registers | 69 | Large feature | Link cable only |
