#ifndef INSTRUCTION_DECODER_H
#define INSTRUCTION_DECODER_H

#include <cstdint>

class InstructionDecoder {
public:
    virtual ~InstructionDecoder() = default;
    virtual void decode(uint32_t instruction) = 0; // ARM instruction decoding
    virtual void decode(uint16_t instruction) = 0; // Thumb instruction decoding
};

#endif
