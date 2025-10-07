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
    // Clear the tiled framebuffer
    memset(tiledFramebuffer, 0, sizeof(tiledFramebuffer));
    

    
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
    // Skip rendering during VBlank
    if (currentScanline >= SCANLINES_VISIBLE) {
        return;
    }
    
    // Check for forced blank
    if (isForcedBlank()) {
        renderBlankScanline(currentScanline);
        return;
    }
    
    // Check video mode from DISPCNT
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    uint16_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    switch (mode) {
        case 0:
            renderMode0Scanline(currentScanline);
            break;
        case 3:
            renderMode3Scanline(currentScanline);
            break;
        // Modes 1, 2, 4, 5 not yet implemented
        default:
            // Unknown mode - render blank
            clearScanlineToBackdrop(currentScanline);
            break;
    }
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

uint16_t* GPU::getFrameBuffer() {
    // Return appropriate framebuffer based on video mode
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    uint16_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    if (mode >= 3) {
        // Bitmap modes (3, 4, 5): VRAM is the framebuffer
        return reinterpret_cast<uint16_t*>(memory.getVRAM());
    } else {
        // Tiled modes (0, 1, 2): use internal framebuffer
        return tiledFramebuffer;
    }
}

// Palette Functions

uint16_t GPU::readBGPaletteRaw(int paletteNum, int colorIndex) {
    // BG palette can be accessed in two ways:
    // - 4bpp mode: 16 palettes × 16 colors (paletteNum 0-15, colorIndex 0-15)
    // - 8bpp mode: 1 palette × 256 colors (paletteNum 0, colorIndex 0-255)
    // Each color is 2 bytes (RGB555)
    
    // Calculate offset based on mode
    uint32_t offset;
    if (colorIndex < 16) {
        // 4bpp mode access
        if (paletteNum < 0 || paletteNum >= 16 || colorIndex < 0) {
            return 0;
        }
        offset = (paletteNum * 16 + colorIndex) * 2;
    } else {
        // 8bpp mode access (colorIndex 0-255, paletteNum should be 0)
        if (colorIndex < 0 || colorIndex >= 256) {
            return 0;
        }
        offset = colorIndex * 2;
    }
    
    uint8_t* paletteRAM = memory.getPaletteRAM();
    
    // Read 16-bit color value (little endian)
    return paletteRAM[offset] | (paletteRAM[offset + 1] << 8);
}

uint16_t GPU::readOBJPaletteRaw(int paletteNum, int colorIndex) {
    // OBJ palette: 16 palettes × 16 colors (starts at offset 0x200)
    // Each color is 2 bytes (RGB555)
    // For 8bpp mode: paletteNum is ignored, colorIndex is 0-255
    // For 4bpp mode: paletteNum is 0-15, colorIndex is 0-15
    
    uint32_t offset;
    if (colorIndex >= 16) {
        // 8bpp mode: direct 256-color palette
        if (colorIndex < 0 || colorIndex >= 256) {
            return 0;
        }
        offset = 0x200 + (colorIndex * 2);
    } else {
        // 4bpp mode: 16 palettes of 16 colors
        if (paletteNum < 0 || paletteNum >= 16 || colorIndex < 0) {
            return 0;
        }
        offset = 0x200 + (paletteNum * 16 + colorIndex) * 2;
    }
    
    uint8_t* paletteRAM = memory.getPaletteRAM();
    
    // Read 16-bit color value (little endian)
    return paletteRAM[offset] | (paletteRAM[offset + 1] << 8);
}

// convertRGB555toARGB8888 and getBGColor are now inline in gpu.h for performance

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

// getTilePixel4bpp and getTilePixel8bpp are now inline in gpu.h for performance

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

// Background Scanline Rendering

void GPU::renderBGScanline(int bgNum, uint16_t scanline) {
    // Render a single scanline of a background layer
    
    // Check if background is valid and enabled
    if (bgNum < 0 || bgNum > 3) return;
    if (!isBGEnabled(bgNum)) return;
    
    // Get background configuration
    BGConfig bgConfig = readBGCNT(bgNum);
    BGScroll scroll = readBGScroll(bgNum);
    
    // Get framebuffer
    uint16_t* framebuffer = getFrameBuffer();
    if (!framebuffer) return;
    
    // Render each pixel in the scanline
    for (int screenX = 0; screenX < 240; screenX++) {
        // Apply scrolling to get background coordinates
        int bgX, bgY;
        applyScroll(bgConfig, scroll, screenX, scanline, bgX, bgY);
        
        // Convert to tile coordinates
        int tileX, tileY, pixelInTileX, pixelInTileY;
        getTileCoords(bgX, bgY, tileX, tileY, pixelInTileX, pixelInTileY);
        
        // Read the screen entry (tile map) for this tile
        ScreenEntry entry = readScreenEntry(bgConfig, tileX, tileY);
        
        // Skip transparent tiles (tile 0 is often used as transparent)
        if (entry.tileNumber == 0) {
            continue;  // Leave background color
        }
        
        // Get the tile address in VRAM
        uint32_t tileAddr = getTileAddress(bgConfig, entry);
        
        // Handle horizontal/vertical flips
        int actualPixelX = entry.hFlip ? (7 - pixelInTileX) : pixelInTileX;
        int actualPixelY = entry.vFlip ? (7 - pixelInTileY) : pixelInTileY;
        
        // Get the palette index for this pixel
        uint8_t paletteIndex;
        if (bgConfig.paletteMode) {
            // 8bpp mode - 256 colors, single palette
            paletteIndex = getTilePixel8bpp(tileAddr, actualPixelX, actualPixelY);
        } else {
            // 4bpp mode - 16 colors per palette
            paletteIndex = getTilePixel4bpp(tileAddr, actualPixelX, actualPixelY);
        }
        
        // Skip transparent pixels (palette index 0)
        if (paletteIndex == 0) {
            continue;
        }
        
        // Get the color from the palette
        uint32_t color;
        if (bgConfig.paletteMode) {
            // 8bpp uses palette 0 for all 256 colors
            color = getBGColor(0, paletteIndex);
        } else {
            // 4bpp uses the palette specified in the screen entry
            color = getBGColor(entry.paletteNum, paletteIndex);
        }
        
        // Convert ARGB8888 back to RGB555 for framebuffer
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        // Convert 8-bit channels back to 5-bit
        uint16_t rgb555 = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
        
        // Write to framebuffer
        int fbOffset = scanline * 240 + screenX;
        framebuffer[fbOffset] = rgb555;
    }
}

void GPU::renderMode0Scanline(uint16_t scanline) {
    // Mode 0: Tiled backgrounds with proper priority-based compositing
    // Priority rules:
    // 1. Lower priority value = higher priority (0 is highest, 3 is lowest)
    // 2. When priorities match, lower BG number wins (BG0 > BG1 > BG2 > BG3)
    // 3. Transparent pixels (palette index 0) don't draw
    // 4. Backdrop color has lowest priority
    
    // First, clear the scanline to backdrop color
    clearScanlineToBackdrop(scanline);
    
    // Read DISPCNT to see which backgrounds are enabled
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    
    // Read BGxCNT for all backgrounds to get priorities
    BGConfig bgConfigs[4];
    bool bgEnabled[4];
    for (int i = 0; i < 4; i++) {
        bgEnabled[i] = (dispcnt & (DISPCNT_BG0_ENABLE << i)) != 0;
        if (bgEnabled[i]) {
            bgConfigs[i] = readBGCNT(i);
        }
    }
    
    // Render each pixel with priority compositing
    for (int screenX = 0; screenX < 240; screenX++) {
        // Track the best pixel so far
        int bestBG = -1;           // -1 means backdrop
        uint8_t bestPriority = 4;  // Start with priority worse than any BG (backdrop priority)
        uint16_t bestColor = 0;    // Will be set when we find a pixel
        
        // Check each enabled background
        for (int bgNum = 0; bgNum < 4; bgNum++) {
            if (!bgEnabled[bgNum]) continue;
            
            const BGConfig& bgConfig = bgConfigs[bgNum];
            BGScroll scroll = readBGScroll(bgNum);
            
            // Apply scrolling to get background coordinates
            int bgX, bgY;
            applyScroll(bgConfig, scroll, screenX, scanline, bgX, bgY);
            
            // Convert to tile coordinates
            int tileX, tileY, pixelInTileX, pixelInTileY;
            getTileCoords(bgX, bgY, tileX, tileY, pixelInTileX, pixelInTileY);
            
            // Read the screen entry for this tile
            ScreenEntry entry = readScreenEntry(bgConfig, tileX, tileY);
            
            // Skip transparent tiles
            if (entry.tileNumber == 0) {
                continue;
            }
            
            // Get the tile address
            uint32_t tileAddr = getTileAddress(bgConfig, entry);
            
            // Handle flips
            int actualPixelX = entry.hFlip ? (7 - pixelInTileX) : pixelInTileX;
            int actualPixelY = entry.vFlip ? (7 - pixelInTileY) : pixelInTileY;
            
            // Get the palette index
            uint8_t paletteIndex;
            if (bgConfig.paletteMode) {
                paletteIndex = getTilePixel8bpp(tileAddr, actualPixelX, actualPixelY);
            } else {
                paletteIndex = getTilePixel4bpp(tileAddr, actualPixelX, actualPixelY);
            }
            
            // Skip transparent pixels
            if (paletteIndex == 0) {
                continue;
            }
            
            // Get the color
            uint32_t color;
            if (bgConfig.paletteMode) {
                color = getBGColor(0, paletteIndex);
            } else {
                color = getBGColor(entry.paletteNum, paletteIndex);
            }
            
            // Convert to RGB555
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            uint16_t rgb555 = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
            
            // Check if this pixel should win based on priority
            // Lower priority value = higher priority
            // Tiebreaker: lower BG number wins
            if (bgConfig.priority < bestPriority || 
                (bgConfig.priority == bestPriority && bgNum < bestBG)) {
                bestBG = bgNum;
                bestPriority = bgConfig.priority;
                bestColor = rgb555;
            }
        }
        
        // Write the winning pixel (or keep backdrop if bestBG == -1)
        if (bestBG != -1) {
            int fbOffset = scanline * 240 + screenX;
            tiledFramebuffer[fbOffset] = bestColor;
        }
    }
}

void GPU::clearScanlineToBackdrop(uint16_t scanline) {
    // Backdrop color is palette entry 0 of the background palette
    uint32_t backdropColor = getBGColor(0, 0);
    
    // Convert ARGB8888 to RGB555
    uint8_t r = (backdropColor >> 16) & 0xFF;
    uint8_t g = (backdropColor >> 8) & 0xFF;
    uint8_t b = backdropColor & 0xFF;
    uint16_t rgb555 = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
    
    // Fill the scanline
    int fbOffset = scanline * 240;
    for (int x = 0; x < 240; x++) {
        tiledFramebuffer[fbOffset + x] = rgb555;
    }
}

void GPU::renderBlankScanline(uint16_t scanline) {
    // Forced blank renders white (0x7FFF = maximum RGB555 value)
    int fbOffset = scanline * 240;
    for (int x = 0; x < 240; x++) {
        tiledFramebuffer[fbOffset + x] = 0x7FFF;
    }
}

// ============================================================================
// OAM (Sprite) Functions
// ============================================================================

void GPU::getOBJDimensions(uint8_t shape, uint8_t size, int& width, int& height) {
    // Sprite dimensions based on shape and size codes
    // Shape: 0=Square, 1=Horizontal, 2=Vertical
    // Size: 0-3 (different meanings per shape)
    
    if (shape == OBJ_SHAPE_SQUARE) {
        // Square sprites
        switch (size) {
            case 0: width = 8;  height = 8;  break;
            case 1: width = 16; height = 16; break;
            case 2: width = 32; height = 32; break;
            case 3: width = 64; height = 64; break;
        }
    } else if (shape == OBJ_SHAPE_HORIZONTAL) {
        // Wide sprites
        switch (size) {
            case 0: width = 16; height = 8;  break;
            case 1: width = 32; height = 8;  break;
            case 2: width = 32; height = 16; break;
            case 3: width = 64; height = 32; break;
        }
    } else if (shape == OBJ_SHAPE_VERTICAL) {
        // Tall sprites
        switch (size) {
            case 0: width = 8;  height = 16; break;
            case 1: width = 8;  height = 32; break;
            case 2: width = 16; height = 32; break;
            case 3: width = 32; height = 64; break;
        }
    } else {
        // Prohibited shape
        width = 0;
        height = 0;
    }
}

OBJAttributes GPU::parseOBJAttributes(uint16_t attr0, uint16_t attr1, uint16_t attr2) {
    OBJAttributes obj;
    
    // Parse Attribute 0 (Y position, rotation, mode, shape)
    obj.y = attr0 & OBJ_ATTR0_Y_MASK;
    obj.rotScaleFlag = (attr0 & OBJ_ATTR0_ROT_SCALE_FLAG) != 0;
    
    if (obj.rotScaleFlag) {
        obj.doubleSize = (attr0 & OBJ_ATTR0_DOUBLE_SIZE) != 0;
    } else {
        // When not rotating, bit 9 is "OBJ disable"
        bool disabled = (attr0 & OBJ_ATTR0_OBJ_DISABLE) != 0;
        obj.doubleSize = false;
        if (disabled) {
            obj.visible = false;
            obj.width = 0;
            obj.height = 0;
            return obj;  // Early exit for disabled sprites
        }
    }
    
    obj.objMode = (attr0 & OBJ_ATTR0_MODE_MASK) >> 10;
    obj.mosaicEnable = (attr0 & OBJ_ATTR0_MOSAIC) != 0;
    obj.paletteMode = (attr0 & OBJ_ATTR0_PALETTE_MODE) != 0;
    obj.shape = (attr0 & OBJ_ATTR0_SHAPE_MASK) >> 14;
    
    // Parse Attribute 1 (X position, flip/rotation, size)
    obj.x = attr1 & OBJ_ATTR1_X_MASK;
    
    if (obj.rotScaleFlag) {
        obj.rotScaleParam = (attr1 & OBJ_ATTR1_ROT_PARAM_MASK) >> 9;
        obj.hFlip = false;
        obj.vFlip = false;
    } else {
        obj.rotScaleParam = 0;
        obj.hFlip = (attr1 & OBJ_ATTR1_HFLIP) != 0;
        obj.vFlip = (attr1 & OBJ_ATTR1_VFLIP) != 0;
    }
    
    obj.size = (attr1 & OBJ_ATTR1_SIZE_MASK) >> 14;
    
    // Parse Attribute 2 (tile number, priority, palette)
    obj.tileNumber = attr2 & OBJ_ATTR2_TILE_MASK;
    obj.priority = (attr2 & OBJ_ATTR2_PRIORITY_MASK) >> 10;
    obj.paletteNum = (attr2 & OBJ_ATTR2_PALETTE_MASK) >> 12;
    
    // Compute sprite dimensions
    getOBJDimensions(obj.shape, obj.size, obj.width, obj.height);
    
    // Check if sprite is visible
    obj.visible = (obj.shape != OBJ_SHAPE_PROHIBITED) && 
                  (obj.objMode != OBJ_MODE_PROHIBITED) &&
                  (obj.width > 0) && (obj.height > 0);
    
    return obj;
}

OBJAttributes GPU::readOBJAttributes(int objNum) {
    // Read OAM attributes for sprite objNum (0-127)
    if (objNum < 0 || objNum >= 128) {
        // Return invalid sprite
        OBJAttributes invalid;
        invalid.visible = false;
        invalid.width = 0;
        invalid.height = 0;
        return invalid;
    }
    
    // Each OBJ entry is 8 bytes (4 × 16-bit attributes, but we only use first 3)
    uint32_t oamAddr = OAM_BASE + (objNum * 8);
    
    uint16_t attr0 = memory.read16(oamAddr + 0);
    uint16_t attr1 = memory.read16(oamAddr + 2);
    uint16_t attr2 = memory.read16(oamAddr + 4);
    
    return parseOBJAttributes(attr0, attr1, attr2);
}

// ============================================================================
// Sprite Rendering Functions
// ============================================================================

uint32_t GPU::getOBJTileAddress(const OBJAttributes& obj, int tileX, int tileY, bool mapping1D) {
    // Calculate tile address for sprite tile at (tileX, tileY) within sprite
    // tileX, tileY are in tile coordinates within the sprite (0-based)
    
    uint32_t baseTileNum = obj.tileNumber;
    uint32_t tileNum;
    
    if (mapping1D) {
        // 1D mapping: tiles are sequential in memory
        // For 4bpp: each tile is 32 bytes (0.5 tile blocks)
        // For 8bpp: each tile is 64 bytes (1 tile block)
        
        int spriteWidthTiles = obj.width / 8;
        int tileOffset = tileY * spriteWidthTiles + tileX;
        
        if (obj.paletteMode) {
            // 8bpp: each tile takes 2 tile numbers (64 bytes each)
            tileNum = baseTileNum + (tileOffset * 2);
        } else {
            // 4bpp: tiles are sequential
            tileNum = baseTileNum + tileOffset;
        }
    } else {
        // 2D mapping: tiles arranged in 32-tile-wide blocks
        // Each row of the tile arrangement is 32 tiles wide
        
        if (obj.paletteMode) {
            // 8bpp: tile numbers arranged in 16-tile-wide rows (because each takes 2 slots)
            tileNum = baseTileNum + (tileY * 32) + (tileX * 2);
        } else {
            // 4bpp: tile numbers arranged in 32-tile-wide rows
            tileNum = baseTileNum + (tileY * 32) + tileX;
        }
    }
    
    // Calculate actual VRAM address
    // Note: For 8bpp, each tile is still addressed as 32-byte blocks in GBA,
    // but occupies 2 blocks (64 bytes total). The tileNum calculation already
    // accounts for this by multiplying by 2.
    return OBJ_TILES_BASE + (tileNum * 32);
}

bool GPU::isSpriteOnScanline(const OBJAttributes& obj, uint16_t scanline) {
    // Check if sprite intersects with the given scanline
    if (!obj.visible) {
        return false;
    }
    
    // Handle Y coordinate wrapping (Y is 0-255, wraps around screen)
    int spriteY = obj.y;
    int spriteHeight = obj.height;
    
    // For affine sprites with double-size mode, rendering bounds are doubled
    if (obj.rotScaleFlag && obj.doubleSize) {
        spriteHeight *= 2;
    }
    
    // Check if scanline is within sprite's Y range
    // Note: Y coordinates can wrap around the 256-pixel vertical space
    if (scanline >= spriteY && scanline < (spriteY + spriteHeight)) {
        return true;
    }
    
    // Handle wraparound case (sprite starts near bottom of 256-pixel space)
    if ((spriteY + spriteHeight) > 256) {
        int wrapY = (spriteY + spriteHeight) - 256;
        if (scanline < wrapY) {
            return true;
        }
    }
    
    return false;
}

void GPU::renderSingleSprite(const OBJAttributes& obj, uint16_t scanline) {
    // Render a single sprite to the current scanline
    if (!obj.visible) {
        return;
    }
    
    // Get DISPCNT to check 1D/2D mapping mode
    DisplayControl dispcnt = readDISPCNT();
    bool mapping1D = dispcnt.obj1DMapping;
    
    // Calculate which row of the sprite we're rendering
    int spriteY = obj.y;
    int rowInSprite = scanline - spriteY;
    
    // Handle Y wraparound
    if (rowInSprite < 0) {
        rowInSprite += 256;
    }
    
    // Check if still outside sprite after wraparound
    if (rowInSprite < 0 || rowInSprite >= obj.height) {
        return;
    }
    
    // Apply vertical flip
    if (obj.vFlip) {
        rowInSprite = obj.height - 1 - rowInSprite;
    }
    
    // Calculate which tile row this corresponds to
    int tileY = rowInSprite / 8;
    int pixelYInTile = rowInSprite % 8;
    
    // Render each pixel of the sprite on this scanline
    for (int pixelX = 0; pixelX < obj.width; pixelX++) {
        // Calculate screen X position
        int screenX = obj.x + pixelX;
        
        // Handle X coordinate (0-511, but treat > 240 as offscreen)
        // X coordinates > 240 can wrap around, but we'll keep it simple for now
        if (screenX >= 511) {
            screenX -= 512;
        }
        
        // Skip if offscreen
        if (screenX < 0 || screenX >= 240) {
            continue;
        }
        
        // Apply horizontal flip
        int spritePixelX = pixelX;
        if (obj.hFlip) {
            spritePixelX = obj.width - 1 - pixelX;
        }
        
        // Calculate which tile column this corresponds to
        int tileX = spritePixelX / 8;
        int pixelXInTile = spritePixelX % 8;
        
        // Get tile address
        uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
        
        // Get pixel color index using direct VRAM access
        uint8_t* vram = memory.getVRAM();
        uint32_t vramOffset = tileAddr - 0x06000000;
        
        uint8_t colorIndex;
        if (obj.paletteMode) {
            // 8bpp mode: 1 byte per pixel
            int pixelIndex = pixelYInTile * 8 + pixelXInTile;
            colorIndex = vram[vramOffset + pixelIndex];
        } else {
            // 4bpp mode: 2 pixels per byte
            int pixelIndex = pixelYInTile * 8 + pixelXInTile;
            int byteOffset = pixelIndex / 2;
            int pixelInByte = pixelIndex % 2;
            uint8_t byte = vram[vramOffset + byteOffset];
            colorIndex = (pixelInByte == 0) ? (byte & 0x0F) : ((byte >> 4) & 0x0F);
        }
        
        // Skip transparent pixels (color index 0)
        if (colorIndex == 0) {
            continue;
        }
        
        // Get color from OBJ palette
        uint32_t color;
        if (obj.paletteMode) {
            // 8bpp: single 256-color palette
            color = getOBJColor(0, colorIndex);
        } else {
            // 4bpp: 16 palettes of 16 colors
            color = getOBJColor(obj.paletteNum, colorIndex);
        }
        
        // Convert ARGB8888 to RGB555 for framebuffer
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        uint16_t rgb555 = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
        
        // Write to framebuffer (TODO: will need priority handling later)
        int fbOffset = scanline * 240 + screenX;
        tiledFramebuffer[fbOffset] = rgb555;
    }
}

void GPU::renderSpriteScanline(uint16_t scanline) {
    // Render all sprites for the current scanline
    // Note: sprites are rendered in reverse OAM order (127 to 0)
    // Lower OAM numbers have higher priority (drawn last, appear on top)
    
    for (int objNum = 127; objNum >= 0; objNum--) {
        OBJAttributes obj = readOBJAttributes(objNum);
        
        if (isSpriteOnScanline(obj, scanline)) {
            // Check if this is an affine sprite
            if (obj.rotScaleFlag) {
                // Affine sprite - read transformation parameters and render
                AffineParams params = readAffineParams(obj.rotScaleParam);
                renderAffineSprite(obj, scanline, params);
            } else {
                // Normal sprite
                renderSingleSprite(obj, scanline);
            }
        }
    }
}

/**
 * Read affine transformation parameters from OAM
 * Params are stored at OAM offsets 0x06, 0x0E, 0x16, 0x1E... (every 32 bytes)
 * There are 32 parameter sets total (0-31)
 * Each set contains 4 words: PA, PB, PC, PD in fixed-point 8.8 format
 */
AffineParams GPU::readAffineParams(uint8_t paramIndex) {
    AffineParams params;
    
    // Each affine parameter set is 32 bytes apart in OAM
    uint32_t baseAddr = OAM_BASE + (paramIndex * 32);
    
    // Read the 4 transformation matrix values
    // They're at offsets +0x06, +0x0E, +0x16, +0x1E within each 32-byte block
    params.pa = static_cast<int16_t>(memory.read16(baseAddr + 0x06));
    params.pb = static_cast<int16_t>(memory.read16(baseAddr + 0x0E));
    params.pc = static_cast<int16_t>(memory.read16(baseAddr + 0x16));
    params.pd = static_cast<int16_t>(memory.read16(baseAddr + 0x1E));
    
    return params;
}

/**
 * Apply affine transformation to convert screen coordinates to texture coordinates
 * Matrix transformation: [textureX] = [pa pb] * [screenX]
 *                        [textureY]   [pc pd]   [screenY]
 * 
 * Fixed-point math: All matrix values and coords are in 8.8 format
 */
void GPU::applyAffineTransform(const AffineParams& params,
                                int screenX, int screenY,
                                int& textureX, int& textureY) {
    // Apply transformation matrix
    // Use 32-bit for intermediate calculation to avoid overflow
    int32_t tx = (params.pa * screenX + params.pb * screenY) >> 8;
    int32_t ty = (params.pc * screenX + params.pd * screenY) >> 8;
    
    textureX = tx;
    textureY = ty;
}

/**
 * Render an affine sprite (with rotation/scaling)
 * 
 * Affine sprites work by transforming screen coordinates back to texture coordinates:
 * 1. For each pixel on screen within sprite bounds
 * 2. Calculate position relative to sprite center
 * 3. Apply inverse transformation to get texture coordinate
 * 4. Sample texture at that coordinate
 * 5. Draw pixel if texture coordinate is valid and not transparent
 */
void GPU::renderAffineSprite(const OBJAttributes& obj, uint16_t scanline, const AffineParams& params) {
    if (!obj.visible || obj.objMode == OBJ_MODE_PROHIBITED) {
        return;
    }
    
    // Get display control for mapping mode
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    bool mapping1D = (dispcnt & DISPCNT_OBJ_1D_MAP) != 0;
    
    // Determine sprite dimensions
    int spriteWidth = obj.width;
    int spriteHeight = obj.height;
    
    // Double-size mode: rendering bounds are double the sprite size
    int renderWidth = spriteWidth;
    int renderHeight = spriteHeight;
    if (obj.doubleSize) {
        renderWidth *= 2;
        renderHeight *= 2;
    }
    
    // Calculate texture center (in texture space)
    int texCenterX = spriteWidth / 2;
    int texCenterY = spriteHeight / 2;
    
    // Calculate which part of the sprite intersects this scanline
    int spriteY = scanline - obj.y;
    if (spriteY < 0 || spriteY >= renderHeight) {
        return;  // Scanline doesn't intersect sprite
    }
    
    // Process each pixel on this scanline within sprite bounds
    for (int spriteX = 0; spriteX < renderWidth; spriteX++) {
        int screenX = obj.x + spriteX;
        
        // Clip to screen bounds
        if (screenX < 0 || screenX >= 240) {
            continue;
        }
        
        // Calculate position relative to sprite center
        int relX = spriteX - renderWidth / 2;
        int relY = spriteY - renderHeight / 2;
        
        // Apply affine transformation to get texture coordinates
        int textureX, textureY;
        applyAffineTransform(params, relX, relY, textureX, textureY);
        
        // Add texture center offset
        textureX += texCenterX;
        textureY += texCenterY;
        
        // Check if texture coordinates are within bounds
        if (textureX < 0 || textureX >= spriteWidth || 
            textureY < 0 || textureY >= spriteHeight) {
            continue;  // Outside texture bounds - transparent
        }
        
        // Calculate which tile and pixel within tile
        int tileX = textureX / 8;
        int tileY = textureY / 8;
        int pixelX = textureX % 8;
        int pixelY = textureY % 8;
        
        // Get tile address
        uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
        
        // Read pixel color index
        uint8_t colorIndex;
        uint8_t* vram = memory.getVRAM();
        uint32_t vramOffset = tileAddr - VRAM_BASE;
        
        if (obj.paletteMode) {
            // 8bpp: 1 byte per pixel
            int pixelIndex = pixelY * 8 + pixelX;
            colorIndex = vram[vramOffset + pixelIndex];
        } else {
            // 4bpp: 4 bits per pixel (2 pixels per byte)
            int pixelIndex = pixelY * 4 + pixelX / 2;
            uint8_t byte = vram[vramOffset + pixelIndex];
            if (pixelX & 1) {
                colorIndex = (byte >> 4) & 0x0F;
            } else {
                colorIndex = byte & 0x0F;
            }
        }
        
        // Transparent pixel?
        if (colorIndex == 0) {
            continue;
        }
        
        // Get color from palette
        uint16_t paletteIndex;
        if (obj.paletteMode) {
            // 8bpp: single 256-color palette
            paletteIndex = colorIndex;
        } else {
            // 4bpp: 16 palettes of 16 colors
            paletteIndex = (obj.paletteNum * 16) + colorIndex;
        }
        
        // Read color from OBJ palette (starts at 0x05000200)
        uint16_t rgb555 = memory.read16(0x05000200 + paletteIndex * 2);
        
        // Write to framebuffer (RGB555 format)
        int fbOffset = scanline * 240 + screenX;
        tiledFramebuffer[fbOffset] = rgb555;
    }
}

/**
 * Priority-aware scanline rendering
 * Combines backgrounds and sprites with correct priority layering
 * 
 * Priority rules:
 * - Lower priority number = rendered on top (0 = front, 3 = back)
 * - Within same priority: BG0 > BG1 > BG2 > BG3 > Sprites
 * - Backdrop (palette 0,0) is behind everything
 * 
 * Optimizations:
 * - Caches register reads
 * - Skips priority levels with no active layers
 * - Reuses line buffers across scanlines
 */
void GPU::renderScanline(uint16_t scanline) {
    // Skip rendering during VBlank
    if (scanline >= SCANLINES_VISIBLE) {
        return;
    }
    
    // Check for forced blank
    if (isForcedBlank()) {
        renderBlankScanline(scanline);
        return;
    }
    
    // Get DISPCNT to check which layers are enabled
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    uint16_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    // Mode 3 doesn't use priority system (direct bitmap)
    if (mode == 3) {
        renderMode3Scanline(scanline);
        return;
    }
    
    // Mode 0: Tiled backgrounds with priority system
    if (mode != 0) {
        // Other modes not implemented yet
        clearScanlineToBackdrop(scanline);
        return;
    }
    
    // Create line buffers for priority-based compositing
    uint16_t lineBuffer[240];
    uint8_t priorityBuffer[240];
    
    // 1. Fill line buffers with backdrop color (lowest priority)
    uint16_t backdrop = memory.read16(0x05000000);  // Palette 0, color 0
    for (int i = 0; i < 240; i++) {
        lineBuffer[i] = backdrop;
        priorityBuffer[i] = 255;  // Lowest possible priority
    }
    
    // 2. Render each priority level (back to front: 3 → 0)
    for (int priority = 3; priority >= 0; priority--) {
        // Render BGs with this priority (BG3 → BG0)
        for (int bg = 3; bg >= 0; bg--) {
            if (dispcnt & (DISPCNT_BG0_ENABLE << bg)) {
                // Check if this BG has the current priority
                uint16_t bgcnt = memory.read16(REG_BG0CNT + (bg * 2));
                uint8_t bgPriority = bgcnt & BGCNT_PRIORITY_MASK;
                
                if (bgPriority == priority) {
                    renderBGScanlineWithPriority(bg, scanline, lineBuffer, priorityBuffer);
                }
            }
        }
        
        // Render sprites with this priority
        if (dispcnt & DISPCNT_OBJ_ENABLE) {
            renderSpritesWithPriority(priority, scanline, lineBuffer, priorityBuffer);
        }
    }
    
    // 4. Copy line buffer to framebuffer
    int fbOffset = scanline * 240;
    for (int x = 0; x < 240; x++) {
        tiledFramebuffer[fbOffset + x] = lineBuffer[x];
    }
}

/**
 * Render a background layer to line buffer with priority checking
 * Only draws pixels if the current priority is higher than what's already drawn
 */
void GPU::renderBGScanlineWithPriority(int bgNum, uint16_t scanline, 
                                        uint16_t* lineBuffer, uint8_t* priorityBuffer) {
    // Get BG configuration
    uint16_t bgcnt = memory.read16(REG_BG0CNT + (bgNum * 2));
    BGConfig bgConfig = parseBGCNT(bgcnt);
    
    // Get scroll values
    BGScroll scroll = readBGScroll(bgNum);
    
    // Calculate priority value for this layer
    // Lower priority number = higher priority = drawn on top
    // Within same priority level: BG0 (0) > BG1 (1) > BG2 (2) > BG3 (3)
    uint8_t layerPriority = (bgConfig.priority * 4) + bgNum;
    
    // For each pixel on this scanline
    for (int screenX = 0; screenX < 240; screenX++) {
        // Apply scrolling
        int bgX, bgY;
        applyScroll(bgConfig, scroll, screenX, scanline, bgX, bgY);
        
        // Get tile coordinates
        int tileX, tileY, pixelInTileX, pixelInTileY;
        getTileCoords(bgX, bgY, tileX, tileY, pixelInTileX, pixelInTileY);
        
        // Get screen entry
        ScreenEntry entry = readScreenEntry(bgConfig, tileX, tileY);
        
        // Get tile address
        uint32_t tileAddr = getTileAddress(bgConfig, entry);
        
        // Read pixel color index
        uint8_t colorIndex;
        if (bgConfig.paletteMode) {
            // 8bpp mode
            int pixelOffset = pixelInTileY * 8 + pixelInTileX;
            colorIndex = memory.read8(tileAddr + pixelOffset);
        } else {
            // 4bpp mode
            int byteOffset = pixelInTileY * 4 + pixelInTileX / 2;
            uint8_t byte = memory.read8(tileAddr + byteOffset);
            if (pixelInTileX & 1) {
                colorIndex = (byte >> 4) & 0x0F;
            } else {
                colorIndex = byte & 0x0F;
            }
        }
        
        // Color 0 is transparent - skip it
        if (colorIndex == 0) {
            continue;
        }
        
        // Only draw if this pixel has higher or equal priority
        if (layerPriority <= priorityBuffer[screenX]) {
            // Get actual color from palette
            uint16_t paletteIndex;
            if (bgConfig.paletteMode) {
                paletteIndex = colorIndex;
            } else {
                paletteIndex = (entry.paletteNum * 16) + colorIndex;
            }
            
            uint16_t rgb555 = memory.read16(0x05000000 + paletteIndex * 2);
            
            // Update line buffer and priority
            lineBuffer[screenX] = rgb555;
            priorityBuffer[screenX] = layerPriority;
        }
    }
}

/**
 * Render all sprites with a specific priority to line buffer
 * Only draws sprite pixels if they have higher or equal priority than what's already drawn
 */
void GPU::renderSpritesWithPriority(uint8_t priority, uint16_t scanline, 
                                     uint16_t* lineBuffer, uint8_t* priorityBuffer) {
    // Get DISPCNT for mapping mode
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    bool mapping1D = (dispcnt & DISPCNT_OBJ_1D_MAP) != 0;
    
    // Render sprites in reverse OAM order (127 → 0)
    // Lower OAM number = higher priority within same priority level
    for (int objNum = 127; objNum >= 0; objNum--) {
        OBJAttributes obj = readOBJAttributes(objNum);
        
        if (!isSpriteOnScanline(obj, scanline)) {
            continue;
        }
        
        // Only process sprites with matching priority
        if (obj.priority != priority) {
            continue;
        }
        
        // Calculate priority value
        // Priority layout: (priority * 4) + 4 (sprites are after BGs within same priority)
        // This makes sprites render after BGs of the same priority level
        uint8_t layerPriority = (priority * 4) + 4;
        
        // Handle affine vs normal sprites
        if (obj.rotScaleFlag) {
            // Affine sprite
            AffineParams params = readAffineParams(obj.rotScaleParam);
            renderAffineSpriteWithPriority(obj, scanline, params, lineBuffer, priorityBuffer, layerPriority, mapping1D);
        } else {
            // Normal sprite
            renderNormalSpriteWithPriority(obj, scanline, lineBuffer, priorityBuffer, layerPriority, mapping1D);
        }
    }
}

/**
 * Render a normal (non-affine) sprite with priority checking
 */
void GPU::renderNormalSpriteWithPriority(const OBJAttributes& obj, uint16_t scanline,
                                          uint16_t* lineBuffer, uint8_t* priorityBuffer,
                                          uint8_t layerPriority, bool mapping1D) {
    if (!obj.visible || obj.objMode == OBJ_MODE_PROHIBITED) {
        return;
    }
    
    // Calculate which row of the sprite we're rendering
    int spriteY = obj.y;
    int rowInSprite = scanline - spriteY;
    
    // Handle Y wraparound (Y can be 0-255, treat 160-255 as negative)
    if (rowInSprite < 0) {
        rowInSprite += 256;
    }
    
    // Check if still outside sprite after wraparound
    if (rowInSprite < 0 || rowInSprite >= obj.height) {
        return;
    }
    
    // Apply vertical flip
    if (obj.vFlip) {
        rowInSprite = obj.height - 1 - rowInSprite;
    }
    
    // For each pixel in this sprite row
    for (int spriteX = 0; spriteX < obj.width; spriteX++) {
        // Calculate screen X position (handle X wraparound)
        int screenX = obj.x + spriteX;
        if (screenX >= 511) {
            screenX -= 512;
        }
        
        // Clip to screen bounds
        if (screenX < 0 || screenX >= 240) {
            continue;
        }
        
        // Apply horizontal flip
        int actualSpriteX = obj.hFlip ? (obj.width - 1 - spriteX) : spriteX;
        
        // Calculate tile and pixel coordinates
        int tileX = actualSpriteX / 8;
        int tileY = rowInSprite / 8;
        int pixelX = actualSpriteX % 8;
        int pixelY = rowInSprite % 8;
        
        // Get tile address
        uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
        
        // Read pixel color index
        uint8_t colorIndex;
        uint8_t* vram = memory.getVRAM();
        uint32_t vramOffset = tileAddr - VRAM_BASE;
        
        if (obj.paletteMode) {
            // 8bpp
            int pixelIndex = pixelY * 8 + pixelX;
            colorIndex = vram[vramOffset + pixelIndex];
        } else {
            // 4bpp
            int pixelIndex = pixelY * 4 + pixelX / 2;
            uint8_t byte = vram[vramOffset + pixelIndex];
            if (pixelX & 1) {
                colorIndex = (byte >> 4) & 0x0F;
            } else {
                colorIndex = byte & 0x0F;
            }
        }
        
        // Transparent pixel?
        if (colorIndex == 0) {
            continue;
        }
        
        // Only draw if this pixel has higher or equal priority
        if (layerPriority > priorityBuffer[screenX]) {
            continue;
        }
        
        // Get color from palette
        uint16_t paletteIndex;
        if (obj.paletteMode) {
            paletteIndex = colorIndex;
        } else {
            paletteIndex = (obj.paletteNum * 16) + colorIndex;
        }
        
        uint16_t rgb555 = memory.read16(0x05000200 + paletteIndex * 2);
        
        // Update line buffer and priority
        lineBuffer[screenX] = rgb555;
        priorityBuffer[screenX] = layerPriority;
    }
}

/**
 * Render an affine sprite with priority checking
 */
void GPU::renderAffineSpriteWithPriority(const OBJAttributes& obj, uint16_t scanline,
                                          const AffineParams& params, uint16_t* lineBuffer,
                                          uint8_t* priorityBuffer, uint8_t layerPriority,
                                          bool mapping1D) {
    if (!obj.visible || obj.objMode == OBJ_MODE_PROHIBITED) {
        return;
    }
    
    // Determine sprite dimensions
    int spriteWidth = obj.width;
    int spriteHeight = obj.height;
    
    // Double-size mode: rendering bounds are double the sprite size
    int renderWidth = spriteWidth;
    int renderHeight = spriteHeight;
    if (obj.doubleSize) {
        renderWidth *= 2;
        renderHeight *= 2;
    }
    
    // Calculate texture center
    int texCenterX = spriteWidth / 2;
    int texCenterY = spriteHeight / 2;
    
    // Calculate which part of the sprite intersects this scanline
    int spriteY = obj.y;
    int rowInSprite = scanline - spriteY;
    
    // Handle Y wraparound
    if (rowInSprite < 0) {
        rowInSprite += 256;
    }
    
    // Check if still outside sprite after wraparound
    if (rowInSprite < 0 || rowInSprite >= renderHeight) {
        return;
    }
    
    // Process each pixel on this scanline within sprite bounds
    for (int spriteX = 0; spriteX < renderWidth; spriteX++) {
        // Calculate screen X position (handle X wraparound)
        int screenX = obj.x + spriteX;
        if (screenX >= 511) {
            screenX -= 512;
        }
        
        // Clip to screen bounds
        if (screenX < 0 || screenX >= 240) {
            continue;
        }
        
        // Only draw if this pixel has higher or equal priority
        if (layerPriority > priorityBuffer[screenX]) {
            continue;
        }
        
        // Calculate position relative to sprite center
        int relX = spriteX - renderWidth / 2;
        int relY = rowInSprite - renderHeight / 2;
        
        // Apply affine transformation
        int textureX, textureY;
        applyAffineTransform(params, relX, relY, textureX, textureY);
        
        // Add texture center offset
        textureX += texCenterX;
        textureY += texCenterY;
        
        // Check bounds
        if (textureX < 0 || textureX >= spriteWidth || 
            textureY < 0 || textureY >= spriteHeight) {
            continue;
        }
        
        // Calculate tile and pixel coordinates
        int tileX = textureX / 8;
        int tileY = textureY / 8;
        int pixelX = textureX % 8;
        int pixelY = textureY % 8;
        
        // Get tile address
        uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
        
        // Read pixel color index
        uint8_t colorIndex;
        uint8_t* vram = memory.getVRAM();
        uint32_t vramOffset = tileAddr - VRAM_BASE;
        
        if (obj.paletteMode) {
            // 8bpp
            int pixelIndex = pixelY * 8 + pixelX;
            colorIndex = vram[vramOffset + pixelIndex];
        } else {
            // 4bpp
            int pixelIndex = pixelY * 4 + pixelX / 2;
            uint8_t byte = vram[vramOffset + pixelIndex];
            if (pixelX & 1) {
                colorIndex = (byte >> 4) & 0x0F;
            } else {
                colorIndex = byte & 0x0F;
            }
        }
        
        // Transparent pixel?
        if (colorIndex == 0) {
            continue;
        }
        
        // Get color from palette
        uint16_t paletteIndex;
        if (obj.paletteMode) {
            paletteIndex = colorIndex;
        } else {
            paletteIndex = (obj.paletteNum * 16) + colorIndex;
        }
        
        uint16_t rgb555 = memory.read16(0x05000200 + paletteIndex * 2);
        
        // Update line buffer and priority
        lineBuffer[screenX] = rgb555;
        priorityBuffer[screenX] = layerPriority;
    }
}

// ============================================================================
// Session 3: Advanced Features - Blend and Window Support
// ============================================================================

BlendControl GPU::readBlendControl() {
    BlendControl blend = {};
    
    uint16_t bldcnt = memory.read16(REG_BLDCNT);
    uint16_t bldalpha = memory.read16(REG_BLDALPHA);
    uint16_t bldy = memory.read16(REG_BLDY);
    
    // Parse BLDCNT
    blend.firstTargets = bldcnt & 0x3F;              // Bits 0-5
    blend.mode = (bldcnt >> 6) & 0x03;               // Bits 6-7
    blend.secondTargets = (bldcnt >> 8) & 0x3F;      // Bits 8-13
    
    // Parse BLDALPHA
    blend.eva = bldalpha & 0x1F;                     // Bits 0-4
    blend.evb = (bldalpha >> 8) & 0x1F;              // Bits 8-12
    
    // Clamp to valid range (0-16)
    if (blend.eva > 16) blend.eva = 16;
    if (blend.evb > 16) blend.evb = 16;
    
    // Parse BLDY
    blend.evy = bldy & 0x1F;                         // Bits 0-4
    if (blend.evy > 16) blend.evy = 16;
    
    return blend;
}

WindowControl GPU::readWindowControl() {
    WindowControl winCtrl = {};
    
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    
    // Window 0
    winCtrl.win0.enabled = (dispcnt & DISPCNT_WIN0_ENABLE) != 0;
    if (winCtrl.win0.enabled) {
        uint16_t win0h = memory.read16(REG_WIN0H);
        uint16_t win0v = memory.read16(REG_WIN0V);
        
        winCtrl.win0.right = win0h & 0xFF;           // Bits 0-7
        winCtrl.win0.left = (win0h >> 8) & 0xFF;     // Bits 8-15
        winCtrl.win0.bottom = win0v & 0xFF;          // Bits 0-7
        winCtrl.win0.top = (win0v >> 8) & 0xFF;      // Bits 8-15
        
        uint16_t winin = memory.read16(REG_WININ);
        winCtrl.win0.control = winin & 0x3F;         // Bits 0-5
    }
    
    // Window 1
    winCtrl.win1.enabled = (dispcnt & DISPCNT_WIN1_ENABLE) != 0;
    if (winCtrl.win1.enabled) {
        uint16_t win1h = memory.read16(REG_WIN1H);
        uint16_t win1v = memory.read16(REG_WIN1V);
        
        winCtrl.win1.right = win1h & 0xFF;
        winCtrl.win1.left = (win1h >> 8) & 0xFF;
        winCtrl.win1.bottom = win1v & 0xFF;
        winCtrl.win1.top = (win1v >> 8) & 0xFF;
        
        uint16_t winin = memory.read16(REG_WININ);
        winCtrl.win1.control = (winin >> 8) & 0x3F;  // Bits 8-13
    }
    
    // Outside window control
    uint16_t winout = memory.read16(REG_WINOUT);
    winCtrl.winOut = winout & 0x3F;                  // Bits 0-5
    winCtrl.winObj = (winout >> 8) & 0x3F;           // Bits 8-13
    
    return winCtrl;
}

bool GPU::isPixelInWindow(int x, int y, const Window& win) {
    if (!win.enabled) {
        return false;
    }
    
    // Handle wraparound for horizontal coordinates
    bool inX;
    if (win.right >= win.left) {
        inX = (x >= win.left && x < win.right);
    } else {
        // Wraparound case (e.g., left=200, right=50 means 200-239 and 0-49)
        inX = (x >= win.left || x < win.right);
    }
    
    // Handle wraparound for vertical coordinates
    bool inY;
    if (win.bottom >= win.top) {
        inY = (y >= win.top && y < win.bottom);
    } else {
        // Wraparound case
        inY = (y >= win.top || y < win.bottom);
    }
    
    return inX && inY;
}

uint8_t GPU::getWindowControlForPixel(int x, int y, const WindowControl& winCtrl) {
    // Check Window 0 first (highest priority)
    if (winCtrl.win0.enabled && isPixelInWindow(x, y, winCtrl.win0)) {
        return winCtrl.win0.control;
    }
    
    // Check Window 1
    if (winCtrl.win1.enabled && isPixelInWindow(x, y, winCtrl.win1)) {
        return winCtrl.win1.control;
    }
    
    // Outside all windows
    return winCtrl.winOut;
}

bool GPU::isLayerVisibleAtPixel(int layerType, int x, int y) {
    uint16_t dispcnt = memory.read16(REG_DISPCNT);
    
    // If no windows enabled, layer is visible if enabled in DISPCNT
    bool anyWindowEnabled = (dispcnt & (DISPCNT_WIN0_ENABLE | DISPCNT_WIN1_ENABLE)) != 0;
    if (!anyWindowEnabled) {
        return true;  // Windows not used, rely on DISPCNT only
    }
    
    // Get window control for this pixel
    WindowControl winCtrl = readWindowControl();
    uint8_t control = getWindowControlForPixel(x, y, winCtrl);
    
    // Check if layer is enabled in window control
    // layerType: 0=BG0, 1=BG1, 2=BG2, 3=BG3, 4=OBJ, 5=Backdrop
    if (layerType >= 0 && layerType <= 3) {
        return (control & (1 << layerType)) != 0;
    } else if (layerType == 4) {
        return (control & WIN_OBJ_ENABLE) != 0;
    }
    
    return true;  // Backdrop always visible
}

uint16_t GPU::applyBrightnessIncrease(uint16_t color, uint8_t evy) {
    // Extract RGB components (5 bits each)
    uint8_t r = color & 0x1F;
    uint8_t g = (color >> 5) & 0x1F;
    uint8_t b = (color >> 10) & 0x1F;
    
    // Apply brightness increase: color + (31 - color) * evy / 16
    r = r + ((31 - r) * evy) / 16;
    g = g + ((31 - g) * evy) / 16;
    b = b + ((31 - b) * evy) / 16;
    
    // Clamp to 5-bit range
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    
    return r | (g << 5) | (b << 10);
}

uint16_t GPU::applyBrightnessDecrease(uint16_t color, uint8_t evy) {
    // Extract RGB components (5 bits each)
    int r = color & 0x1F;
    int g = (color >> 5) & 0x1F;
    int b = (color >> 10) & 0x1F;
    
    // Apply brightness decrease: color - color * evy / 16
    // Use int arithmetic to avoid underflow
    r = r - (r * evy) / 16;
    g = g - (g * evy) / 16;
    b = b - (b * evy) / 16;
    
    // Clamp to valid range (shouldn't go negative, but be safe)
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    
    return (uint16_t)r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
}

uint16_t GPU::applyBlend(uint16_t color1, uint16_t color2, const BlendControl& blend, 
                         int layerType1, int layerType2) {
    // Check if blending is disabled
    if (blend.mode == BLEND_MODE_OFF) {
        return color1;
    }
    
    // Check if layer 1 is a first target
    bool isFirstTarget = (blend.firstTargets & (1 << layerType1)) != 0;
    if (!isFirstTarget) {
        return color1;  // No blending if not a first target
    }
    
    switch (blend.mode) {
        case BLEND_MODE_ALPHA: {
            // Alpha blend: color1 * eva + color2 * evb
            // Only if layer 2 is a second target
            bool isSecondTarget = (blend.secondTargets & (1 << layerType2)) != 0;
            if (!isSecondTarget) {
                return color1;
            }
            
            // Extract RGB components from both colors
            uint8_t r1 = color1 & 0x1F;
            uint8_t g1 = (color1 >> 5) & 0x1F;
            uint8_t b1 = (color1 >> 10) & 0x1F;
            
            uint8_t r2 = color2 & 0x1F;
            uint8_t g2 = (color2 >> 5) & 0x1F;
            uint8_t b2 = (color2 >> 10) & 0x1F;
            
            // Apply alpha blend formula
            uint8_t r = (r1 * blend.eva + r2 * blend.evb) / 16;
            uint8_t g = (g1 * blend.eva + g2 * blend.evb) / 16;
            uint8_t b = (b1 * blend.eva + b2 * blend.evb) / 16;
            
            // Clamp to 5-bit range
            if (r > 31) r = 31;
            if (g > 31) g = 31;
            if (b > 31) b = 31;
            
            return r | (g << 5) | (b << 10);
        }
        
        case BLEND_MODE_BRIGHTEN:
            return applyBrightnessIncrease(color1, blend.evy);
        
        case BLEND_MODE_DARKEN:
            return applyBrightnessDecrease(color1, blend.evy);
        
        default:
            return color1;
    }
}
