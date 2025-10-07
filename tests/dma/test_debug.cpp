#include <iostream>
#include "gba.h"

int main() {
    GBA gba(true);
    
    uint32_t srcAddr = 0x02000000;
    uint32_t destAddr = 0x02001000;
    
    // Fill source
    for (int i = 0; i < 64; i++) {
        gba.getMemory().write8(srcAddr + i, 0xAA + (i & 0xFF));
    }
    
    std::cout << "Setting up DMA1 with V-Blank timing...\n";
    
    // Setup DMA1 with V-Blank timing
    uint16_t control = 0x9000;  // Enable + V-Blank
    gba.getMemory().write32(0x040000BC, srcAddr);     // DMA1 source
    gba.getMemory().write32(0x040000C0, destAddr);    // DMA1 dest
    gba.getMemory().write16(0x040000C4, 32);          // DMA1 word count
    
    std::cout << "Before control write, destAddr[0] = " << (int)gba.getMemory().read8(destAddr) << "\n";
    
    gba.getMemory().write16(0x040000C6, control);     // DMA1 control (triggers setup)
    
    std::cout << "After control write, before V-Blank, destAddr[0] = " << (int)gba.getMemory().read8(destAddr) << "\n";
    
    // Trigger V-Blank
    std::cout << "Triggering V-Blank...\n";
    gba.getDMAController().triggerVBlank();
    
    std::cout << "After V-Blank, destAddr[0] = " << (int)gba.getMemory().read8(destAddr) << "\n";
    std::cout << "After V-Blank, destAddr[1] = " << (int)gba.getMemory().read8(destAddr+1) << "\n";
    
    bool success = true;
    for (int i = 0; i < 64; i++) {
        uint8_t expected = 0xAA + (i & 0xFF);
        uint8_t actual = gba.getMemory().read8(destAddr + i);
        if (actual != expected) {
            std::cout << "Mismatch at offset " << i << ": expected " << (int)expected << ", got " << (int)actual << "\n";
            success = false;
            if (i > 5) break;
        }
    }
    
    if (success) {
        std::cout << "SUCCESS: V-Blank DMA transfer worked!\n";
        return 0;
    } else {
        std::cout << "FAILED: V-Blank DMA transfer did not work\n";
        return 1;
    }
}
