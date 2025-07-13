#include <stdio.h>
#include <assert.h>
#include "arm_execute_phase1.h"
#include "utility_macros.h"

// Mock memory interface functions
uint32_t mock_read32(void* context, uint32_t address) {
    UNUSED(context);
    printf("Mock memory read32 from 0x%08X\n", address);
    return 0xDEADBEEF; // Placeholder value
}

void mock_write32(void* context, uint32_t address, uint32_t value) {
    UNUSED(context);
    printf("Mock memory write32 to 0x%08X: 0x%08X\n", address, value);
}

uint16_t mock_read16(void* context, uint32_t address) {
    UNUSED(context);
    printf("Mock memory read16 from 0x%08X\n", address);
    return 0xBEEF;
}

void mock_write16(void* context, uint32_t address, uint16_t value) {
    UNUSED(context);
    printf("Mock memory write16 to 0x%08X: 0x%04X\n", address, value);
}

uint8_t mock_read8(void* context, uint32_t address) {
    UNUSED(context);
    printf("Mock memory read8 from 0x%08X\n", address);
    return 0xEF;
}

void mock_write8(void* context, uint32_t address, uint8_t value) {
    UNUSED(context);
    printf("Mock memory write8 to 0x%08X: 0x%02X\n", address, value);
}

// Test helpers
void setup_test_cpu(ArmCPUState* state) {
    for (int i = 0; i < 16; i++) {
        state->registers[i] = 0;
    }
    state->cpsr = 0x00000000; // ARM mode, all flags clear
}

// ...existing code for test functions and main...
