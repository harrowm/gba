# GBA Memory Map and I/O Registers

## Complete Memory Map

### General Internal Memory
| Address Range | Size | Description | Bus Width | Wait States |
|---------------|------|-------------|-----------|-------------|
| 0x00000000-0x00003FFF | 16KB | BIOS - System ROM | 32-bit | 1/1/1 |
| 0x00004000-0x01FFFFFF | - | Not used | - | - |
| 0x02000000-0x0203FFFF | 256KB | WRAM - On-board Work RAM | 16-bit | 3/3/6 |
| 0x02040000-0x02FFFFFF | - | Not used | - | - |
| 0x03000000-0x03007FFF | 32KB | WRAM - On-chip Work RAM | 32-bit | 1/1/1 |
| 0x03008000-0x03FFFFFF | - | Not used | - | - |
| 0x04000000-0x040003FE | ~1KB | I/O Registers | 32-bit | 1/1/1 |
| 0x04000400-0x04FFFFFF | - | Not used | - | - |

### Internal Display Memory
| Address Range | Size | Description | Bus Width | Wait States |
|---------------|------|-------------|-----------|-------------|
| 0x05000000-0x050003FF | 1KB | BG/OBJ Palette RAM | 16-bit | 1/1/2* |
| 0x05000400-0x05FFFFFF | - | Not used | - | - |
| 0x06000000-0x06017FFF | 96KB | VRAM - Video RAM | 16-bit | 1/1/2* |
| 0x06018000-0x06FFFFFF | - | Not used | - | - |
| 0x07000000-0x070003FF | 1KB | OAM - OBJ Attributes | 32-bit | 1/1/1* |
| 0x07000400-0x07FFFFFF | - | Not used | - | - |

### External Memory (Game Pak)
| Address Range | Size | Description | Bus Width | Wait States |
|---------------|------|-------------|-----------|-------------|
| 0x08000000-0x09FFFFFF | 32MB | Game Pak ROM - Wait State 0 | 16-bit | 5/5/8** |
| 0x0A000000-0x0BFFFFFF | 32MB | Game Pak ROM - Wait State 1 | 16-bit | 5/5/8** |
| 0x0C000000-0x0DFFFFFF | 32MB | Game Pak ROM - Wait State 2 | 16-bit | 5/5/8** |
| 0x0E000000-0x0E00FFFF | 64KB | Game Pak SRAM | 8-bit | 5 |
| 0x0E010000-0x0FFFFFFF | - | Not used | - | - |
| 0x10000000-0xFFFFFFFF | - | Not used (upper 4 bits unused) | - | - |

**Notes:**
- *: Plus 1 cycle if GBA video controller accessing simultaneously
- **: Default settings, configurable via WAITCNT register
- Wait states shown as 8-bit/16-bit/32-bit access times

## I/O Register Map

### LCD Video Controller
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000000 | 2 | R/W | DISPCNT | LCD Control |
| 0x4000002 | 2 | R/W | - | Undocumented - Green Swap |
| 0x4000004 | 2 | R/W | DISPSTAT | General LCD Status (STAT,LYC) |
| 0x4000006 | 2 | R | VCOUNT | Vertical Counter (LY) |
| 0x4000008 | 2 | R/W | BG0CNT | BG0 Control |
| 0x400000A | 2 | R/W | BG1CNT | BG1 Control |
| 0x400000C | 2 | R/W | BG2CNT | BG2 Control |
| 0x400000E | 2 | R/W | BG3CNT | BG3 Control |
| 0x4000010 | 2 | W | BG0HOFS | BG0 X-Offset |
| 0x4000012 | 2 | W | BG0VOFS | BG0 Y-Offset |
| 0x4000014 | 2 | W | BG1HOFS | BG1 X-Offset |
| 0x4000016 | 2 | W | BG1VOFS | BG1 Y-Offset |
| 0x4000018 | 2 | W | BG2HOFS | BG2 X-Offset |
| 0x400001A | 2 | W | BG2VOFS | BG2 Y-Offset |
| 0x400001C | 2 | W | BG3HOFS | BG3 X-Offset |
| 0x400001E | 2 | W | BG3VOFS | BG3 Y-Offset |
| 0x4000020 | 2 | W | BG2PA | BG2 Rotation/Scaling Parameter A (dx) |
| 0x4000022 | 2 | W | BG2PB | BG2 Rotation/Scaling Parameter B (dmx) |
| 0x4000024 | 2 | W | BG2PC | BG2 Rotation/Scaling Parameter C (dy) |
| 0x4000026 | 2 | W | BG2PD | BG2 Rotation/Scaling Parameter D (dmy) |
| 0x4000028 | 4 | W | BG2X | BG2 Reference Point X-Coordinate |
| 0x400002C | 4 | W | BG2Y | BG2 Reference Point Y-Coordinate |
| 0x4000030 | 2 | W | BG3PA | BG3 Rotation/Scaling Parameter A (dx) |
| 0x4000032 | 2 | W | BG3PB | BG3 Rotation/Scaling Parameter B (dmx) |
| 0x4000034 | 2 | W | BG3PC | BG3 Rotation/Scaling Parameter C (dy) |
| 0x4000036 | 2 | W | BG3PD | BG3 Rotation/Scaling Parameter D (dmy) |
| 0x4000038 | 4 | W | BG3X | BG3 Reference Point X-Coordinate |
| 0x400003C | 4 | W | BG3Y | BG3 Reference Point Y-Coordinate |
| 0x4000040 | 2 | W | WIN0H | Window 0 Horizontal Dimensions |
| 0x4000042 | 2 | W | WIN1H | Window 1 Horizontal Dimensions |
| 0x4000044 | 2 | W | WIN0V | Window 0 Vertical Dimensions |
| 0x4000046 | 2 | W | WIN1V | Window 1 Vertical Dimensions |
| 0x4000048 | 2 | R/W | WININ | Inside of Window 0 and 1 |
| 0x400004A | 2 | R/W | WINOUT | Inside of OBJ Window & Outside of Windows |
| 0x400004C | 2 | W | MOSAIC | Mosaic Size |
| 0x4000050 | 2 | R/W | BLDCNT | Color Special Effects Selection |
| 0x4000052 | 2 | R/W | BLDALPHA | Alpha Blending Coefficients |
| 0x4000054 | 2 | W | BLDY | Brightness (Fade-In/Out) Coefficient |

### Sound Registers
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000060 | 2 | R/W | SOUND1CNT_L | Channel 1 Sweep register (NR10) |
| 0x4000062 | 2 | R/W | SOUND1CNT_H | Channel 1 Duty/Length/Envelope (NR11, NR12) |
| 0x4000064 | 2 | R/W | SOUND1CNT_X | Channel 1 Frequency/Control (NR13, NR14) |
| 0x4000068 | 2 | R/W | SOUND2CNT_L | Channel 2 Duty/Length/Envelope (NR21, NR22) |
| 0x400006C | 2 | R/W | SOUND2CNT_H | Channel 2 Frequency/Control (NR23, NR24) |
| 0x4000070 | 2 | R/W | SOUND3CNT_L | Channel 3 Stop/Wave RAM select (NR30) |
| 0x4000072 | 2 | R/W | SOUND3CNT_H | Channel 3 Length/Volume (NR31, NR32) |
| 0x4000074 | 2 | R/W | SOUND3CNT_X | Channel 3 Frequency/Control (NR33, NR34) |
| 0x4000078 | 2 | R/W | SOUND4CNT_L | Channel 4 Length/Envelope (NR41, NR42) |
| 0x400007C | 2 | R/W | SOUND4CNT_H | Channel 4 Frequency/Control (NR43, NR44) |
| 0x4000080 | 2 | R/W | SOUNDCNT_L | Control Stereo/Volume/Enable (NR50, NR51) |
| 0x4000082 | 2 | R/W | SOUNDCNT_H | Control Mixing/DMA Control |
| 0x4000084 | 2 | R/W | SOUNDCNT_X | Control Sound on/off (NR52) |
| 0x4000088 | 2 | BIOS | SOUNDBIAS | Sound PWM Control |
| 0x4000090 | 32 | R/W | WAVE_RAM | Channel 3 Wave Pattern RAM (2 banks!!) |
| 0x40000A0 | 4 | W | FIFO_A | Channel A FIFO, Data 0-3 |
| 0x40000A4 | 4 | W | FIFO_B | Channel B FIFO, Data 0-3 |

### DMA Transfer Channels
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x40000B0 | 4 | W | DMA0SAD | DMA 0 Source Address |
| 0x40000B4 | 4 | W | DMA0DAD | DMA 0 Destination Address |
| 0x40000B8 | 2 | W | DMA0CNT_L | DMA 0 Word Count |
| 0x40000BA | 2 | R/W | DMA0CNT_H | DMA 0 Control |
| 0x40000BC | 4 | W | DMA1SAD | DMA 1 Source Address |
| 0x40000C0 | 4 | W | DMA1DAD | DMA 1 Destination Address |
| 0x40000C4 | 2 | W | DMA1CNT_L | DMA 1 Word Count |
| 0x40000C6 | 2 | R/W | DMA1CNT_H | DMA 1 Control |
| 0x40000C8 | 4 | W | DMA2SAD | DMA 2 Source Address |
| 0x40000CC | 4 | W | DMA2DAD | DMA 2 Destination Address |
| 0x40000D0 | 2 | W | DMA2CNT_L | DMA 2 Word Count |
| 0x40000D2 | 2 | R/W | DMA2CNT_H | DMA 2 Control |
| 0x40000D4 | 4 | W | DMA3SAD | DMA 3 Source Address |
| 0x40000D8 | 4 | W | DMA3DAD | DMA 3 Destination Address |
| 0x40000DC | 2 | W | DMA3CNT_L | DMA 3 Word Count |
| 0x40000DE | 2 | R/W | DMA3CNT_H | DMA 3 Control |

### Timer Registers
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000100 | 2 | R/W | TM0CNT_L | Timer 0 Counter/Reload |
| 0x4000102 | 2 | R/W | TM0CNT_H | Timer 0 Control |
| 0x4000104 | 2 | R/W | TM1CNT_L | Timer 1 Counter/Reload |
| 0x4000106 | 2 | R/W | TM1CNT_H | Timer 1 Control |
| 0x4000108 | 2 | R/W | TM2CNT_L | Timer 2 Counter/Reload |
| 0x400010A | 2 | R/W | TM2CNT_H | Timer 2 Control |
| 0x400010C | 2 | R/W | TM3CNT_L | Timer 3 Counter/Reload |
| 0x400010E | 2 | R/W | TM3CNT_H | Timer 3 Control |

### Serial Communication
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000120 | 4 | R/W | SIODATA32 | SIO Data (Normal-32bit Mode) |
| 0x4000120 | 2 | R/W | SIOMULTI0 | SIO Data 0 (Parent) (Multi-Player Mode) |
| 0x4000122 | 2 | R/W | SIOMULTI1 | SIO Data 1 (1st Child) (Multi-Player Mode) |
| 0x4000124 | 2 | R/W | SIOMULTI2 | SIO Data 2 (2nd Child) (Multi-Player Mode) |
| 0x4000126 | 2 | R/W | SIOMULTI3 | SIO Data 3 (3rd Child) (Multi-Player Mode) |
| 0x4000128 | 2 | R/W | SIOCNT | SIO Control Register |
| 0x400012A | 2 | R/W | SIOMLT_SEND | SIO Data (Local of MultiPlayer) |
| 0x400012A | 2 | R/W | SIODATA8 | SIO Data (Normal-8bit and UART Mode) |

### Keypad Input
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000130 | 2 | R | KEYINPUT | Key Status |
| 0x4000132 | 2 | R/W | KEYCNT | Key Interrupt Control |

### Serial Communication (2)
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000134 | 2 | R/W | RCNT | SIO Mode Select/General Purpose Data |
| 0x4000136 | - | - | IR | Ancient - Infrared Register (Prototypes only) |
| 0x4000140 | 2 | R/W | JOYCNT | SIO JOY Bus Control |
| 0x4000150 | 4 | R/W | JOY_RECV | SIO JOY Bus Receive Data |
| 0x4000154 | 4 | R/W | JOY_TRANS | SIO JOY Bus Transmit Data |
| 0x4000158 | 2 | R/? | JOYSTAT | SIO JOY Bus Receive Status |

### System Control
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000200 | 2 | R/W | IE | Interrupt Enable Register |
| 0x4000202 | 2 | R/W | IF | Interrupt Request Flags / IRQ Acknowledge |
| 0x4000204 | 2 | R/W | WAITCNT | Game Pak Waitstate Control |
| 0x4000208 | 2 | R/W | IME | Interrupt Master Enable Register |
| 0x4000300 | 1 | R/W | POSTFLG | **Post Boot Flag** |
| 0x4000301 | 1 | W | HALTCNT | Power Down Control |

### Undocumented Registers
| Address | Size | R/W | Name | Description |
|---------|------|-----|------|-------------|
| 0x4000410 | 1 | ? | ? | Purpose Unknown / Bug ??? 0FFh |
| 0x4000800 | 4 | R/W | ? | Internal Memory Control (R/W) |
| 0x4xx0800 | 4 | R/W | ? | Mirrors of 4000800h (repeated each 64K) |

## Critical Boot-Related Registers

### POSTFLG (0x4000300) - Post Boot Flag
This register is crucial for understanding the BIOS boot loop:

```
Bit 0: First Boot Flag (0=First boot, 1=Further boots)
Bit 1-7: Not used
```

**Behavior:**
- After initial reset, BIOS initializes this to 0x01
- Any further execution of Reset vector (0x00000000) will pass control to Debug vector (0x0000001C) when register is still 0x01
- This is likely what the BIOS is checking at address 0x3FFFFFA (which is 0x4000000 - 6)

### HALTCNT (0x4000301) - Power Down Control
```
Bit 0-6: Not used
Bit 7: Power Down Mode (0=Halt, 1=Stop)
```

## Memory Access Notes

### Access Timing
- **Sequential Access**: Faster than random access
- **Video Memory Conflicts**: +1 cycle when CPU and video controller access same memory
- **GamePak ROM**: 16-bit bus requires two accesses for 32-bit ARM instructions

### Default WRAM Usage
The 256 bytes at 0x03007F00-0x03007FFF are reserved for:
- Interrupt vector
- Interrupt Stack
- BIOS Call Stack

The remaining WRAM is free for use, including User Stack (initially at 0x03007F00).

## Memory Region Properties

### Internal Memory (Fast)
- **BIOS ROM**: Read-only, protected after first access
- **Work RAM 32K**: Fastest access, on-chip
- **I/O Registers**: Memory-mapped hardware control

### External Memory (Slower)
- **Work RAM 256K**: On-board, 16-bit bus
- **Video Memory**: Access conflicts with video controller
- **GamePak**: Configurable wait states, 16-bit bus

### Access Width Support
| Region | 8-bit | 16-bit | 32-bit | Notes |
|--------|-------|--------|--------|-------|
| BIOS ROM | ✓ | ✓ | ✓ | Read-only |
| Work RAM 32K | ✓ | ✓ | ✓ | Fastest |
| Work RAM 256K | ✓ | ✓ | ✓ | 16-bit bus |
| I/O Registers | ✓ | ✓ | ✓ | Hardware dependent |
| Palette RAM | ✓ | ✓ | ✓ | 16/32-bit writes only |
| VRAM | ✓ | ✓ | ✓ | 16/32-bit writes only |
| OAM | ✓ | ✓ | ✓ | 16/32-bit writes only |
| GamePak ROM | ✓ | ✓ | ✓ | Read-only |
| GamePak SRAM | ✓ | - | - | 8-bit only |

## References
- GBA Technical Data (GBATEK) by Martin Korth
- Nintendo Game Boy Advance Programming Manual
- No$GBA Documentation
