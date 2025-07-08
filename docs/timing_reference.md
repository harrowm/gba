# GBA Timing Reference

## System Clock
- **Main Clock Frequency**: 16.78MHz (16,780,000 Hz)
- **Clock Period**: ~59.59 nanoseconds per cycle
- **System Bus Width**: Mixed (32-bit internal, 16-bit external)

## CPU Timing

### Instruction Execution
- **ARM Mode**: 32-bit instructions, 1 cycle fetch + execution cycles
- **Thumb Mode**: 16-bit instructions, 1 cycle fetch + execution cycles
- **Branch Taken**: 3 cycles (2S + 1N)
- **Multiply Instructions**: 1-4 cycles depending on operand size

### Memory Access Timing

#### Internal Memory (Fast, 32-bit bus)
| Region | Address Range | Access Time | Notes |
|--------|--------------|-------------|-------|
| BIOS ROM | 0x00000000-0x00003FFF | 1 cycle | Read-only, protected |
| Work RAM (32K) | 0x03000000-0x03007FFF | 1 cycle | On-chip, fastest |
| I/O Registers | 0x04000000-0x040003FE | 1 cycle | Memory-mapped I/O |
| OAM | 0x07000000-0x070003FF | 1 cycle | +1 if video controller busy |
| Palette RAM | 0x05000000-0x050003FF | 1-2 cycles | 16-bit: 1, 32-bit: 2 |

#### External Memory (Slower, 16-bit bus)
| Region | Address Range | 8-bit | 16-bit | 32-bit | Notes |
|--------|--------------|-------|--------|--------|-------|
| Work RAM (256K) | 0x02000000-0x0203FFFF | 3 | 3 | 6 | On-board WRAM |
| VRAM | 0x06000000-0x06017FFF | 1 | 1 | 2 | +1 if video conflict |

#### GamePak Memory (Configurable wait states)
| Region | Address Range | Default Timing | Notes |
|--------|--------------|---------------|-------|
| GamePak ROM | 0x08000000-0x0DFFFFFF | 5/5/8 cycles | N/S/32-bit, configurable |
| GamePak SRAM | 0x0E000000-0x0E00FFFF | 5 cycles | 8-bit only |

### Wait State Configuration
Wait states are configurable via WAITCNT register (0x04000204):
- **WS0**: First ROM region (0x08000000-0x09FFFFFF)
- **WS1**: Second ROM region (0x0A000000-0x0BFFFFFF)  
- **WS2**: Third ROM region (0x0C000000-0x0DFFFFFF)
- **SRAM**: GamePak SRAM region

Default settings: WS0=4,2 clks; SRAM=8 clks; Commercial games often use 3,1 clks.

## Timer System

### Timer Hardware
- **4 Hardware Timers**: Timer 0, Timer 1, Timer 2, Timer 3
- **16-bit Counters**: Count up from reload value to 0xFFFF, then overflow
- **Prescaler Options**: F/1, F/64, F/256, F/1024

### Timer Frequencies
| Prescaler | Frequency | Period | Overflow Time |
|-----------|-----------|---------|---------------|
| F/1 | 16.78 MHz | 59.59 ns | 3.906 ms |
| F/64 | 262.5 kHz | 3.815 μs | 0.25 seconds |
| F/256 | 65.625 kHz | 15.259 μs | 1 second |
| F/1024 | 16.384 kHz | 61.035 μs | 4 seconds |

### Timer Registers
| Timer | Control Address | Counter Address | Description |
|-------|----------------|-----------------|-------------|
| 0 | 0x04000102 | 0x04000100 | Timer 0 (can drive sound) |
| 1 | 0x04000106 | 0x04000104 | Timer 1 (can drive sound) |
| 2 | 0x0400010A | 0x04000108 | Timer 2 |
| 3 | 0x0400010E | 0x0400010C | Timer 3 |

### Timer Control Register (TMxCNT_H)
```
Bit 0-1: Prescaler (0=F/1, 1=F/64, 2=F/256, 3=F/1024)
Bit 2:   Count-up (0=Normal, 1=Count when previous overflows) - Not for Timer 0
Bit 6:   IRQ Enable (0=Disable, 1=Enable overflow interrupt)
Bit 7:   Timer Enable (0=Disable, 1=Enable)
```

## Video Timing

### Display Specifications
- **Resolution**: 240×160 pixels
- **Color Depth**: 15-bit (32,768 colors)
- **Refresh Rate**: ~59.737 Hz

### Horizontal Timing
- **Visible Period**: 240 dots, 960 cycles (4 cycles per dot)
- **H-Blank Period**: 68 dots, 272 cycles
- **Total H-Period**: 308 dots, 1232 cycles (~73.43 μs)

### Vertical Timing
- **Visible Lines**: 160 scanlines
- **V-Blank Lines**: 68 scanlines  
- **Total V-Period**: 228 scanlines (~16.74 ms)

### Video Memory Access
- VRAM can be accessed during H-Blank and V-Blank
- OAM can be accessed during H-Blank only if "H-Blank Interval Free" is set
- +1 cycle penalty when CPU and video controller access same memory

## DMA Timing

### DMA Channels
- **4 DMA Channels**: DMA0 (highest priority) to DMA3 (lowest priority)
- **Transfer Sizes**: 16-bit or 32-bit
- **Timing Modes**: Immediate, VBlank, HBlank, Special

### DMA Transfer Time
For N data units: **2N + 2(N-1)S + xI** cycles
- N = Non-sequential cycles
- S = Sequential cycles  
- I = Internal cycles (2I normally, 4I if both src/dst in GamePak)

### DMA Trigger Timing
| Mode | DMA0 | DMA1 | DMA2 | DMA3 |
|------|------|------|------|------|
| 0 | Immediate | Immediate | Immediate | Immediate |
| 1 | VBlank | VBlank | VBlank | VBlank |
| 2 | HBlank | Sound FIFO | Sound FIFO | HBlank |
| 3 | Special | Sound FIFO | Sound FIFO | Video Capture |

## Sound Timing

### PSG Channels (1-4)
- **Sample Rate**: Software controlled via timers
- **Common Rates**: 8192 Hz, 16384 Hz, 32768 Hz (fractions of 32.768 kHz)

### DMA Sound (Channels A & B)
- **Output Rate**: 32.768 kHz (resampled internally)
- **Timer Driven**: Use Timer 0 and/or Timer 1 for sample rate
- **FIFO Buffer**: 32 bytes (8 samples) per channel

### Sound Timing Examples
| Sample Rate | Timer Reload | Prescaler | Buffer Duration |
|-------------|--------------|-----------|-----------------|
| 32768 Hz | 0x0000 | F/1024 | 0.244 ms |
| 16384 Hz | 0x0000 | F/512 | 0.488 ms |
| 8192 Hz | 0x0000 | F/256 | 0.977 ms |

## Interrupt Timing

### Interrupt Latency
- **IRQ Response**: ~3-4 cycles from trigger to handler
- **Priority**: Hardware interrupts have fixed priority order
- **Nesting**: IRQs can be nested if enabled in handler

### Common Interrupt Sources
| IRQ | Source | Timing |
|-----|--------|--------|
| VBlank | Video | Every 16.74 ms |
| HBlank | Video | Every 73.43 μs |
| Timer | Timers | Programmable |
| DMA | DMA completion | Variable |
| Serial | Communication | Variable |

## Practical Timing Considerations

### Performance Optimization
1. **Use Fast Memory**: Keep critical code in Work RAM (32K) for 1-cycle access
2. **Minimize GamePak Access**: Cache frequently used data in Work RAM
3. **Optimize Wait States**: Configure WAITCNT for your cartridge type
4. **Use Sequential Access**: Sequential memory access is faster than random

### Common Timing Patterns
1. **60 FPS Game Loop**: ~279,620 cycles per frame
2. **VBlank Processing**: ~83,776 cycles available
3. **HBlank Processing**: ~272 cycles available
4. **Audio Streaming**: Timer-driven DMA for consistent sample rates

### Debugging Timing Issues
1. **Cycle Counting**: Track exact cycle usage for critical sections
2. **Timer Profiling**: Use timers to measure execution time
3. **VBlank Monitoring**: Ensure processing fits within VBlank period
4. **Memory Access Patterns**: Profile memory access timing

## References
- GBA Technical Data (GBATEK)
- ARM7TDMI Technical Reference Manual
- Nintendo Game Boy Advance Programming Manual
