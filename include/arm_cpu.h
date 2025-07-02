#ifndef ARM_CPU_H
#define ARM_CPU_H

#include "cpu.h"

class ARMCPU : public CPU {
public:
    ARMCPU(Memory& mem, InterruptController& ic);
    void step(uint32_t cycles) override;
    void decodeAndExecute(uint32_t instruction) override;
    void execute(uint32_t cycles) override; // Added missing declaration
};

#endif
