#ifndef GBA_H
#define GBA_H

#include "cpu.h" // Use CPU as the central instance
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include <thread>
#include <mutex>
#include <condition_variable>

class GBA {
private:
    Memory memory; // Shared memory instance
    CPU* cpu; // Pointer to CPU instance
    GPU gpu;
    InterruptController interruptController;

    std::mutex syncMutex;
    std::condition_variable syncCondition;

public:
    GBA(bool testMode = false) : memory(testMode), gpu(memory), interruptController() {
        cpu = new CPU(memory, interruptController); // Pass Memory and InterruptController to CPU constructor
    }

    ~GBA() {
        delete cpu; // Clean up dynamically allocated CPU
    }

    void run();
    void syncScanline();
    CPU& getCPU() { return *cpu; }
    Memory& getMemory() { return memory; } // Provide access to the Memory instance
};

#endif
