# Memory Clear (memset) Usage Analysis

## Summary
The emulator **does NOT use memset to clear memory during runtime**. All memset calls happen only during **initialization** when memory regions are first allocated. There are **no runtime memory clears** that could cause the screen blanking during the BIOS animation.

## Complete List of memset Usage

### 1. GPU Framebuffer Initialization
**File:** `src/gpu.cpp:14`  
**Context:** GPU constructor  
**Purpose:** Clear the internal tiled framebuffer when GPU is first created  
**When:** Only during emulator startup  
```cpp
memset(tiledFramebuffer, 0, sizeof(tiledFramebuffer));
```

### 2. I/O Memory Initialization (Test Mode)
**File:** `src/memory.cpp:30`  
**Context:** Memory constructor, test mode path  
**Purpose:** Zero-initialize I/O registers for unit tests  
**When:** Only during Memory object construction in test mode  
```cpp
memset(io, 0, 1 * 1024);
```

### 3. Palette RAM Initialization (Test Mode)
**File:** `src/memory.cpp:35`  
**Context:** Memory constructor, test mode path  
**Purpose:** Zero-initialize palette RAM for unit tests  
**When:** Only during Memory object construction in test mode  
```cpp
memset(palette, 0, 1 * 1024);
```

### 4. BIOS Padding (If File Too Small)
**File:** `src/memory.cpp:57`  
**Context:** Memory constructor, BIOS loading  
**Purpose:** Pad BIOS with zeros if file is smaller than 16KB  
**When:** Only during startup if BIOS file is incomplete  
```cpp
memset(bios + read, 0, 16 * 1024 - read);
```

### 5. BIOS Dummy Fill (If File Missing)
**File:** `src/memory.cpp:66`  
**Context:** Memory constructor, BIOS loading failure  
**Purpose:** Fill BIOS area with dummy data (0x01 bytes) if file cannot be opened  
**When:** Only during startup if BIOS file is missing  
```cpp
memset(bios, 0x1, 16 * 1024);
```

### 6. I/O Memory Initialization (Normal Mode)
**File:** `src/memory.cpp:86`  
**Context:** Memory constructor, normal mode path  
**Purpose:** Zero-initialize I/O registers at startup  
**When:** Only during Memory object construction in normal mode  
```cpp
memset(io, 0, 1 * 1024);
```

### 7. ROM Memory Initialization
**File:** `src/memory.cpp:118`  
**Context:** Memory constructor  
**Purpose:** Zero-initialize entire 32MB ROM space at allocation  
**When:** Only during Memory object construction  
```cpp
memset(rom, 0, 32 * 1024 * 1024);
```

### 8. ROM Padding (After Loading)
**File:** `src/memory.cpp:865`  
**Context:** `loadROM()` function  
**Purpose:** Zero-fill remaining ROM space after loading ROM file  
**When:** Only during ROM loading (once per ROM load)  
```cpp
memset(rom + read, 0, 32 * 1024 * 1024 - read);
```

### 9. BIOS Padding (After Loading)
**File:** `src/memory.cpp:918`  
**Context:** `loadBIOS()` function  
**Purpose:** Zero-fill remaining BIOS space if file is smaller than 16KB  
**When:** Only during BIOS loading (once at startup)  
```cpp
memset(bios + read, 0, 16 * 1024 - read);
```

## Conclusion

**No runtime memory clearing occurs.** All memset operations are strictly initialization-only:
- Memory allocation during constructor
- File loading/padding during startup
- Test mode setup

The screen blanking observed during BIOS animation is **NOT** caused by memset operations. The likely causes are:
1. BIOS code writing to VRAM/OAM/Palette RAM to clear graphics
2. DMA transfers initiated by BIOS
3. Display state changes (though DISPCNT shows forced blank stays OFF)
4. Timing issues causing frames to be skipped or rendered incorrectly
