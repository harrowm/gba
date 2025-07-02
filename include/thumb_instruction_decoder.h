#ifndef THUMB_INSTRUCTION_DECODER_H
#define THUMB_INSTRUCTION_DECODER_H

#include <cstdint>

class ThumbInstructionDecoder {
public:
    ThumbInstructionDecoder();
    void decode(uint16_t instruction);
};

#endif
