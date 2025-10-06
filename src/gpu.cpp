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
    // The framebuffer is already in VRAM, so rendering just means
    // the data is available for display. In a real emulator, we'd
    // copy this to the actual display buffer here.
    
    // For now, we just verify the data is accessible
    uint16_t* framebuffer = getFrameBuffer();
    if (framebuffer) {
        // In a full implementation, we'd copy this scanline to the display
        // For example: memcpy(displayBuffer + scanline * 240, framebuffer + scanline * 240, 240 * 2);
        UNUSED(framebuffer); // Suppress warning for now
    }
}
