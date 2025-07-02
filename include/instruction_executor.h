#ifndef INSTRUCTION_EXECUTOR_H
#define INSTRUCTION_EXECUTOR_H

#include <cstdint>

class InstructionExecutor {
public:
    virtual ~InstructionExecutor() = default;
    virtual void execute(uint32_t instruction) = 0; // ARM instruction execution
    virtual void execute(uint16_t instruction) = 0; // Thumb instruction execution
};

#endif
