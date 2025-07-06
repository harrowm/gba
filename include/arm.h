#ifndef ARM_H
#define ARM_H

#include "cpu.h"

class ArmCPU : public CPU {
public:
    void executeARM(uint32_t instruction);

private:
    // Add ARM instruction handlers here
};

#endif // ARM_H
