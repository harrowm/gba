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
    
    // VCOUNT handling
    uint16_t getCurrentScanline() const { return currentScanline; }
    void setCurrentScanline(uint16_t scanline) { currentScanline = scanline; }
    
    // Interrupt callbacks
    void setVBlankCallback(std::function<void()> callback) { vblankCallback = callback; }
    void setHBlankCallback(std::function<void()> callback) { hblankCallback = callback; }
    
    // Get frame buffer (Mode 3: 240x160x16bpp at 0x06000000)
    uint16_t* getFrameBuffer() { return reinterpret_cast<uint16_t*>(memory.getVRAM()); }
    
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
};

#endif
