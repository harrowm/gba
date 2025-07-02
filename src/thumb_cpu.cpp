#include "thumb_cpu.h"
#include "debug.h"
#include "thumb_instruction_decoder.h"
#include "thumb_instruction_executor.h"

ThumbCPU::ThumbCPU(Memory& mem, InterruptController& ic) : CPU(mem, ic), decoder(), executor() {}

void ThumbCPU::step(uint32_t cycles) {
    while (cycles > 0) {
        uint32_t instruction = memory.read16(registers[15]); // Fetch instruction
        decodeAndExecute(instruction);
        cycles -= 1; // Placeholder for cycle deduction
    }
}

void ThumbCPU::decodeAndExecute(uint32_t instruction) {
    uint16_t thumbInstruction = static_cast<uint16_t>(instruction); // Cast to 16-bit
    decoder.decode(thumbInstruction);
    executor.execute(thumbInstruction);
}
