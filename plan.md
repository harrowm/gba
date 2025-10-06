# GBA Emulator Implementation Plan

## Overview

This document outlines a phased approach to building a cycle-accurate GBA emulator. Each phase has measurable milestones and builds incrementally toward running commercial games.

## Current Status

✅ **Completed**:
- ARM7TDMI CPU emulation (ARM instruction set)
- Thumb instruction set implementation
- Comprehensive test coverage (500 ARM core tests + 36 scheduler tests + 33 memory timing tests + 10 video timing tests = **579 tests passing**)
- Basic memory system
- Debug infrastructure with Capstone disassembly
- **✅ Event scheduler fully implemented and tested (36 tests passing)**
- **✅ Timing system foundation complete**
- **✅ All compilation warnings resolved across test suites**
- **✅ CPU-Scheduler integration complete (6 integration tests passing)**
  - CPU advances scheduler during instruction execution
  - Memory wait states actively tracked
  - Both ARM and Thumb instruction execution integrated
- **✅ Video timing system complete (10 video timing tests passing)**
  - Scheduler-driven main loop (280,896 cycles per frame)
  - H-Blank and V-Blank interrupts implemented
  - VCOUNT and DISPSTAT registers functional
  - Mode 3 framebuffer access ready

🚧 **In Progress**:
- Mode 3 bitmap rendering (framebuffer ready, need actual rendering)
- Additional video modes (0, 1, 2)

## Phased Approach

---

## Phase 1: Basic Timing & Memory Framework (Foundation)

**Goal**: Get a minimal cycle-accurate timing system working  
**Timeline**: Weeks 1-2  
**Measurable Milestone**: Simple ROM boots and executes instructions with accurate timing

### Tasks

1. **Implement basic scheduler/timing system** ✅ **COMPLETE**
   - ✅ Event-driven scheduler fully implemented (`scheduler.cpp`)
   - ✅ CPU cycles as the time base
   - ✅ Priority queue for events (timer, DMA, video)
   - ✅ 36 comprehensive tests passing (15 core + 10 integration + 11 mock component tests)
   - ✅ Verified with GPU and Timer mock simulations

2. **Memory timing accuracy** ✅ **COMPLETE**
   - ✅ Add wait states for different memory regions (BIOS, ROM, RAM, I/O)
   - ✅ EWRAM: 3 cycles (8/16-bit), 6 cycles (32-bit)
   - ✅ IWRAM: 1 cycle (fastest)
   - ✅ ROM: 5 cycles (8/16-bit), 8 cycles (32-bit) - default, configurable via WAITCNT
   - ✅ Comprehensive test suite: 33 tests passing
   - ⬜ Sequential vs non-sequential access timing (TODO: requires prefetch buffer)
   - ⬜ WAITCNT register implementation (TODO: configurable wait states)

3. **Wire Scheduler into CPU execution loop** ✅ **COMPLETE**
   - ✅ CPU class has Scheduler pointer and integration methods
   - ✅ `advanceCycles(uint32_t cycles)` implemented
   - ✅ Memory wait cycles actively advance scheduler
   - ✅ ARM `executeOneInstruction()` implemented with timing
   - ✅ Thumb `executeOneInstruction()` implemented with timing
   - ✅ 6 integration tests verifying cycle advancement
   - **Measurable**: Every instruction and memory access advances global cycle counter

4. **Simple test ROMs** 🚧 **READY TO START**
   - ⬜ Use existing ARM/Thumb test ROMs
   - ⬜ Create timing verification tests (measure cycle counts)
   - **Measurable**: Compare cycle counts against mGBA or known values

### Deliverables
- [x] Event scheduler working with cycle accuracy ✅ **DONE**
- [x] Memory wait states implemented ✅ **DONE** (33 tests passing)
- [x] CPU-Scheduler integration complete ✅ **DONE** (6 integration tests passing)
- [ ] Timing test ROMs pass with correct cycle counts (TODO: create dedicated timing tests)

---

## Phase 2: Video Basics (First Visual Output) ✅ **COMPLETE**

**Goal**: Display something on screen  
**Timeline**: Weeks 1-2 (parallel with Phase 1)  
**Measurable Milestone**: Can render static images, display VRAM contents

### Tasks

1. **Scanline timing** ✅ **COMPLETE** (most critical for accuracy)
   - ✅ 280,896 cycles per frame (16.78 MHz / 59.73 Hz)
   - ✅ 1232 cycles per scanline (960 H-Draw + 272 H-Blank)
   - ✅ 228 total scanlines (160 visible + 68 V-Blank)
   - ✅ H-Blank: 272 cycles (scheduled after H-Draw)
   - ✅ V-Blank: 83,776 cycles (68 scanlines starting at scanline 160)
   - ✅ Scheduler-driven with recursive event scheduling
   
2. **Mode 3 (Bitmap) foundation** ✅ **COMPLETE**
   - ✅ Framebuffer access via VRAM (240x160, 16-bit color)
   - ✅ getFrameBuffer() method for direct access
   - ✅ renderMode3Scanline() stub ready for pixel rendering
   - ✅ **Measurable**: Test can write pixels to VRAM and verify access

3. **Basic V-Blank interrupt** ✅ **COMPLETE**
   - ✅ Trigger interrupt at scanline 160
   - ✅ Update VCOUNT register every scanline
   - ✅ DISPSTAT register with V-Blank/H-Blank flags
   - ✅ Interrupt controller with IE/IF/IME registers
   - ✅ H-Blank interrupt also implemented
   - ✅ **Measurable**: 10 comprehensive video timing tests passing

4. **Main loop integration** ✅ **COMPLETE**
   - ✅ GBA::runFrame() advances scheduler by full frame (280,896 cycles)
   - ✅ GPU callbacks wired to interrupt controller
   - ✅ Video timing events automatically scheduled recursively
   - ✅ **Measurable**: Frame count increments, scheduler advances correctly

### Why Mode 3 First?
- Simplest graphics mode
- Many demos/homebrew use it
- No sprites, backgrounds, or tiles to worry about
- Immediate visual feedback

### Deliverables
- [x] V-Blank timing accurate ✅ **DONE** (verified in tests)
- [x] VCOUNT register updates correctly ✅ **DONE** (test: VCountUpdatesEachScanline)
- [x] DISPSTAT register functional ✅ **DONE** (V-Blank/H-Blank flags working)
- [x] Framebuffer accessible ✅ **DONE** (test: Mode3FrameBufferAccessible)
- [x] Main loop integrated with scheduler ✅ **DONE** (runFrame() implemented)
- [ ] Actual Mode 3 rendering to display (TODO: pixel rendering loop in renderMode3Scanline())

---

## Phase 3: Interrupt System (Critical for Games)

**Goal**: Full interrupt support  
**Timeline**: Weeks 3-4  
**Measurable Milestone**: Games can respond to V-Blank, H-Blank, timers

### Tasks

1. **Complete interrupt controller**
   - IE (Interrupt Enable), IF (Interrupt Flag), IME (Master Enable)
   - Proper interrupt handling: save PC+4 to LR_irq, switch to IRQ mode
   - Interrupt priorities

2. **Timer interrupts**
   - 4 hardware timers
   - Cascade mode
   - Overflow detection and interrupts
   - **Measurable**: Timer test ROMs (e.g., from mGBA test suite)

3. **DMA channels** (at least DMA3 for sound)
   - 4 DMA channels
   - Different trigger modes (immediate, V-Blank, H-Blank, sound)
   - Timing: 2 cycles per transfer + setup
   - **Measurable**: DMA copy tests, screen fill tests

### Deliverables
- [ ] All interrupt types working
- [ ] 4 timers implemented with overflow
- [ ] DMA channels 0-3 functional
- [ ] Tonc `first.gba` demo runs correctly

---

## Phase 4: Complete Video (Tile Modes)

**Goal**: Run most 2D games  
**Timeline**: Weeks 5-8  
**Measurable Milestone**: Can run commercial games with backgrounds and sprites

### Tasks

1. **Background modes 0-2**
   - Mode 0: 4 tile backgrounds
   - Mode 1: 2 tile BGs + 1 affine BG
   - Mode 2: 2 affine BGs
   - Scrolling, tile maps, palette modes

2. **Sprite engine (OAM)**
   - 128 sprites (objects)
   - 4bpp/8bpp modes
   - Affine transformations
   - Priority and transparency

3. **Windowing and effects**
   - Blending (alpha, brightness)
   - Windows (0, 1, OBJ)
   - **Measurable**: Run graphics test ROMs (Tonc demos, mGBA suite)

### Implementation Order
1. Mode 0 backgrounds (simplest tile mode)
2. Basic sprites (no affine)
3. Mode 1 & 2 (affine backgrounds)
4. Affine sprites
5. Windows and blending

### Test ROMs to Target
- `suite.gba` from mGBA test suite
- Tonc demos (visual feedback)
- Simple commercial games (Advance Wars, Fire Emblem)

### Deliverables
- [ ] Mode 0 backgrounds rendering
- [ ] Sprites displaying correctly
- [ ] Mode 1 & 2 working
- [ ] Pokemon title screen displays
- [ ] mGBA graphics tests pass

---

## Phase 5: Audio (Basic)

**Goal**: Get sound working  
**Timeline**: Weeks 9-10  
**Measurable Milestone**: Can hear music and sound effects

### Tasks

1. **Direct Sound (PCM)**
   - 2 DMA sound channels (A & B)
   - 8-bit signed samples
   - Mix at 32768 Hz
   - **Measurable**: Play WAV files through DMA

2. **PSG (Pulse/Noise)**
   - 4 channels: 2 square, 1 wave, 1 noise
   - Lower priority than Direct Sound
   - **Measurable**: Play simple tones

3. **Sound timing**
   - Sample generation tied to CPU cycles
   - Buffer management to avoid clicks/pops

### Deliverables
- [ ] Direct Sound channels working
- [ ] PSG channels functional
- [ ] Audio synchronized with video
- [ ] Games have music and sound effects

---

## Phase 6: Cycle Accuracy Refinement

**Goal**: Pass timing test suites  
**Timeline**: Week 12+ (ongoing)  
**Measurable Milestone**: Pass mGBA timing tests, AGS Aging Cartridge

### Tasks

1. **Prefetch/Pipeline**
   - ARM7TDMI prefetch behavior
   - Sequential access optimization
   
2. **DMA timing precision**
   - Cycle stealing from CPU
   - Proper trigger timing

3. **Open bus behavior**
   - Reading unmapped memory returns last value on bus

4. **Edge cases**
   - Mid-scanline effects
   - DMA during H-Blank
   - Timer/interrupt timing edge cases

### Test Suites
- mGBA test suite (`suite.gba`)
- AGS Aging Cartridge
- TONC timing demos
- Hardware test ROMs from community

### Deliverables
- [ ] mGBA test suite passes
- [ ] AGS Aging Cartridge passes
- [ ] Complex games run correctly
- [ ] Cycle-accurate timing verified

---

## Phase 7: Optimization & Polish

**Goal**: Run at full speed with cycle accuracy  
**Timeline**: Ongoing  
**Measurable Milestone**: 60 FPS on target platform

### Tasks

1. **Performance optimization**
   - Dynarec/JIT (optional but recommended)
   - Caching optimizations
   - Fast paths for common operations

2. **Features**
   - Save states
   - Save game support (EEPROM, SRAM, Flash)
   - Rewind functionality

3. **Game-specific fixes**
   - Compatibility database
   - Per-game tweaks if needed

### Deliverables
- [ ] Full speed emulation (60 FPS)
- [ ] Save states working
- [ ] Game saves persist
- [ ] High compatibility rate

---

## Concrete Milestones for Motivation

### Milestone 1: "Hello World" (Week 1-2)
- Display colored rectangle on screen
- Changes color every V-Blank
- **ROM**: Write your own test ROM

### Milestone 2: "It's Alive!" (Week 3-4)
- Run Tonc's `first.gba` demo
- See rotating background
- **ROM**: Tonc tutorials

### Milestone 3: "It's a Game!" (Week 6-8)
- Boot intro screens of commercial games
- See title screens with sprites
- **ROM**: Pokemon, Mario Kart

### Milestone 4: "Playable!" (Week 10-12)
- Play simple games (Tetris, Dr. Mario)
- Audio working
- **ROM**: Puzzle games (simpler than action games)

### Milestone 5: "Cycle Accurate" (Week 12+)
- Pass mGBA test suite
- Complex games run correctly
- **ROM**: Suite.gba, hardware tests

---

## Recommended Resources

### Test ROMs (in order of usefulness)

1. **mGBA test suite** - Comprehensive, well-documented
2. **Tonc demos** - Visual feedback for graphics
3. **AGS Aging Cartridge** - Hardware timing tests
4. **jsmolka's gba-tests** - Specific instruction/timing tests
5. **Your own test ROMs** - Custom tests for specific features

### Reference Emulators

1. **mGBA** (source code) - Best reference for accuracy
2. **NanoBoyAdvance** - Clean, modern C++ implementation
3. **GBATEK** - Hardware documentation

### Tools

1. **no$gba debugger** - Best GBA debugger
2. **mGBA** - Compare traces
3. **gbafix** - Fix ROM headers
4. **devkitARM** - Build test ROMs

### Documentation

- **GBATEK** - Most comprehensive hardware documentation
- **Tonc** - Excellent GBA programming tutorials
- **CowBite Spec** - Alternative hardware docs
- **ARM7TDMI Technical Reference Manual** - CPU documentation

---

## Implementation Priority Matrix

### High Priority (Do First)
1. ✅ CPU (ARM + Thumb) - **DONE** (825 tests passing)
2. ✅ Timing system - **DONE** (Scheduler complete with 36 tests)
3. ⏳ Mode 3 graphics - **IN PROGRESS** (Foundation ready)
4. ⬜ V-Blank interrupt - **NEXT**
5. ⬜ Basic timers - **NEXT**

### Medium Priority (Core Functionality)
6. ⬜ DMA channels
7. ⬜ Interrupt controller
8. ⬜ Mode 0 backgrounds
9. ⬜ Basic sprites
10. ⬜ Direct Sound audio

### Lower Priority (Enhanced Features)
11. ⬜ Mode 1/2 backgrounds
12. ⬜ Affine sprites
13. ⬜ PSG audio
14. ⬜ Windows/blending
15. ⬜ Save game support

### Polish (After Core Complete)
16. ⬜ Optimization/JIT
17. ⬜ Save states
18. ⬜ Game-specific fixes
19. ⬜ User interface improvements

---

## Success Criteria

### Phase 1-2 Success
- ✅ Test ROM displays colored rectangle
- ✅ Rectangle changes on V-Blank
- ✅ Cycle counts match reference emulator

### Phase 3 Success
- ✅ Tonc `first.gba` runs
- ✅ Rotating gradient displays
- ✅ Timer tests pass

### Phase 4 Success
- ✅ Pokemon boots to title screen
- ✅ Sprites visible
- ✅ Background scrolling works
- ✅ mGBA graphics tests pass

### Phase 5 Success
- ✅ Music plays in games
- ✅ Sound effects trigger correctly
- ✅ No audio crackling

### Phase 6 Success
- ✅ mGBA test suite: 100% pass rate
- ✅ AGS Aging Cartridge passes
- ✅ Complex games run without glitches

### Phase 7 Success
- ✅ 60 FPS on target platform
- ✅ Save states work reliably
- ✅ High game compatibility (95%+)

---

## Current Action Items (Starting Point)

### Week 1 Tasks
1. Complete scheduler event system
2. Implement memory wait states
3. Set up Mode 3 framebuffer
4. Create simple test ROM to display pattern

### Week 2 Tasks
1. V-Blank timing and interrupt
2. VCOUNT register
3. Display test pattern synchronized to V-Blank
4. Verify cycle accuracy with timing tests

### Next Steps
- Begin Phase 3 (interrupts and timers)
- Run Tonc demos
- Compare output with mGBA

---

## Notes

- **Cycle accuracy from the start**: Don't compromise on timing accuracy early on. It's much harder to retrofit later.
- **Visual feedback is critical**: Implement graphics early for motivation and debugging.
- **Test constantly**: Run test ROMs after every major change.
- **Reference mGBA source**: When stuck, check how mGBA implements features.
- **Document as you go**: Keep notes on implementation decisions and edge cases.

---

## Revision History

- **2025-10-06**: Initial plan created
  - CPU implementation complete with 825 tests passing
  - Starting Phase 1 & 2 (timing + Mode 3 graphics)

- **2025-10-06 Evening**: Phase 1 major milestones completed
  - Event scheduler fully implemented and tested (36 tests)
  - Memory wait states complete (33 tests)
  - CPU-Scheduler integration complete (6 integration tests)

- **2025-10-06 Late Evening**: Phase 2 video timing complete
  - Video timing system fully implemented (10 comprehensive tests)
  - Scheduler-driven main loop with runFrame() method
  - H-Blank and V-Blank interrupts working
  - VCOUNT and DISPSTAT registers functional
  - Interrupt controller complete (IE/IF/IME registers)
  - Mode 3 framebuffer access ready
  - Total: **579 tests passing** (500 ARM + 36 scheduler + 33 memory + 10 video)
  - Fixed test infrastructure issue (test mode vs normal mode for I/O registers)
  - **All compilation and runtime warnings eliminated** (0 warnings)
    - Fixed sign comparison warnings in test_scheduler_integration.cpp
    - Suppressed Keystone assembler warnings about deprecated PC/SP in STM/LDM (valid for ARM7TDMI)
  - Total: 569 tests passing (500 CPU + 36 scheduler + 33 timing)
  - Linker error resolved (added scheduler.cpp to Makefile)
  - Every instruction and memory access now advances global cycle counter
  - Ready for Phase 2: V-Blank interrupt and graphics timing

- **2025-10-06 (Evening)**: Phase 1 Timing System Complete
  - ✅ Event scheduler fully implemented and tested
  - ✅ 36 scheduler tests passing (0 warnings, 100% pass rate)
  - ✅ Mock GPU and Timer integration tests validated
  - ✅ All compilation warnings resolved across test suites
  - ✅ Memory wait states implemented (33 tests passing)
  - ✅ Cycle-accurate memory timing for all regions:
    - BIOS, IWRAM, I/O: 1 cycle (fastest)
    - EWRAM: 3/6 cycles (16-bit bus)
    - Palette RAM, VRAM: 1/2 cycles  
    - GamePak ROM: 5/8 cycles (configurable)
    - GamePak SRAM: 5 cycles
  - **Next**: Integrate scheduler with CPU, V-Blank interrupt, real Timer implementation
