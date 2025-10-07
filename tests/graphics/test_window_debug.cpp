#include <iostream>
#include "gba.h"
#include "memory.h"
#include "gpu.h"

int main() {
    Memory memory;
    GPU gpu(memory);
    
    // Set up simple BG0
    memory.write16(0x04000000, 0x0100);  // Enable BG0
    memory.write16(0x04000008, 0x0100);  // Priority 0, screenbase 1
    memory.write16(0x05000002, 0x001F);  // Red color
    memory.write16(0x06000800, 0x0001);  // Tilemap: tile 1
    
    // Fill tile 1
    for (int i = 0; i < 32; i++) {
        memory.write8(0x06000000 + 32 + i, 0x11);
    }
    
    // Render without windows
    gpu.renderScanline(0);
    uint16_t* fb = gpu.getTiledFramebuffer();
    
    std::cout << "Without windows: pixel[0] = 0x" << std::hex << fb[0] << std::endl;
    std::cout << "Expected: 0x001F (red)" << std::endl;
    
    return 0;
}
