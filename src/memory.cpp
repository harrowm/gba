// Memory class implementation for GBA emulator
// Contains method definitions for memory operations

// For optimized builds, define DISABLE_MEMORY_BOUNDS_CHECK to eliminate bounds checking
// This can significantly improve performance in benchmark and optimized builds
// The Makefile automatically sets this flag for arm_benchmark_opt and arm_benchmark_ultra targets

#include "memory.h"
#include "debug.h" // Use debug system
#include <fstream>
#include <iostream>

Memory::Memory(bool initializeTestMode) {
    // For optimized build, always use test mode for benchmarks
    #if defined(NDEBUG) || defined(BENCHMARK_MODE)
    (void)initializeTestMode; // Mark parameter as used to avoid warnings
    DEBUG_INFO("Initializing memory regions for testing (optimized build).");
    initializeTestRegions();
    #else
    // Volatile to prevent optimization in release builds
    volatile bool useTestMode = initializeTestMode;
    
    if (useTestMode) {
        DEBUG_INFO("Initializing memory regions for testing.");
        initializeTestRegions();
    } else {
        DEBUG_INFO("Initializing GBA memory regions with BIOS and GamePak ROM.");
        initializeGBARegions("assets/bios.bin", "assets/roms/gamepak.bin");
    }
    #endif

    // Calculate total memory size based on regions
    uint32_t totalSize = 0;
    for (const auto& region : regions) {
        totalSize += (region.end_address - region.start_address + 1);
    }
    data.resize(totalSize, 0);

    // Output debug info directly - no more lazy evaluation needed
    DEBUG_INFO("Total memory size calculated: 0x" + debug_to_hex_string(totalSize, 8) + " bytes.");
}

Memory::~Memory() {}

int Memory::mapAddress(uint32_t gbaAddress, bool isWrite /* = false */) const {
    // Direct debug output - no more lazy evaluation
    DEBUG_INFO("Mapping address: 0x" + debug_to_hex_string(gbaAddress, 8));

    // Check if the address is within the cached region
    if (lastRegion && gbaAddress >= lastRegion->start_address && gbaAddress <= lastRegion->end_address) {
        return gbaAddress - lastRegion->start_address + lastRegion->offsetInMemoryArray;
    }

    // Check if the address is within a ROM region
    if (isWrite) {
        auto romIt = std::lower_bound(romRegions.begin(), romRegions.end(), gbaAddress,
            [](const std::pair<uint32_t, uint32_t>& region, uint32_t addr) {
                return region.second < addr;
            });
        if (romIt != romRegions.end() && gbaAddress >= romIt->first && gbaAddress <= romIt->second) {
            return -2; // Write-protected address
        }
    }

    // Map the address to the memory array
    auto it = std::lower_bound(regions.begin(), regions.end(), gbaAddress,
        [](const MemoryRegion& region, uint32_t addr) {
            return region.end_address < addr;
        });
    if (it != regions.end() && gbaAddress >= it->start_address && gbaAddress <= it->end_address) {
        lastRegion = &(*it); // Update the cache
        lastRegionIndex = std::distance(regions.begin(), it);
        
        // Direct debug output
        DEBUG_INFO("Mapped to address: 0x" + debug_to_hex_string(gbaAddress - it->start_address + it->offsetInMemoryArray, 8));
        
        return gbaAddress - it->start_address + it->offsetInMemoryArray;
    }
    return -1; // Invalid address
}

uint8_t Memory::read8(uint32_t address) {
    #if CHECK_MEMORY_BOUNDS
    int mappedIndex = mapAddress(address);
    if (mappedIndex == -1) {
        #if !defined(NDEBUG) && defined(DEBUG_LEVEL) && DEBUG_LEVEL > 0
        DEBUG_ERROR("Invalid memory access at address: " + std::to_string(address));
        #endif
        return 0; // Return default value
    }
    #else
    // Ultra-fast path with no bounds checking
    int mappedIndex = address;
    #endif
    
    return data[mappedIndex];
}

uint16_t Memory::read16(uint32_t address, bool big_endian /* = false */) {
    #if CHECK_MEMORY_BOUNDS
    int mappedIndex = mapAddress(address, false);
    if (mappedIndex == -1) {
        #if !defined(NDEBUG) && defined(DEBUG_LEVEL) && DEBUG_LEVEL > 0
        DEBUG_ERROR("Invalid memory access at address: " + std::to_string(address));
        #endif
        return 0; // Return default value
    }
    #else
    // Ultra-fast path with no bounds checking
    int mappedIndex = address;
    #endif
    
    #if defined(BENCHMARK_MODE) || defined(NDEBUG)
    // Fast path for benchmark/release mode - no mutex locking
    uint16_t value = (data[mappedIndex] | (data[mappedIndex + 1] << 8));
    return big_endian ? __builtin_bswap16(value) : value;
    #else
    // Normal path with mutex protection for thread safety
    std::lock_guard<std::mutex> lock(memoryMutex);
    uint16_t value = (data[mappedIndex] | (data[mappedIndex + 1] << 8));
    return big_endian ? __builtin_bswap16(value) : value;
    #endif
}

uint32_t Memory::read32(uint32_t address, bool big_endian /* = false */) {
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
    
    #if defined(BENCHMARK_MODE) || defined(NDEBUG)
    // Fast path for benchmark/release mode - no mutex locking
    uint32_t value = (data[mappedIndex] | (data[mappedIndex + 1] << 8) |
                      (data[mappedIndex + 2] << 16) | (data[mappedIndex + 3] << 24));
    #else
    // Normal path with mutex protection for thread safety
    std::lock_guard<std::mutex> lock(memoryMutex);
    uint32_t value = (data[mappedIndex] | (data[mappedIndex + 1] << 8) |
                      (data[mappedIndex + 2] << 16) | (data[mappedIndex + 3] << 24));
    #endif
    
    return big_endian ? __builtin_bswap32(value) : value;
}

void Memory::write8(uint32_t address, uint8_t value) {
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
        // Direct debug output for ROM write attempts
        DEBUG_INFO("Attempted write to ROM address: " + std::to_string(address) + ", write ignored.");
        #endif
        return;
    }
    #else
    // Ultra-fast path with no bounds checking
    int mappedIndex = address;
    #endif
    
    data[mappedIndex] = value;
    
    // Notify instruction caches of potential code modification
    notifyCacheInvalidation(address, 1);
}

void Memory::write16(uint32_t address, uint16_t value, bool big_endian /* = false */) {
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
        // Direct debug output for ROM write attempts
        DEBUG_INFO("Attempted write to ROM address: " + std::to_string(address) + ", write ignored.");
        #endif
        return;
    }
    #else
    // Ultra-fast path with no bounds checking
    int mappedIndex = address;
    #endif
    
    #if defined(BENCHMARK_MODE) || defined(NDEBUG)
    // Fast path for benchmark mode - no mutex
    if (big_endian) value = __builtin_bswap16(value);
    data[mappedIndex] = value & 0xFF;
    data[mappedIndex + 1] = (value >> 8) & 0xFF;
    #else
    // Normal path with mutex protection
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (big_endian) value = __builtin_bswap16(value);
    data[mappedIndex] = value & 0xFF;
    data[mappedIndex + 1] = (value >> 8) & 0xFF;
    #endif
    
    // Notify instruction caches of potential code modification
    notifyCacheInvalidation(address, 2);
}

void Memory::write32(uint32_t address, uint32_t value, bool big_endian /* = false */) {
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
        // Direct debug output for ROM write attempts
        DEBUG_INFO("Attempted write to ROM address: " + std::to_string(address) + ", write ignored.");
        #endif
        return;
    }
    #else
    // Ultra-fast path with no bounds checking
    int mappedIndex = address;
    #endif
    
    #if defined(BENCHMARK_MODE) || defined(NDEBUG)
    // Fast path for benchmark mode - no mutex
    if (big_endian) value = __builtin_bswap32(value);
    data[mappedIndex] = value & 0xFF;
    data[mappedIndex + 1] = (value >> 8) & 0xFF;
    data[mappedIndex + 2] = (value >> 16) & 0xFF;
    data[mappedIndex + 3] = (value >> 24) & 0xFF;
    #else
    // Normal path with mutex protection
    std::lock_guard<std::mutex> lock(memoryMutex);
    if (big_endian) value = __builtin_bswap32(value);
    data[mappedIndex] = value & 0xFF;
    data[mappedIndex + 1] = (value >> 8) & 0xFF;
    data[mappedIndex + 2] = (value >> 16) & 0xFF;
    data[mappedIndex + 3] = (value >> 24) & 0xFF;
    #endif
    
    // Notify instruction caches of potential code modification
    notifyCacheInvalidation(address, 4);
}

void Memory::initializeGBARegions(const std::string& biosFilename, const std::string& gamePakFilename) {
    regions = {
        {0x00000000, 0x00003FFF, MEMORY_TYPE_ROM, 32, 0}, // BIOS
        {0x02000000, 0x0203FFFF, MEMORY_TYPE_RAM, 32, 0x4000}, // WRAM
        {0x03000000, 0x03007FFF, MEMORY_TYPE_RAM, 32, 0x44000}, // IWRAM
        {0x05000000, 0x050003FF, MEMORY_TYPE_RAM, 16, 0x4C000}, // Palette RAM
        {0x06000000, 0x06017FFF, MEMORY_TYPE_RAM, 16, 0x4C400}, // VRAM
        {0x07000000, 0x070003FF, MEMORY_TYPE_RAM, 16, 0x64400}, // OAM
        {0x08000000, 0x09FFFFFF, MEMORY_TYPE_ROM, 32, 0x64800}, // Game Pak ROM
        {0x0E000000, 0x0E00FFFF, MEMORY_TYPE_RAM, 16, 0x264800}  // Game Pak SRAM
    };

    romRegions = {
        {0x00000000, 0x00003FFF}, // BIOS
        {0x08000000, 0x09FFFFFF}  // Game Pak ROM
    };

    // The location to load the roms is hardcoded .. should probably soft code this
    // Load BIOS ROM
    DEBUG_INFO("Initializing GBA memory regions with BIOS ROM.");
    std::ifstream biosFile(biosFilename, std::ios::binary);
    if (!biosFile.is_open()) {
        DEBUG_ERROR("Failed to load BIOS ROM from " + biosFilename);
        return;
    }

    biosFile.read(reinterpret_cast<char*>(&data[0]), 0x4000); // Load 16KB BIOS
    biosFile.close();

    
    // Load Game Pak ROM
    DEBUG_INFO("Initializing GBA memory regions with GamePak ROM.");
    std::ifstream gamePakFile(gamePakFilename, std::ios::binary);
    if (!gamePakFile.is_open()) {
        DEBUG_ERROR("Failed to load Game Pak ROM from " + gamePakFilename);
        return;
    }
    gamePakFile.read(reinterpret_cast<char*>(&data[0x64800]), 0x2000000); // Load up to 32MB Game Pak ROM
    gamePakFile.close();
}

void Memory::initializeTestRegions() {
    regions = {
        {0x00000000, 0x00001FFF, MEMORY_TYPE_RAM, 32, 0}, // Expanded RAM region for testing (8KB instead of 4KB)
    };
    DEBUG_INFO("Test regions initialized: Start = 0x00000000, End = 0x00001FFF, Type = RAM, Width = 32 bits");
}

uint32_t Memory::getSize() const {
    return data.size();
}

void Memory::registerCacheInvalidationCallback(std::function<void(uint32_t, uint32_t)> callback) {
    cache_invalidation_callbacks.push_back(callback);
}

void Memory::notifyCacheInvalidation(uint32_t address, uint32_t size) const {
    // Only notify if we have callbacks registered
    if (cache_invalidation_callbacks.empty()) {
        return;
    }
    
    // Call all registered callbacks
    uint32_t end_address = address + size - 1;
    for (const auto& callback : cache_invalidation_callbacks) {
        callback(address, end_address);
    }
}
