#ifndef THUMB_CPU_H
#define THUMB_CPU_H

#include <cstdint>
#include "cpu.h"
class CPU; // Forward declaration

class ThumbCPU {
private:
    CPU& parentCPU; // Reference to the parent CPU

public:
    explicit ThumbCPU(CPU& cpu);
    ~ThumbCPU();

    void execute(uint32_t cycles);
    void decodeAndExecute(uint16_t instruction);
};

#endif
