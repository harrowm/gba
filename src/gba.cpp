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
    
    // Reset CPU to initial state (PC starts at 0x00000000 - BIOS entry point)
    cpu->reset();
    
    DEBUG_INFO("GBA initialized with scheduler, CPU, GPU, and interrupts wired");
    DEBUG_INFO("CPU will boot from BIOS at 0x00000000");
}

GBA::~GBA() {
    delete cpu;
}

void GBA::skipBIOS() {
    // Skip BIOS boot process and jump directly to ROM
    // Set up registers as if BIOS had already initialized them
    cpu->setMode(CPU::SYS);  // System mode
    cpu->R()[13] = 0x03007F00;  // SP for system mode
    cpu->R()[15] = 0x08000000;  // PC at ROM start
    cpu->CPSR() = 0x0000001F;  // System mode, no flags set
    
    DEBUG_INFO("Skipped BIOS, jumping directly to ROM at 0x08000000");
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
    static int frame_num = 0;
    frame_num++;
    printf("[GBA::runFrame #%d] Starting frame\n", frame_num);
    
    // One frame = 280,896 cycles (228 scanlines * 1232 cycles)
    uint64_t startCycle = scheduler.getCurrentCycle();
    printf("[GBA::runFrame #%d] Got start cycle: %llu\n", frame_num, startCycle);
    uint64_t targetCycle = startCycle + CYCLES_PER_FRAME;
    printf("[GBA::runFrame #%d] Target cycle: %llu\n", frame_num, targetCycle);
    
    // Simply advance the scheduler to process all events for this frame
    // The CPU execution happens separately in the main loop
    scheduler.runUntil(targetCycle);
    
    frameCount++;
    
    // In a real emulator, we'd also:
    // - Handle input
    // - Render to display (already done in main loop)
    // - Synchronize to 59.73 Hz (SDL2 VSync handles this)
    // - Process audio samples
}

void GBA::syncScanline() {
    std::unique_lock<std::mutex> lock(syncMutex);
    syncCondition.notify_all();
    syncCondition.wait(lock);
}
