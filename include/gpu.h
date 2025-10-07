#ifndef GPU_H
#define GPU_H

#include "memory.h"
#include <cstdint>
#include <functional>

// GBA Video Timing Constants
constexpr uint32_t CYCLES_PER_SCANLINE = 1232;
constexpr uint32_t CYCLES_HDRAW = 960;          // 240 pixels * 4 cycles
constexpr uint32_t CYCLES_HBLANK = 272;
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

// DISPSTAT bits
constexpr uint16_t DISPSTAT_VBLANK = 0x0001;
constexpr uint16_t DISPSTAT_HBLANK = 0x0002;
constexpr uint16_t DISPSTAT_VCOUNT_MATCH = 0x0004;
constexpr uint16_t DISPSTAT_VBLANK_IRQ_ENABLE = 0x0008;
constexpr uint16_t DISPSTAT_HBLANK_IRQ_ENABLE = 0x0010;
constexpr uint16_t DISPSTAT_VCOUNT_IRQ_ENABLE = 0x0020;

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
    
    // Callbacks for interrupts
    std::function<void()> vblankCallback;
    std::function<void()> hblankCallback;

public:
    GPU(Memory& mem);
    
    // Setup video timing with scheduler
    void setupTiming(Scheduler* scheduler);
    
    // Rendering
    void renderScanline();
    void renderMode3Scanline(uint16_t scanline);
    void renderMode0Scanline(uint16_t scanline);  // Mode 0: 4 tiled backgrounds
    void renderBGScanline(int bgNum, uint16_t scanline);  // Render a single background scanline
    
    // Helper rendering functions
    void clearScanlineToBackdrop(uint16_t scanline);
    void renderBlankScanline(uint16_t scanline);
    
    // VCOUNT handling
    uint16_t getCurrentScanline() const { return currentScanline; }
    void setCurrentScanline(uint16_t scanline) { currentScanline = scanline; }
    
    // Interrupt callbacks
    void setVBlankCallback(std::function<void()> callback) { vblankCallback = callback; }
    void setHBlankCallback(std::function<void()> callback) { hblankCallback = callback; }
    
    // Get frame buffer
    // Mode 3+ (bitmap modes): returns VRAM directly
    // Mode 0-2 (tiled modes): returns internal tiled framebuffer
    uint16_t* getFrameBuffer();
    uint16_t* getTiledFramebuffer() { return tiledFramebuffer; }
    
    // Palette functions
    uint16_t readBGPaletteRaw(int paletteNum, int colorIndex);
    uint16_t readOBJPaletteRaw(int paletteNum, int colorIndex);
    uint32_t getOBJColor(int paletteNum, int colorIndex);
    
    // Hot-path inline color conversion functions
    inline uint32_t convertRGB555toARGB8888(uint16_t rgb555) {
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
    
    // Tile decoding functions
    void decodeTile4bpp(uint32_t tileAddr, uint8_t* output);
    void decodeTile8bpp(uint32_t tileAddr, uint8_t* output);
    
    // Hot-path inline functions for tile pixel access
    inline uint8_t getTilePixel4bpp(uint32_t tileAddr, int pixelX, int pixelY) {
        // Get a single pixel from a 4bpp tile (pixelX, pixelY in range [0, 7])
        if (pixelX < 0 || pixelX >= 8 || pixelY < 0 || pixelY >= 8) {
            return 0;
        }
        
        uint8_t* vram = memory.getVRAM();
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
        
        uint8_t* vram = memory.getVRAM();
        uint32_t offset = tileAddr - 0x06000000;
        
        // Calculate byte offset (1 pixel per byte)
        int pixelIndex = pixelY * 8 + pixelX;
        return vram[offset + pixelIndex];
    }
    
    // DISPCNT register parsing
    DisplayControl parseDISPCNT(uint16_t dispcnt);
    DisplayControl readDISPCNT();  // Read and parse from memory
    
    // Helper functions to check specific DISPCNT bits
    bool isBGEnabled(int bgNum);     // Check if BG0-3 is enabled
    bool isOBJEnabled();              // Check if sprites are enabled
    bool isForcedBlank();             // Check if display is blanked
    uint8_t getVideoMode();           // Get current video mode (0-5)
    
    // BGxCNT register parsing
    BGConfig parseBGCNT(uint16_t bgcnt);
    BGConfig readBGCNT(int bgNum);    // Read and parse BGxCNT (bgNum: 0-3)
    
    // Helper to get screen dimensions from size code
    void getScreenDimensions(uint8_t sizeCode, int& widthTiles, int& heightTiles);
    
    // Tile map (screen entry) functions
    ScreenEntry parseScreenEntry(uint16_t entry);
    ScreenEntry readScreenEntry(const BGConfig& bgConfig, int tileX, int tileY);
    uint16_t readScreenEntryRaw(const BGConfig& bgConfig, int tileX, int tileY);
    
    // Get tile address from screen entry
    uint32_t getTileAddress(const BGConfig& bgConfig, const ScreenEntry& entry);
    
    // Helper to get screen block offset for large screens
    uint32_t getScreenBlockOffset(const BGConfig& bgConfig, int tileX, int tileY);
    
    // Scroll functions
    BGScroll readBGScroll(int bgNum);                           // Read scroll from registers
    void applyScroll(const BGConfig& bgConfig, const BGScroll& scroll, 
                     int screenX, int screenY, int& bgX, int& bgY);  // Apply scroll to coords
    void getTileCoords(int pixelX, int pixelY, int& tileX, int& tileY, 
                       int& pixelInTileX, int& pixelInTileY);       // Convert pixel to tile coords
    
    // OAM (sprite) functions
    OBJAttributes parseOBJAttributes(uint16_t attr0, uint16_t attr1, uint16_t attr2);
    OBJAttributes readOBJAttributes(int objNum);                // Read OBJ attributes (objNum: 0-127)
    void getOBJDimensions(uint8_t shape, uint8_t size, int& width, int& height);
};

#endif
