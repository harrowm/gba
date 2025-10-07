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

// DISPCNT Register Parsing

DisplayControl GPU::parseDISPCNT(uint16_t dispcnt) {
    DisplayControl dc;
    
    // Extract all fields from DISPCNT register
    dc.videoMode = dispcnt & DISPCNT_MODE_MASK;
    dc.frameSelect = (dispcnt & DISPCNT_FRAME_SELECT) != 0;
    dc.oamHBlankAccess = (dispcnt & DISPCNT_OAM_HBLANK) != 0;
    dc.obj1DMapping = (dispcnt & DISPCNT_OBJ_1D_MAP) != 0;
    dc.forcedBlank = (dispcnt & DISPCNT_FORCED_BLANK) != 0;
    dc.bg0Enable = (dispcnt & DISPCNT_BG0_ENABLE) != 0;
    dc.bg1Enable = (dispcnt & DISPCNT_BG1_ENABLE) != 0;
    dc.bg2Enable = (dispcnt & DISPCNT_BG2_ENABLE) != 0;
    dc.bg3Enable = (dispcnt & DISPCNT_BG3_ENABLE) != 0;
    dc.objEnable = (dispcnt & DISPCNT_OBJ_ENABLE) != 0;
    dc.win0Enable = (dispcnt & DISPCNT_WIN0_ENABLE) != 0;
    dc.win1Enable = (dispcnt & DISPCNT_WIN1_ENABLE) != 0;
    dc.winObjEnable = (dispcnt & DISPCNT_WINOBJ_ENABLE) != 0;
    
    return dc;
}

DisplayControl GPU::readDISPCNT() {
    // Read DISPCNT from memory and parse it
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    return parseDISPCNT(dispcnt);
}

bool GPU::isBGEnabled(int bgNum) {
    // Check if a specific background is enabled (0-3)
    if (bgNum < 0 || bgNum > 3) {
        return false;
    }
    
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    uint16_t bgBit = DISPCNT_BG0_ENABLE << bgNum;  // BG0=bit 8, BG1=bit 9, etc.
    
    return (dispcnt & bgBit) != 0;
}

bool GPU::isOBJEnabled() {
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    return (dispcnt & DISPCNT_OBJ_ENABLE) != 0;
}

bool GPU::isForcedBlank() {
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    return (dispcnt & DISPCNT_FORCED_BLANK) != 0;
}

uint8_t GPU::getVideoMode() {
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    return dispcnt & DISPCNT_MODE_MASK;
}

// BGxCNT Register Parsing

void GPU::getScreenDimensions(uint8_t sizeCode, int& widthTiles, int& heightTiles) {
    // Screen size encoding:
    // 0: 256x256 (32x32 tiles)
    // 1: 512x256 (64x32 tiles)
    // 2: 256x512 (32x64 tiles)
    // 3: 512x512 (64x64 tiles)
    
    switch (sizeCode) {
        case BG_SCREEN_SIZE_256x256:
            widthTiles = 32;
            heightTiles = 32;
            break;
        case BG_SCREEN_SIZE_512x256:
            widthTiles = 64;
            heightTiles = 32;
            break;
        case BG_SCREEN_SIZE_256x512:
            widthTiles = 32;
            heightTiles = 64;
            break;
        case BG_SCREEN_SIZE_512x512:
            widthTiles = 64;
            heightTiles = 64;
            break;
        default:
            widthTiles = 32;
            heightTiles = 32;
            break;
    }
}

BGConfig GPU::parseBGCNT(uint16_t bgcnt) {
    BGConfig config;
    
    // Extract bit fields
    config.priority = bgcnt & BGCNT_PRIORITY_MASK;
    config.charBaseBlock = (bgcnt & BGCNT_CHAR_BASE_MASK) >> 2;
    config.mosaicEnable = (bgcnt & BGCNT_MOSAIC) != 0;
    config.paletteMode = (bgcnt & BGCNT_PALETTE_MODE) != 0;
    config.screenBaseBlock = (bgcnt & BGCNT_SCREEN_BASE_MASK) >> 8;
    config.screenSize = (bgcnt & BGCNT_SCREEN_SIZE_MASK) >> 14;
    
    // Compute actual VRAM addresses
    // Character base: block * 16KB
    config.charBaseAddr = VRAM_BASE + (config.charBaseBlock * 0x4000);
    
    // Screen base: block * 2KB
    config.screenBaseAddr = VRAM_BASE + (config.screenBaseBlock * 0x800);
    
    // Get screen dimensions
    getScreenDimensions(config.screenSize, config.screenWidthTiles, config.screenHeightTiles);
    
    // Convert to pixels (each tile is 8x8)
    config.screenWidthPixels = config.screenWidthTiles * 8;
    config.screenHeightPixels = config.screenHeightTiles * 8;
    
    return config;
}

BGConfig GPU::readBGCNT(int bgNum) {
    // Read BGxCNT from memory and parse it
    if (bgNum < 0 || bgNum > 3) {
        // Return default config for invalid BG number
        return parseBGCNT(0);
    }
    
    uint32_t bgcntAddr = REG_BG0CNT + (bgNum * 2);
    uint16_t bgcnt = memory.read16(bgcntAddr);
    
    return parseBGCNT(bgcnt);
}

// Tile Map (Screen Entry) Functions

ScreenEntry GPU::parseScreenEntry(uint16_t entry) {
    ScreenEntry se;
    
    // Extract bit fields from screen entry
    se.tileNumber = entry & SE_TILE_NUM_MASK;
    se.hFlip = (entry & SE_HFLIP) != 0;
    se.vFlip = (entry & SE_VFLIP) != 0;
    se.paletteNum = (entry & SE_PALETTE_MASK) >> 12;
    
    return se;
}

uint32_t GPU::getScreenBlockOffset(const BGConfig& bgConfig, int tileX, int tileY) {
    // For screens larger than 32x32, they are divided into 32x32 screen blocks
    // Layout for different sizes:
    // 256x256 (32x32): [0]
    // 512x256 (64x32): [0][1]
    // 256x512 (32x64): [0]
    //                  [1]
    // 512x512 (64x64): [0][1]
    //                  [2][3]
    
    int screenBlockX = tileX / 32;
    int screenBlockY = tileY / 32;
    
    uint32_t offset = 0;
    
    switch (bgConfig.screenSize) {
        case BG_SCREEN_SIZE_256x256:
            // Single 32x32 block, no offset needed
            offset = 0;
            break;
            
        case BG_SCREEN_SIZE_512x256:
            // Two 32x32 blocks horizontally: [0][1]
            offset = screenBlockX * 0x800;  // Each block is 2KB
            break;
            
        case BG_SCREEN_SIZE_256x512:
            // Two 32x32 blocks vertically: [0]
            //                               [1]
            offset = screenBlockY * 0x800;
            break;
            
        case BG_SCREEN_SIZE_512x512:
            // Four 32x32 blocks: [0][1]
            //                    [2][3]
            offset = (screenBlockY * 2 + screenBlockX) * 0x800;
            break;
    }
    
    return offset;
}

uint16_t GPU::readScreenEntryRaw(const BGConfig& bgConfig, int tileX, int tileY) {
    // Bounds check
    if (tileX < 0 || tileX >= bgConfig.screenWidthTiles ||
        tileY < 0 || tileY >= bgConfig.screenHeightTiles) {
        return 0;
    }
    
    // Get the screen block offset for large screens
    uint32_t screenBlockOffset = getScreenBlockOffset(bgConfig, tileX, tileY);
    
    // Calculate position within the 32x32 block
    int localX = tileX % 32;
    int localY = tileY % 32;
    
    // Each screen entry is 2 bytes, laid out in row-major order within the block
    uint32_t entryOffset = (localY * 32 + localX) * 2;
    
    // Final address
    uint32_t addr = bgConfig.screenBaseAddr + screenBlockOffset + entryOffset;
    
    return memory.read16(addr);
}

ScreenEntry GPU::readScreenEntry(const BGConfig& bgConfig, int tileX, int tileY) {
    uint16_t entry = readScreenEntryRaw(bgConfig, tileX, tileY);
    return parseScreenEntry(entry);
}

uint32_t GPU::getTileAddress(const BGConfig& bgConfig, const ScreenEntry& entry) {
    // Calculate tile address from character base and tile number
    // 4bpp: 32 bytes per tile
    // 8bpp: 64 bytes per tile
    
    uint32_t bytesPerTile = bgConfig.paletteMode ? 64 : 32;
    uint32_t tileOffset = entry.tileNumber * bytesPerTile;
    
    return bgConfig.charBaseAddr + tileOffset;
}

// Scrolling Functions

BGScroll GPU::readBGScroll(int bgNum) {
    BGScroll scroll;
    
    // Read scroll registers for the specified background
    if (bgNum < 0 || bgNum > 3) {
        scroll.hofs = 0;
        scroll.vofs = 0;
        return scroll;
    }
    
    uint32_t hofsAddr = REG_BG0HOFS + (bgNum * 4);
    uint32_t vofsAddr = REG_BG0VOFS + (bgNum * 4);
    
    // Scroll registers are write-only in hardware, but we read from memory
    // Only lower 9 bits are used (0-511)
    scroll.hofs = memory.read16(hofsAddr) & 0x01FF;
    scroll.vofs = memory.read16(vofsAddr) & 0x01FF;
    
    return scroll;
}

void GPU::applyScroll(const BGConfig& bgConfig, const BGScroll& scroll, 
                      int screenX, int screenY, int& bgX, int& bgY) {
    // Apply scroll offsets to screen coordinates
    // The scroll values indicate how much the background has moved
    bgX = (screenX + scroll.hofs) % bgConfig.screenWidthPixels;
    bgY = (screenY + scroll.vofs) % bgConfig.screenHeightPixels;
    
    // Handle negative wrapping (shouldn't happen with our unsigned math, but be safe)
    if (bgX < 0) bgX += bgConfig.screenWidthPixels;
    if (bgY < 0) bgY += bgConfig.screenHeightPixels;
}

void GPU::getTileCoords(int pixelX, int pixelY, int& tileX, int& tileY, 
                        int& pixelInTileX, int& pixelInTileY) {
    // Convert pixel coordinates to tile coordinates
    // Each tile is 8x8 pixels
    tileX = pixelX / 8;
    tileY = pixelY / 8;
    
    // Get position within the tile (0-7)
    pixelInTileX = pixelX % 8;
    pixelInTileY = pixelY % 8;
    
    // Handle negative coordinates
    if (pixelX < 0) {
        tileX = (pixelX - 7) / 8;
        pixelInTileX = pixelX % 8;
        if (pixelInTileX < 0) pixelInTileX += 8;
    }
    
    if (pixelY < 0) {
        tileY = (pixelY - 7) / 8;
        pixelInTileY = pixelY % 8;
        if (pixelInTileY < 0) pixelInTileY += 8;
    }
}
