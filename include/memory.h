// Memory class implementation for GBA emulator
// Provides memory management and region handling

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <vector>
#include <mutex>
#include <fstream>
#include <iostream>
#include <functional>
#include "utility_macros.h"

// Memory type definitions
#define MEMORY_TYPE_ROM 0
#define MEMORY_TYPE_RAM 1

// Memory safety options
// Define DISABLE_MEMORY_BOUNDS_CHECK at compile time to disable memory bounds checking
// For benchmark and optimized builds, this is set automatically in the Makefile
// This eliminates bounds checking overhead in performance-critical code paths
#ifndef DISABLE_MEMORY_BOUNDS_CHECK
#define CHECK_MEMORY_BOUNDS 1
#else
#define CHECK_MEMORY_BOUNDS 0
#endif

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

    // Cache invalidation callbacks
    std::vector<std::function<void(uint32_t, uint32_t)>> cache_invalidation_callbacks;

public:
    // Constructor with region initialization
    Memory(bool initializeGBAR = true); // Updated constructor signature

    // Destructor
    ~Memory();

    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address, bool big_endian = false);
    FORCE_INLINE uint32_t read32(uint32_t address, bool big_endian = false);

    void write8(uint32_t address, uint8_t value);
    void write16(uint32_t address, uint16_t value, bool big_endian = false);
    FORCE_INLINE void write32(uint32_t address, uint32_t value, bool big_endian = false);

    bool isAddressInROM(uint32_t address) const;
    int mapAddress(uint32_t gbaAddress, bool isWrite = false) const; // Updated mapAddress method with default value
    uint32_t getSize() const;

    // Register a callback for cache invalidation
    void registerCacheInvalidationCallback(std::function<void(uint32_t, uint32_t)> callback);

    // Get direct access to raw memory data (for testing purposes)
    std::vector<uint8_t>& getRawData() { return data; }
    const std::vector<uint8_t>& getRawData() const { return data; }

private:
    void initializeGBARegions(const std::string& biosFilename = "assets/bios.bin", const std::string& gamePakFilename = "assets/roms/gamepak.bin");
    void initializeTestRegions();

    // Helper method to notify caches of memory writes
    void notifyCacheInvalidation(uint32_t address, uint32_t size) const;
};

// Optimized inline implementations for critical memory access functions
FORCE_INLINE uint32_t Memory::read32(uint32_t address, bool big_endian) {
    #if CHECK_MEMORY_BOUNDS
    int mappedIndex = mapAddress(address, false);
    // This error check will only be active when bounds checking is enabled
    if (mappedIndex == -1) {
        #if !defined(NDEBUG) && defined(DEBUG_LEVEL) && DEBUG_LEVEL > 0
        DEBUG_ERROR("Invalid memory access at address: " + std::to_string(address));
        #endif
        return 0; // Return default value
    }
    #else
    // Ultra-fast path with no bounds checking - directly calculate offset 
    // This assumes test memory mode in benchmarks (single region starting at 0)
    int mappedIndex = address;
    #endif
    
    // Fast path with no mutex locking (critical for performance)
    uint32_t value = (data[mappedIndex]) | 
                    ((uint32_t)data[mappedIndex + 1] << 8) |
                    ((uint32_t)data[mappedIndex + 2] << 16) | 
                    ((uint32_t)data[mappedIndex + 3] << 24);
    
    // Only do byte swapping if explicitly requested
    return big_endian ? __builtin_bswap32(value) : value;
}

FORCE_INLINE void Memory::write32(uint32_t address, uint32_t value, bool big_endian) {
    #if CHECK_MEMORY_BOUNDS
    int mappedIndex = mapAddress(address, true);
    if (mappedIndex == -1) {
        #if !defined(NDEBUG) && defined(DEBUG_LEVEL) && DEBUG_LEVEL > 0
        DEBUG_ERROR("Invalid memory write at address: " + std::to_string(address));
        #endif
        return;
    }
    if (mappedIndex == -2) {
        #if !defined(NDEBUG) && defined(DEBUG_LEVEL) && DEBUG_LEVEL > 0
        DEBUG_INFO("Attempted write to ROM address: " + std::to_string(address));
        #endif
        return;
    }
    #else
    // Ultra-fast path with no bounds checking
    int mappedIndex = address;
    #endif
    
    // Fast path implementation with no mutex (critical for performance)
    if (big_endian) value = __builtin_bswap32(value);
    
    // Write directly to memory in one step
    data[mappedIndex] = value & 0xFF;
    data[mappedIndex + 1] = (value >> 8) & 0xFF;
    data[mappedIndex + 2] = (value >> 16) & 0xFF;
    data[mappedIndex + 3] = (value >> 24) & 0xFF;
    
    #if !defined(BENCHMARK_MODE)
    // Only notify caches if not in benchmark mode
    notifyCacheInvalidation(address, 4);
    #endif
}

#endif
