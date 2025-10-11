#ifndef GBA_H
#define GBA_H

#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include "scheduler.h"
#include "timer_controller.h"
#include "dma.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>

class GBA {
private:
    Memory memory;
    Scheduler scheduler;
    CPU* cpu;
    GPU* gpu;  // Changed to pointer to avoid stack overflow (GPU has large framebuffer)
    InterruptController interruptController;
    TimerController timerController;
    DMAController dmaController;

    std::mutex syncMutex;
    std::condition_variable syncCondition;
    
    bool running;
    uint64_t frameCount;

public:
    GBA(bool testMode = false);
    ~GBA();

    // Main emulation loop
    void run();
    void runFrame();  // Run one frame (280,896 cycles)
    void stop() { running = false; }
    
    // ROM and BIOS loading
    bool loadROM(const char* filepath) { return memory.loadROM(filepath); }
    bool loadBIOS(const char* filepath) { return memory.loadBIOS(filepath); }
    
    // Skip BIOS and jump directly to ROM (for simple test ROMs without proper headers)
    void skipBIOS();
    
    // Legacy sync
    void syncScanline();
    
    // Accessors
    CPU& getCPU() { return *cpu; }
    Memory& getMemory() { return memory; }
    GPU& getGPU() { return *gpu; }
    Scheduler& getScheduler() { return scheduler; }
    DMAController& getDMAController() { return dmaController; }
    uint64_t getFrameCount() const { return frameCount; }
};

#endif
