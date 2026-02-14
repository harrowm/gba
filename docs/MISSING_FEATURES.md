# GBA Emulator — Feature Audit

## 🔴 Missing Features (Not Implemented)

| # | Feature | Impact | Notes |
|---|---------|--------|-------|
| 1 | **PSG Channels 1–4** | 🔴 High | All 4 stubs return 0. No square wave, wave table, noise, sweep, envelope, or length counters. Frame sequencer never ticks. Affects nearly all games with music/SFX |
| 2 | **EEPROM saves** (512B / 8KB) | 🔴 High | No serial protocol emulation. Many commercial games use EEPROM |
| 3 | **Flash saves** (64K / 128K) | 🔴 High | No Atmel/SST/Macronix command protocol. Many games use Flash |
| 4 | **Save persistence to disk** | 🔴 High | Even SRAM data is lost on exit |
| 5 | ~~WAITCNT register~~ | ✅ Done | Fully implemented — `updateWaitstates()` parses WAITCNT and updates all ROM/SRAM wait state tables |
| 5b | ~~IF write-to-clear (write8)~~ | ✅ Done | BIOS IRQ handler uses STRB to acknowledge IF — write8 now has write-to-clear semantics |
| 6 | **VCount match / VCount IRQ** | 🔴 High | DISPSTAT VCount setting (bits 8–15) never compared against current scanline. VCount match bit never set, IRQ never fired. Breaks raster effects |
| 7 | ~~Prefetch buffer~~ | ✅ Done | Prefetch buffer with stall/credit model implemented. Passes 87% of timing suite |
| 8 | **Mosaic effect** | 🟠 Medium | Sprite/BG mosaic flags parsed but no stretch/repeat rendering applied. `REG_MOSAIC` not defined |
| 9 | **GPU Mode 5** | 🟠 Medium | Falls through to default in scanline renderer — no per-scanline compositing with effects |
| 10 | **Keypad interrupt** (KEYCNT) | 🟡 Low | `IRQ_KEYPAD` defined but KEYCNT register (`0x04000132`) not handled |
| 11 | **Save states** | 🟡 Low | No serialization/deserialization of emulator state |
| 12 | **Fast forward / frame skip** | 🟡 Low | Fixed 1× speed, no turbo key |
| 13 | **BIOS read protection** | 🟡 Low | BIOS readable from any context (should only be readable when PC is in BIOS region) |
| 14 | **Open bus behavior** | 🟡 Low | Unmapped reads return 0 instead of last prefetched opcode |
| 15 | **Serial / link cable** | 🟡 Low | No SIOCNT/SIODATA/RCNT registers |
| 16 | **RTC** (real-time clock) | 🟡 Low | No GPIO-based RTC for Pokémon etc. |
| 17 | **Solar sensor** | ⚪ Niche | Boktai series only |
| 18 | **Greenswap** (`0x04000002`) | ⚪ Niche | Swaps green channels between even/odd pixels. Rarely used |
| 19 | **Fullscreen / screenshot** | ⚪ QoL | Fixed windowed display only |

## 🟡 Incomplete Features (Partially Working)

| # | Feature | Gap |
|---|---------|-----|
| 1 | ~~SPSR in exception handler~~ | ✅ Fixed — SPSR saved/restored correctly in IRQ/SWI entry. `SUBS PC, LR` restores CPSR from SPSR. BIOS boot works. |
| 2 | **DMA cycle timing** | Per-unit cost model implemented (nonseq first + seq subsequent + region waits + teardown). 52 timing suite failures remain (±1 off). See `docs/TIMING_ISSUES_INVESTIGATED.md` |
| 3 | **VRAM/Palette/OAM bus contention** | TODOs in memory.cpp: "+1 if video controller accessing (not implemented yet)". No extra cycle during active rendering |
| 4 | **GPU Mode 3/4 compositing** | Mode 3 reads VRAM directly. Unclear if sprites, blending, and windowing are applied on top |
| 5 | **Sprite per-scanline cycle limit** | GBA limits to 1210 cycles (normal) / 954 (HBlank-free). Not enforced — all sprites always render |
| 6 | **CPU pipeline modeling** | Comment: "HACK - do we need to model the cpu pipeline?" — no 3-stage pipeline flush timing |
| 7 | **THUMB undefined instruction slots** | `nullptr` entries in `0xB0–0xBF` range — will crash instead of triggering Undefined Instruction exception |
| 8 | **Coprocessor instructions** | CDP/MCR/MRC/LDC/STC log errors but should trigger Undefined Instruction exception |
| 9 | **Game Pak DRQ** | `DMA_CTRL_GAMEPAK_DRQ` bit 11 defined but never checked |

## 🔵 Accuracy Limitations

| # | Area | Issue |
|---|------|-------|
| 1 | **Sequential vs. non-sequential timing** | No tracking of whether ROM/RAM accesses are sequential — always uses nonsequential costs |
| 2 | **EWRAM timing** | Hardcoded 3/6 cycles with no seq/nonseq distinction |
| 3 | **SOUNDBIAS** | Stored but PWM sample rate bits (14–15) not applied |
| 4 | **Forced blank** (DISPCNT bit 7) | Verify white output + wait penalty removal when set |
| 5 | **OAM/VRAM access restrictions** | During rendering, OAM should be DMA-only, active VRAM writes should be blocked |
| 6 | **Internal Memory Control** (`0x04000800`) | Undocumented register used by some games for EWRAM config — not handled |

## Recommended Priority Order

Biggest bang-for-buck improvements for test accuracy and game compatibility:

1. **Timer/IRQ delivery alignment** — ~280 test failures across timers + timer-irq suites. IRQ fires 1 instruction boundary late.
2. **Save support** (SRAM persistence + EEPROM + Flash) — most games can't save at all right now
3. **VCount match/IRQ** — breaks raster effects and VCount-based timing in many games
4. **PSG audio** — nearly every game uses PSG for some SFX/music
