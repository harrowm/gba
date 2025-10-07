# Phase 4 - Day 4: Mode 0 Integration & Multi-Background Rendering

**Date**: October 6, 2025  
**Status**: 123 graphics tests passing  
**Goal**: Integrate background rendering into main render loop, support multiple backgrounds with priorities

---

## Overview

Day 3 built the complete background rendering pipeline for a single background. Day 4 focuses on:
1. Integrating `renderBGScanline()` into the main `renderScanline()` loop
2. Supporting Mode 0 (4 tiled backgrounds)
3. Implementing background priority system
4. Compositing multiple backgrounds correctly
5. Integration testing with real scenarios

---

## Day 4 Session Breakdown

### **Session 1: Main Render Loop Integration (90 minutes)**

**Goal**: Call `renderBGScanline()` from the main rendering loop for Mode 0

**Implementation**:

```cpp
void GPU::renderScanline() {
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    uint8_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    if (isForcedBlank()) {
        // Render white screen
        renderBlankScanline(currentScanline);
        return;
    }
    
    switch (mode) {
        case 0:
            renderMode0Scanline(currentScanline);
            break;
        case 3:
            renderMode3Scanline(currentScanline);
            break;
        // Other modes...
        default:
            break;
    }
}

void GPU::renderMode0Scanline(uint16_t scanline) {
    // Clear scanline to backdrop color
    clearScanlineToBackdrop(scanline);
    
    // Render backgrounds in priority order (3 -> 0, within each priority level)
    // This is a simplified first pass - proper priority comes in Session 2
    for (int bg = 3; bg >= 0; bg--) {
        if (isBGEnabled(bg)) {
            renderBGScanline(bg, scanline);
        }
    }
}

void GPU::clearScanlineToBackdrop(uint16_t scanline) {
    // Read backdrop color (palette entry 0)
    uint16_t backdropColor = readBGPaletteRaw(0, 0);
    uint16_t* fb = getFrameBuffer();
    
    for (int x = 0; x < 240; x++) {
        fb[scanline * 240 + x] = backdropColor;
    }
}

void GPU::renderBlankScanline(uint16_t scanline) {
    // Forced blank renders white
    uint16_t* fb = getFrameBuffer();
    for (int x = 0; x < 240; x++) {
        fb[scanline * 240 + x] = 0x7FFF;  // White
    }
}
```

**Tests** (~15 tests):
1. `Mode0Rendering` - Basic Mode 0 with single BG
2. `Mode0MultipleBGs` - Two backgrounds enabled
3. `Mode0AllBGs` - All 4 backgrounds enabled
4. `BackdropColor` - Verify backdrop color is used
5. `ForcedBlank` - White screen when forced blank enabled
6. `DisabledBGsNotRendered` - Disabled BGs don't render
7. `Mode0WithScroll` - Scrolling works in Mode 0
8. `Mode0With8bpp` - 8bpp backgrounds in Mode 0
9. `PartialScanlineRendering` - Some BGs on, some off
10. `EmptyBackground` - BG enabled but no tiles set
11. `BackdropWithTransparency` - Transparent tiles show backdrop
12. `Mode0FullFrame` - Render all 160 scanlines
13. `SwitchBetweenModes` - Mode 0 <-> Mode 3 switching
14. `MultipleFrames` - Render several complete frames
15. `Mode0Performance` - Benchmark rendering speed

**Success Criteria**:
- ✅ Mode 0 renders correctly with single background
- ✅ Multiple backgrounds can be enabled simultaneously
- ✅ Backdrop color shows through transparent areas
- ✅ Forced blank works correctly
- ✅ All 15 tests passing

---

### **Session 2: Background Priority System (90 minutes)**

**Goal**: Implement proper priority ordering for backgrounds

**Background**: Each background has a priority (0-3, where 0 is highest). Lower priority numbers appear on top. When two backgrounds have the same priority, lower-numbered BGs appear on top (BG0 > BG1 > BG2 > BG3).

**Implementation**:

```cpp
struct BGLayer {
    int bgNum;
    uint8_t priority;
};

void GPU::renderMode0Scanline(uint16_t scanline) {
    clearScanlineToBackdrop(scanline);
    
    // Build list of enabled backgrounds with their priorities
    std::vector<BGLayer> layers;
    for (int bg = 0; bg < 4; bg++) {
        if (isBGEnabled(bg)) {
            BGConfig config = readBGCNT(bg);
            layers.push_back({bg, config.priority});
        }
    }
    
    // Sort by priority (higher priority value = render first = appears below)
    // Then by BG number (higher BG number = render first = appears below)
    std::sort(layers.begin(), layers.end(), [](const BGLayer& a, const BGLayer& b) {
        if (a.priority != b.priority) {
            return a.priority > b.priority;  // Higher priority renders first
        }
        return a.bgNum > b.bgNum;  // Higher BG number renders first
    });
    
    // Render in sorted order
    for (const auto& layer : layers) {
        renderBGScanline(layer.bgNum, scanline);
    }
}
```

**Alternative approach** (per-pixel priorities):

```cpp
struct PixelInfo {
    uint16_t color;
    uint8_t priority;
    int bgNum;
    bool isTransparent;
};

void GPU::renderMode0ScanlineWithPriority(uint16_t scanline) {
    uint16_t* fb = getFrameBuffer();
    uint16_t backdrop = readBGPaletteRaw(0, 0);
    
    for (int x = 0; x < 240; x++) {
        PixelInfo topPixel = {backdrop, 4, -1, false};  // Start with backdrop (lowest priority)
        
        // Check each background
        for (int bg = 0; bg < 4; bg++) {
            if (!isBGEnabled(bg)) continue;
            
            BGConfig config = readBGCNT(bg);
            // Get pixel for this background at (x, scanline)
            PixelInfo pixel = getBackgroundPixel(bg, x, scanline, config);
            
            if (!pixel.isTransparent) {
                // Check if this pixel should be on top
                if (pixel.priority < topPixel.priority ||
                    (pixel.priority == topPixel.priority && bg < topPixel.bgNum)) {
                    topPixel = pixel;
                }
            }
        }
        
        fb[scanline * 240 + x] = topPixel.color;
    }
}
```

**Tests** (~18 tests):
1. `Priority0vs1` - BG with priority 0 appears above priority 1
2. `Priority0vs2` - Priority 0 above priority 2
3. `Priority0vs3` - Priority 0 above priority 3
4. `Priority1vs2` - Priority 1 above priority 2
5. `Priority1vs3` - Priority 1 above priority 3
6. `Priority2vs3` - Priority 2 above priority 3
7. `SamePriorityBG0vsBG1` - BG0 above BG1 (same priority)
8. `SamePriorityBG1vsBG2` - BG1 above BG2 (same priority)
9. `SamePriorityBG2vsBG3` - BG2 above BG3 (same priority)
10. `ComplexPriorityMix` - All 4 BGs with different priorities
11. `AllSamePriority` - All BGs at priority 0
12. `AllDifferentPriority` - BG0=0, BG1=1, BG2=2, BG3=3
13. `PriorityWithTransparency` - Lower priority shows through transparent pixels
14. `ChangingPriority` - Change BG priority mid-frame (test on different scanlines)
15. `OverlappingTiles` - Multiple BGs with overlapping tiles
16. `PriorityStressTest` - All combinations of priorities
17. `BackdropPriority` - Backdrop is lowest priority
18. `PriorityWithScroll` - Priority correct with scrolling backgrounds

**Success Criteria**:
- ✅ Priorities work correctly (0 = highest, 3 = lowest)
- ✅ BG number tiebreaker works (BG0 > BG1 > BG2 > BG3)
- ✅ Transparent pixels allow lower priority to show through
- ✅ All 18 tests passing

---

### **Session 3: Performance Optimization (60 minutes)**

**Goal**: Optimize the rendering pipeline for speed

**Optimizations**:

1. **Cache BG configurations** - Don't read BGxCNT every scanline
2. **Early exit for disabled backgrounds** - Check enable bits once
3. **Inline hot path functions** - Mark getTilePixel functions as inline
4. **Reduce memory reads** - Batch VRAM reads where possible
5. **Skip fully transparent scanlines** - If entire scanline is transparent, skip
6. **SIMD opportunities** - Identify areas for vectorization (future)

```cpp
// Cache configuration at the start of each frame
struct CachedBGState {
    bool enabled;
    BGConfig config;
    BGScroll scroll;
};

class GPU {
private:
    CachedBGState bgStates[4];
    bool configDirty;
    
public:
    void cacheFrameState() {
        for (int i = 0; i < 4; i++) {
            bgStates[i].enabled = isBGEnabled(i);
            if (bgStates[i].enabled) {
                bgStates[i].config = readBGCNT(i);
                bgStates[i].scroll = readBGScroll(i);
            }
        }
        configDirty = false;
    }
    
    void onRegisterWrite(uint32_t addr) {
        // Mark config dirty if any BG register is written
        if (addr >= REG_DISPCNT && addr <= REG_BG3VOFS) {
            configDirty = true;
        }
    }
};
```

**Inline optimizations**:

```cpp
// Mark hot functions as inline
inline uint8_t GPU::getTilePixel4bpp(uint32_t tileAddr, int pixelX, int pixelY) {
    // ... implementation
}

inline uint8_t GPU::getTilePixel8bpp(uint32_t tileAddr, int pixelX, int pixelY) {
    // ... implementation
}
```

**Tests** (~10 tests):
1. `RenderingBenchmark` - Time to render full frame
2. `CachingCorrectness` - Cached config produces same results
3. `ConfigDirtyOnWrite` - Config updates when registers written
4. `MultipleFramesPerformance` - Render 60 frames, measure time
5. `ComplexScenePerformance` - All 4 BGs with scrolling
6. `TransparentScanlineSkip` - Verify skip optimization works
7. `MemoryAccessPattern` - Measure VRAM read efficiency
8. `HotPathProfiling` - Identify slowest functions
9. `OptimizedVsCached` - Compare cached vs uncached performance
10. `RegressionTest` - Ensure optimizations don't break correctness

**Performance Targets**:
- Target: < 1ms per frame (for eventual 60 FPS)
- Stretch: < 500μs per frame
- All 160 scanlines rendered correctly

**Success Criteria**:
- ✅ Frame rendering performance measured
- ✅ Caching system implemented and tested
- ✅ No correctness regressions
- ✅ All 10 tests passing

---

### **Session 4: Integration Testing & Visual Verification (90 minutes)**

**Goal**: Create comprehensive integration tests and visual test patterns

**Implementation**:

1. **Test Pattern Generator**:
```cpp
void createTestPattern(Memory& memory) {
    // Create a recognizable test pattern
    // - Grid pattern for BG0
    // - Checkerboard for BG1
    // - Horizontal stripes for BG2
    // - Vertical stripes for BG3
}
```

2. **Visual Test Cases**:
   - Mario-style level (ground tiles, sky background)
   - Zelda-style dungeon (multiple layers)
   - RPG battle background (layered clouds)
   - Scrolling parallax effect
   - Priority demonstration scene

3. **Screenshot Comparison**:
```cpp
bool compareFramebuffers(uint16_t* fb1, uint16_t* fb2) {
    for (int i = 0; i < 240 * 160; i++) {
        if (fb1[i] != fb2[i]) return false;
    }
    return true;
}

void saveFramebufferPPM(uint16_t* fb, const char* filename) {
    // Save as PPM for visual inspection
}
```

**Tests** (~20 tests):
1. `SimpleScene` - One background, simple tiles
2. `TwoLayerScene` - Two backgrounds with different priorities
3. `FourLayerScene` - All four backgrounds enabled
4. `ScrollingBackground` - Animated scrolling
5. `ParallaxScrolling` - Multiple layers scrolling at different speeds
6. `PriorityDemo` - Visual demonstration of priority system
7. `TransparencyShowcase` - Transparent tiles revealing layers below
8. `ComplexTileset` - 256+ unique tiles
9. `Large512x512Map` - Maximum size tilemap
10. `AnimatedTiles` - Simulate tile animation (change tiles over time)
11. `ColorPalettes` - All 16 palettes in use
12. `8bppScene` - 256-color background
13. `Mixed4and8bpp` - Mix of 4bpp and 8bpp backgrounds
14. `EdgeCases` - Screen edge wrapping, boundary tiles
15. `RapidScrolling` - Fast scroll changes
16. `MidFrameChanges` - Change BG config during scanline rendering
17. `FullGameScene` - Realistic game-like scene
18. `StressTest` - Maximum complexity (4 BGs, all scrolling, max tiles)
19. `CompareWithReference` - Match expected output images
20. `VisualRegression` - Ensure output matches previous known-good frames

**Visual Verification**:
- Generate PPM files for manual inspection
- Create GIF animations showing scrolling
- Document expected vs actual output

**Success Criteria**:
- ✅ All 20 integration tests passing
- ✅ Visual test patterns render correctly
- ✅ No graphical glitches or artifacts
- ✅ Scrolling is smooth and correct
- ✅ Priorities work as expected visually

---

## Day 4 Summary

**Total New Tests**: ~63 tests  
**Running Total**: 186 graphics tests  
**Project Total**: ~819 tests

**Deliverables**:
1. ✅ Mode 0 rendering integrated into main loop
2. ✅ Background priority system working correctly
3. ✅ Performance optimizations implemented
4. ✅ Comprehensive integration tests
5. ✅ Visual test patterns and verification

**Key Files Modified**:
- `src/gpu.cpp` - Main rendering loop, Mode 0 implementation, optimizations (~200 lines added)
- `include/gpu.h` - New function declarations, caching structures (~30 lines)
- `tests/graphics/test_mode0.cpp` - Mode 0 integration tests (~500 lines)
- `tests/graphics/test_priority.cpp` - Priority system tests (~600 lines)
- `tests/graphics/test_performance.cpp` - Performance tests (~300 lines)
- `tests/graphics/test_visual.cpp` - Visual verification tests (~700 lines)

**Dependencies**:
- Day 3 complete (background rendering) ✅
- Scheduler integration (for frame timing)
- Memory system (for register access) ✅

---

## Next Steps After Day 4

**Day 5-6**: Sprite Rendering
- OAM (Object Attribute Memory) parsing
- 128 sprites support
- Sprite sizes (8x8 to 64x64)
- Sprite flipping, rotation (affine sprites in Day 6)
- Sprite priorities
- Sprite-to-sprite and sprite-to-background priorities

**Day 7-8**: Advanced Features
- Window system (WIN0, WIN1, OBJWIN)
- Color special effects (alpha blending, brightness)
- Mosaic effect
- Affine backgrounds (rotation/scaling for BG2/BG3)

**Day 9-10**: Polish & Real ROMs
- Test with actual GBA ROMs
- Fix edge cases discovered
- Performance tuning
- Documentation

---

## Notes

**Priority Algorithm Details**:
- Each background has a priority value (0-3) in BGxCNT bits 0-1
- Lower priority values appear on top: 0 > 1 > 2 > 3
- When two backgrounds have the same priority, lower BG numbers win: BG0 > BG1 > BG2 > BG3
- Transparent pixels (palette index 0) don't block lower priorities
- Backdrop color has the lowest priority (always below all backgrounds)

**Performance Considerations**:
- Mode 0 is the most complex tiled mode (4 backgrounds vs 2 in Mode 1, 1 in Mode 2)
- Each scanline requires reading up to 4 tilemaps, multiple tiles, and palette lookups
- Target: ~3000 cycles per scanline (1232 cycles is the hardware limit, but we're software)
- Modern CPU should handle 60 FPS easily with basic optimizations

**Testing Strategy**:
- Unit tests for individual functions (already done)
- Integration tests for full rendering pipeline (Day 4)
- Visual tests for correctness verification (Day 4)
- Performance tests for optimization (Day 4)
- Real ROM tests for completeness (Day 9-10)

---

## Estimated Time

- **Session 1**: 90 minutes (integration)
- **Session 2**: 90 minutes (priorities)
- **Session 3**: 60 minutes (optimization)
- **Session 4**: 90 minutes (integration tests)

**Total Day 4**: ~5-6 hours  
**Status**: Ready to begin!
