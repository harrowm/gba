#include "gba.h"
#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include "scheduler.h"
#include "debug.h"
#include <thread>
#include <mutex>
#include <condition_variable>

GBA::GBA(bool testMode) 
    : memory(testMode), scheduler(), gpu(memory), interruptController(), 
      running(false), frameCount(0) {
    
    // Create CPU
    cpu = new CPU(memory, interruptController);
    
    // Wire up the scheduler
    memory.setScheduler(&scheduler);
    cpu->setScheduler(&scheduler);
    
    // Wire up interrupt controller
    interruptController.setMemory(&memory);
    
    // Setup interrupt callback (CPU will handle interrupt on next instruction)
    interruptController.setIRQCallback([]() {
        // Set CPU IRQ flag - actual handling happens in CPU execution
        DEBUG_INFO("IRQ callback triggered");
    });
    
    // Setup GPU callbacks
    gpu.setVBlankCallback([this]() {
        interruptController.triggerVBlank();
    });
    
    gpu.setHBlankCallback([this]() {
        interruptController.triggerHBlank();
    });
    
    // Initialize video timing
    gpu.setupTiming(&scheduler);
    
    DEBUG_INFO("GBA initialized with scheduler, CPU, GPU, and interrupts wired");
}

GBA::~GBA() {
    delete cpu;
}

void GBA::run() {
    DEBUG_INFO("Starting GBA main loop");
    running = true;
    
    while (running) {
        runFrame();
    }
    
    DEBUG_INFO("GBA main loop stopped");
}

void GBA::runFrame() {
    // One frame = 280,896 cycles (228 scanlines * 1232 cycles)
    uint64_t targetCycle = scheduler.getCurrentCycle() + CYCLES_PER_FRAME;
    
    // Run scheduler until end of frame
    // The scheduler will process all scanline, H-Blank, and V-Blank events
    scheduler.runUntil(targetCycle);
    
    frameCount++;
    
    // In a real emulator, we'd also:
    // - Handle input
    // - Render to display
    // - Synchronize to 59.73 Hz
    // - Process audio samples
}

void GBA::syncScanline() {
    std::unique_lock<std::mutex> lock(syncMutex);
    syncCondition.notify_all();
    syncCondition.wait(lock);
}
