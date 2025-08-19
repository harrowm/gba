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
        // Load BIOS file
        {
            FILE* biosFile = fopen("/Users/malcolm/gba/assets/bios.bin", "rb");
            if (biosFile) {
                size_t read = fread(bios, 1, 16 * 1024, biosFile);
                fclose(biosFile);
                if (read < 16 * 1024) {
                    DEBUG_ERROR("BIOS file too small, padding with zeros");
                    memset(bios + read, 0, 16 * 1024 - read);
                }
            } else {
                DEBUG_ERROR("Failed to open BIOS file: assets/bios.bin");
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
        io[0x300] = 0x01; // POSTFLG: First Boot Flag
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
        for (uint32_t addr = 0x08000000; addr < 0x0A000000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = rom + (addr - 0x08000000);
        // Load GamePak file
        {
            FILE* romFile = fopen("/Users/malcolm/gba/assets/roms/gamepak.bin", "rb");
            if (romFile) {
                size_t read = fread(rom, 1, 32 * 1024 * 1024, romFile);
                fclose(romFile);
                if (read < 32 * 1024 * 1024) {
                    DEBUG_ERROR("GamePak file too small, padding with zeros");
                    memset(rom + read, 0, 32 * 1024 * 1024 - read);
                }
            } else {
                DEBUG_ERROR("Failed to open GamePak file: assets/roms/gamepak.bin");
                memset(rom, 0, 32 * 1024 * 1024);
            }
        }

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
    // Debug: Print reads from logo and entry point
    if (address == 0x0800009C || address == 0x080000B4) {
        printf("[Memory::read8] Read from 0x%08X: 0x%02X\n", address, base[offset]);
    }
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
    // Debug: Print reads from entry point
    if (address == 0x080000B4) {
        printf("[Memory::read32] Read from 0x%08X: 0x%08X\n", address, val);
    }
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
