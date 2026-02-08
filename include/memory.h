// Memory class implementation for GBA emulator (region pointer table version)

#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <cstddef>

// Forward declaration to avoid circular dependency
class Scheduler;
class TimerController;
class DMAController;
class APU;
class InterruptController;

class Memory {
public:
    static constexpr size_t BLOCK_SIZE = 64 * 1024; // 64KB
    static constexpr size_t NUM_BLOCKS = 0x10000000 / BLOCK_SIZE; // 256MB / 64KB = 4096

    explicit Memory(bool testMode = false);
    ~Memory();
    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    // Scheduler integration
    void setScheduler(Scheduler* sched) { scheduler = sched; }
    void setTimerController(TimerController* tc) { timerController = tc; }
    void setDMAController(DMAController* dma) { dmaController = dma; }
    void setAPU(APU* a) { apu = a; }
    void setCPU(class CPU* c) { cpu = c; }
    void setInterruptController(InterruptController* ic) { interruptController = ic; }

    // GPU rendering guard: suppress wait cycles during hardware rendering
    void setWaitCyclesBypass(bool bypass) { disableWaitCycles = bypass; }

    // Instruction-level cycle accumulation (mGBA model):
    // During CPU instruction execution, data access wait cycles are accumulated
    // in pendingDataCycles instead of advancing the scheduler immediately.
    // The CPU drains them at the end of each instruction so that I/O side effects
    // (timer enable/read) see instruction-boundary cycle values, not mid-instruction ones.
    void beginInstructionCycles() const { accumulatingCycles = true; pendingDataCycles = 0; }
    uint32_t endInstructionCycles() const { accumulatingCycles = false; uint32_t c = pendingDataCycles; pendingDataCycles = 0; return c; }

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
    
    // Direct I/O read (bypasses wait cycle counting)
    // Used by tracer and debugging code that shouldn't affect timing
    uint8_t readDirectIO8(uint32_t address) const;
    uint16_t readDirectIO16(uint32_t address) const;
    uint32_t readDirectIO32(uint32_t address) const;

    // Get wait states for an address (for testing/debugging)
    uint32_t getWaitStates(uint32_t address, uint32_t accessWidth) const;
    
    // ROM and BIOS loading
    bool loadROM(const char* filepath);
    bool loadBIOS(const char* filepath);
    
    // Direct access to memory regions (for GPU, DMA, etc.)
    uint8_t* getVRAM() { return vram; }
    uint8_t* getPaletteRAM() { return palette; }
    
    // Key input - set KEYINPUT register state directly (bypasses read-only protection)
    void setKeyState(uint16_t keyState);

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
    
    // Actual loaded ROM size (for open bus detection)
    size_t romSize = 0;

    // Scheduler for cycle-accurate timing
    Scheduler* scheduler = nullptr;
    
    // Timer controller for timer register handling
    TimerController* timerController = nullptr;
    
    // DMA controller for DMA register handling
    DMAController* dmaController = nullptr;
    
    // APU for sound register handling
    APU* apu = nullptr;
    
    // CPU for HALT control
    class CPU* cpu = nullptr;
    
    // Interrupt controller for IRQ checks on IE/IME writes
    InterruptController* interruptController = nullptr;

public:
    // BIOS read protection: last prefetch value when CPU was in BIOS region.
    // When CPU reads BIOS from outside the BIOS region, this latch is returned
    // instead of the actual BIOS data (ARM7TDMI BIOS protection mechanism).
    uint32_t biosPrefetch = 0;

    // Tracks whether the CPU is currently executing from the BIOS region.
    // Set by the CPU at the start of each instruction; used by read8/16/32
    // to decide whether BIOS protection should return biosPrefetch.
    bool cpuInBios = false;
    
    // Whether the Game Pak prefetch buffer is enabled (WAITCNT bit 14)
    bool prefetchEnabled = false;
    
    // Get sequential/non-sequential wait states for a region (for CPU fetch cycle computation)
    uint8_t getSeqWaitCycles16(uint32_t address) const { return waitstatesSeq16[(address >> 24) & 0xFF]; }
    uint8_t getSeqWaitCycles32(uint32_t address) const { return waitstatesSeq32[(address >> 24) & 0xFF]; }
    uint8_t getNonseqWaitCycles16(uint32_t address) const { return waitstatesNonseq16[(address >> 24) & 0xFF]; }
    uint8_t getNonseqWaitCycles32(uint32_t address) const { return waitstatesNonseq32[(address >> 24) & 0xFF]; }

private:
    
    // Flag to temporarily disable wait cycles (for tracer reads that shouldn't affect timing)
    mutable bool disableWaitCycles = false;
    
    // Cycle accumulation: when true, addWaitCycles adds to pendingDataCycles
    // instead of advancing the scheduler (used during CPU instruction execution)
    mutable bool accumulatingCycles = false;
    mutable uint32_t pendingDataCycles = 0;
    
    // Wait state tables (matching mGBA's model)
    // These track non-sequential and sequential access times for each memory region
    // Index is the high byte of the address (address >> 24)
    uint8_t waitstatesNonseq32[256] = {0};
    uint8_t waitstatesNonseq16[256] = {0};
    uint8_t waitstatesSeq32[256] = {0};
    uint8_t waitstatesSeq16[256] = {0};
    
    // Helper to add wait state cycles
    void addWaitCycles(uint32_t address, uint32_t accessWidth) const;
    
    // I/O register read handler: implements per-register read behavior
    // (write-only → open bus, readable → value & mask, unused → 0)
    // offset is the 16-bit aligned register offset from 0x04000000
    uint16_t ioRead16(uint16_t offset) const;
    
    // Calculate wait states for different memory regions
    uint32_t calculateWaitStates(uint32_t address, uint32_t accessWidth) const;
    
    // Initialize wait state tables
    void initWaitStateTables();
    
    // Update wait state tables when WAITCNT register is written
    void updateWaitstates(uint16_t waitcnt);
    
    // Get non-sequential wait states for an address
    uint32_t getNonseqWaitStates(uint32_t address, uint32_t accessWidth) const;
};

#endif // MEMORY_H
