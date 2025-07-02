#include "gba.h"
#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include <thread>
#include <mutex>
#include <condition_variable>

void GBA::run() {
    std::thread cpuThread(&CPU::execute, &cpu);
    std::thread gpuThread(&GPU::renderScanline, &gpu);

    cpuThread.join();
    gpuThread.join();
}

void GBA::syncScanline() {
    std::unique_lock<std::mutex> lock(syncMutex);
    syncCondition.notify_all();
    syncCondition.wait(lock);
}
