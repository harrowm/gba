# GBA Sound Implementation Plan

## Overview

The GBA has 6 sound channels:
- **Channels 1-4**: Legacy PSG (Programmable Sound Generator) from Game Boy
- **Channels A & B**: Direct Sound (DMA-fed PCM audio) - new to GBA

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Sound Mixer                             │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │  Ch 1   │ │  Ch 2   │ │  Ch 3   │ │  Ch 4   │  PSG      │
│  │ Square  │ │ Square  │ │  Wave   │ │  Noise  │  Channels │
│  │ +Sweep  │ │         │ │         │ │         │           │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘           │
│       └──────────┴──────────┴──────────┴───────┐           │
│                                                 │           │
│  ┌─────────────┐    ┌─────────────┐            │           │
│  │   FIFO A    │    │   FIFO B    │  Direct    ▼           │
│  │  (8-bit PCM)│    │  (8-bit PCM)│  Sound   ┌───┐        │
│  └──────┬──────┘    └──────┬──────┘          │MIX│────► SDL│
│         └────────────────────────────────────►───┘        │
└─────────────────────────────────────────────────────────────┘
```

## Implementation Phases

### Phase 1: Foundation (~2-3 hours)
1. **Create `include/apu.h` and `src/apu.cpp`**
   - APU class with sample generation
   - Sound registers (0x04000060-0x040000A7)
   - SOUNDCNT_L/H/X control registers

2. **SDL Audio Setup**
   - Initialize SDL audio subsystem
   - Create audio callback for sample output
   - Ring buffer for audio samples
   - Target: 32768 Hz sample rate, stereo

3. **Timer Integration**
   - FIFO A/B are clocked by Timer 0 or Timer 1
   - Hook timer overflow to trigger FIFO playback

### Phase 2: Direct Sound (FIFO) (~2-3 hours)
*Most GBA games primarily use Direct Sound*

1. **FIFO Implementation**
   - Two 32-byte (8 sample) FIFOs
   - DMA refill when FIFO half-empty
   - 8-bit signed PCM samples

2. **DMA Sound Triggers**
   - FIFO A: Triggered by DMA1 or DMA2
   - FIFO B: Triggered by DMA1 or DMA2
   - Timer-based sample rate

3. **SOUNDCNT_H Register**
   - Volume control (50%/100%)
   - Left/Right enable
   - Timer select (0 or 1)
   - FIFO reset

### Phase 3: PSG Channels (~3-4 hours)
*Legacy Game Boy sound - lower priority*

1. **Channel 1: Square + Sweep**
   - Frequency sweep (increase/decrease)
   - Duty cycle (12.5%, 25%, 50%, 75%)
   - Envelope (volume fade)

2. **Channel 2: Square**
   - Same as Channel 1 without sweep

3. **Channel 3: Wave**
   - 32 x 4-bit samples in Wave RAM
   - Programmable waveform

4. **Channel 4: Noise**
   - LFSR-based noise generation
   - 7-bit or 15-bit shift register

### Phase 4: Mixing & Polish (~1-2 hours)
1. **Audio Mixing**
   - PSG master volume (SOUNDCNT_L)
   - Direct Sound volume
   - Left/Right channel mixing

2. **SOUNDBIAS**
   - DC offset for PWM output
   - Sample rate resolution

3. **Sync & Buffering**
   - Audio/video synchronization
   - Prevent buffer underruns

## Key Registers

| Address | Name | Description |
|---------|------|-------------|
| 0x04000060 | SOUND1CNT_L | Ch1 Sweep |
| 0x04000062 | SOUND1CNT_H | Ch1 Duty/Envelope |
| 0x04000064 | SOUND1CNT_X | Ch1 Frequency |
| 0x04000068 | SOUND2CNT_L | Ch2 Duty/Envelope |
| 0x0400006C | SOUND2CNT_H | Ch2 Frequency |
| 0x04000070 | SOUND3CNT_L | Ch3 Control |
| 0x04000072 | SOUND3CNT_H | Ch3 Length/Volume |
| 0x04000074 | SOUND3CNT_X | Ch3 Frequency |
| 0x04000078 | SOUND4CNT_L | Ch4 Envelope |
| 0x0400007C | SOUND4CNT_H | Ch4 Frequency |
| 0x04000080 | SOUNDCNT_L | PSG Volume/Enable |
| 0x04000082 | SOUNDCNT_H | DMA Sound Control |
| 0x04000084 | SOUNDCNT_X | Master Enable/Status |
| 0x04000088 | SOUNDBIAS | PWM Control |
| 0x04000090 | WAVE_RAM | Ch3 Wave Pattern (16 bytes) |
| 0x040000A0 | FIFO_A | DMA Sound A Data |
| 0x040000A4 | FIFO_B | DMA Sound B Data |

## Recommended Order

1. **Start with FIFO/Direct Sound** - Most games use this primarily
2. **Then PSG channels** - Needed for retro-style games and sound effects
3. **Sonic Advance uses both** - FIFO for music, PSG for some effects

## Files to Create/Modify

**New files:**
- `include/apu.h` - APU class definition
- `src/apu.cpp` - APU implementation

**Modify:**
- `include/gba.h` - Add APU member
- `src/gba.cpp` - Initialize/tick APU
- `src/memory.cpp` - Route sound register reads/writes
- `src/dma.cpp` - Add FIFO DMA triggers
- `src/timer.c` / `timer_controller.cpp` - Timer overflow → FIFO sample

## Technical Details

### GBA Clock Rates
- CPU Clock: 16.78 MHz (16,777,216 Hz)
- Audio sample rate: Typically 32768 Hz
- Cycles per sample: 16777216 / 32768 = 512 cycles

### FIFO Timing
- FIFO holds 32 bytes (8 x 32-bit writes = 32 samples)
- When FIFO has ≤16 bytes, request DMA refill
- Sample played on timer overflow

### PSG Timing  
- Frame sequencer runs at 512 Hz (32768 cycles)
- Controls length, envelope, sweep timing
