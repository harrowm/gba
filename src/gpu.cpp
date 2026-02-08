#include "gpu.h"
#include "memory.h"
#include "scheduler.h"
#include "debug.h"
#include "utility_macros.h"
#include <cstdint>
#include <cstring>
#include <set>

extern uint32_t g_current_frame;
static uint32_t g_gpu_scanlines_this_frame = 0;  // Count scanlines processed per frame

GPU::GPU(Memory& mem) 
    : memory(mem), currentScanline(0), inVBlank(false), inHBlank(false) {
    // Initialize GPU state
    // Clear the tiled framebuffer
    memset(tiledFramebuffer, 0, sizeof(tiledFramebuffer));
    
    // Initialize VCOUNT to 0 (some tests check this before any scanlines run)
    memory.write16(REG_VCOUNT, 0);
    
    DEBUG_INFO("GPU initialized");
}

// Helper to schedule one complete scanline
void GPU::scheduleScanline(Scheduler* scheduler) {
    static int schedule_count = 0;
    uint64_t currentCycle = scheduler->getCurrentCycle();
    if (schedule_count < 10) {
        LOG_TRACE_CAT("[GPU SCHEDULE] Scanline %d scheduled at cycle %llu (HDRAW ends at %llu)\n",
               currentScanline, currentCycle, currentCycle + CYCLES_HDRAW);
        schedule_count++;
    }
    
    // Schedule H-Draw completion
    scheduler->schedule(CYCLES_HDRAW, [this, scheduler]() {
        // H-Draw complete, enter H-Blank
        inHBlank = true;
        // Use readDirectIO16/writeDirectIO to avoid adding spurious wait
        // cycles to the scheduler.  The GPU is a hardware component, not a
        // CPU bus master, so its register updates must be zero-cost.
        uint16_t dispstat = memory.readDirectIO16(REG_DISPSTAT);
        dispstat |= DISPSTAT_HBLANK;
        memory.writeDirectIO(REG_DISPSTAT, dispstat);
        
        // HBlank callback triggers both IRQ and DMA.
        // During visible scanlines (0-159): trigger both DMA and IRQ
        // During VBlank (160-227): HBlank DMA should NOT trigger
        // The callback sets isVisibleScanline so DMA can decide
        if (hblankCallback) {
            hblankCallback();
        }

        if (currentScanline < SCANLINES_VISIBLE) {
            // GPU rendering is a hardware operation — it reads VRAM, OAM, and
            // palette directly without consuming CPU bus cycles.  Disable wait
            // cycle accounting during renderScanline() so the scheduler isn't
            // polluted with spurious cycles.
            memory.setWaitCyclesBypass(true);
            renderScanline();
            memory.setWaitCyclesBypass(false);
        }
        
        // Schedule H-Blank end
        scheduler->schedule(CYCLES_HBLANK, [this, scheduler]() {
            inHBlank = false;
            uint16_t dispstat = memory.readDirectIO16(REG_DISPSTAT);
            dispstat &= ~DISPSTAT_HBLANK;
            memory.writeDirectIO(REG_DISPSTAT, dispstat);
            
            // Move to next scanline
            currentScanline++;
            g_gpu_scanlines_this_frame++;
            if (currentScanline >= SCANLINES_TOTAL) {
                currentScanline = 0;
            }
            memory.writeDirectIO(REG_VCOUNT, currentScanline);
            
            // Debug
            if (currentScanline >= 158 && currentScanline <= 162) {
                LOG_TRACE_CAT("[GPU] Scanline %d starts at cycle %llu\n", 
                       currentScanline, scheduler->getCurrentCycle());
            }
            
            // Handle V-Blank transition
            if (currentScanline == SCANLINES_VISIBLE) {
                inVBlank = true;
                dispstat = memory.readDirectIO16(REG_DISPSTAT);
                dispstat |= DISPSTAT_VBLANK;
                memory.writeDirectIO(REG_DISPSTAT, dispstat);
                
                g_gpu_scanlines_this_frame = 0;
                
                // Always call vblankCallback for DMA triggering
                // The interrupt controller internally checks if VBlank IRQ is enabled
                if (vblankCallback) {
                    vblankCallback();
                }
            } else if (currentScanline == 0) {
                inVBlank = false;
                dispstat = memory.readDirectIO16(REG_DISPSTAT);
                dispstat &= ~DISPSTAT_VBLANK;
                memory.writeDirectIO(REG_DISPSTAT, dispstat);
            }
            
            // Schedule next scanline immediately (new scanline starts now)
            scheduleScanline(scheduler);
        }, EventType::VIDEO_SCANLINE, 1);
    }, EventType::VIDEO_HBLANK, 1);
}

void GPU::setupTiming(Scheduler* scheduler) {
    if (!scheduler) {
        DEBUG_ERROR("GPU::setupTiming called with null scheduler");
        return;
    }
    
    uint64_t currentCycle = scheduler->getCurrentCycle();
    LOG_TRACE_CAT("[GPU SETUP] Setting up GPU video timing at cycle %llu\n", currentCycle);
    scheduleScanline(scheduler);
}

void GPU::renderScanline() {
    // Debug: trace scanlines periodically
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    
    // SEGA logo effect debug: log BG scroll registers during frames 30-90
    // to understand the scrolling/trail effect
    #define SEGA_LOGO_DEBUG 0
    #if SEGA_LOGO_DEBUG
    if (g_current_frame >= 30 && g_current_frame <= 60) {
        // Log once per frame at specific scanlines to see variation
        if (currentScanline == 0 || currentScanline == 40 || currentScanline == 80 || currentScanline == 120) {
            uint16_t dispcnt_dbg = memory.readDirectIO16(REG_DISPCNT);
            int mode_dbg = dispcnt_dbg & 0x7;
            uint16_t bg0hofs = memory.readDirectIO16(REG_BG0HOFS) & 0x1FF;
            uint16_t bg0vofs = memory.readDirectIO16(REG_BG0VOFS) & 0x1FF;
            uint16_t bg1hofs = memory.readDirectIO16(REG_BG1HOFS) & 0x1FF;
            uint16_t bg1vofs = memory.readDirectIO16(REG_BG1VOFS) & 0x1FF;
            uint16_t bg2hofs = memory.readDirectIO16(REG_BG2HOFS) & 0x1FF;
            uint16_t bg2vofs = memory.readDirectIO16(REG_BG2VOFS) & 0x1FF;
            fprintf(stderr, "[SEGA F%d S%d] Mode=%d BG0(%d,%d) BG1(%d,%d) BG2(%d,%d)\n",
                    g_current_frame, currentScanline, mode_dbg, bg0hofs, bg0vofs, 
                    bg1hofs, bg1vofs, bg2hofs, bg2vofs);
        }
    }
    #endif
    
    // Skip rendering during VBlank
    if (currentScanline >= SCANLINES_VISIBLE) {
        return;
    }
    
    // Check for forced blank
    if (isForcedBlank()) {
        renderBlankScanline(currentScanline);
        return;
    }
    
    // Check video mode from DISPCNT (reuse from debug above)
    uint16_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    // Debug: Log DISPCNT value once per frame (on scanline 0)
    static int dispcntLogCounter = 0;
    static int lastMode2WithBG3Frame = 0;
    if (currentScanline == 0) {
        // Check if this is Mode 2 with BG3 enabled
        bool isMode2WithBG3 = (mode == 2) && (dispcnt & (1 << 11));
        
        if (dispcntLogCounter++ < 10 || (isMode2WithBG3 && lastMode2WithBG3Frame < 5)) {
            if (isMode2WithBG3) lastMode2WithBG3Frame++;
            LOG_TRACE_CAT("[GPU] Scanline 0: DISPCNT=0x%04X, Mode=%d, BGs=%d%d%d%d, OBJ=%d\n",
                   dispcnt, mode,
                   (dispcnt >> 8) & 1, (dispcnt >> 9) & 1, (dispcnt >> 10) & 1, (dispcnt >> 11) & 1,
                   (dispcnt >> 12) & 1);
            
            // If Mode 2 with BG3, log the affine parameters
            if (isMode2WithBG3) {
                int16_t bg3pa = (int16_t)memory.readDirectIO16(0x04000030);
                int16_t bg3pb = (int16_t)memory.readDirectIO16(0x04000032);
                int16_t bg3pc = (int16_t)memory.readDirectIO16(0x04000034);
                int16_t bg3pd = (int16_t)memory.readDirectIO16(0x04000036);
                uint32_t bg3x = memory.readDirectIO32(0x04000038);
                uint32_t bg3y = memory.readDirectIO32(0x0400003C);
                uint16_t bg3cnt = memory.readDirectIO16(0x0400000E);
                LOG_TRACE_CAT("[GPU] BG3 Affine: PA=%d PB=%d PC=%d PD=%d X=0x%08X Y=0x%08X BGCNT=0x%04X\n",
                       bg3pa, bg3pb, bg3pc, bg3pd, bg3x, bg3y, bg3cnt);
            }
        }
        
        // Debug: Log OAM for all visible sprites to identify the sweeping highlight
        static int oamLogFrames = 0;
        if (oamLogFrames >= 60 && oamLogFrames < 260) { // Log frames 60-260 (when animation happens)
            if ((oamLogFrames - 60) % 10 == 0) { // Log every 10th frame
                LOG_TRACE_CAT("\n[OAM FRAME %d]\n", oamLogFrames);
                for (int objNum = 0; objNum < 16; objNum++) { // Check first 16 sprites
                    uint32_t oamAddr = OAM_BASE + (objNum * 8);
                    uint16_t attr0 = memory.readDirectIO16(oamAddr + 0);
                    uint16_t attr1 = memory.readDirectIO16(oamAddr + 2);
                    uint16_t attr2 = memory.readDirectIO16(oamAddr + 4);
                    
                    // Check if sprite is enabled (not disabled by bit 9 when not affine)
                    bool isAffine = (attr0 & (1 << 8)) != 0;
                    bool isDisabled = !isAffine && ((attr0 & (1 << 9)) != 0);
                    if (isDisabled) continue;
                    
                    int y = attr0 & 0xFF;
                    int objMode = (attr0 >> 10) & 0x3; // 0=Normal, 1=Semi-Transparent, 2=Obj Window, 3=Prohibited
                    int gfxMode = (attr0 >> 8) & 0x3; // 0=Normal, 1=Affine, 2=Disabled, 3=Affine Double-Size
                    int shape = (attr0 >> 14) & 0x3; // 0=Square, 1=Horizontal, 2=Vertical
                    
                    int x = attr1 & 0x1FF;
                    int size = (attr1 >> 14) & 0x3;
                    
                    int tile = attr2 & 0x3FF;
                    int priority = (attr2 >> 10) & 0x3;
                    int palette = (attr2 >> 12) & 0xF;
                    
                    // Log all sprites regardless of Y position to catch animation
                    LOG_TRACE_CAT("  OBJ%d: X=%3d Y=%3d ObjMode=%d GfxMode=%d Shape=%d Size=%d Tile=%d Prio=%d Pal=%d\n",
                           objNum, x, y, objMode, gfxMode, shape, size, tile, priority, palette);
                }
            }
        }
        oamLogFrames++;
    }
    
    // Initialize sprite ordering buffer
    // 0xFF000000 means no sprite has written to this pixel yet
    for (int i = 0; i < 240; i++) {
        spriteOrderLayer[i] = 0xFF000000;
    }

    switch (mode) {
        case 0:
            // Mode 0: Use priority-aware renderer for proper sprite support
            renderScanline(currentScanline);
            break;
        case 1:
        case 2:
            // Mode 1: BG0, BG1 (regular), BG2 (affine)
            // Mode 2: BG2, BG3 (affine) + sprites
            // Use priority-aware renderer for sprite support
            renderScanline(currentScanline);
            break;
        case 3:
            renderMode3Scanline(currentScanline);
            break;
        case 4:
            renderMode4Scanline(currentScanline);
            break;
        // Mode 5 not yet implemented
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

void GPU::renderMode4Scanline(uint16_t scanline) {
    if (scanline >= SCANLINES_VISIBLE) {
        return; // Don't render during V-Blank
    }
    
    // Mode 4: 240x160 @ 8bpp indexed color bitmap (requires BG2 enabled)
    // Two frame buffers at 0x06000000 and 0x0600A000 (selected by DISPCNT bit 4)
    // Each pixel is 1 byte (palette index 0-255)
    // Output to tiledFramebuffer as RGB555
    
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    
    // Check if BG2 is enabled (bit 10)
    bool bg2Enabled = (dispcnt & (1 << 10)) != 0;
    if (!bg2Enabled) {
        // BG2 not enabled, render backdrop
        clearScanlineToBackdrop(scanline);
        return;
    }
    
    bool useSecondFrame = (dispcnt & (1 << 4)) != 0; // Bit 4: frame select
    
    uint8_t* vram = memory.getVRAM();
    uint32_t frameOffset = useSecondFrame ? 0xA000 : 0x0000;
    uint32_t scanlineOffset = scanline * 240; // 240 pixels per scanline, 1 byte each
    
    // Output scanline starts at tiledFramebuffer[scanline * 240]
    uint16_t* output = &tiledFramebuffer[scanline * 240];
    
    // Debug: Check BOTH frame buffers for non-zero pixels
    static int debugFrameCount = 0;
    if (debugFrameCount < 3 && scanline == 76) {
        LOG_TRACE_CAT("[Mode 4 Debug Frame %d] Scanline 76, useSecondFrame=%d\n", debugFrameCount, useSecondFrame);
        
        // Check both frame buffers
        for (int frame = 0; frame < 2; frame++) {
            uint32_t frameOff = frame ? 0xA000 : 0x0000;
            int nonZeroCount = 0;
            LOG_TRACE_CAT("  Frame %d (offset 0x%05X): ", frame, frameOff);
            for (int i = 0; i < 240; i++) {
                uint8_t idx = vram[frameOff + scanlineOffset + i];
                if (idx != 0) {
                    nonZeroCount++;
                    if (nonZeroCount <= 10) {
                        LOG_TRACE_CAT("[X%d=idx%d] ", i, idx);
                    }
                }
            }
            LOG_TRACE_CAT(" -> %d non-zero pixels\n", nonZeroCount);
        }
        debugFrameCount++;
    }
    
    for (int x = 0; x < 240; x++) {
        uint8_t paletteIndex = vram[frameOffset + scanlineOffset + x];
        // Look up color in BG palette (8bpp mode uses full 256-color palette)
        uint16_t color = readBGPaletteRaw(0, paletteIndex);
        output[x] = color;
    }
}

uint16_t* GPU::getFrameBuffer() {
    // Return appropriate framebuffer based on video mode
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    uint16_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    if (mode == 3) {
        // Mode 3: Direct 16bpp bitmap in VRAM
        uint8_t* vram_ptr = memory.getVRAM();
        static int debugCount = 0;
        if (debugCount < 3) {
            LOG_TRACE_CAT("[GPU::getFrameBuffer] Mode %d, returning VRAM=%p\n", mode, (void*)vram_ptr);
            debugCount++;
        }
        return reinterpret_cast<uint16_t*>(vram_ptr);
    } else if (mode == 4) {
        // Mode 4: 8bpp indexed, rendered to tiledFramebuffer
        return tiledFramebuffer;
    } else if (mode == 5) {
        // Mode 5: 16bpp bitmap (160x128), VRAM is the framebuffer
        uint8_t* vram_ptr = memory.getVRAM();
        return reinterpret_cast<uint16_t*>(vram_ptr);
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
        
        // Debug: Log palette 0, color 3 reads
        static int debugCount = 0;
        if (paletteNum == 0 && colorIndex == 3 && debugCount < 3) {
            uint16_t* paletteRAM = reinterpret_cast<uint16_t*>(memory.getPaletteRAM());
            uint16_t value = paletteRAM[offset / 2];
            LOG_TRACE_CAT("[Palette Debug] Reading BG palette[0][3]: offset=%d, value=0x%04X\n", offset, value);
            debugCount++;
        }
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
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    return parseDISPCNT(dispcnt);
}

bool GPU::isBGEnabled(int bgNum) {
    // Check if a specific background is enabled (0-3)
    if (bgNum < 0 || bgNum > 3) {
        return false;
    }
    
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    uint16_t bgBit = DISPCNT_BG0_ENABLE << bgNum;  // BG0=bit 8, BG1=bit 9, etc.
    
    return (dispcnt & bgBit) != 0;
}

bool GPU::isForcedBlank() {
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    return (dispcnt & DISPCNT_FORCED_BLANK) != 0;
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
    uint16_t bgcnt = memory.readDirectIO16(bgcntAddr);
    
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
    
    return memory.readDirectIO16(addr);
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
    scroll.hofs = memory.readDirectIO16(hofsAddr) & 0x01FF;
    scroll.vofs = memory.readDirectIO16(vofsAddr) & 0x01FF;
    
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
    
    uint16_t attr0 = memory.readDirectIO16(oamAddr + 0);
    uint16_t attr1 = memory.readDirectIO16(oamAddr + 2);
    uint16_t attr2 = memory.readDirectIO16(oamAddr + 4);
    
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
    params.pa = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 0x06));
    params.pb = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 0x0E));
    params.pc = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 0x16));
    params.pd = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 0x1E));
    
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
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    uint16_t mode = dispcnt & DISPCNT_MODE_MASK;
    
    // Mode 3 doesn't use priority system (direct bitmap)
    if (mode == 3) {
        renderMode3Scanline(scanline);
        return;
    }
    
    // Mode 0, Mode 1 and Mode 2: Tiled backgrounds with priority system and sprites
    // Mode 0: BG0-3 (regular tiled)
    // Mode 1: BG0-1 (regular tiled) + BG2 (affine)
    // Mode 2: BG2-3 (affine) + sprites
    if (mode != 0 && mode != 1 && mode != 2) {
        // Other modes not implemented yet
        clearScanlineToBackdrop(scanline);
        return;
    }

    // Read blend and window control once per scanline (optimization)
    BlendControl blend = readBlendControl();
    WindowControl winCtrl = readWindowControl();
    


    bool windowsEnabled = (dispcnt & (DISPCNT_WIN0_ENABLE | DISPCNT_WIN1_ENABLE | DISPCNT_WINOBJ_ENABLE)) != 0;
    
    // Create line buffers for priority-based compositing
    uint16_t lineBuffer[240];
    uint8_t priorityBuffer[240];
    uint8_t layerTypeBuffer[240];  // Track which layer type each pixel came from
    
    // Second layer tracking for alpha blending
    uint16_t secondLayerBuffer[240];
    uint8_t secondLayerTypeBuffer[240];
    
    // 1. Fill line buffers with backdrop color (lowest priority)
    uint16_t backdrop = memory.readDirectIO16(0x05000000);  // Palette 0, color 0
    
    for (int i = 0; i < 240; i++) {
        lineBuffer[i] = backdrop;
        priorityBuffer[i] = 255;  // Lowest possible priority
        layerTypeBuffer[i] = 5;   // 5 = Backdrop
        secondLayerBuffer[i] = backdrop;
        secondLayerTypeBuffer[i] = 5;  // 5 = Backdrop
    }
    
    // 1.5. TWO-PASS SPRITE RENDERING - Pass 1: Preprocess all sprites
    // This collects all sprite pixels into spriteLayer BEFORE compositing
    // Enables correct semi-transparent blending with final backgrounds
    bool mapping1D = (dispcnt & DISPCNT_OBJ_1D_MAP) != 0;
    WindowControl emptyWinCtrl = {};
    emptyWinCtrl.winOut = 0x3F;  // All layers visible by default
    
    // Initialize sprite buffers unconditionally
    // This is critical because objWindowMask is used for windowing even if OBJ rendering is disabled
    for (int i = 0; i < 240; i++) {
        spriteLayer[i] = FLAG_UNWRITTEN;
        objWindowMask[i] = false;
    }
    
    if (dispcnt & DISPCNT_OBJ_ENABLE) {
        preprocessSprites(scanline, mapping1D, windowsEnabled ? winCtrl : emptyWinCtrl);
    }
    
    // 2. Render each priority level (back to front: 3 → 0)
    static int render_trace = 0;
    if (scanline == 0 && render_trace < 5) {
        render_trace++;
        LOG_TRACE_CAT("[RENDER] Mode=%d DISPCNT=0x%04X windowsEnabled=%d\n", mode, dispcnt, windowsEnabled);
        for (int bg = 0; bg < 4; bg++) {
            bool enabled = (dispcnt & (DISPCNT_BG0_ENABLE << bg)) != 0;
            LOG_TRACE_CAT("  BG%d: enabled=%d\n", bg, enabled);
        }
    }
    
    for (int priority = 3; priority >= 0; priority--) {
        // Render BGs with this priority (BG3 → BG0)
        // Mode 2 only supports BG2 and BG3 (affine backgrounds), skip BG0 and BG1
        for (int bg = 3; bg >= 0; bg--) {
            // Skip BG0/BG1 in Mode 2 (they don't exist)
            if (mode == 2 && (bg == 0 || bg == 1)) {
                continue;
            }
            
            if (dispcnt & (DISPCNT_BG0_ENABLE << bg)) {
                // Check if this BG has the current priority
                uint16_t bgcnt = memory.readDirectIO16(REG_BG0CNT + (bg * 2));
                uint8_t bgPriority = bgcnt & BGCNT_PRIORITY_MASK;
                
                if (bgPriority == priority) {
                    // Track which pixels existed before rendering this layer
                    uint16_t oldLineBuffer[240];
                    uint8_t oldPriorityBuffer[240];
                    for (int x = 0; x < 240; x++) {
                        oldLineBuffer[x] = lineBuffer[x];
                        oldPriorityBuffer[x] = priorityBuffer[x];
                    }
                    
                    // Determine if this is an affine background
                    // Mode 2: BG2 and BG3 are affine
                    // Mode 1: BG2 is affine, BG0/BG1 are regular
                    bool isAffineBG = (mode == 2 && (bg == 2 || bg == 3)) ||
                                      (mode == 1 && bg == 2);
                    
                    // Render with window checking if windows are enabled
                    if (windowsEnabled) {
                        if (isAffineBG) {
                            renderAffineBGScanlineWithPriorityAndWindow(bg, scanline, lineBuffer, 
                                                                        priorityBuffer, layerTypeBuffer,
                                                                        secondLayerBuffer, secondLayerTypeBuffer, winCtrl);
                        } else {
                            renderBGScanlineWithPriorityAndWindow(bg, scanline, lineBuffer, 
                                                                   priorityBuffer, layerTypeBuffer,
                                                                   secondLayerBuffer, secondLayerTypeBuffer, winCtrl);
                        }
                    } else {
                        if (isAffineBG) {
                            renderAffineBGScanlineWithPriority(bg, scanline, lineBuffer, priorityBuffer);
                        } else {
                            renderBGScanlineWithPriority(bg, scanline, lineBuffer, priorityBuffer);
                        }
                        // Update layer type buffer AND second layer buffer for pixels that changed
                        for (int x = 0; x < 240; x++) {
                            if (lineBuffer[x] != oldLineBuffer[x] || priorityBuffer[x] != oldPriorityBuffer[x]) {
                                // Save old pixel as second layer
                                secondLayerBuffer[x] = oldLineBuffer[x];
                                secondLayerTypeBuffer[x] = layerTypeBuffer[x];
                                // Update first layer type
                                layerTypeBuffer[x] = bg;
                            }
                        }
                    }
                }
            }
        }
        
        // TWO-PASS SPRITE RENDERING - Pass 2: Composite sprites at this priority
        // Sprites were already preprocessed before the priority loop
        // Now composite them onto lineBuffer with proper blending
        if (dispcnt & DISPCNT_OBJ_ENABLE) {
            postprocessSprites(priority, scanline, lineBuffer, priorityBuffer, layerTypeBuffer,
                              secondLayerBuffer, secondLayerTypeBuffer, 
                              windowsEnabled ? winCtrl : emptyWinCtrl);
        }
    }
    
    // 3. Apply blend effects if enabled
    if (blend.mode != BLEND_MODE_OFF) {
        applyBlendToScanline(lineBuffer, layerTypeBuffer, secondLayerBuffer, 
                             secondLayerTypeBuffer, scanline, blend, windowsEnabled ? winCtrl : emptyWinCtrl);
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
    uint16_t bgcnt = memory.readDirectIO16(REG_BG0CNT + (bgNum * 2));
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
            colorIndex = memory.readDirectIO8(tileAddr + pixelOffset);
        } else {
            // 4bpp mode
            int byteOffset = pixelInTileY * 4 + pixelInTileX / 2;
            uint8_t byte = memory.readDirectIO8(tileAddr + byteOffset);
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
            
            uint16_t rgb555 = memory.readDirectIO16(0x05000000 + paletteIndex * 2);
            
            // Update line buffer and priority
            lineBuffer[screenX] = rgb555;
            priorityBuffer[screenX] = layerPriority;
        }
    }
}

/**
 * Read affine background parameters from IO registers
 * BG2: 0x04000020-0x0400002F
 * BG3: 0x04000030-0x0400003F
 */
AffineBackgroundParams GPU::readAffineBGParams(int bgNum) {
    AffineBackgroundParams params;
    
    uint32_t baseAddr = (bgNum == 2) ? REG_BG2PA : REG_BG3PA;
    
    // Read 8.8 fixed-point transformation matrix parameters
    params.pa = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 0));   // dx
    params.pb = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 2));   // dmx
    params.pc = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 4));   // dy
    params.pd = static_cast<int16_t>(memory.readDirectIO16(baseAddr + 6));   // dmy
    
    // Read 19.8 fixed-point reference points (28-bit signed)
    // BG2X/BG3X at offset 8, BG2Y/BG3Y at offset 12
    uint32_t refXRaw = memory.readDirectIO32(baseAddr + 8);
    uint32_t refYRaw = memory.readDirectIO32(baseAddr + 12);
    
    // Sign extend from 28 bits to 32 bits
    params.refX = (refXRaw & 0x08000000) ? (refXRaw | 0xF0000000) : (refXRaw & 0x0FFFFFFF);
    params.refY = (refYRaw & 0x08000000) ? (refYRaw | 0xF0000000) : (refYRaw & 0x0FFFFFFF);
    
    return params;
}

/**
 * Get affine background map size based on BGCNT screen size bits
 * Affine BGs have different sizes than regular BGs:
 *   0: 128x128 (16x16 tiles)
 *   1: 256x256 (32x32 tiles)
 *   2: 512x512 (64x64 tiles)
 *   3: 1024x1024 (128x128 tiles)
 */
void GPU::getAffineBGDimensions(uint8_t sizeCode, int& widthPixels, int& heightPixels, int& widthTiles) {
    switch (sizeCode) {
        case 0: widthPixels = 128;  heightPixels = 128;  widthTiles = 16;  break;
        case 1: widthPixels = 256;  heightPixels = 256;  widthTiles = 32;  break;
        case 2: widthPixels = 512;  heightPixels = 512;  widthTiles = 64;  break;
        case 3: widthPixels = 1024; heightPixels = 1024; widthTiles = 128; break;
        default: widthPixels = 128; heightPixels = 128; widthTiles = 16;  break;
    }
}

/**
 * Render an affine background scanline with priority checking
 * Affine backgrounds (Mode 1 BG2, Mode 2 BG2/BG3) use rotation/scaling
 * 
 * Key differences from regular BGs:
 * - Map entries are 8-bit tile indices (not 16-bit screen entries)
 * - Always 256-color mode (8bpp tiles)
 * - No horizontal/vertical flip per-tile
 * - Uses PA/PB/PC/PD matrix + reference point for transformation
 */
void GPU::renderAffineBGScanlineWithPriority(int bgNum, uint16_t scanline, 
                                              uint16_t* lineBuffer, uint8_t* priorityBuffer) {
    // Read BG control register
    uint16_t bgcnt = memory.readDirectIO16(REG_BG0CNT + (bgNum * 2));
    
    // Parse BGCNT for affine BG
    uint8_t priority = bgcnt & BGCNT_PRIORITY_MASK;
    uint8_t charBaseBlock = (bgcnt >> 2) & 0x03;
    uint8_t screenBaseBlock = (bgcnt >> 8) & 0x1F;
    uint8_t screenSize = (bgcnt >> 14) & 0x03;
    bool wrapAround = (bgcnt & 0x2000) != 0;  // Bit 13: Display Area Overflow
    
    // Calculate VRAM addresses
    uint32_t charBaseAddr = VRAM_BASE + (charBaseBlock * 0x4000);
    uint32_t screenBaseAddr = VRAM_BASE + (screenBaseBlock * 0x800);
    
    // Get map dimensions
    int mapWidthPixels, mapHeightPixels, mapWidthTiles;
    getAffineBGDimensions(screenSize, mapWidthPixels, mapHeightPixels, mapWidthTiles);
    
    // Read affine parameters
    AffineBackgroundParams params = readAffineBGParams(bgNum);
    
    // Debug: trace affine params on first scanline
    static int affine_trace_count = 0;
    if (scanline == 0 && affine_trace_count < 10) {
        affine_trace_count++;
        LOG_TRACE_CAT("[AFFINE BG%d] PA=%d PB=%d PC=%d PD=%d refX=%d refY=%d BGCNT=0x%04X\n",
               bgNum, params.pa, params.pb, params.pc, params.pd, 
               params.refX, params.refY, bgcnt);
    }
    
    // If PA is 0 and PB is 0, the affine matrix hasn't been set up properly
    // (identity would be PA=0x100, PB=0, PC=0, PD=0x100)
    // In this case, just skip rendering to avoid garbage
    if (params.pa == 0 && params.pb == 0 && params.pc == 0 && params.pd == 0) {
        // Uninitialized affine parameters, don't render garbage
        static int skip_count = 0;
        if (scanline == 0 && skip_count < 5) {
            skip_count++;
            LOG_TRACE_CAT("[AFFINE BG%d] SKIPPING - all params are 0\n", bgNum);
        }
        return;
    }
    
    // Calculate layer priority (same as regular BGs)
    uint8_t layerPriority = (priority * 4) + bgNum;
    
    // Calculate texture coordinates for this scanline
    // Starting point: refX + scanline * dmx, refY + scanline * dmy
    // These are in 8.8 fixed point (reference point is 19.8, but we work in 8.8)
    int32_t texX = params.refX + (scanline * params.pb);  // refX + y * dmx
    int32_t texY = params.refY + (scanline * params.pd);  // refY + y * dmy
    
    // Render each screen pixel
    for (int screenX = 0; screenX < 240; screenX++) {
        // Convert from 8.8 fixed point to integer coordinates
        int32_t bgX = texX >> 8;
        int32_t bgY = texY >> 8;
        
        // Handle wraparound or out-of-bounds
        if (wrapAround) {
            // Wrap texture coordinates
            bgX = bgX & (mapWidthPixels - 1);
            bgY = bgY & (mapHeightPixels - 1);
        } else {
            // Check bounds - transparent if outside
            if (bgX < 0 || bgX >= mapWidthPixels || bgY < 0 || bgY >= mapHeightPixels) {
                // Advance to next pixel
                texX += params.pa;  // x += dx
                texY += params.pc;  // y += dy
                continue;
            }
        }
        
        // Get tile coordinates
        int tileX = bgX >> 3;  // divide by 8
        int tileY = bgY >> 3;
        int pixelInTileX = bgX & 7;  // mod 8
        int pixelInTileY = bgY & 7;
        
        // Read map entry (8-bit tile index for affine BGs)
        uint32_t mapOffset = tileY * mapWidthTiles + tileX;
        uint8_t tileIndex = memory.readDirectIO8(screenBaseAddr + mapOffset);
        
        // Get tile address (affine BGs always use 8bpp = 64 bytes per tile)
        uint32_t tileAddr = charBaseAddr + (tileIndex * 64);
        
        // Read pixel color index (8bpp)
        uint32_t pixelOffset = pixelInTileY * 8 + pixelInTileX;
        uint8_t colorIndex = memory.readDirectIO8(tileAddr + pixelOffset);
        
        // Color 0 is transparent
        if (colorIndex == 0) {
            // Advance to next pixel
            texX += params.pa;
            texY += params.pc;
            continue;
        }
        
        // Only draw if this pixel has higher or equal priority
        if (layerPriority <= priorityBuffer[screenX]) {
            // Get color from BG palette (256-color mode uses single palette)
            uint16_t rgb555 = memory.readDirectIO16(0x05000000 + colorIndex * 2);
            
            // Update line buffer and priority
            lineBuffer[screenX] = rgb555;
            priorityBuffer[screenX] = layerPriority;
        }
        
        // Advance texture coordinates
        texX += params.pa;  // x += dx
        texY += params.pc;  // y += dy
    }
}

// ============================================================================
// Session 3: Advanced Features - Blend and Window Support
// ============================================================================

BlendControl GPU::readBlendControl() {
    BlendControl blend = {};
    
    uint16_t bldcnt = memory.readDirectIO16(REG_BLDCNT);
    uint16_t bldalpha = memory.readDirectIO16(REG_BLDALPHA);
    uint16_t bldy = memory.readDirectIO16(REG_BLDY);
    
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
    
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    
    // Window 0
    winCtrl.win0.enabled = (dispcnt & DISPCNT_WIN0_ENABLE) != 0;
    if (winCtrl.win0.enabled) {
        uint16_t win0h = memory.readDirectIO16(REG_WIN0H);
        uint16_t win0v = memory.readDirectIO16(REG_WIN0V);
        
        winCtrl.win0.right = win0h & 0xFF;           // Bits 0-7
        winCtrl.win0.left = (win0h >> 8) & 0xFF;     // Bits 8-15
        winCtrl.win0.bottom = win0v & 0xFF;          // Bits 0-7
        winCtrl.win0.top = (win0v >> 8) & 0xFF;      // Bits 8-15
        
        uint16_t winin = memory.readDirectIO16(REG_WININ);
        winCtrl.win0.control = winin & 0x3F;         // Bits 0-5
    }
    
    // Window 1
    winCtrl.win1.enabled = (dispcnt & DISPCNT_WIN1_ENABLE) != 0;
    if (winCtrl.win1.enabled) {
        uint16_t win1h = memory.readDirectIO16(REG_WIN1H);
        uint16_t win1v = memory.readDirectIO16(REG_WIN1V);
        
        winCtrl.win1.right = win1h & 0xFF;
        winCtrl.win1.left = (win1h >> 8) & 0xFF;
        winCtrl.win1.bottom = win1v & 0xFF;
        winCtrl.win1.top = (win1v >> 8) & 0xFF;
        
        uint16_t winin = memory.readDirectIO16(REG_WININ);
        winCtrl.win1.control = (winin >> 8) & 0x3F;  // Bits 8-13
    }
    
    // Outside window control
    uint16_t winout = memory.readDirectIO16(REG_WINOUT);
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
    
    // Check OBJ Window (built from OBJ_MODE_OBJ_WINDOW sprites)
    // OBJ Window has priority between WIN0/WIN1 and WINOUT
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    bool objWinEnabled = (dispcnt & DISPCNT_WINOBJ_ENABLE) != 0;
    if (objWinEnabled && x >= 0 && x < 240 && objWindowMask[x]) {
        return winCtrl.winObj;  // Return OBJ Window control settings
    }
    
    // Outside all windows
    return winCtrl.winOut;
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

// ============================================================================
// Session 3 Integration: Blend and Window Integration with Rendering
// ============================================================================

void GPU::renderBGScanlineWithPriorityAndWindow(int bgNum, uint16_t scanline, uint16_t* lineBuffer,
                                                  uint8_t* priorityBuffer, uint8_t* layerTypeBuffer,
                                                  uint16_t* secondLayerBuffer, uint8_t* secondLayerTypeBuffer,
                                                  const WindowControl& winCtrl) {
    // Get BG configuration
    uint16_t bgcnt = memory.readDirectIO16(REG_BG0CNT + (bgNum * 2));
    BGConfig bgConfig = parseBGCNT(bgcnt);
    
    // Get scroll values
    BGScroll scroll = readBGScroll(bgNum);
    
    // Calculate priority value for this layer
    uint8_t layerPriority = (bgConfig.priority * 4) + bgNum;
    
    // For each pixel on this scanline
    for (int screenX = 0; screenX < 240; screenX++) {
        // Check window visibility first
        uint8_t control = getWindowControlForPixel(screenX, scanline, winCtrl);
        bool layerVisible = (control & (1 << bgNum)) != 0;
        
        if (!layerVisible) {
            continue;  // Skip this pixel - layer masked by window
        }
        
        // Apply scrolling
        int bgX, bgY;
        applyScroll(bgConfig, scroll, screenX, scanline, bgX, bgY);
        
        // Get tile coordinates
        int tileX, tileY, pixelInTileX, pixelInTileY;
        getTileCoords(bgX, bgY, tileX, tileY, pixelInTileX, pixelInTileY);
        
        // Wrap coordinates for screen size
        tileX = tileX % bgConfig.screenWidthTiles;
        tileY = tileY % bgConfig.screenHeightTiles;
        if (tileX < 0) tileX += bgConfig.screenWidthTiles;
        if (tileY < 0) tileY += bgConfig.screenHeightTiles;
        
        // Read screen entry
        ScreenEntry entry = readScreenEntry(bgConfig, tileX, tileY);
        
        // Get tile address
        uint32_t tileAddr = getTileAddress(bgConfig, entry);
        
        // Apply horizontal/vertical flip
        int finalPixelX = entry.hFlip ? (7 - pixelInTileX) : pixelInTileX;
        int finalPixelY = entry.vFlip ? (7 - pixelInTileY) : pixelInTileY;
        
        // Read pixel color index
        uint8_t colorIndex;
        if (bgConfig.paletteMode) {
            colorIndex = getTilePixel8bpp(tileAddr, finalPixelX, finalPixelY);
        } else {
            colorIndex = getTilePixel4bpp(tileAddr, finalPixelX, finalPixelY);
        }
        
        // Transparent pixel?
        if (colorIndex == 0) {
            continue;
        }
        
        // Only draw if this pixel has higher or equal priority
        if (layerPriority <= priorityBuffer[screenX]) {
            // Get color from palette
            uint16_t rgb555;
            if (bgConfig.paletteMode) {
                rgb555 = memory.readDirectIO16(0x05000000 + colorIndex * 2);
            } else {
                uint16_t paletteIndex = (entry.paletteNum * 16) + colorIndex;
                rgb555 = memory.readDirectIO16(0x05000000 + paletteIndex * 2);
            }
            
            // Save current pixel as second layer (for alpha blending)
            secondLayerBuffer[screenX] = lineBuffer[screenX];
            secondLayerTypeBuffer[screenX] = layerTypeBuffer[screenX];
            
            // Update buffers
            lineBuffer[screenX] = rgb555;
            priorityBuffer[screenX] = layerPriority;
            layerTypeBuffer[screenX] = bgNum;  // 0-3 for BG0-BG3
        }
    }
}

/**
 * Render an affine background scanline with priority and window checking
 * This is the window-aware version of renderAffineBGScanlineWithPriority
 */
void GPU::renderAffineBGScanlineWithPriorityAndWindow(int bgNum, uint16_t scanline, 
                                                       uint16_t* lineBuffer, uint8_t* priorityBuffer,
                                                       uint8_t* layerTypeBuffer, 
                                                       uint16_t* secondLayerBuffer, uint8_t* secondLayerTypeBuffer,
                                                       const WindowControl& winCtrl) {
    // Read BG control register
    uint16_t bgcnt = memory.readDirectIO16(REG_BG0CNT + (bgNum * 2));
    
    // Parse BGCNT for affine BG
    uint8_t priority = bgcnt & BGCNT_PRIORITY_MASK;
    uint8_t charBaseBlock = (bgcnt >> 2) & 0x03;
    uint8_t screenBaseBlock = (bgcnt >> 8) & 0x1F;
    uint8_t screenSize = (bgcnt >> 14) & 0x03;
    bool wrapAround = (bgcnt & 0x2000) != 0;  // Bit 13: Display Area Overflow
    
    // Calculate VRAM addresses
    uint32_t charBaseAddr = VRAM_BASE + (charBaseBlock * 0x4000);
    uint32_t screenBaseAddr = VRAM_BASE + (screenBaseBlock * 0x800);
    
    // Get map dimensions
    int mapWidthPixels, mapHeightPixels, mapWidthTiles;
    getAffineBGDimensions(screenSize, mapWidthPixels, mapHeightPixels, mapWidthTiles);
    
    // Read affine parameters
    AffineBackgroundParams params = readAffineBGParams(bgNum);
    

    
    // If PA is 0 and PB is 0, the affine matrix hasn't been set up properly
    // In this case, just skip rendering to avoid garbage
    if (params.pa == 0 && params.pb == 0 && params.pc == 0 && params.pd == 0) {
        return;
    }
    
    // Calculate layer priority (same as regular BGs)
    uint8_t layerPriority = (priority * 4) + bgNum;
    
    // Calculate texture coordinates for this scanline
    int32_t texX = params.refX + (scanline * params.pb);
    int32_t texY = params.refY + (scanline * params.pd);
    
    // Render each screen pixel
    for (int screenX = 0; screenX < 240; screenX++) {
        // Check window visibility first
        uint8_t control = getWindowControlForPixel(screenX, scanline, winCtrl);
        bool layerVisible = (control & (1 << bgNum)) != 0;
        
        if (!layerVisible) {
            // Advance texture coordinates even if not visible
            texX += params.pa;
            texY += params.pc;
            continue;
        }
        
        // Convert from 8.8 fixed point to integer coordinates
        int32_t bgX = texX >> 8;
        int32_t bgY = texY >> 8;
        
        // Handle wraparound or out-of-bounds
        if (wrapAround) {
            bgX = bgX & (mapWidthPixels - 1);
            bgY = bgY & (mapHeightPixels - 1);
        } else {
            if (bgX < 0 || bgX >= mapWidthPixels || bgY < 0 || bgY >= mapHeightPixels) {
                texX += params.pa;
                texY += params.pc;
                continue;
            }
        }
        
        // Get tile coordinates
        int tileX = bgX >> 3;
        int tileY = bgY >> 3;
        int pixelInTileX = bgX & 7;
        int pixelInTileY = bgY & 7;
        
        // Read map entry (8-bit tile index)
        uint32_t mapOffset = tileY * mapWidthTiles + tileX;
        uint8_t tileIndex = memory.readDirectIO8(screenBaseAddr + mapOffset);
        
        // Get tile address (8bpp = 64 bytes per tile)
        uint32_t tileAddr = charBaseAddr + (tileIndex * 64);
        
        // Read pixel color index
        uint32_t pixelOffset = pixelInTileY * 8 + pixelInTileX;
        uint8_t colorIndex = memory.readDirectIO8(tileAddr + pixelOffset);
        
        // Color 0 is transparent
        if (colorIndex == 0) {
            texX += params.pa;
            texY += params.pc;
            continue;
        }
        
        // Only draw if this pixel has higher or equal priority
        if (layerPriority <= priorityBuffer[screenX]) {
            uint16_t rgb555 = memory.readDirectIO16(0x05000000 + colorIndex * 2);
            
            // Save current pixel as second layer
            secondLayerBuffer[screenX] = lineBuffer[screenX];
            secondLayerTypeBuffer[screenX] = layerTypeBuffer[screenX];
            
            // Update buffers
            lineBuffer[screenX] = rgb555;
            priorityBuffer[screenX] = layerPriority;
            layerTypeBuffer[screenX] = bgNum;
        }
        
        // Advance texture coordinates
        texX += params.pa;
        texY += params.pc;
    }
}

void GPU::applyBlendToScanline(uint16_t* lineBuffer, uint8_t* layerTypeBuffer, 
                                uint16_t* secondLayerBuffer, uint8_t* secondLayerTypeBuffer,
                                uint16_t scanline, const BlendControl& blend, const WindowControl& winCtrl) {
    // Apply blend effects based on mode
    switch (blend.mode) {
        case BLEND_MODE_ALPHA:
            // Full alpha blending with second layer tracking
            for (int x = 0; x < 240; x++) {
                // Check Window Special Effects Enable (Bit 5)
                uint8_t winControl = getWindowControlForPixel(x, scanline, winCtrl);
                if (!(winControl & 0x20)) {
                    continue; // Blending disabled by window
                }

                uint8_t firstLayerType = layerTypeBuffer[x];
                uint8_t secondLayerType = secondLayerTypeBuffer[x];
                
                // Skip ALL sprite layers (type 4 and 254)
                // - Type 4: Normal sprites never blend via applyBlendToScanline
                // - Type 254: Semi-transparent sprites already blended during rendering
                if (firstLayerType == 4 || firstLayerType == 254) {
                    continue;
                }
                
                // Check if first layer is a first target
                bool isFirstTarget = (blend.firstTargets & (1 << firstLayerType)) != 0;
                if (!isFirstTarget) {
                    continue;  // No blending if not a first target
                }
                
                // Check if second layer is a second target
                bool isSecondTarget = (blend.secondTargets & (1 << secondLayerType)) != 0;
                if (!isSecondTarget) {
                    continue;  // No blending if second layer not a second target
                }
                
                // Apply alpha blend between first and second layer
                lineBuffer[x] = applyBlend(lineBuffer[x], secondLayerBuffer[x], blend, 
                                           firstLayerType, secondLayerType);
            }
            break;
            
        case BLEND_MODE_BRIGHTEN:
            // Brighten all first target pixels
            for (int x = 0; x < 240; x++) {
                // Check Window Special Effects Enable (Bit 5)
                uint8_t winControl = getWindowControlForPixel(x, scanline, winCtrl);
                if (!(winControl & 0x20)) {
                    continue; // Blending disabled by window
                }

                uint8_t layerType = layerTypeBuffer[x];
                bool isFirstTarget = (blend.firstTargets & (1 << layerType)) != 0;
                
                if (isFirstTarget) {
                    lineBuffer[x] = applyBrightnessIncrease(lineBuffer[x], blend.evy);
                }
            }
            break;
            
        case BLEND_MODE_DARKEN:
            // Darken all first target pixels
            for (int x = 0; x < 240; x++) {
                // Check Window Special Effects Enable (Bit 5)
                uint8_t winControl = getWindowControlForPixel(x, scanline, winCtrl);
                if (!(winControl & 0x20)) {
                    continue; // Blending disabled by window
                }

                uint8_t layerType = layerTypeBuffer[x];
                bool isFirstTarget = (blend.firstTargets & (1 << layerType)) != 0;
                
                if (isFirstTarget) {
                    lineBuffer[x] = applyBrightnessDecrease(lineBuffer[x], blend.evy);
                }
            }
            break;
            
        default:
            // BLEND_MODE_OFF - do nothing
            break;
    }
}

/**
 * Render an OBJ Window sprite to the objWindowMask buffer
 * Non-transparent pixels set the mask to true, indicating they are inside the OBJ Window region
 */
void GPU::renderObjWindowToMask(const OBJAttributes& obj, int objNum, uint16_t scanline, bool mapping1D) {
    (void)objNum;  // Unused for mask rendering
    
    int spriteY = (obj.y < 160) ? obj.y : (obj.y - 256);
    int rowInSprite = scanline - spriteY;
    
    // Get sprite dimensions
    int width = obj.width;
    int height = obj.height;
    
    int spriteX = (obj.x < 240) ? obj.x : (obj.x - 512);
    
    if (obj.rotScaleFlag) {
        // AFFINE OBJ Window sprite
        AffineParams params = readAffineParams(obj.rotScaleParam);
        
        int renderWidth = obj.doubleSize ? width * 2 : width;
        int renderHeight = obj.doubleSize ? height * 2 : height;
        
        int texCenterX = width / 2;
        int texCenterY = height / 2;
        
        for (int spritePixelX = 0; spritePixelX < renderWidth; spritePixelX++) {
            int screenX = spriteX + spritePixelX;
            if (screenX < 0 || screenX >= 240) continue;
            
            int relX = spritePixelX - renderWidth / 2;
            int relY = rowInSprite - renderHeight / 2;
            
            int textureX = ((params.pa * relX + params.pb * relY) >> 8) + texCenterX;
            int textureY = ((params.pc * relX + params.pd * relY) >> 8) + texCenterY;
            
            if (textureX < 0 || textureX >= width || textureY < 0 || textureY >= height) {
                continue;
            }
            
            // Get color index using shared helper
            int tileX = textureX / 8;
            int tileY = textureY / 8;
            int pixelInTileX = textureX % 8;
            int pixelInTileY = textureY % 8;
            
            uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
            uint8_t colorIndex;
            
            if (obj.paletteMode) {
                tileAddr += pixelInTileY * 8 + pixelInTileX;
                colorIndex = memory.readDirectIO8(tileAddr);
            } else {
                tileAddr += pixelInTileY * 4 + (pixelInTileX / 2);
                uint8_t tileData = memory.readDirectIO8(tileAddr);
                colorIndex = (pixelInTileX & 1) ? (tileData >> 4) : (tileData & 0x0F);
            }
            
            // Non-transparent pixels set the OBJ Window mask
            if (colorIndex != 0) {
                objWindowMask[screenX] = true;
            }
        }
    } else {
        // NORMAL OBJ Window sprite
        int effectiveRow = obj.vFlip ? (height - 1 - rowInSprite) : rowInSprite;
        
        for (int spritePixelX = 0; spritePixelX < width; spritePixelX++) {
            int screenX = spriteX + spritePixelX;
            if (screenX < 0 || screenX >= 240) continue;
            
            int effectiveX = obj.hFlip ? (width - 1 - spritePixelX) : spritePixelX;
            
            int tileX = effectiveX / 8;
            int tileY = effectiveRow / 8;
            int pixelInTileX = effectiveX % 8;
            int pixelInTileY = effectiveRow % 8;
            
            uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
            uint8_t colorIndex;
            
            if (obj.paletteMode) {
                tileAddr += pixelInTileY * 8 + pixelInTileX;
                colorIndex = memory.readDirectIO8(tileAddr);
            } else {
                tileAddr += pixelInTileY * 4 + (pixelInTileX / 2);
                uint8_t tileData = memory.readDirectIO8(tileAddr);
                colorIndex = (pixelInTileX & 1) ? (tileData >> 4) : (tileData & 0x0F);
            }
            
            // Non-transparent pixels set the OBJ Window mask
            if (colorIndex != 0) {
                objWindowMask[screenX] = true;
            }
        }
    }
}

/**
 * Pass 1: Preprocess all sprites into spriteLayer buffer
 * This is the first pass of mgba's two-pass sprite rendering
 * - Processes ALL sprites (127->0) regardless of priority
 * - Writes to spriteLayer instead of lineBuffer
 * - NO blending happens here - just collect sprite pixels
 * - Handles sprite ordering (lower OBJ# overwrites higher OBJ#)
 * - Even transparent pixels update order flags
 */
void GPU::preprocessSprites(uint16_t scanline, bool mapping1D, const WindowControl& winCtrl) {
    
    // spriteLayer and objWindowMask are initialized in renderScanline
    
    // Check if OBJ Window is enabled in DISPCNT
    uint16_t dispcnt = memory.readDirectIO16(REG_DISPCNT);
    bool objWinEnabled = (dispcnt & DISPCNT_WINOBJ_ENABLE) != 0;
    

    
    // Read BLDCNT to check if NORMAL sprites should be blend targets
    // Per mGBA: NORMAL sprites can be 1st targets if BLDCNT enables them + alpha mode
    uint16_t bldcnt = memory.readDirectIO16(REG_BLDCNT);
    bool objIsFirstTarget = (bldcnt & 0x0010) != 0;  // Bit 4: OBJ is 1st target
    uint8_t blendMode = (bldcnt >> 6) & 0x03;        // Bits 6-7: blend mode
    bool normalSpritesCanBlend = objIsFirstTarget && (blendMode == 1);  // Alpha blend mode
    
    // Compute 2nd targets like mGBA does (for semi-transparent / alpha blend sprites)
    // BLDCNT bits 8-13: BG0, BG1, BG2, BG3, OBJ, BD as 2nd targets
    bool target2Bd = (bldcnt & 0x2000) != 0;   // Bit 13: Backdrop is 2nd target
    bool target2Bg0 = (bldcnt & 0x0100) != 0;  // Bit 8
    bool target2Bg1 = (bldcnt & 0x0200) != 0;  // Bit 9
    bool target2Bg2 = (bldcnt & 0x0400) != 0;  // Bit 10
    bool target2Bg3 = (bldcnt & 0x0800) != 0;  // Bit 11
    
    // Check which BGs are enabled in DISPCNT
    bool bg0Enabled = (dispcnt & 0x0100) != 0;
    bool bg1Enabled = (dispcnt & 0x0200) != 0;
    bool bg2Enabled = (dispcnt & 0x0400) != 0;
    bool bg3Enabled = (dispcnt & 0x0800) != 0;
    
    // mGBA's target2 calculation: any enabled BG that's also a 2nd target, OR backdrop
    bool anyValidTarget2 = target2Bd 
                         || (target2Bg0 && bg0Enabled)
                         || (target2Bg1 && bg1Enabled)
                         || (target2Bg2 && bg2Enabled)
                         || (target2Bg3 && bg3Enabled);
    
    // Suppress unused variable warnings for debug vars
    (void)target2Bd; (void)target2Bg0; (void)target2Bg1;
    (void)target2Bg2; (void)target2Bg3; (void)bg0Enabled;
    (void)bg1Enabled; (void)bg2Enabled; (void)bg3Enabled;
    (void)anyValidTarget2;
    
    // First pass: Build OBJ Window mask from OBJ_MODE_OBJ_WINDOW sprites
    if (objWinEnabled) {
        for (int objNum = 127; objNum >= 0; objNum--) {
            OBJAttributes obj = readOBJAttributes(objNum);
            
            if (!obj.visible || obj.objMode != OBJ_MODE_OBJ_WINDOW) {
                continue;
            }
            
            if (!isSpriteOnScanline(obj, scanline)) {
                continue;
            }
            
            // Render this OBJ Window sprite's non-transparent pixels to the mask
            renderObjWindowToMask(obj, objNum, scanline, mapping1D);
        }
    }
    
    // Second pass: Process visible sprites (non-OBJ_WINDOW)
    for (int objNum = 127; objNum >= 0; objNum--) {
        OBJAttributes obj = readOBJAttributes(objNum);
        
        // Skip if not visible
        if (!obj.visible) {
            continue;
        }
        
        // Skip prohibited mode
        if (obj.objMode == OBJ_MODE_PROHIBITED) {
            continue;
        }
        
        // Skip OBJ Window mode sprites - they should NOT be rendered as visible sprites
        // They are only used for window masking (when OBJ Window is enabled in DISPCNT)
        if (obj.objMode == OBJ_MODE_OBJ_WINDOW) {
            continue;
        }
        
        // Check if sprite is on this scanline
        if (!isSpriteOnScanline(obj, scanline)) {
            continue;
        }

        // Build flags for this sprite
        uint32_t flags = (obj.priority << OFFSET_PRIORITY) | (objNum << OFFSET_ORDER);
        

        
        // Add blend target flags
        // Semi-transparent sprites are ALWAYS first targets
        // NORMAL sprites can also be 1st targets if BLDCNT enables OBJ + alpha blend mode
        if (obj.objMode == OBJ_MODE_SEMI_TRANSPARENT) {
            flags |= FLAG_TARGET_1 | FLAG_SEMI_TRANSPARENT;
        } else if (normalSpritesCanBlend) {
            flags |= FLAG_TARGET_1;
        }
        
        // Note: OBJ_MODE_OBJ_WINDOW sprites are skipped above - they don't render visibly
        
        // Handle affine vs normal sprites differently
        if (obj.rotScaleFlag) {
            // AFFINE SPRITE - use transformation matrix
            AffineParams params = readAffineParams(obj.rotScaleParam);
            
            int spriteY = (obj.y < 160) ? obj.y : (obj.y - 256);
            int rowInSprite = scanline - spriteY;
            
            // Get rendering dimensions (may be doubled for double-size mode)
            int renderWidth = obj.doubleSize ? obj.width * 2 : obj.width;
            int renderHeight = obj.doubleSize ? obj.height * 2 : obj.height;
            
            // Texture center (original sprite dimensions, not doubled)
            int texCenterX = obj.width / 2;
            int texCenterY = obj.height / 2;
            
            int spriteX = (obj.x < 240) ? obj.x : (obj.x - 512);
            
            // Render each pixel across the affine sprite's render width
            for (int spritePixelX = 0; spritePixelX < renderWidth; spritePixelX++) {
                int screenX = spriteX + spritePixelX;
                
                if (screenX < 0 || screenX >= 240) {
                    continue;
                }
                
                // Check window visibility
                uint8_t control = getWindowControlForPixel(screenX, scanline, winCtrl);
                bool objVisible = (control & WIN_OBJ_ENABLE) != 0;
                
                if (!objVisible) {
                    continue;
                }
                
                // Get current spriteLayer pixel
                uint32_t current = spriteLayer[screenX];
                
                // Check sprite ordering - lower OBJ# has higher priority
                if (current != FLAG_UNWRITTEN) {
                    uint32_t currentObjNum = (current & FLAG_ORDER_MASK) >> OFFSET_ORDER;
                    if (currentObjNum < (uint32_t)objNum) {
                        continue;
                    }
                }
                
                // Apply affine transformation
                int relX = spritePixelX - renderWidth / 2;
                int relY = rowInSprite - renderHeight / 2;
                
                int textureX, textureY;
                applyAffineTransform(params, relX, relY, textureX, textureY);
                
                textureX += texCenterX;
                textureY += texCenterY;
                
                // Check if transformed coordinates are within original sprite bounds
                if (textureX < 0 || textureX >= obj.width || 
                    textureY < 0 || textureY >= obj.height) {
                    continue;
                }
                
                // Calculate tile and pixel positions
                int tileX = textureX / 8;
                int tileY = textureY / 8;
                int pixelX = textureX % 8;
                int pixelY = textureY % 8;
                
                uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
                
                // Get color index from VRAM
                uint8_t colorIndex;
                uint8_t* vram = memory.getVRAM();
                uint32_t vramOffset = tileAddr - VRAM_BASE;
                
                if (obj.paletteMode) {
                    int pixelIndex = pixelY * 8 + pixelX;
                    colorIndex = vram[vramOffset + pixelIndex];
                } else {
                    int pixelIndex = pixelY * 4 + pixelX / 2;
                    uint8_t byte = vram[vramOffset + pixelIndex];
                    colorIndex = (pixelX & 1) ? ((byte >> 4) & 0x0F) : (byte & 0x0F);
                }
                
                // Skip transparent pixels
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
                
                uint16_t rgb555 = memory.readDirectIO16(0x05000200 + paletteIndex * 2);
                
                // Store in spriteLayer
                spriteLayer[screenX] = rgb555 | flags;
            }
        } else {
            // NORMAL SPRITE - simple x/y with flip
            int spriteY = (obj.y < 160) ? obj.y : (obj.y - 256);
            int rowInSprite = scanline - spriteY;
            
            if (obj.vFlip) {
                rowInSprite = obj.height - 1 - rowInSprite;
            }
            
            int tileY = rowInSprite / 8;
            int pixelY = rowInSprite % 8;
            int spriteX = (obj.x < 240) ? obj.x : (obj.x - 512);
            
            // Render each pixel of the normal sprite
        for (int spritePixelX = 0; spritePixelX < obj.width; spritePixelX++) {
            int screenX = spriteX + spritePixelX;
            
            if (screenX < 0 || screenX >= 240) {
                continue;
            }
            
            // Check window visibility
            uint8_t control = getWindowControlForPixel(screenX, scanline, winCtrl);
            bool objVisible = (control & WIN_OBJ_ENABLE) != 0;
            
            if (!objVisible) {
                continue;
            }
            
            // Get current spriteLayer pixel
            uint32_t current = spriteLayer[screenX];
            
            // Check sprite ordering - lower OBJ# has higher priority
            if (current != FLAG_UNWRITTEN) {
                uint32_t currentObjNum = (current & FLAG_ORDER_MASK) >> OFFSET_ORDER;
                if (currentObjNum < (uint32_t)objNum) {
                    continue;
                }
            }
            
            // Calculate pixel position in sprite
            int pixelX = obj.hFlip ? (obj.width - 1 - spritePixelX) : spritePixelX;
            int tileX = pixelX / 8;
            int pixelInTileX = pixelX % 8;
            
            uint32_t tileAddr = getOBJTileAddress(obj, tileX, tileY, mapping1D);
            
            // Get color index from VRAM
            uint8_t colorIndex;
            uint8_t* vram = memory.getVRAM();
            uint32_t vramOffset = tileAddr - VRAM_BASE;
            
            if (obj.paletteMode) {
                int pixelIndex = pixelY * 8 + pixelInTileX;
                colorIndex = vram[vramOffset + pixelIndex];
            } else {
                int pixelIndex = pixelY * 4 + pixelInTileX / 2;
                uint8_t byte = vram[vramOffset + pixelIndex];
                colorIndex = (pixelInTileX & 1) ? ((byte >> 4) & 0x0F) : (byte & 0x0F);
            }
            
            // Skip transparent pixels entirely - don't update spriteLayer
            // This allows lower-priority sprites to render through transparent areas
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
            
            uint16_t rgb555 = memory.readDirectIO16(0x05000200 + paletteIndex * 2);
            
            // Color lookup successful
            
            // Store in spriteLayer: color (lower 16 bits) + flags (upper 16 bits)
            // NO blending happens here - just store raw color
            spriteLayer[screenX] = rgb555 | flags;
            }
        }
    }
    
    // Preprocessing complete
}

/**
 * Pass 2: Composite sprites from spriteLayer onto lineBuffer
 * This is the second pass of mgba's two-pass sprite rendering
 * - Processes sprites at specified priority level only
 * - THIS IS WHERE SEMI-TRANSPARENT BLENDING HAPPENS
 * - Blends with FINAL background (whatever is in lineBuffer now)
 * - Checks priority and window visibility
 */
void GPU::postprocessSprites(uint8_t priority, uint16_t scanline, uint16_t* lineBuffer,
                             uint8_t* priorityBuffer, uint8_t* layerTypeBuffer,
                             uint16_t* secondLayerBuffer, uint8_t* secondLayerTypeBuffer,
                             const WindowControl& winCtrl) {
    (void)winCtrl;   // Already checked in preprocess
    
    // Calculate layer priority value for this sprite priority level
    uint8_t layerPriority = (priority * 4) + 3;  // Sprites drawn after BGs at same priority
    
    // Read blend control 
    // Note: Semi-transparent OBJs always use alpha blending regardless of BLDCNT mode
    BlendControl blend = readBlendControl();
    
    // Debug: Track frame count for logging
    static int postFrame = 0;
    static int postLastScan = -1;
    if (scanline == 0 && postLastScan != 0) postFrame++;
    postLastScan = scanline;
    
    // Composite sprites at this priority level
    for (int x = 0; x < 240; x++) {
        uint32_t spritePixel = spriteLayer[x];
        
        // Skip if no sprite OR if transparent (check color bits only)
        // Transparent pixels have FLAG_UNWRITTEN (0xFFFF) in lower 16 bits
        if ((spritePixel & 0xFFFF) == 0xFFFF) {
            continue;
        }
        
        // Extract priority from flags
        uint8_t spritePriority = (spritePixel & FLAG_PRIORITY) >> OFFSET_PRIORITY;
        
        // Only process sprites at current priority level
        if (spritePriority != priority) {
            continue;
        }
        
        // Check if sprite can overwrite current pixel
        if (layerPriority >= priorityBuffer[x]) {
            continue;
        }
        
        // Extract color and flags
        uint16_t spriteColor = spritePixel & 0xFFFF;
        bool isSemiTransparent = (spritePixel & FLAG_SEMI_TRANSPARENT) != 0;
        bool isBlendTarget = (spritePixel & FLAG_TARGET_1) != 0;
        
        // Debug: Log sprite info during highlight phase (frame ~690 is when highlight is active)
        // Log at multiple X positions to see what's happening
        if (postFrame >= 690 && postFrame <= 695 && scanline == 110 && (x == 40 || x == 80 || x == 120 || x == 160)) {
            uint32_t objNum = (spritePixel & FLAG_ORDER_MASK) >> OFFSET_ORDER;
            LOG_TRACE_CAT("[POSTPROC F%d L%d x%d] obj%d color=0x%04X isBlend=%d isSemi=%d mode=%d 1stTgt=%d\n",
                   postFrame, scanline, x, objNum, spriteColor, isBlendTarget, isSemiTransparent,
                   blend.mode, blend.firstTargets);
        }
        
        // Save current pixel as second layer
        secondLayerBuffer[x] = lineBuffer[x];
        secondLayerTypeBuffer[x] = layerTypeBuffer[x];
        
        // Handle blending for sprites that are blend targets
        // Two cases:
        // 1. Semi-transparent sprites (mode=1): ALWAYS use alpha blend regardless of BLDCNT
        //    Per GBATEK: "other layers (BG0-3,BD,OBJ) as second target (regardless of BLDCNT settings)"
        //    But OBJ-to-OBJ blending is still NOT possible
        // 2. Normal sprites with FLAG_TARGET_1: Use BLDCNT settings (already verified as alpha)
        //    These need a valid 2nd target in BLDCNT for blending to occur
        if (isBlendTarget) {
            // Check if the layer behind is a valid 2nd target
            // Layer type 5 = backdrop, 0-3 = BG0-3, 4 = OBJ
            uint8_t behindLayerType = layerTypeBuffer[x];
            
            // OBJ-to-OBJ blending is NOT possible per GBATEK
            // "alpha blending can be used for OBJ-to-BG or BG-to-OBJ, but not for OBJ-to-OBJ"
            if (behindLayerType == 4) {
                // Can't blend OBJ with OBJ - just overwrite
                lineBuffer[x] = spriteColor;
                priorityBuffer[x] = layerPriority;
                layerTypeBuffer[x] = 4;
                continue;
            }
            
            // Check if blending should occur
            // Semi-transparent sprites: ALWAYS blend (except OBJ-to-OBJ checked above)
            // Normal sprites with FLAG_TARGET_1: Only blend if 2nd target is enabled in BLDCNT
            bool shouldBlend;
            if (isSemiTransparent) {
                // Semi-transparent sprites blend with ANY layer (BG0-3, BD) per GBATEK
                shouldBlend = true;
            } else {
                // Normal sprites: check if the second layer is enabled as 2nd target in BLDCNT
                // Bit 8=BG0, 9=BG1, 10=BG2, 11=BG3, 12=OBJ, 13=BD (backdrop)
                shouldBlend = (blend.secondTargets & (1 << behindLayerType)) != 0;
            }
            
            // Debug: Log blending decision with full blend control info
            // OBJ9 at F690: Y=104, X=92, 32x32 -> covers scanlines 104-135, x=92-123
            // Capture at scanline 110, x=100 (center-ish of sprite)
            if (postFrame >= 690 && postFrame <= 692 && scanline == 110 && x == 100) {
                uint32_t objNum = (spritePixel & FLAG_ORDER_MASK) >> OFFSET_ORDER;
                uint16_t bgColor = lineBuffer[x];
                uint16_t bldcnt = memory.readDirectIO16(0x04000050);
                uint16_t bldalpha = memory.readDirectIO16(0x04000052);
                uint16_t dispcnt = memory.readDirectIO16(0x04000000);
                uint16_t palette0 = memory.readDirectIO16(0x05000000);
                LOG_TRACE_CAT("=== BLEND DEBUG F%d L%d x%d ===\n", postFrame, scanline, x);
                LOG_TRACE_CAT("  DISPCNT: 0x%04X (mode=%d, BGs=%d%d%d%d, OBJ=%d)\n",
                       dispcnt, dispcnt & 7,
                       (dispcnt >> 8) & 1, (dispcnt >> 9) & 1, (dispcnt >> 10) & 1, (dispcnt >> 11) & 1,
                       (dispcnt >> 12) & 1);
                LOG_TRACE_CAT("  Palette 0: 0x%04X\n", palette0);
                LOG_TRACE_CAT("  OBJ#: %d\n", objNum);
                LOG_TRACE_CAT("  Sprite color: 0x%04X (R=%d G=%d B=%d)\n", 
                       spriteColor, spriteColor & 0x1F, (spriteColor >> 5) & 0x1F, (spriteColor >> 10) & 0x1F);
                LOG_TRACE_CAT("  BG color (lineBuffer): 0x%04X (R=%d G=%d B=%d)\n",
                       bgColor, bgColor & 0x1F, (bgColor >> 5) & 0x1F, (bgColor >> 10) & 0x1F);
                LOG_TRACE_CAT("  Behind layer type: %d (0-3=BG, 4=OBJ, 5=Backdrop)\n", behindLayerType);
                LOG_TRACE_CAT("  BLDCNT: 0x%04X (mode=%d, 1st=0x%02X, 2nd=0x%02X)\n", 
                       bldcnt, (bldcnt >> 6) & 3, bldcnt & 0x3F, (bldcnt >> 8) & 0x3F);
                LOG_TRACE_CAT("  BLDALPHA: 0x%04X (EVA=%d, EVB=%d)\n", bldalpha, blend.eva, blend.evb);
                LOG_TRACE_CAT("  isSemiTransparent: %d, isBlendTarget: %d\n", isSemiTransparent, isBlendTarget);
                LOG_TRACE_CAT("  shouldBlend: %d\n", shouldBlend);
                // Also show what BG3 is doing
                uint16_t bg3cnt = memory.readDirectIO16(0x04000000 + 0x0E);  // BG3CNT
                LOG_TRACE_CAT("  BG3CNT: 0x%04X (priority=%d, charBase=%d, screenBase=%d)\n",
                       bg3cnt, bg3cnt & 3, (bg3cnt >> 2) & 3, (bg3cnt >> 8) & 0x1F);
                
                uint16_t winin = memory.readDirectIO16(0x04000048);
                uint16_t winout = memory.readDirectIO16(0x0400004A);
                LOG_TRACE_CAT("  WININ: 0x%04X, WINOUT: 0x%04X\n", winin, winout);
                
                // Check if any OBJ Window sprites exist
                bool hasObjWin = false;
                LOG_TRACE_CAT("  OBJ Window Sprites:\n");
                for (int i = 0; i < 128; i++) {
                    OBJAttributes obj = readOBJAttributes(i);
                    if (obj.objMode == OBJ_MODE_OBJ_WINDOW && obj.visible) {
                        hasObjWin = true;
                        LOG_TRACE_CAT("    OBJ%d at (%d,%d) size=%dx%d\n", i, obj.x, obj.y, obj.width, obj.height);
                    }
                }
                if (!hasObjWin) LOG_TRACE_CAT("    None.\n");
                
                // Check Window Control for this pixel
                uint8_t winControlByte = getWindowControlForPixel(x, scanline, winCtrl);
                bool winObjEnable = (winControlByte & 0x10) != 0; // Bit 4
                bool winBlendEnable = (winControlByte & 0x20) != 0; // Bit 5
                
                LOG_TRACE_CAT("  Window Control at (%d,%d): objEnable=%d, blendEnable=%d, raw=0x%02X\n",
                       x, scanline, winObjEnable, winBlendEnable, winControlByte);
                       
                if (shouldBlend) {
                    uint8_t r1 = spriteColor & 0x1F;
                    uint8_t g1 = (spriteColor >> 5) & 0x1F;
                    uint8_t b1 = (spriteColor >> 10) & 0x1F;
                    uint8_t r2 = bgColor & 0x1F;
                    uint8_t g2 = (bgColor >> 5) & 0x1F;
                    uint8_t b2 = (bgColor >> 10) & 0x1F;
                    uint8_t rOut = (r1 * blend.eva + r2 * blend.evb) / 16;
                    uint8_t gOut = (g1 * blend.eva + g2 * blend.evb) / 16;
                    uint8_t bOut = (b1 * blend.eva + b2 * blend.evb) / 16;
                    LOG_TRACE_CAT("  Blended result: R=%d G=%d B=%d\n", rOut > 31 ? 31 : rOut, gOut > 31 ? 31 : gOut, bOut > 31 ? 31 : bOut);
                } else {
                    LOG_TRACE_CAT("  Final (no blend): 0x%04X\n", spriteColor);
                }
                LOG_TRACE_CAT("==============================\n");
            }
            
            if (shouldBlend) {
                // Blend sprite color with current lineBuffer color
                uint16_t bgColor = lineBuffer[x];
                
                // Debug: Log blending during highlight phase
                if (postFrame >= 690 && postFrame <= 695 && scanline == 110 && x == 120) {
                    uint32_t objNum = (spritePixel & FLAG_ORDER_MASK) >> OFFSET_ORDER;
                    LOG_TRACE_CAT("[BLEND F%d L%d x%d] obj%d spriteColor=0x%04X bgColor=0x%04X eva=%d evb=%d behindType=%d\n",
                           postFrame, scanline, x, objNum, spriteColor, bgColor, blend.eva, blend.evb, behindLayerType);
                }
            
                uint8_t r1 = spriteColor & 0x1F;
                uint8_t g1 = (spriteColor >> 5) & 0x1F;
                uint8_t b1 = (spriteColor >> 10) & 0x1F;
            
                uint8_t r2 = bgColor & 0x1F;
                uint8_t g2 = (bgColor >> 5) & 0x1F;
                uint8_t b2 = (bgColor >> 10) & 0x1F;
            
                // Apply alpha blend formula: (sprite * EVA + bg * EVB) / 16
                uint8_t r = (r1 * blend.eva + r2 * blend.evb) / 16;
                uint8_t g = (g1 * blend.eva + g2 * blend.evb) / 16;
                uint8_t b = (b1 * blend.eva + b2 * blend.evb) / 16;
            
                // Clamp to 5-bit range
                if (r > 31) r = 31;
                if (g > 31) g = 31;
                if (b > 31) b = 31;
            
                spriteColor = r | (g << 5) | (b << 10);
            }
            // If shouldBlend is false (only for normal sprites with no valid 2nd target),
            // sprite is rendered without blending
        }
        
        // Write to lineBuffer
        lineBuffer[x] = spriteColor;
        priorityBuffer[x] = layerPriority;
        layerTypeBuffer[x] = 4;  // Layer type 4 for sprites
    }
    
}
