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
    void renderBGScanline(int bgNum, uint16_t scanline);  // Render a single background scanline
    
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
    uint32_t convertRGB555toARGB8888(uint16_t rgb555);
    uint32_t getBGColor(int paletteNum, int colorIndex);
    uint32_t getOBJColor(int paletteNum, int colorIndex);
    
    // Tile decoding functions
    void decodeTile4bpp(uint32_t tileAddr, uint8_t* output);
    void decodeTile8bpp(uint32_t tileAddr, uint8_t* output);
    uint8_t getTilePixel4bpp(uint32_t tileAddr, int pixelX, int pixelY);
    uint8_t getTilePixel8bpp(uint32_t tileAddr, int pixelX, int pixelY);
    
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
};

#endif
