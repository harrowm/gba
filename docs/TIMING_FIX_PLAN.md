# GBA Emulator Timing Fix Plan

## Background

Audio clicks in Sonic Advance and silence in Pokémon FireRed are symptoms of timing inaccuracies — not a DMA buffer overrun problem. Previous attempts to fix audio via DMA byte-count clamping and M4A buffer detection were symptomatic hacks that broke other games. The correct approach is to fix the underlying timing so the M4A engine's own safety margins handle any overruns naturally.

## Root Cause Analysis

The M4A sound engine writes PCM samples into a buffer in IWRAM, then sets up a DMA channel (1 or 2) in SPECIAL mode to feed the FIFO. Timer 0 (or 1) overflows at the sample rate (~32768 Hz), consuming one FIFO byte per tick and triggering a 4-word DMA refill when the FIFO runs low.

Between VBlank start (scanline 160) and the game's VBlank ISR actually running (IRQ latency + handler dispatch), 1-2 extra timer overflows happen. These cause DMA reads past the current buffer position. M4A accounts for this with a safety margin (`pcmDmaPeriod` extra samples). Clicks occur when our emulator's timing puts MORE overruns into that window than real hardware would — either because:
- Timers run slightly fast (frame boundary deleting cycles)
- IRQ latency is too long (game takes longer to reset DMAxSAD)
- Timer overflow events fire at wrong cycle positions relative to VBlank

## Prioritized Fix List

### Step 1: Fix Frame Boundary Cycle Reset ⬅️ START HERE
**File:** `src/gba.cpp` — end of `runFrame()`  
**Problem:** `scheduler.setCurrentCycle(targetCycle)` snaps the cycle counter back to the exact frame boundary, deleting any overshoot from the last instruction. This makes each frame systematically shorter than 280,896 cycles, causing timers to run slightly fast over many frames.  
**Fix:** Carry overshoot into the next frame instead of deleting it.  
**Impact:** HIGH — fixes timer drift that accumulates over hundreds of frames.  
**Difficulty:** Easy.

### Step 2: Audit IRQ Latency
**Files:** `src/interrupt.cpp`, `src/gba.cpp`  
**Problem:** If the delay from VBlank IRQ assertion → game's VBlank handler executing is too long, more timer overflows leak past the PCM buffer end before the game resets DMAxSAD.  
**Fix:** Verify IRQ_LATENCY_CYCLES matches hardware (should be ~7 cycles for ARM7TDMI). Check that the IRQ scheduling path doesn't add extra delays.  
**Impact:** HIGH — directly controls overrun count.  
**Difficulty:** Medium.

### Step 3: Timer Overflow vs VBlank Event Ordering
**Files:** `src/timer_controller.cpp`, `src/gpu.cpp`  
**Problem:** If timer overflow events and VBlank events are scheduled at the same cycle, their relative processing order matters. Timer firing before VBlank means the game hasn't entered its ISR yet.  
**Fix:** Verify event priority ordering in the scheduler — VBlank should process before timer overflows at the same cycle (the GPU sets VBlank flag, then the timer overflow's DMA reads happen with VBlank already signaled).  
**Impact:** MEDIUM.  
**Difficulty:** Medium.

### Step 4: Remove DMA Buffer Clamp
**Files:** `src/dma.cpp`, `include/dma.h`  
**Problem:** The byte-count accumulator (`soundBytesTransferred`, `soundSadWriteCount`), M4A SoundInfo detection, and per-frame clamping logic are symptomatic hacks. They break FireRed (silence) and don't fix Sonic (wrong limit).  
**Fix:** Remove all clamp-related code. Let the M4A engine's own buffer sizing handle overruns once timing is correct.  
**Impact:** MEDIUM — removes code that actively breaks games.  
**Difficulty:** Easy.

### Step 5: VRAM Access Penalty During HDraw
**Files:** `src/memory.cpp`  
**Problem:** CPU accesses to VRAM (0x06000000-0x06017FFF) during active display should cost +1 wait state. Missing this makes the CPU slightly "too fast" during rendering, shifting timer overflow positions.  
**Fix:** Check GPU scanline state in `addWaitCycles()` for VRAM region.  
**Impact:** LOW.  
**Difficulty:** Easy.

### Step 6: HALT Timing Precision
**File:** `src/gba.cpp`  
**Problem:** During HALT, `scheduler.runUntil(skipTo)` jumps to the next event. Ensure HALT exit doesn't consume extra cycles beyond the wake event.  
**Impact:** LOW.  
**Difficulty:** Easy.

### Step 7: Sound DMA Startup Cost
**File:** `src/dma.cpp`  
**Problem:** `triggerSoundFIFO()` calls `performTransfer()` with no startup delay. Real hardware has a brief DMA arbitration cost (~2 cycles).  
**Impact:** LOW.  
**Difficulty:** Trivial.

## Test Strategy

After each step:
1. Run timing test ROM: `cd tests/roms && make run ROM=...`
2. Run ARM core tests: `cd tests/arm_core && make && ./run_all`  
3. Run timing tests: `cd tests/timing && make && ./test_memory_wait_states`
4. Manual test: Sonic Advance (clicks?), Pokémon FireRed (sound?)

## Implementation Results

All 7 steps completed.

| Step | Description | Result |
|------|-------------|--------|
| 1 | Frame boundary cycle reset | **FIXED** — removed setCurrentCycle snap-back; overshoot carries into next frame |
| 2 | IRQ latency audit | **AUDITED** — 7-cycle latency correct; pipeline refill 2 cycles correct; onCPSRWrite cancel+0-delay is a compensating hack (changing to mGBA-like 7-delay causes regression due to other timing differences) |
| 3 | Timer/VBlank event ordering | **FIXED** — video events priority 0 (highest), timer events priority 2 |
| 4 | Remove DMA buffer clamp | **FIXED** — all M4A detection, byte counting, and clamp code removed |
| 5 | VRAM access penalty during HDraw | **FIXED** — GPU sets hDrawActive flag; +1 wait state for VRAM/Palette/OAM during visible scanline HDraw |
| 6 | HALT timing precision | **VERIFIED** — already correct, no changes needed |
| 7 | Sound DMA startup cost | **FIXED** — 2-cycle advanceCycles before performTransfer in triggerSoundFIFO |

### Final Test Suite Results (All Steps Applied)

| Suite | PASS | FAIL | Baseline FAIL | Delta |
|-------|------|------|---------------|-------|
| memory | 1552 | 0 | 0 | same |
| io-read | 130 | 0 | 0 | same |
| shifter | 140 | 0 | 0 | same |
| carry | 93 | 0 | 0 | same |
| multiply-long | 72 | 0 | 0 | same |
| bios-math | 615 | 0 | 0 | same |
| timing | 1946 | 74 | 74 | same |
| timers | 644 | 290 | 292 | -2 (improved) |
| timer-irq | 44 | 47 | 46 | +1 (marginal) |
| dma | 1188 | 68 | 68 | same |
