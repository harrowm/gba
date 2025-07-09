#include "gpu.h"
#include "memory.h"
#include "debug.h"
#include <cstdint>

GPU::GPU(Memory& mem) : memory(mem) {
    // Initialize GPU state
}

void GPU::renderScanline() {
    DEBUG_INFO("Rendering scanline");
    // Stub implementation for rendering a scanline
    DEBUG_INFO("Accessing memory for rendering");
    uint8_t testValue = memory.read8(0x0); // Example memory access
    DEBUG_INFO("Test value from memory: " + std::to_string(testValue));
}
