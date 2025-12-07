# Sprite Layer Bit Layout Comparison: mGBA vs Our Emulator

This document compares the 32-bit pixel format used in `spriteLayer[]` between mGBA and our emulator.

---

## Visual Bit Layout Diagrams

### mGBA 32-bit Pixel Format
```
Bit:  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
      ├──┴──┼──┴──┼──┼──┼──┼──┼──────────────────────────────────────────────────────────────────────┤
      │PRIO │ IDX │BG│RB│T1│T2│                        COLOR (mColor - configurable)                │
      │     │     │  │  │  │OW│                                                                      │
      └─────┴─────┴──┴──┴──┴──┴──────────────────────────────────────────────────────────────────────┘

PRIO = Priority (0-3)           Bits 31-30  FLAG_PRIORITY = 0xC0000000
IDX  = Sprite Index (0-3)       Bits 29-28  FLAG_INDEX = 0x30000000
BG   = Is Background            Bit  27     FLAG_IS_BACKGROUND = 0x08000000
RB   = Reblend                  Bit  26     FLAG_REBLEND = 0x04000000
T1   = First Blend Target       Bit  25     FLAG_TARGET_1 = 0x02000000
T2/OW= Target2 / ObjWin (SHARED) Bit 24     FLAG_TARGET_2 = FLAG_OBJWIN = 0x01000000
COLOR= mColor (see note below)  Bits 23-0

FLAG_UNWRITTEN = 0xFC000000 (bits 31-26 all set = unwritten pixel)
FLAG_ORDER_MASK = 0xF8000000 (priority + index + is_background for ordering)

NOTE: mGBA's color format is configurable at compile time:
  - #ifdef COLOR_16_BIT: Uses 16-bit mColor (bits 0-15 used, same as us)
  - #else: Uses 32-bit mColor expanded from RGB555 for easier math

The GBA hardware ALWAYS uses 15-bit RGB555 (stored in 16-bit words).
mGBA reads RGB555 from VRAM/palette and converts via mColorFrom555():
  - 16-bit mode: color stays as RGB555 
  - 32-bit mode: expanded to 0x00BBGGRR (8 bits per channel)

Either way, bits 24+ are reserved for flags - the color never exceeds 24 bits.
```

### Our Emulator 32-bit Pixel Format
```
Bit:  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
      ├──┴──┼──┼──┼──┼──┼──┴──┼───────┴───────┴───────┼───────┴───────┴───────┴───────┴───────┴──────┤
      │ -- │ST│OW│T2│T1│PRIO │    SPRITE NUMBER      │              COLOR (16-bit RGB555)           │
      │    │  │  │  │  │     │       (0-127)         │                                              │
      └────┴──┴──┴──┴──┴─────┴───────────────────────┴──────────────────────────────────────────────┘

--   = Reserved/Unused          Bits 31-30
ST   = Semi-Transparent         Bit  29     FLAG_SEMI_TRANSPARENT = 0x20000000
OW   = ObjWin                   Bit  28     FLAG_OBJWIN = 0x10000000
T2   = Second Blend Target      Bit  27     FLAG_TARGET_2 = 0x08000000
T1   = First Blend Target       Bit  26     FLAG_TARGET_1 = 0x04000000
PRIO = Priority (0-3)           Bits 25-24  FLAG_PRIORITY = 0x03000000, OFFSET_PRIORITY = 24
NUM  = Sprite Number (0-127)    Bits 23-16  FLAG_ORDER_MASK = 0x00FF0000, OFFSET_ORDER = 16
COLOR= 16-bit RGB555 color      Bits 15-0

FLAG_UNWRITTEN = 0xFFFFFFFF (special sentinel value)
```

---

## Bit Overlap Analysis for Our Emulator

### ✅ NO OVERLAPS DETECTED

| Bits    | Usage              | Mask/Value         |
|---------|--------------------|--------------------|
| 31-30   | Reserved           | (unused)           |
| 29      | Semi-Transparent   | 0x20000000         |
| 28      | ObjWin             | 0x10000000         |
| 27      | Target 2           | 0x08000000         |
| 26      | Target 1           | 0x04000000         |
| 25-24   | Priority (0-3)     | 0x03000000         |
| 23-16   | Sprite Number      | 0x00FF0000         |
| 15-0    | RGB555 Color       | 0x0000FFFF         |

All flags use distinct, non-overlapping bits. ✅

---

## Flag-by-Flag Comparison

| Flag Purpose         | mGBA                      | Our Emulator              | Status     |
|----------------------|---------------------------|---------------------------|------------|
| Priority             | 0xC0000000 (bits 31-30)   | 0x03000000 (bits 25-24)   | ✅ Have    |
| Sprite ordering      | 0x30000000 (bits 29-28)   | 0x00FF0000 (bits 23-16)   | ✅ Have    |
| Is Background        | 0x08000000 (bit 27)       | N/A                       | ❌ Missing |
| Reblend              | 0x04000000 (bit 26)       | N/A                       | ❌ Missing |
| First Blend Target   | 0x02000000 (bit 25)       | 0x04000000 (bit 26)       | ✅ Have    |
| Second Blend Target  | 0x01000000 (bit 24)       | 0x08000000 (bit 27)       | ✅ Have    |
| ObjWin               | 0x01000000 (bit 24)*      | 0x10000000 (bit 28)       | ✅ Have    |
| Semi-Transparent     | N/A (uses FLAG_REBLEND)   | 0x20000000 (bit 29)       | ⚠️ Different approach |
| Unwritten sentinel   | 0xFC000000 (bits 31-26)   | 0xFFFFFFFF (all bits)     | ✅ Have    |

*Note: mGBA shares bit 24 for FLAG_TARGET_2 and FLAG_OBJWIN (mutually exclusive contexts)

---

## Missing Flags Analysis

### 1. FLAG_IS_BACKGROUND (mGBA bit 27)
**Purpose in mGBA**: Distinguishes background pixels from sprite pixels in the composite buffer.
**Our approach**: We use separate buffers (`lineBuffer` for BG, `spriteLayer` for sprites) so we don't need this flag in the sprite layer.
**Impact**: None - our architecture handles this differently.

### 2. FLAG_REBLEND (mGBA bit 26) ⚠️ CRITICAL
**Purpose in mGBA**: Marks pixels that need special re-blending in postprocessing. Set when:
- Semi-transparent sprite (mode=1) has a valid 2nd target layer available
- Used to trigger alpha blending with backdrop in `GBAVideoSoftwareRendererPostprocessBuffer()`

**mGBA logic** (from `software-obj.c` lines 179-190):
```c
if (GBAObjAttributesAGetMode(sprite->a) == OBJ_MODE_SEMITRANSPARENT || 
    (renderer->target1Obj && renderer->blendEffect == BLEND_ALPHA) || objwinSlowPath) {
    int target2 = renderer->target2Bd;
    target2 |= renderer->bg[0].target2 && renderer->bg[0].enabled;
    // ... check other BGs ...
    if (target2) {
        renderer->forceTarget1 = true;
        flags |= FLAG_REBLEND;
    } else {
        flags &= ~FLAG_TARGET_1;  // No valid target2, don't blend
    }
}
```

**Our approach**: We use `FLAG_SEMI_TRANSPARENT` (bit 29) to mark semi-transparent sprites, then check in postprocessing whether to always blend.

**Impact**: May cause different behavior - need to verify our semi-transparent blending matches mGBA.

---

## Architectural Differences

### Priority/Ordering Comparison
| Aspect | mGBA | Our Emulator |
|--------|------|--------------|
| Priority comparison | Uses `color >= current` with flags in high bits | Uses separate `priorityBuffer[]` |
| Sprite order | 2 bits (0-3) in FLAG_INDEX | 8 bits (0-127) for full sprite number |

mGBA encodes priority in the highest bits so a simple integer comparison (`>=`) determines which pixel wins. We use separate buffers for priority tracking.

### Semi-Transparent Sprite Handling
| Aspect | mGBA | Our Emulator |
|--------|------|--------------|
| Flag used | FLAG_TARGET_1 + FLAG_REBLEND | FLAG_SEMI_TRANSPARENT |
| Blend decision | Checks if valid target2 exists | Always blends (per GBATEK) |
| Backdrop blend | Separate postprocess pass | Inline during sprite composite |

---

## Recommendations

1. **Consider adding FLAG_REBLEND** (use bit 30 or 31) if we need mGBA's exact postprocessing behavior
2. **Verify semi-transparent blend logic** matches mGBA's behavior for edge cases
3. **Our bit layout is clean** - no overlaps, all necessary flags present for basic operation

---

## Source References

- mGBA: `include/mgba/internal/gba/renderers/video-software.h` (lines 47-63)
- mGBA: `src/gba/renderers/software-obj.c` (lines 149-205)
- mGBA: `src/gba/renderers/software-private.h` (lines 41-63)
- Our code: `include/gpu.h` (lines 345-360)
