#ifndef ARM_CPU_H
#define ARM_CPU_H

#include <cstdint>
#include "cpu.h"
class CPU; // Forward declaration

class ARMCPU {
private:
    CPU& parentCPU; // Reference to the parent CPU
    void decodeAndExecute(uint32_t instruction);

public:
    explicit ARMCPU(CPU& cpu);
    ~ARMCPU();

    void execute(uint32_t cycles);
};

#endif
