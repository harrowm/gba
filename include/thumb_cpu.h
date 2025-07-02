#ifndef THUMB_CPU_H
#define THUMB_CPU_H

#include "cpu.h"
#include "thumb_instruction_decoder.h"
#include "thumb_instruction_executor.h"

class ThumbCPU : public CPU {
public:
    ThumbCPU(Memory& mem, InterruptController& ic);
    void step(uint32_t cycles) override;
    void decodeAndExecute(uint32_t instruction) override; // Updated parameter type to match base class

private:
    ThumbInstructionDecoder decoder;
    ThumbInstructionExecutor executor;
};

#endif
