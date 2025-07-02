#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include "interrupt.h"
#include "debug.h"
#include <cstdint>

class ThumbCPU; // Forward declaration
class ARMCPU;   // Forward declaration

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
    void updateCPSRFlags(uint32_t result, uint8_t carryOut);
    void setMode(bool thumbMode);
    bool isThumbMode() const;
    void execute(uint32_t cycles);
};

#endif
