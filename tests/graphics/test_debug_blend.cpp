#include <gtest/gtest.h>
#include <iostream>
#include "gba.h"
#include "memory.h"
#include "gpu.h"

TEST(DebugTest, BlendTargets) {
    Memory memory;
    GPU gpu(memory);
    
    // Set up BG0 with red
    memory.write16(0x04000000, 0x0100);  // Enable BG0
    memory.write16(0x04000008, (1 << 8));  // Screen base 1, priority 1
    memory.write16(0x05000002, 0x001F);  // Red at palette 0, color 1
    for (int i = 0; i < 32*32; i++) {
        memory.write16(0x06000800 + i*2, 0x0001);
    }
    for (int i = 0; i < 32; i++) {
        memory.write8(0x06000000 + 32 + i, 0x11);
    }
    
    // Set up BG1 with blue
    memory.write16(0x04000000, 0x0200);  // Enable BG1
    memory.write16(0x0400000A, (2 << 8) | (1 << 12));  // Screen base 2, palette 1, priority 0
    memory.write16(0x05000022, 0x7C00);  // Blue at palette 1, color 1
    for (int i = 0; i < 32*32; i++) {
        memory.write16(0x06001000 + i*2, 0x0001);
    }
    
    // Enable both
    memory.write16(0x04000000, 0x0300);
    
    // Render without blend
    gpu.renderScanline(0);
    uint16_t* fb = gpu.getTiledFramebuffer();
    
    std::cout << "Pixel[0] = 0x" << std::hex << fb[0] << std::endl;
    std::cout << "Expected blue (0x7C00) since BG1 has priority 0" << std::endl;
}
