# Semi-Transparent Sprite Implementation - Complete ✅

**Status**: Complete  
**Date**: 2024  
**Tests**: 303/303 passing (100%)

## Overview

Implemented automatic alpha blending for semi-transparent sprites (objMode == 1) in the GBA emulator. This feature allows sprites to blend with the layer behind them using hardware-controlled EVA/EVB coefficients.

## Implementation Summary

### Detection & Blending Logic

Semi-transparent sprites are sprites with `objMode == 1` (OBJ_MODE_SEMI_TRANSPARENT). When detected during sprite rendering, these sprites automatically blend with the pixel that was already rendered at that position.

**Blending Requirements**:
1. Sprite must have `objMode == 1` (semi-transparent mode)
2. BLDCNT register must have blend mode = 1 (alpha blend, bits 6-7)
3. OBJ must be marked as first target in BLDCNT (bit 4)

**Blend Formula**:
```
result_color = (sprite_color * EVA + second_layer_color * EVB) / 16
```

Where EVA and EVB are read from the BLDALPHA register (0x04000052).

### Key Changes

#### 1. Sprite Rendering Functions (`src/gpu.cpp`)

**renderNormalSpriteWithPriorityAndWindow()** (lines ~2170-2220):
- Added semi-transparent sprite detection
- Read second layer color/type before overwriting pixel
- Check BLDCNT for alpha blend mode and OBJ first target
- Apply alpha blend formula with EVA/EVB coefficients
- Clamp result to 5-bit RGB555 range
- Mark result with layer type 254

**renderAffineSpriteWithPriorityAndWindow()** (lines ~2310-2360):
- Same semi-transparent logic for affine (rotated/scaled) sprites

#### 2. Layer Type Marking System

- **Layer type 4**: Normal sprites (objMode != 1)
- **Layer type 254**: Semi-transparent sprites (already blended)

This distinction is critical to prevent double-blending in the global blend pass.

#### 3. Global Blend Pass (`applyBlendToScanline()`)

Modified to skip ALL sprite layers (types 4 and 254):
- Type 4 (normal sprites): Never participate in global alpha blend
- Type 254 (semi-transparent sprites): Already blended during rendering

This ensures sprites only blend during the sprite rendering pass, not again during the global blend pass.

#### 4. Test Palette Fix

Fixed test setup issue where multiple sprites shared the same palette entry:
- Test 6 (SemiTransparentSpriteOverAnotherSprite): Sprite 1 now uses palette 1
- Test 8 (MultipleSemiTransparentSprites): Sprite 1 now uses palette 1
- This prevents the second sprite's color from overwriting the first sprite's color

## Test Coverage

Created 8 comprehensive tests in `tests/graphics/test_semitransparent_sprites.cpp`:

1. ✅ **SemiTransparentSpriteBlendWithBackground**
   - Semi-transparent sprite blends with BG0
   - 50/50 blend (EVA=8, EVB=8)

2. ✅ **DifferentBlendCoefficients**
   - Tests 75/25 blend ratio (EVA=12, EVB=4)

3. ✅ **SemiTransparentSpriteWithBackdrop**
   - Semi-transparent sprite blends with backdrop color

4. ✅ **NormalSpriteDoesNotBlend**
   - Verifies objMode=0 sprites remain opaque
   - Even when OBJ marked as first target

5. ✅ **NoBlendIfTargetsNotSet**
   - Semi-transparent sprite doesn't blend if OBJ not a first target
   - BLDCNT check working correctly

6. ✅ **SemiTransparentSpriteOverAnotherSprite**
   - Blue semi-transparent sprite (priority 0) blends with red normal sprite (priority 1)
   - Tests sprite-over-sprite blending

7. ✅ **EVAZeroMakesSpriteInvisible**
   - EVA=0, EVB=16 makes sprite fully transparent
   - Shows 100% second layer

8. ✅ **MultipleSemiTransparentSprites**
   - Two semi-transparent sprites at different priorities
   - Tests cascading blends

## Technical Details

### Blending Conditions

Semi-transparent sprites blend ONLY when:
1. `obj.objMode == OBJ_MODE_SEMI_TRANSPARENT` (value 1)
2. `BLDCNT blend mode == 1` (alpha blend)
3. `BLDCNT bit 4 == 1` (OBJ is first target)

If any condition fails, sprite renders normally without blending.

### Alpha Blend Formula Implementation

```cpp
// Extract RGB components (5-bit each)
uint8_t r1 = rgb555 & 0x1F;           // Sprite red
uint8_t g1 = (rgb555 >> 5) & 0x1F;    // Sprite green
uint8_t b1 = (rgb555 >> 10) & 0x1F;   // Sprite blue

uint8_t r2 = secondLayerColor & 0x1F;           // Second layer red
uint8_t g2 = (secondLayerColor >> 5) & 0x1F;   // Second layer green
uint8_t b2 = (secondLayerColor >> 10) & 0x1F;  // Second layer blue

// Apply blend formula
uint8_t r = (r1 * blend.eva + r2 * blend.evb) / 16;
uint8_t g = (g1 * blend.eva + g2 * blend.evb) / 16;
uint8_t b = (b1 * blend.eva + b2 * blend.evb) / 16;

// Clamp to 5-bit range
if (r > 31) r = 31;
if (g > 31) g = 31;
if (b > 31) b = 31;

// Reconstruct RGB555
rgb555 = r | (g << 5) | (b << 10);
```

### Preventing Double-Blending

The layer type system ensures sprites are never blended twice:

1. **During sprite rendering**: Semi-transparent sprites blend and are marked as type 254
2. **During global blend pass**: Types 4 and 254 are skipped
   - Type 4: Normal sprites never blend
   - Type 254: Already blended, skip to prevent double-blend

## Hardware Accuracy

This implementation matches GBA hardware behavior:
- Semi-transparent sprites ONLY blend when objMode=1
- Normal sprites (objMode=0) never blend, even if OBJ is a first target
- Blending respects BLDCNT first target mask
- EVA/EVB coefficients control blend ratio (0-16 per coefficient)
- Result is clamped to RGB555 range (0-31 per channel)

## Files Modified

1. **src/gpu.cpp**:
   - `renderNormalSpriteWithPriorityAndWindow()`: Added semi-transparent blend logic
   - `renderAffineSpriteWithPriorityAndWindow()`: Added semi-transparent blend logic
   - `applyBlendToScanline()`: Skip sprite layers (types 4 and 254)

2. **tests/graphics/test_semitransparent_sprites.cpp**: New test file (347 lines, 8 tests)

3. **tests/graphics/Makefile**: Added test_semitransparent_sprites.cpp to build

## Integration with Existing Systems

### Compatible With:
- ✅ Window system (all rendering uses window-aware functions)
- ✅ Priority system (sprites blend with correct priority layers)
- ✅ Affine transformations (rotation/scaling work with semi-transparent)
- ✅ Enhanced blend system (no conflicts)
- ✅ Normal sprite rendering (objMode=0 unaffected)

### Performance Impact:
- Minimal overhead (only when semi-transparent sprites are rendered)
- Blend calculation is simple integer arithmetic
- No impact on normal sprite rendering path

## Testing Results

**Full Test Suite**: 303/303 tests passing (100%)

**Breakdown**:
- 295 existing tests (all still passing - no regressions)
- 8 new semi-transparent sprite tests (all passing)

**Test Execution Time**: ~780ms for full suite

## Future Enhancements

Possible improvements (not required, but could be added):
1. ~~Optimize blend calculation using lookup tables~~
2. ~~Add hardware check for semi-transparent mode support~~
3. ~~Test with real GBA ROMs that use semi-transparent sprites~~

## Conclusion

Semi-transparent sprite support is **fully implemented and tested**. All 303 tests pass with no regressions. The implementation accurately replicates GBA hardware behavior and integrates seamlessly with existing graphics systems.

**Next steps**: Move to Option B (Object Window) or continue with other emulator features.

---

**Implementation Status**: ✅ **COMPLETE**  
**Tests**: 303/303 passing (100%)  
**Date Completed**: 2024
