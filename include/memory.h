// Memory class implementation for GBA emulator (region pointer table version)

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

class Memory {
public:
    static constexpr size_t BLOCK_SIZE = 64 * 1024; // 64KB
    static constexpr size_t NUM_BLOCKS = 0x10000000 / BLOCK_SIZE; // 256MB / 64KB = 4096

    Memory(bool testMode = false);
    ~Memory();

    // Accessors
    uint8_t read8(uint32_t address) const;
    void write8(uint32_t address, uint8_t value);
    uint16_t read16(uint32_t address) const;
    void write16(uint32_t address, uint16_t value);
    uint32_t read32(uint32_t address) const;
    void write32(uint32_t address, uint32_t value);

private:
    // Region pointer table: each entry points to the start of a mapped region or nullptr
    uint8_t* regionTable[NUM_BLOCKS] = {nullptr};

    // Buffers for each region (allocated as needed)
    uint8_t* bios = nullptr;
    uint8_t* wram = nullptr;
    uint8_t* iwram = nullptr;
    uint8_t* io = nullptr;
    uint8_t* palette = nullptr;
    uint8_t* vram = nullptr;
    uint8_t* oam = nullptr;
    uint8_t* rom = nullptr;
    uint8_t* sram = nullptr;
    uint8_t* test_ram = nullptr;
};

#endif // MEMORY_H
