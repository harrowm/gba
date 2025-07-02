#ifndef GBA_H
#define GBA_H

#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include <thread>
#include <mutex>
#include <condition_variable>

class GBA {
private:
    Memory memory; // Shared memory instance
    CPU cpu;
    GPU gpu;
    InterruptController interruptController;

    std::mutex syncMutex;
    std::condition_variable syncCondition;

public:
    GBA() : cpu(memory, interruptController), gpu(memory) {}
    void run();
    void syncScanline();
};

#endif
