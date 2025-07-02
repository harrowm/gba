#include "arm_cpu.h"
#include "debug.h"

ARMCPU::ARMCPU(Memory& mem, InterruptController& ic) : CPU(mem, ic) {}

void ARMCPU::step(uint32_t cycles) {
    while (cycles > 0) {
        uint32_t instruction = memory.read32(registers[15]); // Fetch instruction
        decodeAndExecute(instruction);
        cycles -= 1; // Placeholder for cycle deduction
    }
}

void ARMCPU::execute(uint32_t cycles) {
    Debug::log::info("Executing ARM CPU for " + std::to_string(cycles) + " cycles");
    step(cycles); // Delegate to step method
}

void ARMCPU::decodeAndExecute(uint32_t instruction) {
    Debug::log::info("Decoding and executing ARM instruction: 0x" + std::to_string(instruction));
    // Implement ARM instruction decoding and execution logic
}
