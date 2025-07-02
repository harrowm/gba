#ifndef GPU_H
#define GPU_H

#include "memory.h"

class GPU {
private:
    Memory& memory;

public:
    GPU(Memory& mem) : memory(mem) {}
    void renderScanline();
};

#endif
