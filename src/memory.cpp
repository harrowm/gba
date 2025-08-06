// Memory class implementation for GBA emulator (region pointer table version)
#include "memory.h"
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

        // --- Palette RAM: 1KB at 0x05000000 ---
        palette = (uint8_t*)std::malloc(1 * 1024);
        regionTable[0x05000000 / BLOCK_SIZE] = palette;

        // --- VRAM: 96KB at 0x06000000, mirrored in 128KB ---
        vram = (uint8_t*)std::malloc(96 * 1024);
        // Map 0x06000000–0x06017FFF (96KB)
        regionTable[0x06000000 / BLOCK_SIZE] = vram;
        // Mirror: 0x06010000–0x0601FFFF (next 64KB block, points to vram + 64KB)
        regionTable[0x06010000 / BLOCK_SIZE] = vram + (0x010000); // 64KB offset

        // --- OAM: 1KB at 0x07000000, mirrored in 8KB ---
        oam = (uint8_t*)std::malloc(1 * 1024);
        for (uint32_t addr = 0x07000000; addr < 0x07002000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = oam;

        // --- Game Pak ROM: up to 32MB at 0x08000000 ---
        rom = (uint8_t*)std::malloc(32 * 1024 * 1024);
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
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFF;
    return base[offset];
}

void Memory::write8(uint32_t address, uint8_t value) {
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    base[offset] = value;
}

uint16_t Memory::read16(uint32_t address) const {
    uint32_t rot = (address & 1) * 8;
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFFFF;
    uint16_t val = base[offset] | (base[(offset + 1) % Memory::BLOCK_SIZE] << 8);
    return (val >> rot) | (val << (16 - rot));
}

void Memory::write16(uint32_t address, uint16_t value) {
    uint32_t rot = (address & 1) * 8;
    uint16_t val = (value << rot) | (value >> (16 - rot));
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    base[offset] = val & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (val >> 8) & 0xFF;
}

uint32_t Memory::read32(uint32_t address) const {
    uint32_t rot = (address & 3) * 8;
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFFFFFFFF;
    uint32_t val = base[offset]
        | (base[(offset + 1) % Memory::BLOCK_SIZE] << 8)
        | (base[(offset + 2) % Memory::BLOCK_SIZE] << 16)
        | (base[(offset + 3) % Memory::BLOCK_SIZE] << 24);
    return (val >> rot) | (val << (32 - rot));
}

void Memory::write32(uint32_t address, uint32_t value) {
    uint32_t rot = (address & 3) * 8;
    uint32_t val = (value << rot) | (value >> (32 - rot));
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    base[offset] = val & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (val >> 8) & 0xFF;
    base[(offset + 2) % Memory::BLOCK_SIZE] = (val >> 16) & 0xFF;
    base[(offset + 3) % Memory::BLOCK_SIZE] = (val >> 24) & 0xFF;
}
