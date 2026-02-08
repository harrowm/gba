#ifndef GPU_H
#define GPU_H

#include "memory.h"
#include <cstdint>
#include <functional>

// GBA Video Timing Constants
constexpr uint32_t CYCLES_PER_SCANLINE = 1232;  // 308 dots × 4 cycles per dot (GBA hardware spec)
constexpr uint32_t CYCLES_HDRAW = 960;          // 240 pixels * 4 cycles
constexpr uint32_t CYCLES_HBLANK = 272;         // 68 dots × 4 cycles
constexpr uint32_t SCANLINES_VISIBLE = 160;
constexpr uint32_t SCANLINES_VBLANK = 68;
constexpr uint32_t SCANLINES_TOTAL = 228;
constexpr uint32_t CYCLES_PER_FRAME = CYCLES_PER_SCANLINE * SCANLINES_TOTAL; // 280,896

// I/O Register addresses
constexpr uint32_t REG_DISPCNT = 0x04000000;
constexpr uint32_t REG_DISPSTAT = 0x04000004;
constexpr uint32_t REG_VCOUNT = 0x04000006;

// Background control registers
constexpr uint32_t REG_BG0CNT = 0x04000008;
constexpr uint32_t REG_BG1CNT = 0x0400000A;
constexpr uint32_t REG_BG2CNT = 0x0400000C;
constexpr uint32_t REG_BG3CNT = 0x0400000E;

// Background scroll registers
constexpr uint32_t REG_BG0HOFS = 0x04000010;
constexpr uint32_t REG_BG0VOFS = 0x04000012;
constexpr uint32_t REG_BG1HOFS = 0x04000014;
constexpr uint32_t REG_BG1VOFS = 0x04000016;
constexpr uint32_t REG_BG2HOFS = 0x04000018;
constexpr uint32_t REG_BG2VOFS = 0x0400001A;
constexpr uint32_t REG_BG3HOFS = 0x0400001C;
constexpr uint32_t REG_BG3VOFS = 0x0400001E;

// BG2 affine parameters (rotation/scaling)
constexpr uint32_t REG_BG2PA = 0x04000020;  // dx/dx (pa) - 8.8 fixed point
constexpr uint32_t REG_BG2PB = 0x04000022;  // dmx/dy (pb) - 8.8 fixed point
constexpr uint32_t REG_BG2PC = 0x04000024;  // dy/dx (pc) - 8.8 fixed point
constexpr uint32_t REG_BG2PD = 0x04000026;  // dmy/dy (pd) - 8.8 fixed point
constexpr uint32_t REG_BG2X = 0x04000028;   // Reference point X - 28-bit signed (19.8 fixed)
constexpr uint32_t REG_BG2Y = 0x0400002C;   // Reference point Y - 28-bit signed (19.8 fixed)

// BG3 affine parameters (rotation/scaling)
constexpr uint32_t REG_BG3PA = 0x04000030;  // dx/dx (pa) - 8.8 fixed point
constexpr uint32_t REG_BG3PB = 0x04000032;  // dmx/dy (pb) - 8.8 fixed point
constexpr uint32_t REG_BG3PC = 0x04000034;  // dy/dx (pc) - 8.8 fixed point
constexpr uint32_t REG_BG3PD = 0x04000036;  // dmy/dy (pd) - 8.8 fixed point
constexpr uint32_t REG_BG3X = 0x04000038;   // Reference point X - 28-bit signed (19.8 fixed)
constexpr uint32_t REG_BG3Y = 0x0400003C;   // Reference point Y - 28-bit signed (19.8 fixed)

// Window registers
constexpr uint32_t REG_WIN0H = 0x04000040;
constexpr uint32_t REG_WIN1H = 0x04000042;
constexpr uint32_t REG_WIN0V = 0x04000044;
constexpr uint32_t REG_WIN1V = 0x04000046;
constexpr uint32_t REG_WININ = 0x04000048;
constexpr uint32_t REG_WINOUT = 0x0400004A;

// Blend registers
constexpr uint32_t REG_BLDCNT = 0x04000050;
constexpr uint32_t REG_BLDALPHA = 0x04000052;
constexpr uint32_t REG_BLDY = 0x04000054;

// VRAM addresses
constexpr uint32_t VRAM_BASE = 0x06000000;
constexpr uint32_t VRAM_SIZE = 0x18000;  // 96KB

// DISPCNT bits
constexpr uint16_t DISPCNT_MODE_MASK = 0x0007;
constexpr uint16_t DISPCNT_MODE_3 = 0x0003;
constexpr uint16_t DISPCNT_FRAME_SELECT = 0x0010;  // Bit 4: Frame select (BG modes 4,5)
constexpr uint16_t DISPCNT_OAM_HBLANK = 0x0020;    // Bit 5: Allow OAM access in HBlank
constexpr uint16_t DISPCNT_OBJ_1D_MAP = 0x0040;    // Bit 6: OBJ char mapping (0=2D, 1=1D)
constexpr uint16_t DISPCNT_FORCED_BLANK = 0x0080;  // Bit 7: Forced blank
constexpr uint16_t DISPCNT_BG0_ENABLE = 0x0100;    // Bit 8: BG0 enable
constexpr uint16_t DISPCNT_BG1_ENABLE = 0x0200;    // Bit 9: BG1 enable
constexpr uint16_t DISPCNT_BG2_ENABLE = 0x0400;    // Bit 10: BG2 enable
constexpr uint16_t DISPCNT_BG3_ENABLE = 0x0800;    // Bit 11: BG3 enable
constexpr uint16_t DISPCNT_OBJ_ENABLE = 0x1000;    // Bit 12: OBJ enable
constexpr uint16_t DISPCNT_WIN0_ENABLE = 0x2000;   // Bit 13: Window 0 enable
constexpr uint16_t DISPCNT_WIN1_ENABLE = 0x4000;   // Bit 14: Window 1 enable
constexpr uint16_t DISPCNT_WINOBJ_ENABLE = 0x8000; // Bit 15: OBJ Window enable

// Structure to hold parsed DISPCNT values
struct DisplayControl {
    uint8_t videoMode;      // Bits 0-2: Video mode (0-5)
    bool frameSelect;       // Bit 4: Frame buffer select (modes 4-5)
    bool oamHBlankAccess;   // Bit 5: Allow OAM access during HBlank
    bool obj1DMapping;      // Bit 6: OBJ character mapping (0=2D, 1=1D)
    bool forcedBlank;       // Bit 7: Forced blank (white screen)
    bool bg0Enable;         // Bit 8: BG0 display
    bool bg1Enable;         // Bit 9: BG1 display
    bool bg2Enable;         // Bit 10: BG2 display
    bool bg3Enable;         // Bit 11: BG3 display
    bool objEnable;         // Bit 12: OBJ display
    bool win0Enable;        // Bit 13: Window 0 display
    bool win1Enable;        // Bit 14: Window 1 display
    bool winObjEnable;      // Bit 15: OBJ Window display
};

// BGxCNT bits
constexpr uint16_t BGCNT_PRIORITY_MASK = 0x0003;       // Bits 0-1: Priority (0-3)
constexpr uint16_t BGCNT_CHAR_BASE_MASK = 0x000C;      // Bits 2-3: Character base block (0-3)
constexpr uint16_t BGCNT_MOSAIC = 0x0040;              // Bit 6: Mosaic enable
constexpr uint16_t BGCNT_PALETTE_MODE = 0x0080;        // Bit 7: Palette mode (0=16/16, 1=256/1)
constexpr uint16_t BGCNT_SCREEN_BASE_MASK = 0x1F00;    // Bits 8-12: Screen base block (0-31)
constexpr uint16_t BGCNT_SCREEN_SIZE_MASK = 0xC000;    // Bits 14-15: Screen size

// Screen size constants (in tiles)
constexpr int BG_SCREEN_SIZE_256x256 = 0;   // 32x32 tiles
constexpr int BG_SCREEN_SIZE_512x256 = 1;   // 64x32 tiles
constexpr int BG_SCREEN_SIZE_256x512 = 2;   // 32x64 tiles
constexpr int BG_SCREEN_SIZE_512x512 = 3;   // 64x64 tiles

// Structure to hold parsed BGxCNT values
struct BGConfig {
    uint8_t priority;           // Bits 0-1: Priority (0-3, 0=highest)
    uint8_t charBaseBlock;      // Bits 2-3: Character base block (0-3)
    bool mosaicEnable;          // Bit 6: Mosaic effect enable
    bool paletteMode;           // Bit 7: 0=16/16 (4bpp), 1=256/1 (8bpp)
    uint8_t screenBaseBlock;    // Bits 8-12: Screen base block (0-31)
    uint8_t screenSize;         // Bits 14-15: Screen size (0-3)
    
    // Computed values for convenience
    uint32_t charBaseAddr;      // Actual VRAM address for character data
    uint32_t screenBaseAddr;    // Actual VRAM address for screen data
    int screenWidthTiles;       // Screen width in tiles
    int screenHeightTiles;      // Screen height in tiles
    int screenWidthPixels;      // Screen width in pixels
    int screenHeightPixels;     // Screen height in pixels
};

// Screen Entry (Tile Map Entry) bits - each entry is 16 bits
constexpr uint16_t SE_TILE_NUM_MASK = 0x03FF;      // Bits 0-9: Tile number (0-1023)
constexpr uint16_t SE_HFLIP = 0x0400;              // Bit 10: Horizontal flip
constexpr uint16_t SE_VFLIP = 0x0800;              // Bit 11: Vertical flip
constexpr uint16_t SE_PALETTE_MASK = 0xF000;       // Bits 12-15: Palette number (4bpp only)

// Structure to hold parsed Screen Entry (tile map entry)
struct ScreenEntry {
    uint16_t tileNumber;        // Bits 0-9: Which tile to use (0-1023)
    bool hFlip;                 // Bit 10: Flip tile horizontally
    bool vFlip;                 // Bit 11: Flip tile vertically
    uint8_t paletteNum;         // Bits 12-15: Palette bank (4bpp only, 0-15)
};

// Structure to hold BG scroll offsets
struct BGScroll {
    uint16_t hofs;              // Horizontal offset (0-511 for normal BGs)
    uint16_t vofs;              // Vertical offset (0-511 for normal BGs)
};

// Structure to hold blend control settings
struct BlendControl {
    uint8_t firstTargets;       // Bitmask: which layers are 1st targets (bits 0-5)
    uint8_t mode;               // Blend mode (0-3)
    uint8_t secondTargets;      // Bitmask: which layers are 2nd targets (bits 0-5)
    uint8_t eva;                // Alpha coefficient for 1st target (0-16)
    uint8_t evb;                // Alpha coefficient for 2nd target (0-16)
    uint8_t evy;                // Brightness coefficient (0-16)
};

// Structure to hold window dimensions
struct Window {
    uint8_t left;               // Left edge (0-240)
    uint8_t right;              // Right edge (0-240)
    uint8_t top;                // Top edge (0-160)
    uint8_t bottom;             // Bottom edge (0-160)
    uint8_t control;            // Control flags (which layers visible)
    bool enabled;               // Whether window is enabled in DISPCNT
};

// Structure to hold window control settings
struct WindowControl {
    Window win0;                // Window 0
    Window win1;                // Window 1
    uint8_t winOut;             // Control for outside windows
    uint8_t winObj;             // Control for OBJ window
};

// DISPSTAT bits
constexpr uint16_t DISPSTAT_VBLANK = 0x0001;
constexpr uint16_t DISPSTAT_HBLANK = 0x0002;
constexpr uint16_t DISPSTAT_VCOUNT_MATCH = 0x0004;
constexpr uint16_t DISPSTAT_VBLANK_IRQ_ENABLE = 0x0008;
constexpr uint16_t DISPSTAT_HBLANK_IRQ_ENABLE = 0x0010;
constexpr uint16_t DISPSTAT_VCOUNT_IRQ_ENABLE = 0x0020;

// BLDCNT (Blend Control) bits
constexpr uint16_t BLDCNT_BG0_1ST = 0x0001;    // Bit 0: BG0 1st target
constexpr uint16_t BLDCNT_BG1_1ST = 0x0002;    // Bit 1: BG1 1st target
constexpr uint16_t BLDCNT_BG2_1ST = 0x0004;    // Bit 2: BG2 1st target
constexpr uint16_t BLDCNT_BG3_1ST = 0x0008;    // Bit 3: BG3 1st target
constexpr uint16_t BLDCNT_OBJ_1ST = 0x0010;    // Bit 4: OBJ 1st target
constexpr uint16_t BLDCNT_BD_1ST = 0x0020;     // Bit 5: Backdrop 1st target
constexpr uint16_t BLDCNT_MODE_MASK = 0x00C0;  // Bits 6-7: Blend mode
constexpr uint16_t BLDCNT_BG0_2ND = 0x0100;    // Bit 8: BG0 2nd target
constexpr uint16_t BLDCNT_BG1_2ND = 0x0200;    // Bit 9: BG1 2nd target
constexpr uint16_t BLDCNT_BG2_2ND = 0x0400;    // Bit 10: BG2 2nd target
constexpr uint16_t BLDCNT_BG3_2ND = 0x0800;    // Bit 11: BG3 2nd target
constexpr uint16_t BLDCNT_OBJ_2ND = 0x1000;    // Bit 12: OBJ 2nd target
constexpr uint16_t BLDCNT_BD_2ND = 0x2000;     // Bit 13: Backdrop 2nd target

// Blend modes
constexpr uint8_t BLEND_MODE_OFF = 0;          // No blending
constexpr uint8_t BLEND_MODE_ALPHA = 1;        // Alpha blend (1st + 2nd)
constexpr uint8_t BLEND_MODE_BRIGHTEN = 2;     // Brighten 1st target
constexpr uint8_t BLEND_MODE_DARKEN = 3;       // Darken 1st target

// WININ/WINOUT bits (8 bits per window)
constexpr uint8_t WIN_BG0_ENABLE = 0x01;       // Bit 0: Enable BG0
constexpr uint8_t WIN_BG1_ENABLE = 0x02;       // Bit 1: Enable BG1
constexpr uint8_t WIN_BG2_ENABLE = 0x04;       // Bit 2: Enable BG2
constexpr uint8_t WIN_BG3_ENABLE = 0x08;       // Bit 3: Enable BG3
constexpr uint8_t WIN_OBJ_ENABLE = 0x10;       // Bit 4: Enable OBJ
constexpr uint8_t WIN_BLEND_ENABLE = 0x20;     // Bit 5: Enable blend

// Palette RAM addresses
constexpr uint32_t PALETTE_BG_START = 0x05000000;
constexpr uint32_t PALETTE_OBJ_START = 0x05000200;
constexpr uint32_t PALETTE_SIZE = 512;  // 512 bytes for BG, 512 for OBJ

// OAM (Object Attribute Memory) addresses
constexpr uint32_t OAM_BASE = 0x07000000;
constexpr uint32_t OAM_SIZE = 0x400;        // 1KB (128 objects × 8 bytes)
constexpr uint32_t OBJ_TILES_BASE = 0x06010000;  // Sprite tiles start at 64KB into VRAM

// OAM Attribute 0 bits (Y position, size, mode)
constexpr uint16_t OBJ_ATTR0_Y_MASK = 0x00FF;           // Bits 0-7: Y coordinate (0-255)
constexpr uint16_t OBJ_ATTR0_ROT_SCALE_FLAG = 0x0100;   // Bit 8: Rotation/Scaling flag
constexpr uint16_t OBJ_ATTR0_DOUBLE_SIZE = 0x0200;      // Bit 9: Double size (when rot/scale on)
constexpr uint16_t OBJ_ATTR0_OBJ_DISABLE = 0x0200;      // Bit 9: OBJ disable (when rot/scale off)
constexpr uint16_t OBJ_ATTR0_MODE_MASK = 0x0C00;        // Bits 10-11: OBJ mode
constexpr uint16_t OBJ_ATTR0_MOSAIC = 0x1000;           // Bit 12: Mosaic
constexpr uint16_t OBJ_ATTR0_PALETTE_MODE = 0x2000;     // Bit 13: 0=16/16 (4bpp), 1=256/1 (8bpp)
constexpr uint16_t OBJ_ATTR0_SHAPE_MASK = 0xC000;       // Bits 14-15: OBJ shape

// OAM Attribute 1 bits (X position, size, flip)
constexpr uint16_t OBJ_ATTR1_X_MASK = 0x01FF;           // Bits 0-8: X coordinate (0-511)
constexpr uint16_t OBJ_ATTR1_ROT_PARAM_MASK = 0x3E00;   // Bits 9-13: Rotation parameter (when rot/scale on)
constexpr uint16_t OBJ_ATTR1_HFLIP = 0x1000;            // Bit 12: H flip (when rot/scale off)
constexpr uint16_t OBJ_ATTR1_VFLIP = 0x2000;            // Bit 13: V flip (when rot/scale off)
constexpr uint16_t OBJ_ATTR1_SIZE_MASK = 0xC000;        // Bits 14-15: OBJ size

// OAM Attribute 2 bits (tile, priority, palette)
constexpr uint16_t OBJ_ATTR2_TILE_MASK = 0x03FF;        // Bits 0-9: Tile number (0-1023)
constexpr uint16_t OBJ_ATTR2_PRIORITY_MASK = 0x0C00;    // Bits 10-11: Priority (0-3)
constexpr uint16_t OBJ_ATTR2_PALETTE_MASK = 0xF000;     // Bits 12-15: Palette number (4bpp only)

// OBJ modes (Attribute 0, bits 10-11)
constexpr uint8_t OBJ_MODE_NORMAL = 0;      // Normal rendering
constexpr uint8_t OBJ_MODE_SEMI_TRANSPARENT = 1;  // Alpha blending
constexpr uint8_t OBJ_MODE_OBJ_WINDOW = 2;  // OBJ window
constexpr uint8_t OBJ_MODE_PROHIBITED = 3;  // Prohibited (don't render)

// OBJ shapes (Attribute 0, bits 14-15)
constexpr uint8_t OBJ_SHAPE_SQUARE = 0;     // Square (8×8, 16×16, 32×32, 64×64)
constexpr uint8_t OBJ_SHAPE_HORIZONTAL = 1; // Wide (16×8, 32×8, 32×16, 64×32)
constexpr uint8_t OBJ_SHAPE_VERTICAL = 2;   // Tall (8×16, 8×32, 16×32, 32×64)
constexpr uint8_t OBJ_SHAPE_PROHIBITED = 3; // Prohibited

// Structure to hold parsed OAM attributes
struct OBJAttributes {
    // Attribute 0
    uint8_t y;                  // Y coordinate (0-255)
    bool rotScaleFlag;          // Rotation/scaling enabled
    bool doubleSize;            // Double size when rotating (or disabled when not rotating)
    uint8_t objMode;            // 0=Normal, 1=Semi-transparent, 2=OBJ Window, 3=Prohibited
    bool mosaicEnable;          // Mosaic effect
    bool paletteMode;           // 0=16/16 (4bpp), 1=256/1 (8bpp)
    uint8_t shape;              // 0=Square, 1=Horizontal, 2=Vertical, 3=Prohibited
    
    // Attribute 1
    uint16_t x;                 // X coordinate (0-511, treated as signed -256 to 255)
    bool hFlip;                 // Horizontal flip (only when rotScaleFlag=false)
    bool vFlip;                 // Vertical flip (only when rotScaleFlag=false)
    uint8_t rotScaleParam;      // Rotation/scaling parameter select (only when rotScaleFlag=true)
    uint8_t size;               // Size code (0-3), combined with shape
    
    // Attribute 2
    uint16_t tileNumber;        // Base tile number (0-1023)
    uint8_t priority;           // Priority (0-3, 0=highest)
    uint8_t paletteNum;         // Palette bank (4bpp only, 0-15)
    
    // Computed values
    int width;                  // Sprite width in pixels
    int height;                 // Sprite height in pixels
    bool visible;               // Whether sprite should be rendered
};

// Affine transformation parameters
// Stored in OAM at offsets 0x06, 0x0E, 0x16, 0x1E... (every 32 bytes)
// Fixed-point 8.8 format (1 sign bit + 7 integer bits + 8 fractional bits)
struct AffineParams {
    int16_t pa;  // [0][0] - Horizontal scaling / rotation
    int16_t pb;  // [0][1] - Horizontal rotation / shearing
    int16_t pc;  // [1][0] - Vertical rotation / shearing
    int16_t pd;  // [1][1] - Vertical scaling / rotation
};

// Affine transformation parameters for backgrounds (BG2/BG3 in Mode 1/2)
// Registers: BG2PA-BG2Y (0x04000020-0x0400002F) and BG3PA-BG3Y (0x04000030-0x0400003F)
struct AffineBackgroundParams {
    int16_t pa, pb, pc, pd;  // 8.8 fixed-point transformation matrix
    int32_t refX, refY;      // 19.8 fixed-point reference point (28-bit signed)
    // Internal rendering state (updated per-scanline)
    int32_t currentX, currentY;  // Current texture coordinates for scanline start
};

class Scheduler;  // Forward declaration

class GPU {
private:
    Memory& memory;
    uint16_t currentScanline;
    bool inVBlank;
    bool inHBlank;
    
    // Framebuffer for tiled modes (Mode 0-2)
    // In Mode 3+, we use VRAM directly as framebuffer
    uint16_t tiledFramebuffer[240 * 160];
    
    // Sprite ordering buffer (stores sprite index and priority)
    // Bits 31-24: sprite number (0-127)
    // Bits 23-16: priority (0-3)
    // Bit 0: FLAG_WRITTEN
    uint32_t spriteOrderLayer[240];
    
    // Sprite layer buffer for two-pass rendering (mgba approach)
    // Lower 24 bits: RGB555 color or flags
    // Upper 8 bits: priority, objMode, sprite order
    // Format matches mgba: color | (priority << 24) | (objNum << 16) | flags
    uint32_t spriteLayer[240];
    
    // OBJ Window mask buffer - true if pixel is inside OBJ Window region
    // Built from OBJ_MODE_OBJ_WINDOW sprites during preprocessSprites
    bool objWindowMask[240];
    
    // Sprite layer flag constants (mgba compatible)
    // IMPORTANT: Flags must NOT overlap with color bits 0-15 (RGB555 color)!
    // Layout:
    //   Bits 0-15: RGB555 color (or 0xFFFF for transparent)
    //   Bits 16-23: sprite number (0-127)
    //   Bits 24-25: priority (0-3)
    //   Bits 26-29: blend flags (target1, target2, objwin, semi-transparent)
    //   Bits 30-31: reserved
    static constexpr uint32_t FLAG_UNWRITTEN = 0xFFFFFFFF;
    static constexpr uint32_t FLAG_ORDER_MASK = 0x00FF0000;  // Bits 16-23: sprite number
    static constexpr uint32_t FLAG_PRIORITY = 0x03000000;     // Bits 24-25: priority (only need 2 bits)
    static constexpr uint32_t FLAG_TARGET_1 = 0x04000000;     // Bit 26: Sprite is first blend target
    static constexpr uint32_t FLAG_TARGET_2 = 0x08000000;     // Bit 27: Sprite is second blend target
    static constexpr uint32_t FLAG_OBJWIN = 0x10000000;       // Bit 28: OBJ window mode
    static constexpr uint32_t FLAG_SEMI_TRANSPARENT = 0x20000000; // Bit 29: Semi-transparent sprite (mode=1)
    static constexpr int OFFSET_PRIORITY = 24;
    static constexpr int OFFSET_ORDER = 16;
    
    // Callbacks for interrupts
    std::function<void()> vblankCallback;
    std::function<void()> hblankCallback;

public:
    explicit GPU(Memory& mem);
    
    // Setup video timing with scheduler
    void setupTiming(Scheduler* scheduler);
    void scheduleScanline(Scheduler* scheduler);  // Helper for scanline scheduling
    
    // Rendering
    void renderScanline();
    void renderScanline(uint16_t scanline);  // New priority-aware renderer
    static void renderMode3Scanline(uint16_t scanline);
    void renderMode4Scanline(uint16_t scanline);  // Mode 4: 8bpp indexed bitmap
    void renderMode2Scanline(uint16_t scanline);  // Mode 2: 2 affine backgrounds (BG2, BG3)
    
    // Priority-aware rendering (new)
    void renderBGScanlineWithPriority(int bgNum, uint16_t scanline, uint16_t* lineBuffer, uint8_t* priorityBuffer);
    
    // Priority + window rendering (Session 3 integration)
    void renderBGScanlineWithPriorityAndWindow(int bgNum, uint16_t scanline, uint16_t* lineBuffer, 
                                                uint8_t* priorityBuffer, uint8_t* layerTypeBuffer,
                                                uint16_t* secondLayerBuffer, uint8_t* secondLayerTypeBuffer,
                                                const WindowControl& winCtrl);
    // Two-pass sprite rendering (mgba approach)
    void preprocessSprites(uint16_t scanline, bool mapping1D, const WindowControl& winCtrl);
    void renderObjWindowToMask(const OBJAttributes& obj, int objNum, uint16_t scanline, bool mapping1D);
    void postprocessSprites(uint8_t priority, uint16_t scanline, uint16_t* lineBuffer,
                           uint8_t* priorityBuffer, uint8_t* layerTypeBuffer,
                           uint16_t* secondLayerBuffer, uint8_t* secondLayerTypeBuffer,
                           const WindowControl& winCtrl);
    
    void applyBlendToScanline(uint16_t* lineBuffer, const uint8_t* layerTypeBuffer, 
                              const uint16_t* secondLayerBuffer, const uint8_t* secondLayerTypeBuffer,
                              uint16_t scanline, const BlendControl& blend, const WindowControl& winCtrl);
    
    // Helper rendering functions
    void clearScanlineToBackdrop(uint16_t scanline);
    void renderBlankScanline(uint16_t scanline);
    
    // VCOUNT handling
    uint16_t getCurrentScanline() const { return currentScanline; }
    
    // Interrupt callbacks
    void setVBlankCallback(std::function<void()> callback) { vblankCallback = callback; }
    void setHBlankCallback(std::function<void()> callback) { hblankCallback = callback; }
    
    // Get frame buffer
    // Mode 3+ (bitmap modes): returns VRAM directly
    // Mode 0-2 (tiled modes): returns internal tiled framebuffer
    uint16_t* getFrameBuffer();
    
    // Palette functions
    uint16_t readBGPaletteRaw(int paletteNum, int colorIndex);
    uint16_t readOBJPaletteRaw(int paletteNum, int colorIndex);
    uint32_t getOBJColor(int paletteNum, int colorIndex);
    
    // Hot-path inline color conversion functions
    static inline uint32_t convertRGB555toARGB8888(uint16_t rgb555) {
        // RGB555 format: 0BBBBBGGGGGRRRRR (5 bits per channel)
        uint8_t r5 = (rgb555 & 0x001Fu);
        uint8_t g5 = (rgb555 & 0x03E0u) >> 5;
        uint8_t b5 = (rgb555 & 0x7C00u) >> 10;
        
        // Convert to 8-bit: (value << 3) | (value >> 2)
        uint8_t r8 = (r5 << 3) | (r5 >> 2);
        uint8_t g8 = (g5 << 3) | (g5 >> 2);
        uint8_t b8 = (b5 << 3) | (b5 >> 2);
        
        // Return ARGB8888 format (0xAARRGGBB)
        return 0xFF000000u | (r8 << 16) | (g8 << 8) | b8;
    }
    
    inline uint32_t getBGColor(int paletteNum, int colorIndex) {
        // Read raw RGB555 color from BG palette and convert to ARGB8888
        uint16_t rgb555 = readBGPaletteRaw(paletteNum, colorIndex);
        return convertRGB555toARGB8888(rgb555);
    }
    
    // Hot-path inline functions for tile pixel access
    inline uint8_t getTilePixel4bpp(uint32_t tileAddr, int pixelX, int pixelY) {
        // Get a single pixel from a 4bpp tile (pixelX, pixelY in range [0, 7])
        if (pixelX < 0 || pixelX >= 8 || pixelY < 0 || pixelY >= 8) {
            return 0;
        }
        
        const uint8_t* vram = memory.getVRAM();
        uint32_t offset = tileAddr - 0x06000000;
        
        // Calculate byte offset (2 pixels per byte)
        int pixelIndex = pixelY * 8 + pixelX;
        int byteOffset = pixelIndex / 2;
        int pixelInByte = pixelIndex % 2;  // 0 = low nibble, 1 = high nibble
        
        uint8_t byte = vram[offset + byteOffset];
        return (pixelInByte == 0) ? (byte & 0x0F) : ((byte >> 4) & 0x0F);
    }
    
    inline uint8_t getTilePixel8bpp(uint32_t tileAddr, int pixelX, int pixelY) {
        // Get a single pixel from an 8bpp tile (pixelX, pixelY in range [0, 7])
        if (pixelX < 0 || pixelX >= 8 || pixelY < 0 || pixelY >= 8) {
            return 0;
        }
        
        const uint8_t* vram = memory.getVRAM();
        uint32_t offset = tileAddr - 0x06000000;
        
        // Calculate byte offset (1 pixel per byte)
        int pixelIndex = pixelY * 8 + pixelX;
        return vram[offset + pixelIndex];
    }
    
    // DISPCNT register parsing
    static DisplayControl parseDISPCNT(uint16_t dispcnt);
    DisplayControl readDISPCNT();  // Read and parse from memory
    
    // Helper functions to check specific DISPCNT bits
    bool isBGEnabled(int bgNum);     // Check if BG0-3 is enabled
    bool isForcedBlank();             // Check if display is blanked
    
    // BGxCNT register parsing
    static BGConfig parseBGCNT(uint16_t bgcnt);
    BGConfig readBGCNT(int bgNum);    // Read and parse BGxCNT (bgNum: 0-3)
    
    // Helper to get screen dimensions from size code
    static void getScreenDimensions(uint8_t sizeCode, int& widthTiles, int& heightTiles);
    
    // Tile map (screen entry) functions
    static ScreenEntry parseScreenEntry(uint16_t entry);
    ScreenEntry readScreenEntry(const BGConfig& bgConfig, int tileX, int tileY);
    uint16_t readScreenEntryRaw(const BGConfig& bgConfig, int tileX, int tileY);
    
    // Get tile address from screen entry
    static uint32_t getTileAddress(const BGConfig& bgConfig, const ScreenEntry& entry);
    
    // Helper to get screen block offset for large screens
    static uint32_t getScreenBlockOffset(const BGConfig& bgConfig, int tileX, int tileY);
    
    // Scroll functions
    BGScroll readBGScroll(int bgNum);                           // Read scroll from registers
    static void applyScroll(const BGConfig& bgConfig, const BGScroll& scroll, 
                     int screenX, int screenY, int& bgX, int& bgY);  // Apply scroll to coords
    static void getTileCoords(int pixelX, int pixelY, int& tileX, int& tileY, 
                       int& pixelInTileX, int& pixelInTileY);       // Convert pixel to tile coords
    
    // OAM (sprite) functions
    static OBJAttributes parseOBJAttributes(uint16_t attr0, uint16_t attr1, uint16_t attr2);
    OBJAttributes readOBJAttributes(int objNum);                // Read OBJ attributes (objNum: 0-127)
    static void getOBJDimensions(uint8_t shape, uint8_t size, int& width, int& height);
    
    // Sprite rendering functions
    static uint32_t getOBJTileAddress(const OBJAttributes& obj, int tileX, int tileY, bool mapping1D);
    static bool isSpriteOnScanline(const OBJAttributes& obj, uint16_t scanline);
    AffineParams readAffineParams(uint8_t paramIndex);          // Read affine params (0-31) from OAM
    static void applyAffineTransform(const AffineParams& params, 
                               int screenX, int screenY,        // Screen coords relative to sprite center
                               int& textureX, int& textureY);   // Output: texture coords
    
    // Affine background functions (Mode 1/2)
    AffineBackgroundParams readAffineBGParams(int bgNum);       // Read BG2/BG3 affine parameters
    static void getAffineBGDimensions(uint8_t sizeCode, int& widthPixels, int& heightPixels, int& widthTiles);
    void renderAffineBGScanlineWithPriority(int bgNum, uint16_t scanline, 
                                            uint16_t* lineBuffer, uint8_t* priorityBuffer);
    void renderAffineBGScanlineWithPriorityAndWindow(int bgNum, uint16_t scanline, 
                                                     uint16_t* lineBuffer, uint8_t* priorityBuffer,
                                                     uint8_t* layerTypeBuffer, 
                                                     uint16_t* secondLayerBuffer, uint8_t* secondLayerTypeBuffer,
                                                     const WindowControl& winCtrl);
    
    // Window-aware sprite rendering
    
    // Blend and window functions (Session 3: Advanced Features)
    BlendControl readBlendControl();                            // Read and parse blend registers
    WindowControl readWindowControl();                          // Read and parse window registers
    static bool isPixelInWindow(int x, int y, const Window& win);     // Check if pixel is in window
    uint8_t getWindowControlForPixel(int x, int y, const WindowControl& winCtrl);  // Get control flags
    static uint16_t applyBlend(uint16_t color1, uint16_t color2, const BlendControl& blend, 
                       int layerType1, int layerType2);         // Apply blend effect
    static uint16_t applyBrightnessIncrease(uint16_t color, uint8_t evy);  // Brighten color
    static uint16_t applyBrightnessDecrease(uint16_t color, uint8_t evy);  // Darken color
};

#endif
