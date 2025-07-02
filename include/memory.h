// Memory class implementation for GBA emulator
// Provides memory management and region handling

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <vector>
#include <mutex>
#include <fstream>
#include <iostream>

#define MEMORY_TYPE_ROM 0
#define MEMORY_TYPE_RAM 1

class Memory {
private:
    struct MemoryRegion {
        uint32_t start_address;
        uint32_t end_address;
        uint8_t type;
        uint8_t width;
        uint32_t offsetInMemoryArray; // Offset in the memory array
    };

    std::vector<uint8_t> data;
    std::vector<MemoryRegion> regions;
    std::vector<std::pair<uint32_t, uint32_t>> romRegions; // Added romRegions
    std::mutex memoryMutex;

    // Cache for last accessed region
    // This will help avoid repeated lookups for the same region
    mutable const MemoryRegion* lastRegion = nullptr;
    mutable uint32_t lastRegionIndex = 0;

public:
    // Constructor with region initialization
    Memory(bool initializeGBAR = true); // Updated constructor signature

    // Destructor
    ~Memory();

    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address, bool big_endian = false);
    uint32_t read32(uint32_t address, bool big_endian = false);

    void write8(uint32_t address, uint8_t value);
    void write16(uint32_t address, uint16_t value, bool big_endian = false);
    void write32(uint32_t address, uint32_t value, bool big_endian = false);

    bool isAddressInROM(uint32_t address) const;
    int mapAddress(uint32_t gbaAddress, bool isWrite = false) const; // Updated mapAddress method with default value
    uint32_t getSize() const;

private:
    void initializeGBARegions(const std::string& biosFilename = "assets/bios.bin", const std::string& gamePakFilename = "assets/roms/gamepak.bin");
    void initializeTestRegions();
};

#endif
