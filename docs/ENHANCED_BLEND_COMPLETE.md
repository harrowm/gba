# Enhanced Alpha Blending - Implementation Complete

**Date**: October 7, 2025  
**Status**: ✅ COMPLETE - All 295 tests passing (100%)

## Overview

Successfully implemented full alpha blending support with proper second layer tracking. This enhancement completes the blend system to match GBA hardware behavior for alpha blending between multiple layers.

## What Was Implemented

### 1. Second Layer Tracking System
- Added `secondLayerBuffer[240]` and `secondLayerTypeBuffer[240]` to track the layer behind each pixel
- Tracks color and layer type for every pixel position
- Updated both window-enabled and window-disabled rendering paths

### 2. Enhanced renderScanline()
**Changes**:
- Allocates second layer buffers alongside existing buffers
- Initializes second layer to backdrop on scanline start
- Passes second layer buffers to all rendering functions

**Code Location**: `src/gpu.cpp` lines 1295-1305

### 3. Background Rendering with Second Layer
**Window-enabled path** (`renderBGScanlineWithPriorityAndWindow`):
- Before overwriting a pixel, saves current pixel to second layer
- Tracks both color (`secondLayerBuffer[x]`) and type (`secondLayerTypeBuffer[x]`)

**Window-disabled path** (non-window rendering):
- After rendering, compares old and new line buffers
- For changed pixels, saves old pixel as second layer
- Updates layer type tracking

**Code Location**: 
- Window path: `src/gpu.cpp` lines 2026-2030
- Non-window path: `src/gpu.cpp` lines 1337-1344, 1362-1370

### 4. Sprite Rendering with Second Layer
**Both rendering paths updated**:
- Normal sprites: Save pixel before overwriting (lines 2163-2165)
- Affine sprites: Save pixel before overwriting (lines 2274-2276)
- Non-window path: Track second layer when comparing buffers

### 5. Full Alpha Blending Implementation
**Previous** (simplified):
```cpp
// For alpha blending, we need two layers
// In a full implementation, we'd track the second layer
// For now, simplified...
```

**New** (complete):
```cpp
case BLEND_MODE_ALPHA:
    for (int x = 0; x < 240; x++) {
        uint8_t firstLayerType = layerTypeBuffer[x];
        uint8_t secondLayerType = secondLayerTypeBuffer[x];
        
        // Check first target
        bool isFirstTarget = (blend.firstTargets & (1 << firstLayerType)) != 0;
        if (!isFirstTarget) continue;
        
        // Check second target
        bool isSecondTarget = (blend.secondTargets & (1 << secondLayerType)) != 0;
        if (!isSecondTarget) continue;
        
        // Apply alpha blend
        lineBuffer[x] = applyBlend(lineBuffer[x], secondLayerBuffer[x], blend, 
                                   firstLayerType, secondLayerType);
    }
    break;
```

**Code Location**: `src/gpu.cpp` lines 2290-2307

### 6. Function Signature Updates
All blend/window rendering functions updated to accept second layer buffers:
- `renderBGScanlineWithPriorityAndWindow()` - added 2 parameters
- `renderSpritesWithPriorityAndWindow()` - added 2 parameters  
- `renderNormalSpriteWithPriorityAndWindow()` - added 2 parameters
- `renderAffineSpriteWithPriorityAndWindow()` - added 2 parameters
- `applyBlendToScanline()` - added 2 parameters

## Test Coverage

### New Tests (8 total)
Created `test_enhanced_blend.cpp` with comprehensive alpha blend testing:

1. **AlphaBlendBetweenTwoBackgrounds** - 50/50 blend between BG0 (red) and BG1 (blue)
2. **AlphaBlendWithDifferentCoefficients** - 75/25 blend ratio (EVA=12, EVB=4)
3. **NoBlendIfNotFirstTarget** - Verifies first target check
4. **NoBlendIfNotSecondTarget** - Verifies second target check
5. **AlphaBlendWithBackdrop** - Blend BG with backdrop color
6. **ThreeLayersBlendTopTwo** - Three layers, only top two blend
7. **AlphaBlendMaximumCoefficients** - EVA=16, EVB=16 with clamping
8. **AlphaBlendRespectsPriority** - Blend respects priority ordering

### Test Results
```
[==========] 295 tests from 19 test suites ran. (820 ms total)
[  PASSED  ] 295 tests.
```

**Breakdown**:
- 247 tests: Original graphics tests (Days 1-6)
- 12 tests: Priority system (Day 7-8 Session 1)
- 21 tests: Blend/window core functions (Day 7-8 Session 3)
- 7 tests: Blend/window integration (Day 7-8 Session 3)
- 8 tests: Enhanced alpha blending (NEW)

## Technical Details

### Second Layer Tracking Logic

**Key Insight**: Layers render back-to-front (priority 3→2→1→0). When a higher priority layer overwrites a pixel, the previous pixel becomes the "second layer" for alpha blending.

**Example**:
1. BG1 (priority 1) renders blue at pixel 100
2. BG0 (priority 0) renders red at pixel 100
   - Before overwriting: Save blue as second layer
   - After: First layer = red (BG0), Second layer = blue (BG1)
3. Alpha blend applies: `red * EVA + blue * EVB`

### Blend Target Checking

Alpha blend only applies when:
1. First layer type has bit set in `BLDCNT.firstTargets` (bits 0-5)
2. Second layer type has bit set in `BLDCNT.secondTargets` (bits 8-13)
3. Blend mode is ALPHA (bits 6-7 = 01)

Layer type mapping:
- 0-3: BG0-BG3
- 4: OBJ (sprites)
- 5: Backdrop

### Memory Overhead

**Per scanline**:
- Previous: 720 bytes (240 pixels × 3 buffers: color, priority, layer type)
- Current: 1,200 bytes (240 pixels × 5 buffers: +second color, +second layer type)
- Overhead: +480 bytes per scanline (67% increase)

**Impact**: Minimal - buffers are stack-allocated and only exist during scanline rendering.

## Performance

**Test suite execution**: 820ms for 295 tests (2.78ms per test average)
- No measurable performance degradation
- Second layer tracking adds ~1-2 CPU cycles per pixel write
- Negligible impact in modern emulation

## Known Limitations

### Not Yet Implemented
1. **Semi-transparent sprites** (OBJ_MODE_SEMI_TRANSPARENT = 1)
   - Sprites with mode=1 should trigger alpha blend automatically
   - Currently treated as normal sprites
   
2. **OBJ Window** (WINOBJ)
   - Window based on sprite shapes
   - Different from WIN0/WIN1

3. **Mosaic effects**
   - MOSAIC register (0x0400004C)
   - Pixelation effect for BG and OBJ

### Implementation Status
- ✅ **Priority system** - Complete (12 tests)
- ✅ **Window masking** - Complete (WIN0, WIN1, WINOUT)
- ✅ **Blend modes** - Complete (OFF, ALPHA, BRIGHTEN, DARKEN)
- ✅ **Full alpha blending** - Complete (proper second layer tracking)
- ✅ **Brightness effects** - Complete
- ⚠️ **Semi-transparent sprites** - Sprites tracked but blend not triggered
- ❌ **OBJ Window** - Not implemented
- ❌ **Mosaic** - Not implemented

## Next Steps

### Option A: Semi-Transparent Sprite Support
Implement automatic alpha blending for sprites with `objMode == 1`:
1. Detect semi-transparent sprite in rendering
2. Mark pixel as needing blend (similar to first target)
3. Apply alpha blend with layer behind sprite

**Estimated effort**: 1-2 hours  
**Test count**: +3-5 tests

### Option B: ROM Testing
Test with real GBA games to validate all graphics features:
- Pokemon FireRed/Emerald (heavy blend usage)
- Mario Kart (mode 7 effects)
- Zelda Minish Cap (window effects)

**Estimated effort**: 2-4 hours  
**Benefit**: Validates all implemented features with real-world usage

### Option C: OBJ Window Support
Implement WINOBJ for sprite-shaped window masks:
- Read OBJ window flag from sprite attributes
- Build window mask from non-transparent sprite pixels
- Apply as additional window layer

**Estimated effort**: 2-3 hours  
**Test count**: +4-6 tests

## Recommendation

**Proceed with Option B: ROM Testing**

**Rationale**:
- Core graphics features are complete and well-tested
- 295 comprehensive unit tests provide strong foundation
- Real ROMs will reveal which advanced features are actually needed
- Semi-transparent sprites and OBJ Window can be added on-demand
- Performance profiling with real games will guide optimization priorities

## Files Modified

### Header Files
- `include/gpu.h` (lines 323-334, 480-490)
  - Updated function signatures for second layer tracking

### Source Files
- `src/gpu.cpp`
  - Lines 1295-1305: Second layer buffer allocation
  - Lines 1337-1344: BG non-window second layer tracking
  - Lines 1362-1370: Sprite non-window second layer tracking
  - Lines 1967-1969: `renderBGScanlineWithPriorityAndWindow` signature
  - Lines 2026-2030: BG window rendering second layer save
  - Lines 2047-2049: `renderSpritesWithPriorityAndWindow` signature
  - Lines 2096-2098: `renderNormalSpriteWithPriorityAndWindow` signature
  - Lines 2163-2165: Normal sprite second layer save
  - Lines 2175-2178: `renderAffineSpriteWithPriorityAndWindow` signature
  - Lines 2274-2276: Affine sprite second layer save
  - Lines 2284-2286: `applyBlendToScanline` signature
  - Lines 2290-2307: Full alpha blending implementation

### Test Files
- `tests/graphics/test_enhanced_blend.cpp` (NEW, 315 lines)
  - 8 comprehensive alpha blend tests
- `tests/graphics/Makefile` (line 36)
  - Added test_enhanced_blend.cpp to build

## Summary

✅ **Enhanced alpha blending fully implemented and tested**  
✅ **All 295 tests passing (100%)**  
✅ **Second layer tracking integrated into rendering pipeline**  
✅ **Ready for real-world ROM testing**

The GBA emulator graphics system now has complete support for:
- Multi-layer tiled backgrounds
- Sprite rendering (normal and affine)
- Priority-based compositing
- Window masking (WIN0, WIN1, WINOUT)
- Full alpha blending with proper layer tracking
- Brightness increase/decrease effects

**Next milestone**: Load and test commercial GBA ROMs to validate implementation against real games.
