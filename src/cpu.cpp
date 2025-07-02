#include "cpu.h"
#include "debug.h"
#include "thumb_cpu.h" // Include complete definition
#include "arm_cpu.h"   // Include complete definition

CPU::CPU(Memory& mem, InterruptController& ic) : memory(mem), interruptController(ic) {
    Debug::log::info("Initializing CPU with default CPSR value for ARM mode");
    thumbCPU = new ThumbCPU(*this); // Pass itself as the parent reference
    Debug::log::info("ThumbCPU instance created");
    armCPU = new ARMCPU(*this);     // Pass itself as the parent reference
    Debug::log::info("ARMCPU instance created");
    std::fill(std::begin(registers), std::end(registers), 0); // Reset all registers to zero using std::fill
    Debug::log::info("Registers initialized to zero");
    cpsr = 0; // Sets us up in ARM mode and little endian by default
}

void CPU::updateCPSRFlags(uint32_t result, uint8_t carryOut) {
    Debug::log::info("Updating CPSR flags");
    cpsr = (result == 0) ? (cpsr | (1 << 30)) : (cpsr & ~(1 << 30)); // Zero flag
    cpsr = (result & (1 << 31)) ? (cpsr | (1 << 31)) : (cpsr & ~(1 << 31)); // Negative flag
    cpsr = carryOut ? (cpsr | (1 << 29)) : (cpsr & ~(1 << 29)); // Carry flag
}

void CPU::setFlag(uint32_t flag) {
    cpsr |= flag;
}

void CPU::clearFlag(uint32_t flag) {
    cpsr &= ~flag;
}

bool CPU::checkFlag(uint32_t flag) const {
    return (cpsr & flag) != 0;
}


void CPU::setCPUState(const CPUState& state) {
    Debug::log::info("Setting CPU state");
    for (int i = 0; i < 16; ++i) {
        registers[i] = state.registers[i];
    }
    cpsr = state.cpsr;
}

CPU::CPUState CPU::getCPUState() const {
    Debug::log::info("Getting CPU state");
    CPUState state;
    for (int i = 0; i < 16; ++i) {
        state.registers[i] = registers[i];
    }
    state.cpsr = cpsr;
    return state;
}

void CPU::execute(uint32_t cycles) {
    Debug::log::info("Executing CPU for " + std::to_string(cycles) + " cycles");

    if (checkFlag(FLAG_T)) {
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

    stateStr += "CPSR: " + Debug::toHexString(cpsr, 8);

    stateStr += "Flags: Z:" + std::to_string(checkFlag(FLAG_Z)) + " N:" + std::to_string(checkFlag(FLAG_N)) + " V:" + std::to_string(checkFlag(FLAG_V)) + " C:" + std::to_string(checkFlag(FLAG_C)) + " T:" + std::to_string(checkFlag(FLAG_T)) + " E:" + std::to_string(checkFlag(FLAG_E)) + "\n";

    Debug::log::info(stateStr);
}

// Destructor
CPU::~CPU() {
    delete thumbCPU;
    delete armCPU;
}
