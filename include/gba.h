#ifndef GBA_H
#define GBA_H

#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include "scheduler.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdint>

class GBA {
private:
    Memory memory;
    Scheduler scheduler;
    CPU* cpu;
    GPU gpu;
    InterruptController interruptController;

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
    
    // Legacy sync
    void syncScanline();
    
    // Accessors
    CPU& getCPU() { return *cpu; }
    Memory& getMemory() { return memory; }
    GPU& getGPU() { return gpu; }
    Scheduler& getScheduler() { return scheduler; }
    uint64_t getFrameCount() const { return frameCount; }
};

#endif
