// Memory class implementation for GBA emulator
// Contains method definitions for memory operations

#include "memory.h"
#include "debug.h" // Include Debug class
#include <fstream>
#include <iostream>

Memory::Memory(uint32_t size, bool initializeGBA) : data(size, 0) {
    if (initializeGBA) {
        initializeGBARegions("assets/bios.bin", "assets/roms/gamepak.bin");
    } else {
        initializeTestRegions();
    }
}

Memory::~Memory() {}

uint8_t Memory::read8(uint32_t address) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    return data[address];
}

uint16_t Memory::read16(uint32_t address, bool big_endian /* = false */) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    uint16_t value = (data[address] | (data[address + 1] << 8));
    return big_endian ? __builtin_bswap16(value) : value;
}

uint32_t Memory::read32(uint32_t address, bool big_endian /* = false */) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    uint32_t value = (data[address] | (data[address + 1] << 8) |
                      (data[address + 2] << 16) | (data[address + 3] << 24));
    return big_endian ? __builtin_bswap32(value) : value;
}

bool Memory::isAddressInROM(uint32_t address) const {
    auto it = std::lower_bound(romRegions.begin(), romRegions.end(), address,
        [](const std::pair<uint32_t, uint32_t>& region, uint32_t addr) {
            return region.second < addr;
        });
    return (it != romRegions.end() && it->first <= address && address <= it->second);
}

void Memory::write8(uint32_t address, uint8_t value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (isAddressInROM(address)) {
        Debug::log::info("Attempted write to ROM address " + std::to_string(address) + ", write ignored.");
        return;
    }
    data[address] = value;
}

void Memory::write16(uint32_t address, uint16_t value, bool big_endian /* = false */) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (isAddressInROM(address)) {
        Debug::log::info("Attempted write to ROM address " + std::to_string(address) + ", write ignored.");
        return;
    }
    if (big_endian) value = __builtin_bswap16(value);
    data[address] = value & 0xFF;
    data[address + 1] = (value >> 8) & 0xFF;
}

void Memory::write32(uint32_t address, uint32_t value, bool big_endian /* = false */) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (isAddressInROM(address)) {
        Debug::log::info("Attempted write to ROM address " + std::to_string(address) + ", write ignored.");
        return;
    }
    if (big_endian) value = __builtin_bswap32(value);
    data[address] = value & 0xFF;
    data[address + 1] = (value >> 8) & 0xFF;
    data[address + 2] = (value >> 16) & 0xFF;
    data[address + 3] = (value >> 24) & 0xFF;
}

void Memory::initializeGBARegions(const std::string& biosFilename, const std::string& gamePakFilename) {
    regions = {
        {0x00000000, 0x00003FFF, MEMORY_TYPE_ROM, 32}, // BIOS
        {0x02000000, 0x0203FFFF, MEMORY_TYPE_RAM, 32}, // WRAM
        {0x03000000, 0x03007FFF, MEMORY_TYPE_RAM, 32}, // IWRAM
        {0x05000000, 0x050003FF, MEMORY_TYPE_RAM, 16}, // Palette RAM
        {0x06000000, 0x06017FFF, MEMORY_TYPE_RAM, 16}, // VRAM
        {0x07000000, 0x070003FF, MEMORY_TYPE_RAM, 16}, // OAM
        {0x08000000, 0x09FFFFFF, MEMORY_TYPE_ROM, 32}, // Game Pak ROM
        {0x0E000000, 0x0E00FFFF, MEMORY_TYPE_RAM, 16}  // Game Pak SRAM
    };

    romRegions = {
        {0x00000000, 0x00003FFF}, // BIOS
        {0x08000000, 0x09FFFFFF}  // Game Pak ROM
    };

    // Load BIOS ROM
    std::ifstream biosFile(biosFilename, std::ios::binary);
    if (biosFile.is_open()) {
        biosFile.read(reinterpret_cast<char*>(&data[0x00000000]), 0x4000); // Load 16KB BIOS
        biosFile.close();
    } else {
        Debug::log::error("Failed to load BIOS ROM from " + biosFilename);
    }

    // Load Game Pak ROM
    std::ifstream gamePakFile(gamePakFilename, std::ios::binary);
    if (gamePakFile.is_open()) {
        gamePakFile.read(reinterpret_cast<char*>(&data[0x08000000]), 0x2000000); // Load up to 32MB Game Pak ROM
        gamePakFile.close();
    } else {
        Debug::log::error("Failed to load Game Pak ROM from " + gamePakFilename);
    }
}

void Memory::initializeTestRegions() {
    regions = {
        {0x00000000, 0x00000FFF, MEMORY_TYPE_RAM, 32}, // Small RAM region for testing
    };
}
