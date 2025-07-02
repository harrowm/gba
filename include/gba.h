#ifndef GBA_H
#define GBA_H

#include "thumb_cpu.h" // Use ThumbCPU as the concrete CPU implementation
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include <thread>
#include <mutex>
#include <condition_variable>

class GBA {
private:
    Memory memory; // Shared memory instance
    ThumbCPU cpu;  // Use ThumbCPU instead of abstract CPU
    GPU gpu;
    InterruptController interruptController;

    std::mutex syncMutex;
    std::condition_variable syncCondition;

public:
    GBA() : memory(0x40000), cpu(memory, interruptController), gpu(memory) {} // Pass size to Memory constructor
    void run();
    void syncScanline();
};

#endif
