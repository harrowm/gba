// Memory class implementation for GBA emulator (region pointer table version)
#include "memory.h"
#include "scheduler.h"
#include "timer_controller.h"
#include "dma.h"
#include "debug.h"
#include <cstring>
#include <cstdint>
#include <cstdlib>

Memory::Memory(bool testMode) {
    if (testMode) {
        // Only allocate and map test RAM at 0x00000000 (32KB)
        test_ram = (uint8_t*)std::malloc(32 * 1024);
        for (uint32_t addr = 0x00000000; addr < 0x00008000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = test_ram + (addr - 0x00000000);
        // All other region pointers remain null
        bios = wram = iwram = io = palette = vram = oam = rom = sram = nullptr;

    } else {
        // --- BIOS: 16KB at 0x00000000 ---
        bios = (uint8_t*)std::malloc(16 * 1024);
        regionTable[0x00000000 / BLOCK_SIZE] = bios;
        // Load BIOS file - try relative path first, then absolute path
        {
            FILE* biosFile = fopen("assets/bios.bin", "rb");
            if (!biosFile) {
                biosFile = fopen("/Users/malcolm/gba/assets/bios.bin", "rb");
            }
            if (biosFile) {
                size_t read = fread(bios, 1, 16 * 1024, biosFile);
                fclose(biosFile);
                if (read < 16 * 1024) {
                    DEBUG_ERROR("BIOS file too small, padding with zeros");
                    memset(bios + read, 0, 16 * 1024 - read);
                }
                DEBUG_INFO("BIOS loaded successfully: " + std::to_string(read) + " bytes");
            } else {
                DEBUG_ERROR("Failed to open BIOS file: assets/bios.bin");
                DEBUG_ERROR("BIOS is required for proper ROM execution");
                memset(bios, 0x1, 16 * 1024);
            }
        }

        // --- WRAM: 256KB at 0x02000000 ---
        wram = (uint8_t*)std::malloc(256 * 1024);
        for (uint32_t addr = 0x02000000; addr < 0x02040000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = wram + (addr - 0x02000000);

        // --- IWRAM: 32KB at 0x03000000 ---
        iwram = (uint8_t*)std::malloc(32 * 1024);
        regionTable[0x03000000 / BLOCK_SIZE] = iwram;

        // --- I/O: 1KB at 0x04000000 ---
        io = (uint8_t*)std::malloc(1 * 1024);
        regionTable[0x04000000 / BLOCK_SIZE] = io;
        // Initialize critical boot-related registers for clean BIOS boot
        io[0x300] = 0x00; // POSTFLG: Boot Flag (0=First boot from power-on, 1=Further boot/reset)
        io[0x301] = 0x00; // HALTCNT: Power Down Control
        io[0x204] = 0x00; // WAITCNT: Game Pak Waitstate Control
        io[0x200] = 0x00; // IE: Interrupt Enable Register
        io[0x202] = 0x00; // IF: Interrupt Request Flags
        io[0x208] = 0x00; // IME: Interrupt Master Enable
        io[0x130] = 0xFF; // KEYINPUT low byte: All buttons unpressed
        io[0x131] = 0x03; // KEYINPUT high byte: All buttons unpressed (0x03FF)

        // --- Palette RAM: 1KB at 0x05000000 ---
        palette = (uint8_t*)std::malloc(1 * 1024);
        regionTable[0x05000000 / BLOCK_SIZE] = palette;

        // --- VRAM: 96KB at 0x06000000, mirrored in 128KB ---
        vram = (uint8_t*)std::malloc(96 * 1024);
        regionTable[0x06000000 / BLOCK_SIZE] = vram;
        regionTable[0x06010000 / BLOCK_SIZE] = vram + (0x010000); // 64KB offset

        // --- OAM: 1KB at 0x07000000, mirrored in 8KB ---
        oam = (uint8_t*)std::malloc(1 * 1024);
        for (uint32_t addr = 0x07000000; addr < 0x07002000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = oam;

        // --- Game Pak ROM: up to 32MB at 0x08000000 ---
        rom = (uint8_t*)std::malloc(32 * 1024 * 1024);
        memset(rom, 0, 32 * 1024 * 1024);  // Initialize with zeros
        for (uint32_t addr = 0x08000000; addr < 0x0A000000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = rom + (addr - 0x08000000);

        // --- Game Pak SRAM: 64KB at 0x0E000000 ---
        sram = (uint8_t*)std::malloc(64 * 1024);
        regionTable[0x0E000000 / BLOCK_SIZE] = sram;

        // Test RAM not used in normal mode
        test_ram = nullptr;
    }

}

Memory::~Memory() {
    std::free(bios);
    std::free(wram);
    std::free(iwram);
    std::free(io);
    std::free(palette);
    std::free(vram);
    std::free(oam);
    std::free(rom);
    std::free(sram);
    std::free(test_ram);
}

// Helper: get region base and offset, handling VRAM/OAM mirroring

inline uint8_t* get_region_base(uint8_t* const* regionTable, uint32_t address, uint32_t& offset) {
    uint32_t block = (address & 0x0FFFFFFF) / Memory::BLOCK_SIZE;
    uint8_t* base = regionTable[block];
    offset = address % Memory::BLOCK_SIZE;
    // VRAM mirroring: 0x06000000–0x0601FFFF, 96KB mirrored in 128KB
    if (address >= 0x06000000 && address < 0x06020000 && base) {
        offset = (address - 0x06000000) % (96 * 1024);
    }
    // OAM mirroring: 0x07000000–0x07001FFF, 1KB mirrored in 8KB
    if (address >= 0x07000000 && address < 0x07002000 && base) {
        offset = (address - 0x07000000) % 1024;
    }
    return base;
}

uint8_t Memory::read8(uint32_t address) const {
    addWaitCycles(address, 8);
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFF;
    // Debug: Print reads from logo and entry point
    if (address == 0x0800009C || address == 0x080000B4) {
        printf("[Memory::read8] Read from 0x%08X: 0x%02X\n", address, base[offset]);
    }
    // Debug: Print POSTFLG reads
    if (address == 0x04000300) {
        printf("[Memory::read8] POSTFLG read: 0x%02X\n", base[offset]);
    }
    return base[offset];
}

void Memory::write8(uint32_t address, uint8_t value) {
    addWaitCycles(address, 8);
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    // Debug: Track VRAM writes (commented out - verified working)
    // if (address >= 0x06000000 && address < 0x06018000) {
    //     printf("[VRAM Write8] Address: 0x%08X, Value: 0x%02X\n", address, value);
    // }
    
    // Debug: Track POSTFLG writes
    if (address == 0x04000300) {
        printf("[Memory::write8] POSTFLG write: 0x%02X (was 0x%02X)\n", value, base[offset]);
    }
    
    base[offset] = value;
}

uint16_t Memory::read16(uint32_t address) const {
    addWaitCycles(address, 16);
    
    // Handle DMA register reads (word count and control only)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 8) {  // Word count (DMAxCNT_L)
                return dmaController->readWordCount(channelID);
            } else if (regOffset == 10) {  // Control (DMAxCNT_H)
                return dmaController->readControl(channelID);
            }
        }
    }
    
    // Handle timer register reads
    if (timerController) {
        if (address >= 0x04000100 && address <= 0x0400010E) {
            int timerID = (address - 0x04000100) / 4;
            bool isControl = ((address - 0x04000100) % 4) == 2;
            
            if (isControl) {
                return timerController->readControl(timerID);
            } else {
                return timerController->readCounter(timerID);
            }
        }
    }
    
    uint32_t rot = (address & 1) * 8;
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFFFF;
    uint16_t val = base[offset] | (base[(offset + 1) % Memory::BLOCK_SIZE] << 8);
    return (val >> rot) | (val << (16 - rot));
}

void Memory::write16(uint32_t address, uint16_t value) {
    addWaitCycles(address, 16);
    
    // Handle DMA register writes (word count and control only)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 8) {  // Word count (DMAxCNT_L)
                dmaController->writeWordCount(channelID, value);
                return;  // Don't write to memory
            } else if (regOffset == 10) {  // Control (DMAxCNT_H)
                dmaController->writeControl(channelID, value);
                return;  // Don't write to memory
            }
        }
    }
    
    // Handle timer register writes
    if (timerController) {
        if (address >= 0x04000100 && address <= 0x0400010E) {
            int timerID = (address - 0x04000100) / 4;
            bool isControl = ((address - 0x04000100) % 4) == 2;
            
            if (isControl) {
                timerController->writeControl(timerID, value);
            } else {
                timerController->writeReload(timerID, value);
            }
            // Don't return - also write to memory for debugging
        }
    }
    
    uint32_t rot = (address & 1) * 8;
    uint16_t val = (value << rot) | (value >> (16 - rot));
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    // Special handling for IF register (write 1 to clear)
    if (address == 0x04000202) {  // REG_IF
        uint16_t currentIF = base[offset] | (base[(offset + 1) % Memory::BLOCK_SIZE] << 8);
        uint16_t newIF = currentIF & ~val;  // Clear bits where value has 1
        base[offset] = newIF & 0xFF;
        base[(offset + 1) % Memory::BLOCK_SIZE] = (newIF >> 8) & 0xFF;
        printf("[REG Write] IF acknowledge = 0x%04X, new IF = 0x%04X\n", val, newIF);
        return;
    }
    
    // Debug: Track VRAM writes (commented out - verified working)
    // if (address >= 0x06000000 && address < 0x06018000) {
    //     printf("[VRAM Write16] Address: 0x%08X, Value: 0x%04X (RGB555)\n", address, value);
    // }
    
    // Debug: Track important register writes
    if (address == 0x04000000) { // REG_DISPCNT
        printf("[REG Write] DISPCNT = 0x%04X (Mode: %d, BG0-3: %d%d%d%d, OBJ: %d)\n",
               value, value & 0x7,
               (value >> 8) & 1, (value >> 9) & 1, (value >> 10) & 1, (value >> 11) & 1,
               (value >> 12) & 1);
    } else if (address == 0x04000208) { // REG_IME
        printf("[REG Write] IME = 0x%04X (Interrupts %s)\n", 
               value, (value & 1) ? "ENABLED" : "DISABLED");
    } else if (address == 0x04000200) { // REG_IE
        printf("[REG Write] IE = 0x%04X (Enabled interrupts)\n", value);
    } else if (address == 0x04000202) { // REG_IF
        printf("[REG Write] IF = 0x%04X (Acknowledge interrupts)\n", value);
    }
    
    base[offset] = val & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (val >> 8) & 0xFF;
}

void Memory::writeDirectIO(uint32_t address, uint16_t value) {
    // Direct write to I/O registers (bypasses write-to-clear and other special handling)
    // Used by hardware components to set registers
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    base[offset] = value & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (value >> 8) & 0xFF;
}

uint32_t Memory::read32(uint32_t address) const {
    addWaitCycles(address, 32);
    
    // Handle DMA register reads (source and dest addresses)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 0) {  // Source address (DMAxSAD)
                return dmaController->readSourceAddress(channelID);
            } else if (regOffset == 4) {  // Dest address (DMAxDAD)
                return dmaController->readDestAddress(channelID);
            }
        }
    }
    
    uint32_t rot = (address & 3) * 8;
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFFFFFFFF;
    uint32_t val = base[offset]
        | (base[(offset + 1) % Memory::BLOCK_SIZE] << 8)
        | (base[(offset + 2) % Memory::BLOCK_SIZE] << 16)
        | (base[(offset + 3) % Memory::BLOCK_SIZE] << 24);
    // Debug: Print reads from entry point
    if (address == 0x080000B4) {
        printf("[Memory::read32] Read from 0x%08X: 0x%08X\n", address, val);
    }
    return (val >> rot) | (val << (32 - rot));
}

void Memory::write32(uint32_t address, uint32_t value) {
    addWaitCycles(address, 32);
    
    // Handle DMA register writes (source and dest addresses)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 0) {  // Source address (DMAxSAD)
                dmaController->writeSourceAddress(channelID, value);
                return;  // Don't write to memory
            } else if (regOffset == 4) {  // Dest address (DMAxDAD)
                dmaController->writeDestAddress(channelID, value);
                return;  // Don't write to memory
            }
        }
    }
    
    uint32_t rot = (address & 3) * 8;
    uint32_t val = (value << rot) | (value >> (32 - rot));
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    // Debug: Track VRAM writes (commented out - verified working)
    // if (address >= 0x06000000 && address < 0x06018000) {
    //     printf("[VRAM Write32] Address: 0x%08X, Value: 0x%08X (2 pixels)\n", address, value);
    // }
    
    base[offset] = val & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (val >> 8) & 0xFF;
    base[(offset + 2) % Memory::BLOCK_SIZE] = (val >> 16) & 0xFF;
    base[(offset + 3) % Memory::BLOCK_SIZE] = (val >> 24) & 0xFF;
}

// ============================================================================
// Memory Timing (Wait States)
// ============================================================================

uint32_t Memory::calculateWaitStates(uint32_t address, uint32_t accessWidth) const {
    // Mask to 28-bit address space (GBA mirrors upper 4 bits)
    uint32_t addr = address & 0x0FFFFFFF;
    
    // Determine memory region and calculate wait states
    switch (addr >> 24) {
        case 0x00: // BIOS ROM (16KB)
            // 1 cycle for any access
            return 1;
            
        case 0x02: // EWRAM (256KB on-board Work RAM)
            // 16-bit bus: 3 cycles for 8/16-bit, 6 cycles for 32-bit
            return (accessWidth == 32) ? 6 : 3;
            
        case 0x03: // IWRAM (32KB on-chip Work RAM)
            // 32-bit bus: 1 cycle for any access (fastest)
            return 1;
            
        case 0x04: // I/O Registers
            // 1 cycle for any access
            return 1;
            
        case 0x05: // Palette RAM (1KB)
            // 16-bit bus: 1 cycle for 16-bit, 2 cycles for 32-bit
            // +1 if video controller accessing (not implemented yet)
            return (accessWidth == 32) ? 2 : 1;
            
        case 0x06: // VRAM (96KB)
            // 16-bit bus: 1 cycle for 16-bit, 2 cycles for 32-bit
            // +1 if video controller accessing (not implemented yet)
            return (accessWidth == 32) ? 2 : 1;
            
        case 0x07: // OAM (1KB)
            // 32-bit bus: 1 cycle for any access
            // +1 if video controller accessing (not implemented yet)
            return 1;
            
        case 0x08: // Game Pak ROM Wait State 0
        case 0x09:
        case 0x0A: // Game Pak ROM Wait State 1
        case 0x0B:
        case 0x0C: // Game Pak ROM Wait State 2
        case 0x0D:
            // Default: 5 cycles for first access (non-sequential)
            // TODO: Make configurable via WAITCNT register
            // 16-bit bus: 5 cycles for 8/16-bit, 8 cycles for 32-bit (5+3 sequential)
            return (accessWidth == 32) ? 8 : 5;
            
        case 0x0E: // Game Pak SRAM
            // 8-bit bus: 5 cycles for any access
            // TODO: Make configurable via WAITCNT register
            return 5;
            
        default:
            // Unmapped memory - no wait states (open bus)
            return 1;
    }
}

void Memory::addWaitCycles(uint32_t address, uint32_t accessWidth) const {
    if (scheduler) {
        uint32_t cycles = calculateWaitStates(address, accessWidth);
        // Advance scheduler by wait state cycles
        uint64_t targetCycle = scheduler->getCurrentCycle() + cycles;
        scheduler->runUntil(targetCycle);
    }
}

uint32_t Memory::getWaitStates(uint32_t address, uint32_t accessWidth) const {
    return calculateWaitStates(address, accessWidth);
}

bool Memory::loadROM(const char* filepath) {
    FILE* romFile = fopen(filepath, "rb");
    if (!romFile) {
        fprintf(stderr, "Error: Failed to open ROM file: %s\n", filepath);
        return false;
    }
    
    // Get file size
    fseek(romFile, 0, SEEK_END);
    long fileSize = ftell(romFile);
    fseek(romFile, 0, SEEK_SET);
    
    if (fileSize < 0) {
        fprintf(stderr, "Error: Failed to determine ROM file size\n");
        fclose(romFile);
        return false;
    }
    
    // Validate ROM size (typical GBA ROMs are up to 32MB)
    if (fileSize > 32 * 1024 * 1024) {
        fprintf(stderr, "Warning: ROM file larger than 32MB (%ld bytes), truncating\n", fileSize);
        fileSize = 32 * 1024 * 1024;
    }
    
    // Minimum valid GBA ROM size (must have header)
    if (fileSize < 192) {
        fprintf(stderr, "Error: ROM file too small to be valid GBA ROM (%ld bytes)\n", fileSize);
        fclose(romFile);
        return false;
    }
    
    // Read ROM into buffer
    size_t read = fread(rom, 1, fileSize, romFile);
    fclose(romFile);
    
    if (read != (size_t)fileSize) {
        fprintf(stderr, "Error: Failed to read complete ROM file (read %zu of %ld bytes)\n", read, fileSize);
        return false;
    }
    
    // Zero-fill remaining ROM space
    if (read < 32 * 1024 * 1024) {
        memset(rom + read, 0, 32 * 1024 * 1024 - read);
    }
    
    printf("ROM loaded: %zu bytes from %s\n", read, filepath);
    
    // Display ROM header info
    char title[13];
    memcpy(title, rom + 0xA0, 12);
    title[12] = '\0';
    printf("ROM Title: %s\n", title);
    printf("Game Code: %.4s\n", rom + 0xAC);
    printf("Maker Code: %.2s\n", rom + 0xB0);
    
    return true;
}
