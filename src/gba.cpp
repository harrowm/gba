#include "gba.h"
#include "cpu.h"
#include "gpu.h"
#include "memory.h"
#include "interrupt.h"
#include <thread>
#include <mutex>
#include <condition_variable>

void GBA::run() {
    std::thread cpuThread([this]() { cpu.execute(1231); }); // Bind execute method to cpu object
    std::thread gpuThread([this]() { gpu.renderScanline(); }); // Bind renderScanline method to gpu object

    cpuThread.join();
    gpuThread.join();
}

void GBA::syncScanline() {
    std::unique_lock<std::mutex> lock(syncMutex);
    syncCondition.notify_all();
    syncCondition.wait(lock);
}
