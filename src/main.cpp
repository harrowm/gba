#include "gba.h"
#include "display.h"
#include "debug.h"
#include <cstdio>
#include <cstring>

// Simple test pattern generator
void fillTestPattern(uint16_t* framebuffer) {
    // Create a gradient test pattern
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 240; x++) {
            // Create RGB555 gradient
            // Red increases left to right
            // Green increases top to bottom
            // Blue stays at max
            int r = (x * 31) / 239;      // 0-31 across width
            int g = (y * 31) / 159;      // 0-31 across height
            int b = 31;                   // Full blue
            
            uint16_t color = (b << 10) | (g << 5) | r;
            framebuffer[y * 240 + x] = color;
        }
    }
}

int main(int argc, char* argv[]) {
    (void)argc;  // Unused for now
    (void)argv;  // Unused for now
    
    printf("GBA Emulator Starting...\n");
    
    try {
        // Create display with 3x scaling (720x480 window)
        Display display(3);
        printf("SDL2 Display initialized\n");
        
        // Create GBA instance
        GBA gba;
        printf("GBA initialized\n");
        
        // Get direct access to VRAM for test pattern
        GPU& gpu = gba.getGPU();
        uint16_t* vram = gpu.getFrameBuffer();
        
        // Fill with test pattern
        fillTestPattern(vram);
        printf("Test pattern loaded into VRAM\n");
        
        // Set GPU to Mode 3 (bitmap mode)
        // DISPCNT at 0x04000000, Mode 3 = bits 0-2 = 3, BG2 enable = bit 10
        Memory& mem = gba.getMemory();
        mem.write16(0x04000000, 0x0403);  // Mode 3 | BG2 enable
        printf("GPU set to Mode 3\n");
        
        printf("\nStarting main loop...\n");
        printf("Press ESC or close window to quit\n\n");
        
        int frameCount = 0;
        
        // Main loop
        while (!display.shouldQuit()) {
            // Run one frame of emulation (280,896 cycles)
            // This will trigger scanline rendering and V-Blank
            gba.runFrame();
            
            // Render the frame to display
            display.renderFrame(vram);
            
            // Handle SDL events (keyboard, window close)
            display.handleEvents();
            
            frameCount++;
            if (frameCount % 60 == 0) {
                printf("Frame %d\n", frameCount);
            }
        }
        
        printf("\nEmulator shutdown cleanly\n");
        printf("Total frames rendered: %d\n", frameCount);
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
