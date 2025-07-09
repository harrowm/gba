#include "cpu.h"
#include "debug.h" // Use macro-based debug system
#include "thumb_cpu.h" // Include complete definition
#include "arm_cpu.h"   // Include complete definition
#include "timing.h"

CPU::CPU(Memory& mem, InterruptController& ic) : memory(mem), interruptController(ic) {
    DEBUG_INFO("Initializing CPU with default CPSR value for ARM mode");
    thumbCPU = new ThumbCPU(*this); // Pass itself as the parent reference
    DEBUG_INFO("ThumbCPU instance created");
    armCPU = new ARMCPU(*this);     // Pass itself as the parent reference
    DEBUG_INFO("ARMCPU instance created");
    std::fill(std::begin(registers), std::end(registers), 0); // Reset all registers to zero using std::fill
    DEBUG_INFO("Registers initialized to zero");
    cpsr = 0; // Sets us up in ARM mode and little endian by default
    
    // Initialize timing system
    timing_init(&timing);
    DEBUG_INFO("Timing system initialized");
}

void CPU::setFlag(uint32_t flag) {
    cpsr |= flag;
}

void CPU::clearFlag(uint32_t flag) {
    cpsr &= ~flag;
}

bool CPU::getFlag(uint32_t flag) const {
    return (cpsr & flag) != 0;
}


void CPU::setCPUState(const CPUState& state) {
    DEBUG_INFO("Setting CPU state");
    for (int i = 0; i < 16; ++i) {
        registers[i] = state.registers[i];
    }
    cpsr = state.cpsr;
}

CPU::CPUState CPU::getCPUState() const {
    DEBUG_INFO("Getting CPU state");
    CPUState state;
    for (int i = 0; i < 16; ++i) {
        state.registers[i] = registers[i];
    }
    state.cpsr = cpsr;
    return state;
}

void CPU::execute(uint32_t cycles) {
    // Use macro-based debug system
    DEBUG_INFO("Executing CPU for " + std::to_string(cycles) + " cycles");

    if (getFlag(FLAG_T)) {
        DEBUG_LOG("Executing Thumb instructions");
        thumbCPU->execute(cycles); // Use pointer
    } else {
        DEBUG_LOG("Executing ARM instructions");
        armCPU->execute(cycles); // Use pointer
    }
}

// New timing-aware execution method
void CPU::executeWithTiming(uint32_t cycles) {
    // Use macro-based debug system
    DEBUG_INFO("Executing CPU with timing for " + std::to_string(cycles) + " cycles");
    
    if (getFlag(FLAG_T)) {
        DEBUG_LOG("Executing Thumb instructions with timing");
        thumbCPU->executeWithTiming(cycles, &timing);
    } else {
        DEBUG_LOG("Executing ARM instructions with timing");
        armCPU->executeWithTiming(cycles, &timing);
    }
}

void CPU::printCPUState() const {
    DEBUG_INFO("Printing CPU state");

    std::string stateStr = "\nCPU State:\n";
    for (int i = 0; i < 16; ++i) {
        stateStr += "R" + std::to_string(i) + (i < 10 ? " : " : ": ") + debug_to_hex_string(registers[i], 8) + "\t";
        if ((i + 1) % 4 == 0) {
            stateStr += "\n"; // New line every 4 registers
        }
    }

    stateStr += "CPSR: " + debug_to_hex_string(cpsr, 8);

    stateStr += " Flags: Z:" + std::to_string(getFlag(FLAG_Z)) + " N:" + std::to_string(getFlag(FLAG_N)) + 
              " V:" + std::to_string(getFlag(FLAG_V)) + " C:" + std::to_string(getFlag(FLAG_C)) + 
              " T:" + std::to_string(getFlag(FLAG_T)) + " E:" + std::to_string(getFlag(FLAG_E)) + "\n";

    DEBUG_INFO(stateStr);
}

// Destructor
CPU::~CPU() {
    delete thumbCPU;
    delete armCPU;
}
