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

void printUsage(const char* programName) {
    printf("GBA Emulator - Game Boy Advance Emulator\n\n");
    printf("Usage: %s [options] [rom_path]\n\n", programName);
    printf("Arguments:\n");
    printf("  rom_path            Path to GBA ROM file (.gba)\n\n");
    printf("Options:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -t, --test-pattern  Use test pattern instead of ROM\n\n");
    printf("Examples:\n");
    printf("  %s game.gba                    # Load and run game.gba\n", programName);
    printf("  %s assets/roms/sonic.bin       # Load ROM from assets\n", programName);
    printf("  %s --test-pattern              # Run with gradient test pattern\n", programName);
    printf("  %s                             # Run with test pattern (default)\n\n", programName);
    printf("Controls:\n");
    printf("  ESC                 Quit emulator\n");
}

int main(int argc, char* argv[]) {
    printf("GBA Emulator Starting...\n");
    
    // Parse command-line arguments
    const char* romPath = nullptr;
    bool useTestPattern = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--test-pattern") == 0 || strcmp(argv[i], "-t") == 0) {
            useTestPattern = true;
        } else if (argv[i][0] != '-') {
            // Assume it's a ROM path
            if (romPath != nullptr) {
                fprintf(stderr, "Error: Multiple ROM files specified\n");
                printUsage(argv[0]);
                return 1;
            }
            romPath = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }
    
    try {
        // Create display with 3x scaling (720x480 window)
        Display display(3);
        printf("SDL2 Display initialized\n");
        
        // Create GBA instance
        GBA gba;
        printf("GBA initialized\n");
        
        // Load ROM or use test pattern
        if (useTestPattern || !romPath) {
            if (!romPath && !useTestPattern) {
                printf("No ROM specified, using test pattern (use --help for usage)\n\n");
            }
            // Use test pattern mode
            GPU& gpu = gba.getGPU();
            uint16_t* vram = gpu.getFrameBuffer();
            fillTestPattern(vram);
            printf("Test pattern loaded into VRAM\n");
        } else {
            // Load ROM from file
            printf("\nLoading ROM: %s\n", romPath);
            if (!gba.loadROM(romPath)) {
                fprintf(stderr, "Failed to load ROM file\n");
                return 1;
            }
            printf("\n");
            
            // Check if this is our simple test ROM (test_pixels.gba)
            if (romPath && strstr(romPath, "test_pixels") != nullptr) {
                printf("Detected test_pixels.gba - skipping BIOS boot\n");
                gba.skipBIOS();
            }
        }
        
        // Set GPU to Mode 3 (bitmap mode)
        // DISPCNT at 0x04000000, Mode 3 = bits 0-2 = 3, BG2 enable = bit 10
        Memory& mem = gba.getMemory();
        mem.write16(0x04000000, 0x0403);  // Mode 3 | BG2 enable
        printf("GPU set to Mode 3\n");
        
        // Get VRAM pointer for rendering
        GPU& gpu = gba.getGPU();
        uint16_t* vram = gpu.getFrameBuffer();
        
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
