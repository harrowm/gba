#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include <cstdint>
#include <array>

class ThumbCPU; // Forward declaration
class ARMCPU;   // Forward declaration

struct CPUState {
    std::array<uint32_t, 16> registers; // General-purpose registers
    uint32_t cpsr;                     // Current Program Status Register
    bool bigEndian;                    // Memory endianness
};

class CPU {
protected:
    Memory& memory;
    InterruptController& interruptController;
    uint32_t registers[16]; // General-purpose registers
    uint32_t cpsr;          // Current Program Status Register
    bool bigEndian;         // Memory endianness

private:
    ThumbCPU* thumbCPU; // Changed to pointer
    ARMCPU* armCPU;     // Changed to pointer

public:
    CPU(Memory& mem, InterruptController& ic);
    virtual ~CPU() = default;

    virtual void step(uint32_t cycles) = 0; // Abstract method for stepping through instructions
    virtual void decodeAndExecute(uint32_t instruction) = 0; // Abstract method for decoding and executing instructions
    virtual void execute(uint32_t cycles); // Made virtual to allow overriding

    void updateCPSRFlags(uint32_t result, uint8_t carryOut);
    void setMode(bool thumbMode);
    bool isThumbMode() const;

    void setCPUState(const CPUState& state); // Set CPU state
    CPUState getCPUState() const;           // Get CPU state
    void printCPUState() const;

    // Flag manipulation methods for CPSR
    static constexpr uint32_t FLAG_ZERO = 1 << 30;
    static constexpr uint32_t FLAG_NEGATIVE = 1 << 31;
    static constexpr uint32_t FLAG_OVERFLOW = 1 << 28;
    static constexpr uint32_t FLAG_CARRY = 1 << 29;
    static constexpr uint32_t FLAG_THUMB = 1 << 5;

    static bool isFlagSet(uint32_t cpsr, uint32_t flag) {
        return (cpsr & flag) != 0;
    }

    static void setFlag(uint32_t& cpsr, uint32_t flag) {
        cpsr |= flag;
    }

    static void clearFlag(uint32_t& cpsr, uint32_t flag) {
        cpsr &= ~flag;
    }
};

#endif
