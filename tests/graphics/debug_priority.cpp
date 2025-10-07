#include <iostream>
#include "gba.h"
#include "gpu.h"
#include "memory.h"

int main() {
    GBA gba(false);
    GPU* gpu = &gba.getGPU();
    Memory* memory = &gba.getMemory();
    
    // Setup backdrop
    memory->write16(0x05000000, 0x0000); // Black
    
    // Setup BG0 with priority 0, red at screen base 30, char base 0
    uint32_t bg0cnt = 0 | (0 << 2) | (30 << 8); // Priority 0, char 0, screen 30
    memory->write16(0x04000008, bg0cnt);
    std::cout << "BG0CNT: 0x" << std::hex << bg0cnt << std::endl;
    
    // Setup BG1 with priority 1, green at screen base 29, char base 0
    uint32_t bg1cnt = 1 | (0 << 2) | (29 << 8); // Priority 1, char 0, screen 29
    memory->write16(0x0400000A, bg1cnt);
    std::cout << "BG1CNT: 0x" << std::hex << bg1cnt << std::endl;
    
    // Setup tile 1 in char base 0
    uint32_t tileAddr = 0x06000000 + 32;
    for (int i = 0; i < 32; i++) {
        memory->write8(tileAddr + i, 0x11); // Palette index 1
    }
    
    // Setup tilemap for BG0 (screen base 30)
    uint32_t tilemap0 = 0x06000000 + (30 * 0x800);
    memory->write16(tilemap0, 1); // Tile 1
    std::cout << "BG0 tilemap addr: 0x" << std::hex << tilemap0 << std::endl;
    
    // Setup tilemap for BG1 (screen base 29)
    uint32_t tilemap1 = 0x06000000 + (29 * 0x800);
    memory->write16(tilemap1, 1); // Tile 1
    std::cout << "BG1 tilemap addr: 0x" << std::hex << tilemap1 << std::endl;
    
    // Setup palettes
    memory->write16(0x05000000 + 2, 0x001F); // Palette 0, index 1 = red
    memory->write16(0x05000000 + 4, 0x03E0); // Palette 0, index 2 = green
    
    // Enable BG0 and BG1
    memory->write16(0x04000000, 0x0300); // Mode 0, BG0 + BG1
    
    // Render scanline 0
    gpu->renderScanline();
    
    // Check framebuffer
    const uint16_t* fb = gpu->getTiledFramebuffer();
    std::cout << "FB[0] = 0x" << std::hex << fb[0] << " (expect red 0x001F)" << std::endl;
    std::cout << "FB[1] = 0x" << std::hex << fb[1] << std::endl;
    
    // Read back BGxCNT values
    BGConfig bg0 = gpu->readBGCNT(0);
    BGConfig bg1 = gpu->readBGCNT(1);
    std::cout << "BG0 priority: " << std::dec << (int)bg0.priority << std::endl;
    std::cout << "BG1 priority: " << std::dec << (int)bg1.priority << std::endl;
    
    return 0;
}
