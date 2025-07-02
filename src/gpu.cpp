#include "gpu.h"
#include "debug.h"

GPU::GPU(Memory& mem) : memory(mem) {
    // Initialize GPU state
}

void GPU::renderScanline() {
    Debug::log::info("Rendering scanline");
    // Stub implementation for rendering a scanline
    Debug::log::info("Accessing memory for rendering");
    uint8_t testValue = memory.read8(0x06000000); // Example memory access
    Debug::log::info("Test value from memory: " + std::to_string(testValue));
}
