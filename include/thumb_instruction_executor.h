#ifndef THUMB_INSTRUCTION_EXECUTOR_H
#define THUMB_INSTRUCTION_EXECUTOR_H

#include <cstdint>

class ThumbInstructionExecutor {
public:
    ThumbInstructionExecutor();
    void execute(uint16_t instruction);
};

#endif
