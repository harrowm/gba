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
};

#endif
