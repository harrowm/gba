#include "cpu.h"
#include "debug.h"
#include "thumb_cpu.h" // Include complete definition
#include "arm_cpu.h"   // Include complete definition

CPU::CPU(Memory& mem, InterruptController& ic) : memory(mem), interruptController(ic), cpsr(0), bigEndian(false) {
    thumbCPU = new ThumbCPU(mem, ic); // Dynamically allocate ThumbCPU
    armCPU = new ARMCPU(mem, ic);     // Dynamically allocate ARMCPU
    for (int i = 0; i < 16; ++i) {
        registers[i] = 0;
    }
}

void CPU::updateCPSRFlags(uint32_t result, uint8_t carryOut) {
    Debug::log::info("Updating CPSR flags");
    cpsr = (result == 0) ? (cpsr | (1 << 30)) : (cpsr & ~(1 << 30)); // Zero flag
    cpsr = (result & (1 << 31)) ? (cpsr | (1 << 31)) : (cpsr & ~(1 << 31)); // Negative flag
    cpsr = carryOut ? (cpsr | (1 << 29)) : (cpsr & ~(1 << 29)); // Carry flag
}

void CPU::setMode(bool thumbMode) {
    Debug::log::info("Setting CPU mode");
    if (thumbMode) {
        cpsr |= (1 << 5); // Thumb mode flag
    } else {
        cpsr &= ~(1 << 5); // Clear Thumb mode flag
    }
}

bool CPU::isThumbMode() const {
    return cpsr & (1 << 5);
}

void CPU::setCPUState(const CPUState& state) {
    Debug::log::info("Setting CPU state");
    for (int i = 0; i < 16; ++i) {
        registers[i] = state.registers[i];
    }
    cpsr = state.cpsr;
    bigEndian = state.bigEndian;
}

CPUState CPU::getCPUState() const {
    Debug::log::info("Getting CPU state");
    CPUState state;
    for (int i = 0; i < 16; ++i) {
        state.registers[i] = registers[i];
    }
    state.cpsr = cpsr;
    state.bigEndian = bigEndian;
    return state;
}

void CPU::execute(uint32_t cycles) {
    Debug::log::info("Executing CPU for " + std::to_string(cycles) + " cycles");

    if (isThumbMode()) {
        Debug::log::debug("Executing Thumb instructions");
        thumbCPU->execute(cycles); // Use pointer
    } else {
        Debug::log::debug("Executing ARM instructions");
        armCPU->execute(cycles); // Use pointer
    }
}

void CPU::printCPUState() const {
    Debug::log::info("Printing CPU state");

    std::string stateStr = "\nCPU State:\n";
    for (int i = 0; i < 16; ++i) {
        stateStr += "R" + std::to_string(i) + (i < 10 ? " : " : ": ") + Debug::toHexString(registers[i], 8) + "\t";
        if ((i + 1) % 4 == 0) {
            stateStr += "\n"; // New line every 4 registers
        }
    }

    stateStr += "CPSR: " + Debug::toHexString(cpsr, 8) + "\tBig Endian: " + (bigEndian ? "true" : "false") + "\n";

    stateStr += "Flags: Z:" + std::to_string(isFlagSet(cpsr, FLAG_ZERO)) + " N:" + std::to_string(isFlagSet(cpsr, FLAG_NEGATIVE)) + " V:" + std::to_string(isFlagSet(cpsr, FLAG_OVERFLOW)) + " C:" + std::to_string(isFlagSet(cpsr, FLAG_CARRY)) + " T:" + std::to_string(isFlagSet(cpsr, FLAG_THUMB)) + "\n";

    Debug::log::info(stateStr);
}
