// Memory class implementation for GBA emulator (region pointer table version)

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

// Forward declaration to avoid circular dependency
class Scheduler;
class TimerController;
class DMAController;

class Memory {
public:
    static constexpr size_t BLOCK_SIZE = 64 * 1024; // 64KB
    static constexpr size_t NUM_BLOCKS = 0x10000000 / BLOCK_SIZE; // 256MB / 64KB = 4096

    Memory(bool testMode = false);
    ~Memory();

    // Scheduler integration
    void setScheduler(Scheduler* sched) { scheduler = sched; }
    void setTimerController(TimerController* tc) { timerController = tc; }
    void setDMAController(DMAController* dma) { dmaController = dma; }

    // Accessors (now with cycle-accurate timing)
    uint8_t read8(uint32_t address) const;
    void write8(uint32_t address, uint8_t value);
    uint16_t read16(uint32_t address) const;
    void write16(uint32_t address, uint16_t value);
    uint32_t read32(uint32_t address) const;
    void write32(uint32_t address, uint32_t value);
    
    // Direct I/O write (bypasses special handling like write-to-clear)
    // Used by hardware components to set registers
    void writeDirectIO(uint32_t address, uint16_t value);

    // Get wait states for an address (for testing/debugging)
    uint32_t getWaitStates(uint32_t address, uint32_t accessWidth) const;
    
    // Temporarily disable wait cycles (for tracer reads that shouldn't affect timing)
    void setDisableWaitCycles(bool disable) { disableWaitCycles = disable; }
    
    // ROM and BIOS loading
    bool loadROM(const char* filepath);
    bool loadBIOS(const char* filepath);
    
    // Direct access to memory regions (for GPU, DMA, etc.)
    uint8_t* getVRAM() { return vram; }
    uint8_t* getOAM() { return oam; }
    uint8_t* getPaletteRAM() { return palette; }
    uint8_t* getROM() { return rom; }

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

    // Scheduler for cycle-accurate timing
    Scheduler* scheduler = nullptr;
    
    // Timer controller for timer register handling
    TimerController* timerController = nullptr;
    
    // DMA controller for DMA register handling
    DMAController* dmaController = nullptr;
    
    // Flag to temporarily disable wait cycles (for tracer reads that shouldn't affect timing)
    mutable bool disableWaitCycles = false;
    
    // Wait state tables (matching mGBA's model)
    // These track non-sequential and sequential access times for each memory region
    // Index is the high byte of the address (address >> 24)
    uint8_t waitstatesNonseq32[256] = {0};
    uint8_t waitstatesNonseq16[256] = {0};
    uint8_t waitstatesSeq32[256] = {0};
    uint8_t waitstatesSeq16[256] = {0};
    
    // Helper to add wait state cycles
    void addWaitCycles(uint32_t address, uint32_t accessWidth) const;
    
    // Calculate wait states for different memory regions
    uint32_t calculateWaitStates(uint32_t address, uint32_t accessWidth) const;
    
    // Initialize wait state tables
    void initWaitStateTables();
    
    // Get sequential and non-sequential wait states for an address
    uint32_t getNonseqWaitStates(uint32_t address, uint32_t accessWidth) const;
    uint32_t getSeqWaitStates(uint32_t address, uint32_t accessWidth) const;
};

#endif // MEMORY_H
