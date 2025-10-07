# Phase 4: Tile-Based Graphics & Sprites - Implementation Plan

**Status**: 🚀 **READY TO START**  
**Prerequisites**: ✅ DMA Complete (65 tests passing), ✅ Display System Ready, ✅ BIOS/ROM Loading Working  
**Goal**: Render tile-based backgrounds and sprites - see actual game graphics!  
**Timeline**: 6-10 days (aggressive but achievable given our momentum)

---

## Current State Assessment

### What We Have ✅
- **Display System**: SDL2 rendering at 60 FPS, Mode 3 (bitmap) working
- **DMA**: All 4 channels functional, V-Blank/H-Blank triggers, tested with 65 tests
- **Memory**: VRAM (0x06000000), OAM (0x07000000), Palette RAM (0x05000000) all mapped
- **Timing**: Cycle-accurate scanline timing, H-Blank/V-Blank interrupts
- **ROM Loading**: Can load and execute commercial ROMs

### What We Need 🎯
- **Tile Rendering**: Read tile data from VRAM, render 8x8 tiles
- **Background Layers**: BG0-BG3 with tile maps, scrolling, priorities
- **Sprite Engine**: Read OAM, render sprites with transparency and priorities
- **Palette System**: 16-color and 256-color palette modes
- **Pixel Pipeline**: Combine backgrounds, sprites, and handle priorities

---

## Implementation Strategy: Bottom-Up Approach

We'll build from pixels up to full scenes, testing each component:

```
Layer 1: Palette & Tile Basics  (Day 1-2)  ← Start here
Layer 2: Background Rendering   (Day 3-4)
Layer 3: Sprite Rendering       (Day 5-6)
Layer 4: Priority & Compositing (Day 7-8)
Layer 5: Polish & Test ROMs     (Day 9-10)
```

---

## Day 1-2: Palette System & Tile Decoding

**Goal**: Decode tiles and palettes, render single tiles to screen

### Session 1: Palette System (2-3 hours)

**Background Palette RAM** (0x05000000 - 0x050001FF, 512 bytes):
- 16 palettes × 16 colors = 256 colors
- Each color: RGB555 format (2 bytes)
- Color 0 of each palette is transparent

**Sprite Palette RAM** (0x05000200 - 0x050003FF, 512 bytes):
- Same structure as background palettes
- Separate from BG palettes

**Tasks**:
1. Create `Palette` class or helper functions in `gpu.cpp`
   - `uint16_t readBGPalette(int paletteNum, int colorIndex)`
   - `uint32_t convertRGB555toARGB8888(uint16_t rgb555)` (already have this?)
   - `uint32_t getBGColor(int paletteNum, int colorIndex)` → returns ARGB
2. Write tests for palette reading
3. Test palette loading via DMA

**Deliverables**:
- Can read palette data from memory
- Correct RGB555 → ARGB8888 conversion
- 5-10 palette tests passing

---

### Session 2: Tile Data Format (2-3 hours)

**Tile Formats**:
- **4bpp (4 bits per pixel)**: 32 bytes per tile (8×8 pixels, 4 bits each)
  - Each pixel is an index into a 16-color palette
  - 2 pixels per byte
- **8bpp (8 bits per pixel)**: 64 bytes per tile
  - Each pixel is an index into a 256-color palette
  - 1 pixel per byte

**VRAM Layout**:
- Character Base Blocks: 4 blocks × 16KB = 64KB
- Each block can hold 256 tiles (4bpp) or 512 tiles (8bpp)

**Tasks**:
1. Create tile decoding functions in `gpu.cpp`
   - `void decodeTile4bpp(uint32_t tileAddr, uint8_t* output)`
     - Reads 32 bytes from VRAM
     - Outputs 64 palette indices (8×8 pixels)
   - `void decodeTile8bpp(uint32_t tileAddr, uint8_t* output)`
     - Reads 64 bytes from VRAM
     - Outputs 64 palette indices
2. Create test: Load tile data via DMA, decode it, verify indices
3. Render a single tile to screen (for visual verification)

**Deliverables**:
- Tile decoding functions working
- Can extract palette indices from tile data
- Visual test showing a single tile on screen

---

### Session 3: DISPCNT Register Deep Dive (1-2 hours)

**DISPCNT (0x04000000)** - Display Control:
```
Bit   Purpose
0-2   BG Mode (0-5)
3     CGB Mode (ignore for now)
4     Display Frame Select (Mode 4/5 only)
5     H-Blank Interval Free (for OAM access)
6     Object Character VRAM Mapping (1D vs 2D)
7     Forced Blank
8     Enable BG0
9     Enable BG1
10    Enable BG2
11    Enable BG3
12    Enable OBJ (sprites)
13    Enable Window 0
14    Enable Window 1
15    Enable OBJ Window
```

**Tasks**:
1. Parse DISPCNT in `gpu.cpp`
   - `uint8_t getVideoMode()` → bits 0-2
   - `bool isBGEnabled(int bgNum)` → bits 8-11
   - `bool areSpritesEnabled()` → bit 12
   - `bool isForcedBlank()` → bit 7
2. Update `renderScanline()` to check forced blank
3. Write tests for DISPCNT parsing

**Deliverables**:
- DISPCNT register fully parsed
- Mode detection working
- BG enable flags accessible

---

## Day 3-4: Background Rendering (Mode 0)

**Goal**: Render tile-based backgrounds with scrolling

### Session 1: BGxCNT Registers (2-3 hours)

**BG0CNT-BG3CNT (0x04000008-0x0400000E)** - BG Control:
```
Bit   Purpose
0-1   Priority (0-3, 0=highest)
2-3   Character Base Block (0-3)
6     Mosaic
7     Colors/Palettes (0=16/16, 1=256/1)
8-12  Screen Base Block (0-31)
13    BG Wrap (affine only)
14-15 Screen Size (depends on mode)
```

**Screen Sizes (Mode 0)**:
```
Value  Size        Tiles
00     256×256     32×32
01     512×256     64×32
10     256×512     32×64
11     512×512     64×64
```

**Tasks**:
1. Parse BGxCNT registers
   - `uint8_t getBGPriority(int bgNum)`
   - `uint8_t getBGCharBase(int bgNum)` → base address / 16KB
   - `uint8_t getBGScreenBase(int bgNum)` → base address / 2KB
   - `bool isBG256Color(int bgNum)` → palette mode
   - `void getBGSize(int bgNum, int& width, int& height)` → in pixels
2. Create `struct BGConfig` to hold parsed values
3. Write tests for register parsing

**Deliverables**:
- All BGxCNT registers parsed
- BG configuration accessible
- 5-10 tests for register parsing

---

### Session 2: Tile Maps (2-3 hours)

**Tile Map Entry Format (16-bit)**:
```
Bit   Purpose
0-9   Tile Number (0-1023)
10    Horizontal Flip
11    Vertical Flip
12-15 Palette Number (4bpp mode only)
```

**Tile Map Layout**:
- Screen Base Block contains tile map
- Each entry is 2 bytes (uint16_t)
- 32×32 tiles = 2KB
- Larger maps use multiple 2KB blocks

**Tasks**:
1. Create tile map reading functions
   - `uint16_t readTileMapEntry(int bgNum, int tileX, int tileY)`
   - `void parseTileMapEntry(uint16_t entry, TileInfo& info)`
     - Extract tile number, flips, palette
2. Implement tile coordinate to map offset calculation
3. Handle wrapping for different screen sizes
4. Write tests for tile map reading

**Deliverables**:
- Can read tile map entries
- Tile map coordinates calculated correctly
- Flip flags and palette numbers extracted

---

### Session 3: Scrolling (1-2 hours)

**Scroll Registers**:
- BG0HOFS (0x04000010): BG0 X offset
- BG0VOFS (0x04000012): BG0 Y offset
- Similar for BG1-BG3

**Tasks**:
1. Parse scroll registers
   - `uint16_t getBGScrollX(int bgNum)`
   - `uint16_t getBGScrollY(int bgNum)`
2. Apply scrolling to tile coordinate calculations
3. Handle wrapping at screen edges
4. Write tests for scroll calculations

---

### Session 4: Render BG Scanline (3-4 hours)

**The Big Integration**: Combine everything to render a background scanline

**Algorithm**:
```cpp
void renderBGScanline(int bgNum, int scanline, uint32_t* lineBuffer) {
    // 1. Check if BG is enabled
    if (!isBGEnabled(bgNum)) return;
    
    // 2. Get BG config (priority, char base, screen base, size)
    BGConfig cfg = getBGConfig(bgNum);
    
    // 3. Get scroll position
    uint16_t scrollX = getBGScrollX(bgNum);
    uint16_t scrollY = getBGScrollY(bgNum);
    
    // 4. Calculate which tile row we're rendering
    int worldY = (scanline + scrollY) % cfg.heightPixels;
    int tileY = worldY / 8;
    int pixelY = worldY % 8;
    
    // 5. For each pixel on the scanline (0-239)
    for (int x = 0; x < 240; x++) {
        int worldX = (x + scrollX) % cfg.widthPixels;
        int tileX = worldX / 8;
        int pixelX = worldX % 8;
        
        // 6. Read tile map entry
        uint16_t mapEntry = readTileMapEntry(bgNum, tileX, tileY);
        TileInfo tile;
        parseTileMapEntry(mapEntry, tile);
        
        // 7. Apply flips to pixel coordinates
        int finalPixelX = tile.hFlip ? (7 - pixelX) : pixelX;
        int finalPixelY = tile.vFlip ? (7 - pixelY) : pixelY;
        
        // 8. Get tile data address
        uint32_t tileAddr = cfg.charBase + tile.number * (cfg.is256Color ? 64 : 32);
        
        // 9. Read pixel index from tile
        uint8_t paletteIndex = readTilePixel(tileAddr, finalPixelX, finalPixelY, cfg.is256Color);
        
        // 10. If pixel is transparent (index 0), skip
        if (paletteIndex == 0) continue;
        
        // 11. Get color from palette
        uint32_t color = getBGColor(cfg.is256Color ? 0 : tile.paletteNum, paletteIndex);
        
        // 12. Write to line buffer (will handle priority later)
        lineBuffer[x] = color;
    }
}
```

**Tasks**:
1. Implement `renderBGScanline()` function
2. Call it from `renderScanline()` for each enabled BG
3. Test with a simple tile map
4. Create visual test ROM or test data

**Deliverables**:
- Background rendering working
- Can see tiles on screen
- Scrolling works
- Flip flags respected

---

## Day 5-6: Sprite Rendering (OAM)

**Goal**: Render sprites with transparency and priorities

### Session 1: OAM Structure (2-3 hours)

**OAM (Object Attribute Memory)** - 0x07000000, 1KB:
- 128 sprites (objects)
- Each sprite: 8 bytes (3 × uint16_t attributes + 2 bytes padding)

**Attribute 0 (0x04xx0000 + objNum*8 + 0)**:
```
Bit   Purpose
0-7   Y Coordinate
8-9   Object Mode (Normal, Semi-transparent, OBJ Window, Disabled)
10-11 GFX Mode (Normal, Affine, Disable, Affine+Double)
12    Mosaic
13    Colors (0=16/16 palettes, 1=256/1 palette)
14-15 Shape (Square, Horizontal, Vertical, Prohibited)
```

**Attribute 1 (0x04xx0000 + objNum*8 + 2)**:
```
Bit   Purpose
0-8   X Coordinate
9-13  Affine Parameter (if affine mode) OR H-Flip (if not affine)
14    V-Flip (non-affine only)
15-15 Size (combined with shape)
```

**Attribute 2 (0x04xx0000 + objNum*8 + 4)**:
```
Bit   Purpose
0-9   Tile Number
10-11 Priority (relative to BGs)
12-15 Palette Number (4bpp mode only)
```

**Sprite Sizes**:
```
Shape\Size  00      01      10      11
Square      8×8     16×16   32×32   64×64
Horizontal  16×8    32×8    32×16   64×32
Vertical    8×16    8×32    16×32   32×64
```

**Tasks**:
1. Create `struct SpriteAttributes` to hold parsed OAM data
2. Implement OAM parsing functions
   - `void parseOAM(int objNum, SpriteAttributes& sprite)`
   - `void getSpriteSize(uint8_t shape, uint8_t size, int& width, int& height)`
3. Write tests for OAM parsing
4. Test reading OAM data

**Deliverables**:
- OAM structure parsed correctly
- Sprite attributes accessible
- Size calculation working

---

### Session 2: Sprite Tile Data (2-3 hours)

**Sprite Tile Mapping**:
- **1D Mapping** (DISPCNT bit 6 = 1): Tiles are consecutive in memory
  - If sprite is 2×2 tiles (16×16), tiles are at: base, base+1, base+2, base+3
- **2D Mapping** (DISPCNT bit 6 = 0): Tiles arranged in 32-tile-wide grid
  - 16×16 sprite tiles at: base, base+1, base+32, base+33

**Sprite Character Base**: 0x06010000 (after BG tiles)

**Tasks**:
1. Implement sprite tile addressing
   - `uint32_t getSpriteTileAddr(int tileNum, bool is1D, bool is256Color)`
2. Implement sprite pixel reading
   - Handle multi-tile sprites (iterate over tiles)
   - Apply H-Flip and V-Flip
3. Write tests for tile addressing in both modes
4. Render a single sprite to verify

**Deliverables**:
- Sprite tile addressing correct
- Can read sprite pixel data
- Visual test of a single sprite

---

### Session 3: Sprite Rendering Pipeline (3-4 hours)

**Algorithm**:
```cpp
void renderSprites(int scanline, uint32_t* lineBuffer, uint8_t* priorityBuffer) {
    if (!areSpritesEnabled()) return;
    
    // Render sprites in reverse order (127 → 0 for priority)
    for (int i = 127; i >= 0; i--) {
        SpriteAttributes sprite;
        parseOAM(i, sprite);
        
        // Skip if disabled or not on this scanline
        if (sprite.disabled || !spriteOnScanline(sprite, scanline)) continue;
        
        // Calculate Y offset within sprite
        int spriteY = scanline - sprite.y;
        if (sprite.vFlip) spriteY = sprite.height - 1 - spriteY;
        
        // For each pixel in sprite width
        for (int spriteX = 0; spriteX < sprite.width; spriteX++) {
            int screenX = sprite.x + spriteX;
            if (screenX < 0 || screenX >= 240) continue;
            
            // Apply H-Flip
            int finalX = sprite.hFlip ? (sprite.width - 1 - spriteX) : spriteX;
            
            // Get pixel from sprite tile data
            uint8_t paletteIndex = getSpriteTilePixel(sprite, finalX, spriteY);
            
            // Skip transparent pixels
            if (paletteIndex == 0) continue;
            
            // Check priority (if pixel already drawn with higher priority, skip)
            if (priorityBuffer[screenX] < sprite.priority) continue;
            
            // Get color from sprite palette
            uint32_t color = getOBJColor(sprite.paletteNum, paletteIndex);
            
            // Write to line buffer
            lineBuffer[screenX] = color;
            priorityBuffer[screenX] = sprite.priority;
        }
    }
}
```

**Tasks**:
1. Implement `renderSprites()` function
2. Add priority buffer to track which pixels are drawn
3. Handle sprite-sprite priority (lower OBJ number = higher priority)
4. Test with multiple overlapping sprites
5. Verify transparency works

**Deliverables**:
- Sprites render correctly
- Transparency works (color 0 is transparent)
- Sprite-sprite priority correct
- H-Flip and V-Flip working

---

## Day 7-8: Priority System & Compositing

**Goal**: Combine backgrounds and sprites with correct priority

### Session 1: Priority Layers (2-3 hours)

**GBA Priority System**:
- Each BG has priority 0-3 (0 = front, 3 = back)
- Sprites have priority 0-3 (relative to BGs)
- **Priority 0**: Front-most objects/BGs
- **Priority 3**: Back-most objects/BGs
- Within same priority: BG0 > BG1 > BG2 > BG3 > OBJ

**Rendering Order** (back to front):
```
1. BG priority 3
2. OBJ priority 3
3. BG priority 2
4. OBJ priority 2
5. BG priority 1
6. OBJ priority 1
7. BG priority 0
8. OBJ priority 0
```

**Tasks**:
1. Refactor rendering to use priority-based approach
2. Create priority buffer (stores which layer drew each pixel)
3. Render all layers to separate buffers
4. Composite based on priority
5. Write tests for priority ordering

**Deliverables**:
- Priority system working
- BGs and sprites layered correctly
- Tests verify priority order

---

### Session 2: Backdrop Color (1 hour)

**Backdrop**: The color shown when no BG or sprite pixel is drawn
- Located at palette address 0x05000000 (first color of first BG palette)

**Tasks**:
1. Clear framebuffer to backdrop color each scanline
2. Ensure backdrop is behind everything
3. Test with transparent areas

---

### Session 3: Integration & Optimization (2-3 hours)

**Complete the rendering pipeline**:
```cpp
void GPU::renderScanline(int scanline) {
    // 1. Get backdrop color
    uint32_t backdrop = getBGColor(0, 0);
    
    // 2. Create line buffers
    uint32_t lineBuffer[240];
    uint8_t priorityBuffer[240];
    
    // 3. Fill with backdrop
    for (int i = 0; i < 240; i++) {
        lineBuffer[i] = backdrop;
        priorityBuffer[i] = 255; // Lowest priority
    }
    
    // 4. Render each priority level (back to front)
    for (int priority = 3; priority >= 0; priority--) {
        // Render BGs with this priority
        for (int bg = 3; bg >= 0; bg--) {
            if (getBGPriority(bg) == priority) {
                renderBGScanline(bg, scanline, lineBuffer, priorityBuffer);
            }
        }
        
        // Render sprites with this priority
        renderSpritesWithPriority(priority, scanline, lineBuffer, priorityBuffer);
    }
    
    // 5. Copy line buffer to framebuffer
    copyToFramebuffer(scanline, lineBuffer);
}
```

**Tasks**:
1. Implement complete rendering pipeline
2. Test with complex scenes (multiple BGs + sprites)
3. Profile and optimize hot paths
4. Ensure 60 FPS maintained

**Deliverables**:
- Complete rendering pipeline
- Maintains 60 FPS
- Correct visual output

---

## Day 9-10: Testing & Polish

### Session 1: Test ROMs (3-4 hours)

**Test with real games**:
1. **Tonc Demos** - Educational GBA programming examples
   - `first.gba` - Simple tile display
   - `brin.gba` - Background scrolling
   - `obj_demo.gba` - Sprite examples
2. **mGBA Test Suite** - Graphics tests
3. **Simple Commercial ROMs**
   - Advance Wars (strategy game with tiles)
   - Fire Emblem (similar)
   - Pokémon (if brave!)

**Tasks**:
1. Run test ROMs and identify rendering issues
2. Debug incorrect graphics
3. Fix edge cases and bugs
4. Document any limitations

---

### Session 2: Visual Debugging Tools (2-3 hours)

**Create debugging visualizations**:
1. Tile viewer - Show all tiles in VRAM
2. Background viewer - Show each BG layer separately
3. Sprite viewer - Show all active sprites
4. Palette viewer - Display all palettes
5. OAM viewer - Show sprite attributes

**Tasks**:
1. Add debug rendering modes (keyboard shortcuts?)
2. Implement visualization tools
3. Test usefulness for debugging

---

### Session 3: Documentation & Cleanup (2-3 hours)

1. Document rendering architecture
2. Add inline comments to complex functions
3. Create usage guide for test ROMs
4. Update plan.md with Phase 4 completion
5. Celebrate! 🎉

---

## Testing Strategy

### Unit Tests
- Palette reading/conversion (10 tests)
- Tile decoding 4bpp/8bpp (10 tests)
- BGxCNT register parsing (10 tests)
- Tile map reading (10 tests)
- OAM parsing (15 tests)
- Sprite size calculations (10 tests)
- Priority system (10 tests)
- **Total: ~75 new graphics tests**

### Integration Tests
- Render single tile
- Render background with scrolling
- Render single sprite
- Render multiple overlapping sprites
- Combine BGs and sprites with priorities
- Test with DMA transfers

### Visual Tests
- Run test ROMs and compare screenshots
- Record expected output for regression testing

---

## Success Criteria

✅ **Minimum Viable** (Ready to proceed to Phase 5):
- [ ] Mode 0 backgrounds rendering correctly
- [ ] Sprites rendering with transparency
- [ ] Priority system working
- [ ] Scrolling functional
- [ ] Can run `first.gba` from Tonc
- [ ] At least one test ROM displays correctly

🎯 **Full Success** (Phase 4 Complete):
- [ ] All tile modes (0-2) working
- [ ] All sprite features (flip, priority, 1D/2D mapping)
- [ ] 75+ graphics tests passing
- [ ] Multiple test ROMs running correctly
- [ ] Maintains 60 FPS with complex scenes
- [ ] Visual debugging tools available

🚀 **Stretch Goals**:
- [ ] Mosaic effects
- [ ] Affine backgrounds (Mode 1-2)
- [ ] Affine sprites (rotation/scaling)
- [ ] Window system (basic)

---

## Risk Mitigation

**Potential Blockers**:
1. **Performance**: Rendering is CPU-intensive
   - Mitigation: Profile early, optimize hot paths, consider dirty rectangles
2. **Complexity**: Many interacting systems
   - Mitigation: Build incrementally, test each component
3. **Debugging**: Visual bugs are hard to diagnose
   - Mitigation: Build debugging tools early, compare with working emulators

**Fallback Plan**:
If we get stuck on complex features, we can:
- Skip affine transformations initially
- Defer windows/blending to Phase 5
- Focus on getting one test ROM working perfectly

---

## Resource References

**Documentation**:
- GBATEK: https://problemkaputt.de/gbatek.htm
- Tonc Tutorial: https://www.coranac.com/tonc/text/
- CowBite Spec: https://www.cs.rit.edu/~tjh8300/CowBite/CowBiteSpec.htm

**Test ROMs**:
- Tonc Demos: https://www.coranac.com/projects/tonc/
- mGBA Test Suite: https://github.com/mgba-emu/suite

**Working Emulators** (for reference):
- mGBA: Most accurate
- VisualBoyAdvance-M: Good debugging tools
- No$GBA: Excellent debugger

---

## Let's Get Started! 🚀

**Recommended First Step**: 
Start with Day 1, Session 1 - Palette System. This is the foundation for everything else, and we can see results quickly by displaying colored tiles.

**Command to begin**:
```bash
# Create test file
touch tests/graphics/test_palette.cpp

# Or jump straight into gpu.cpp and add palette functions
```

Ready to see some actual game graphics? Let's make it happen! 🎮✨
