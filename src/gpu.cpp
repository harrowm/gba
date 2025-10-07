#include "gpu.h"
#include "memory.h"
#include "scheduler.h"
#include "debug.h"
#include "utility_macros.h"
#include <cstdint>
#include <cstring>

GPU::GPU(Memory& mem) 
    : memory(mem), currentScanline(0), inVBlank(false), inHBlank(false) {
    // Initialize GPU state
    DEBUG_INFO("GPU initialized");
}

void GPU::setupTiming(Scheduler* scheduler) {
    if (!scheduler) {
        DEBUG_ERROR("GPU::setupTiming called with null scheduler");
        return;
    }
    
    DEBUG_INFO("Setting up GPU video timing with scheduler");
    
    // Schedule the first scanline event
    scheduler->schedule(CYCLES_HDRAW, [this, scheduler]() {
        // H-Draw complete, enter H-Blank
        inHBlank = true;
        
        // Update DISPSTAT register
        uint16_t dispstat = memory.read16(REG_DISPSTAT);
        dispstat |= DISPSTAT_HBLANK;
        memory.write16(REG_DISPSTAT, dispstat);
        
        // Trigger H-Blank interrupt if enabled
        if ((dispstat & DISPSTAT_HBLANK_IRQ_ENABLE) && hblankCallback) {
            hblankCallback();
        }
        
        // Render this scanline if in visible area
        if (currentScanline < SCANLINES_VISIBLE) {
            renderScanline();
        }
        
        // Schedule end of H-Blank (start of next scanline)
        scheduler->schedule(CYCLES_HBLANK, [this, scheduler]() {
            inHBlank = false;
            
            // Clear H-Blank bit in DISPSTAT
            uint16_t dispstat = memory.read16(REG_DISPSTAT);
            dispstat &= ~DISPSTAT_HBLANK;
            memory.write16(REG_DISPSTAT, dispstat);
            
            // Move to next scanline
            currentScanline++;
            if (currentScanline >= SCANLINES_TOTAL) {
                currentScanline = 0;
            }
            
            // Update VCOUNT register
            memory.write16(REG_VCOUNT, currentScanline);
            
            // Check for V-Blank transition
            if (currentScanline == SCANLINES_VISIBLE) {
                // Entering V-Blank
                inVBlank = true;
                dispstat = memory.read16(REG_DISPSTAT);
                dispstat |= DISPSTAT_VBLANK;
                memory.write16(REG_DISPSTAT, dispstat);
                
                // Trigger V-Blank interrupt if enabled
                if ((dispstat & DISPSTAT_VBLANK_IRQ_ENABLE) && vblankCallback) {
                    DEBUG_INFO("V-Blank interrupt triggered at scanline 160");
                    vblankCallback();
                }
            } else if (currentScanline == 0) {
                // Exiting V-Blank, start new frame
                inVBlank = false;
                dispstat = memory.read16(REG_DISPSTAT);
                dispstat &= ~DISPSTAT_VBLANK;
                memory.write16(REG_DISPSTAT, dispstat);
            }
            
            // Schedule next scanline's H-Draw period
            setupTiming(scheduler);
        }, EventType::VIDEO_SCANLINE, 1);
        
    }, EventType::VIDEO_HBLANK, 1);
}

void GPU::renderScanline() {
    // Check video mode from DISPCNT
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    uint16_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    if (mode == DISPCNT_MODE_3) {
        renderMode3Scanline(currentScanline);
    }
    // Other modes not yet implemented
}

void GPU::renderMode3Scanline(uint16_t scanline) {
    if (scanline >= SCANLINES_VISIBLE) {
        return; // Don't render during V-Blank
    }
    
    // Mode 3: 240x160 @ 16bpp bitmap at VRAM 0x06000000
    // In Mode 3, VRAM directly contains the framebuffer in RGB555 format
    // One scanline = 240 pixels * 2 bytes = 480 bytes
    
    // The actual rendering happens when Display::renderFrame() is called
    // which reads directly from VRAM via getFrameBuffer()
    // This method is called during H-Draw to simulate scanline rendering timing
    
    // In a more accurate emulator, we might do per-pixel effects here,
    // but for basic Mode 3, the framebuffer is already in the correct format
}

// Palette Functions

uint16_t GPU::readBGPaletteRaw(int paletteNum, int colorIndex) {
    // BG palette: 16 palettes × 16 colors
    // Each color is 2 bytes (RGB555)
    if (paletteNum < 0 || paletteNum >= 16 || colorIndex < 0 || colorIndex >= 16) {
        return 0;
    }
    
    uint32_t offset = (paletteNum * 16 + colorIndex) * 2;
    uint8_t* paletteRAM = memory.getPaletteRAM();
    
    // Read 16-bit color value (little endian)
    return paletteRAM[offset] | (paletteRAM[offset + 1] << 8);
}

uint16_t GPU::readOBJPaletteRaw(int paletteNum, int colorIndex) {
    // OBJ palette: 16 palettes × 16 colors (starts at offset 0x200)
    // Each color is 2 bytes (RGB555)
    if (paletteNum < 0 || paletteNum >= 16 || colorIndex < 0 || colorIndex >= 16) {
        return 0;
    }
    
    uint32_t offset = 0x200 + (paletteNum * 16 + colorIndex) * 2;
    uint8_t* paletteRAM = memory.getPaletteRAM();
    
    // Read 16-bit color value (little endian)
    return paletteRAM[offset] | (paletteRAM[offset + 1] << 8);
}

uint32_t GPU::convertRGB555toARGB8888(uint16_t rgb555) {
    // RGB555 format: 0BBBBBGGGGGRRRRR (5 bits per channel)
    // Extract 5-bit components
    uint8_t r5 = (rgb555 & 0x001F);
    uint8_t g5 = (rgb555 & 0x03E0) >> 5;
    uint8_t b5 = (rgb555 & 0x7C00) >> 10;
    
    // Convert to 8-bit by scaling: (value * 255) / 31
    // Optimized: (value << 3) | (value >> 2) gives similar result
    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g5 << 3) | (g5 >> 2);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);
    
    // Return ARGB8888 format (0xAARRGGBB)
    return 0xFF000000 | (r8 << 16) | (g8 << 8) | b8;
}

uint32_t GPU::getBGColor(int paletteNum, int colorIndex) {
    // Read raw RGB555 color from BG palette
    uint16_t rgb555 = readBGPaletteRaw(paletteNum, colorIndex);
    
    // Convert to ARGB8888 for display
    return convertRGB555toARGB8888(rgb555);
}

uint32_t GPU::getOBJColor(int paletteNum, int colorIndex) {
    // Read raw RGB555 color from OBJ palette
    uint16_t rgb555 = readOBJPaletteRaw(paletteNum, colorIndex);
    
    // Convert to ARGB8888 for display
    return convertRGB555toARGB8888(rgb555);
}

// Tile Decoding Functions

void GPU::decodeTile4bpp(uint32_t tileAddr, uint8_t* output) {
    // 4bpp: 4 bits per pixel, 2 pixels per byte
    // 8×8 pixels = 64 pixels = 32 bytes
    // Each byte contains 2 pixels: low nibble (first pixel), high nibble (second pixel)
    
    if (!output) return;
    
    uint8_t* vram = memory.getVRAM();
    uint32_t offset = tileAddr - 0x06000000;  // Convert address to VRAM offset
    
    for (int i = 0; i < 32; i++) {
        uint8_t byte = vram[offset + i];
        
        // Low nibble (first pixel in pair)
        output[i * 2] = byte & 0x0F;
        
        // High nibble (second pixel in pair)
        output[i * 2 + 1] = (byte >> 4) & 0x0F;
    }
}

void GPU::decodeTile8bpp(uint32_t tileAddr, uint8_t* output) {
    // 8bpp: 8 bits per pixel, 1 pixel per byte
    // 8×8 pixels = 64 pixels = 64 bytes
    
    if (!output) return;
    
    uint8_t* vram = memory.getVRAM();
    uint32_t offset = tileAddr - 0x06000000;  // Convert address to VRAM offset
    
    for (int i = 0; i < 64; i++) {
        output[i] = vram[offset + i];
    }
}

uint8_t GPU::getTilePixel4bpp(uint32_t tileAddr, int pixelX, int pixelY) {
    // Get a single pixel from a 4bpp tile
    // pixelX, pixelY are in range [0, 7]
    
    if (pixelX < 0 || pixelX >= 8 || pixelY < 0 || pixelY >= 8) {
        return 0;
    }
    
    uint8_t* vram = memory.getVRAM();
    uint32_t offset = tileAddr - 0x06000000;
    
    // Calculate byte offset and pixel position within byte
    // Tiles are stored row by row
    int pixelIndex = pixelY * 8 + pixelX;
    int byteOffset = pixelIndex / 2;  // 2 pixels per byte
    int pixelInByte = pixelIndex % 2;  // 0 = low nibble, 1 = high nibble
    
    uint8_t byte = vram[offset + byteOffset];
    
    if (pixelInByte == 0) {
        // Low nibble
        return byte & 0x0F;
    } else {
        // High nibble
        return (byte >> 4) & 0x0F;
    }
}

uint8_t GPU::getTilePixel8bpp(uint32_t tileAddr, int pixelX, int pixelY) {
    // Get a single pixel from an 8bpp tile
    // pixelX, pixelY are in range [0, 7]
    
    if (pixelX < 0 || pixelX >= 8 || pixelY < 0 || pixelY >= 8) {
        return 0;
    }
    
    uint8_t* vram = memory.getVRAM();
    uint32_t offset = tileAddr - 0x06000000;
    
    // Calculate byte offset (1 pixel per byte)
    int pixelIndex = pixelY * 8 + pixelX;
    
    return vram[offset + pixelIndex];
}
