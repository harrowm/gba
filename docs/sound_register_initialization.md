# GBA Sound Register Initialization

## Summary
When the GBA boots or resets, certain I/O registers must be initialized to specific default values. For sound subsystem, the main register that needs initialization is **SOUNDBIAS**.

## Key Findings from mGBA Source Code

### GBAIOInit Function
Located in `src/gba/io.c` lines 279-286:

```c
void GBAIOInit(struct GBA* gba) {
	gba->memory.io[GBA_REG(DISPCNT)] = 0x0080;
	gba->memory.io[GBA_REG(RCNT)] = RCNT_INITIAL;      // -0x8000
	gba->memory.io[GBA_REG(KEYINPUT)] = 0x3FF;
	gba->memory.io[GBA_REG(SOUNDBIAS)] = 0x200;        // <-- Sound initialization!
	gba->memory.io[GBA_REG(BG2PA)] = 0x100;
	gba->memory.io[GBA_REG(BG2PD)] = 0x100;
	gba->memory.io[GBA_REG(BG3PA)] = 0x100;
	gba->memory.io[GBA_REG(BG3PD)] = 0x100;
	gba->memory.io[GBA_REG(INTERNAL_EXWAITCNT_LO)] = 0x20;
	gba->memory.io[GBA_REG(INTERNAL_EXWAITCNT_HI)] = 0xD00;

	if (!gba->biosVf) {
		gba->memory.io[GBA_REG(VCOUNT)] = 0x7E;
		gba->memory.io[GBA_REG(POSTFLG)] = 1;
	}
}
```

### SOUNDBIAS Register Details

**Address**: `0x04000088`  
**Default Value**: `0x0200` (512 decimal)

**Bitfield** (from `include/mgba/internal/gba/audio.h`):
```c
DECL_BITFIELD(GBARegisterSOUNDBIAS, uint16_t);
DECL_BITS(GBARegisterSOUNDBIAS, Bias, 0, 10);       // Bits 0-9: Bias level (default 0x200)
DECL_BITS(GBARegisterSOUNDBIAS, Resolution, 14, 2);  // Bits 14-15: Amplitude resolution
```

### Purpose of SOUNDBIAS

According to GBATEK documentation:
- **Bias Level** (bits 0-9): Converts signed audio samples to unsigned (default=0x200)
- **Resolution** (bits 14-15): Controls sampling cycle/amplitude resolution (default=0)

The register controls the final PWM audio output. The default setting is `0x0200`, and it's normally not required to change this value during operation.

### GBAAudioReset Function

Located in `src/gba/audio.c` lines 45-86, sound registers are reset:

```c
void GBAAudioReset(struct GBAAudio* audio) {
	GBAudioReset(&audio->psg);
	// ... schedule audio events ...
	
	audio->chA.dmaSource = 1;
	audio->chB.dmaSource = 2;
	audio->chA.fifoWrite = 0;
	audio->chA.fifoRead = 0;
	// ... clear FIFOs ...
	
	audio->soundbias = 0x200;  // <-- Default bias value
	audio->volume = 0;
	audio->volumeChA = false;
	audio->volumeChB = false;
	audio->lastSample = 0;
	audio->sampleIndex = 0;
	audio->chARight = false;
	audio->chALeft = false;
	audio->chATimer = false;
	audio->chBRight = false;
	audio->chBLeft = false;
	audio->chBTimer = false;
	audio->enable = false;
	// ...
}
```

## RegisterRamReset (BIOS Function 0x01)

When the BIOS `RegisterRamReset` function is called with bit 6 set, it resets sound registers (from `src/gba/bios.c` lines 58-79):

```c
if (registers & 0x40) {
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND1CNT_LO, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND1CNT_HI, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND1CNT_X, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND2CNT_LO, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND2CNT_HI, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND3CNT_LO, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND3CNT_HI, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND3CNT_X, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND4CNT_LO, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUND4CNT_HI, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUNDCNT_LO, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUNDCNT_HI, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUNDCNT_X, 0, 0);
	cpu->memory.store16(cpu, GBA_BASE_IO | GBA_REG_SOUNDBIAS, 0x200, 0);  // <-- Restored to 0x200!
	memset(gba->audio.psg.ch3.wavedata32, 0, sizeof(gba->audio.psg.ch3.wavedata32));
}
```

## Other Initialized Registers (Non-Sound)

For completeness, here are all registers initialized by `GBAIOInit`:

| Register | Address | Default Value | Purpose |
|----------|---------|---------------|---------|
| DISPCNT | 0x04000000 | 0x0080 | Display Control (forced blank on) |
| RCNT | 0x04000134 | 0x8000 | SIO Mode Select |
| KEYINPUT | 0x04000130 | 0x03FF | Key Status (all released) |
| **SOUNDBIAS** | **0x04000088** | **0x0200** | **Sound PWM Control** |
| BG2PA | 0x04000020 | 0x0100 | BG2 Rotation/Scaling Parameter A |
| BG2PD | 0x04000026 | 0x0100 | BG2 Rotation/Scaling Parameter D |
| BG3PA | 0x04000030 | 0x0100 | BG3 Rotation/Scaling Parameter A |
| BG3PD | 0x04000036 | 0x0100 | BG3 Rotation/Scaling Parameter D |

**When BIOS is skipped:**
| Register | Address | Default Value | Purpose |
|----------|---------|---------------|---------|
| VCOUNT | 0x04000006 | 0x007E | Vertical Counter (line 126) |
| POSTFLG | 0x04000300 | 0x0001 | Post Boot Flag |

## Implementation for Our Emulator

Since we haven't implemented the sound subsystem yet, we should still initialize SOUNDBIAS to avoid divergence from mGBA during BIOS execution:

```cpp
// In gba.cpp or memory.cpp initialization:
void GBA::reset() {
    // ... other initialization ...
    
    // Initialize I/O registers (see GBAIOInit in mGBA)
    memory.write16(0x04000000, 0x0080);    // DISPCNT: Forced blank
    memory.write16(0x04000130, 0x03FF);    // KEYINPUT: All keys released
    memory.write16(0x04000088, 0x0200);    // SOUNDBIAS: Default audio bias
    memory.write16(0x04000020, 0x0100);    // BG2PA: Identity matrix
    memory.write16(0x04000026, 0x0100);    // BG2PD: Identity matrix
    memory.write16(0x04000030, 0x0100);    // BG3PA: Identity matrix
    memory.write16(0x04000036, 0x0100);    // BG3PD: Identity matrix
    
    // If BIOS is skipped:
    if (!biosLoaded) {
        memory.write16(0x04000006, 0x007E);  // VCOUNT: Line 126
        memory.write8(0x04000300, 0x01);     // POSTFLG: Post-boot
    }
}
```

## Bug Found: Address 0x808 Divergence

Our emulator diverged from mGBA at instruction 38,073 (PC=0x808) because:
- The BIOS reads SOUNDBIAS at 0x04000088
- mGBA returns 0x0200 (correctly initialized)
- Our emulator returned 0x0000 (not initialized)

This causes the BIOS execution path to potentially differ since the register value affects conditional branches.

## References

- mGBA source: `src/gba/io.c` - GBAIOInit function
- mGBA source: `src/gba/audio.c` - GBAAudioReset function  
- mGBA source: `src/gba/bios.c` - RegisterRamReset function
- mGBA source: `include/mgba/internal/gba/io.h` - Register address definitions
- mGBA source: `include/mgba/internal/gba/audio.h` - SOUNDBIAS bitfield definition
- GBATEK documentation: I/O register map and SOUNDBIAS details
