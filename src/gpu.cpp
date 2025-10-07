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
    if (paletteNum < 0 || paletteNum >= 16 || colorIndex < 0 || colorIndex >= 16) {
        return 0;
    }
    
    uint32_t offset = 0x200 + (paletteNum * 16 + colorIndex) * 2;
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
