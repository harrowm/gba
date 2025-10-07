# Day 7 Session 1: Priority & Compositing System - COMPLETE ✅

**Date**: Session completed successfully  
**Status**: 259/259 tests passing (100%)  
**New Tests**: 12 priority system tests  
**Implementation**: ~400 lines across gpu.h and gpu.cpp

## Summary

Successfully implemented the GBA's priority-based compositing system, enabling proper layering of backgrounds and sprites with correct priority ordering. This is a critical feature for game compatibility as it controls which layers appear in front of others.

## Priority System Rules Implemented

### Priority Levels
- **Priority Range**: 0-3 (0 = highest/front, 3 = lowest/back)
- Each BG layer has a priority set in BGxCNT (bits 0-1)
- Each sprite has a priority set in OAM attr2 (bits 10-11)
- Backdrop color (palette 0, color 0) is behind everything

### Layer Ordering (Same Priority)
When multiple layers have the same priority value:
- BG0 > BG1 > BG2 > BG3 > Sprites
- Within sprites: Lower OAM number = higher priority

### Transparency
- Color index 0 is always transparent (reveals lower layers)
- Non-transparent pixels block all layers behind them

## Implementation Details

### Core Architecture
```cpp
void renderScanline(uint16_t scanline) {
    // 1. Create line buffers
    uint16_t lineBuffer[240];      // Final colors
    uint8_t priorityBuffer[240];   // Priority tracking
    
    // 2. Fill with backdrop (priority 255)
    for (int i = 0; i < 240; i++) {
        lineBuffer[i] = backdrop_color;
        priorityBuffer[i] = 255;
    }
    
    // 3. Render back-to-front (priority 3 → 0)
    for (priority = 3; priority >= 0; priority--) {
        // Render BGs at this priority (BG3 → BG0)
        for (bg = 3; bg >= 0; bg--) {
            if (bg_priority == priority) {
                renderBGScanlineWithPriority(...);
            }
        }
        
        // Render sprites at this priority
        renderSpritesWithPriority(priority, ...);
    }
    
    // 4. Copy line buffer to framebuffer
}
```

### Priority Value Calculation
To ensure correct ordering within same priority levels:
```cpp
// Background layers
layerPriority = (bg_priority * 4) + bg_number;
// BG0 at priority 0 = 0
// BG1 at priority 0 = 1
// BG2 at priority 0 = 2
// BG3 at priority 0 = 3

// Sprites (come after BGs)
layerPriority = (sprite_priority * 4) + 4;
// Sprite at priority 0 = 4
// Sprite at priority 1 = 8
```

### Key Functions Added

**gpu.h**:
- `void renderScanline(uint16_t scanline)` - Priority-aware rendering entry point
- `void renderBGScanlineWithPriority()` - Render BG with priority checking
- `void renderSpritesWithPriority()` - Render sprites at specific priority
- `void renderNormalSpriteWithPriority()` - Helper for normal sprites
- `void renderAffineSpriteWithPriority()` - Helper for affine sprites

**gpu.cpp**:
- ~400 lines of implementation
- Proper X/Y coordinate wraparound handling
- Priority buffer management
- Correct transparency handling

## Critical Bug Fixes

### Issue 1: Uninitialized OAM Rendering
**Problem**: All 128 OAM entries were being rendered, including uninitialized ones (attr0=0x0000)  
**Root Cause**: GBA interprets attr0=0x0000 as a valid, visible sprite at (0,0)  
**Solution**: Test helper now disables unused sprites (attr0 bit 9 = OBJ disable)  
**Impact**: Prevented ghost sprites from appearing on screen

### Issue 2: Sprite Coordinate Wraparound
**Problem**: Sprites with Y > 160 or X > 240 were not rendering correctly  
**Root Cause**: Didn't handle GBA's coordinate wraparound (Y: 0-255, X: 0-511)  
**Solution**: Added wraparound logic to match original renderSingleSprite()  
```cpp
// Y wraparound
if (rowInSprite < 0) {
    rowInSprite += 256;
}

// X wraparound
if (screenX >= 511) {
    screenX -= 512;
}
```

## Test Coverage (12 Tests)

✅ **BackdropColor** - Backdrop shows when nothing is drawn  
✅ **SingleBGLayer** - Single BG renders over backdrop  
✅ **SpriteOverBackdrop** - Sprite visibility over backdrop  
✅ **BGPriorityOrdering** - BG0 > BG1 > BG2 > BG3 within same priority  
✅ **BGPriorityLevels** - Priority 0 beats priority 1  
✅ **SpriteOverBGSamePriority** - BG wins over sprite at same priority  
✅ **SpriteOverBGHigherPriority** - Sprite priority 0 over BG priority 1  
✅ **BGOverSpriteHigherPriority** - BG priority 0 over sprite priority 1  
✅ **ComplexLayering** - Multiple priorities mixed correctly  
✅ **TransparencyInPriority** - Transparent pixels reveal lower layers  
✅ **MultipleSpritePriorities** - Overlapping sprites with different priorities  
✅ **AllLayersEnabled** - All 4 BGs + sprites working together  

## Test Results

```
[==========] 259 tests from 16 test suites
[  PASSED  ] 259 tests

Test Breakdown:
- Palette Tests: 17
- Tile Tests: 30
- Background Tests: 92
- Display Control Tests: 8
- OAM Tests: 26
- Sprite Tests: 20
- Affine Sprite Tests: 15
- Priority System Tests: 12 (NEW)
- Other: 39
```

## Performance Characteristics

### Time Complexity
- Per scanline: O(240 × 4 priorities × (4 BGs + N sprites))
- Worst case: O(240 × 16 + 240 × 128) = ~34K operations per scanline
- Full frame (160 scanlines): ~5.4M operations

### Space Complexity
- Line buffers: 240 × 2 bytes (color) + 240 × 1 byte (priority) = 720 bytes per scanline
- Stack allocated (no dynamic memory)

### Optimization Opportunities (Future)
- Skip priority levels with no enabled layers
- Early exit when priority buffer is full
- SIMD for line buffer operations
- Cache BGxCNT register reads

## Integration Status

### Works With
✅ Mode 0 backgrounds (tiled)  
✅ Mode 3 backgrounds (bitmap - bypasses priority system)  
✅ Normal sprites (all sizes, flips)  
✅ Affine sprites (rotation, scaling, double-size)  
✅ 4bpp and 8bpp color modes  
✅ Transparency  

### Not Yet Implemented
❌ Mode 1 backgrounds (tiled + affine)  
❌ Mode 2 backgrounds (affine only)  
❌ Mode 4/5 backgrounds (bitmap)  
❌ Blend effects (alpha, brightness)  
❌ Window effects  
❌ Mosaic effects  

## Code Quality

### Strengths
- Clear documentation with examples
- Consistent naming conventions
- Proper encapsulation in GPU class
- Extensive test coverage
- Handles edge cases (wraparound, transparency)

### Technical Debt
- Line buffers are stack-allocated (720 bytes) - consider moving to GPU member
- Priority calculation formula could be clearer with constants
- Some code duplication between normal/affine sprite rendering

## Next Steps

### Day 7-8 Session 2: Optimization & Polish
- Profile hot paths in priority rendering
- Optimize priority buffer checks
- Add performance benchmarks
- Consider caching frequently accessed values

### Day 7-8 Session 3: Advanced Features
- Implement blend effects (if time permits)
- Window support (if time permits)
- Test with real ROM files

### Day 9-10: Testing & Polish
- Test with commercial ROMs
- Fix any discovered bugs
- Performance tuning
- Documentation updates

## Lessons Learned

1. **Uninitialized Memory Matters**: GBA hardware interprets all OAM as potentially valid sprites. Tests must explicitly disable unused entries.

2. **Coordinate Systems Are Tricky**: GBA uses wraparound coordinates (Y: 0-255, X: 0-511) that can represent negative offscreen positions.

3. **Priority Is More Than Numbers**: Within same priority level, layer type and number determine ordering. Required careful priority value calculation.

4. **Test-Driven Development Works**: Writing tests first revealed the uninitialized OAM bug immediately and guided the implementation.

## References

- GBATEK: Priority and layers (search for "OBJ Priority")
- CowBite Spec: OAM Attributes
- PHASE4_GRAPHICS_PLAN.md: Original priority system requirements
