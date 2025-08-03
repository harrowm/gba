#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

void ARMCPU::exec_arm_mul(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mul: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

    uint8_t rd = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    
    // Multiply two registers and store the result
    uint32_t op1 = parentCPU.R()[rm];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t result = op1 * op2;
    DEBUG_LOG("MUL operands: Rm=" + DEBUG_TO_HEX_STRING(op1, 8) + ", Rs=" + DEBUG_TO_HEX_STRING(op2, 8) + ", result=" + DEBUG_TO_HEX_STRING(result, 8));
    parentCPU.R()[rd] = result;
    
    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_mla(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mla: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

    uint8_t rd = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t rn = bits<15,12>(instruction);

    // Multiply and accumulate: Rd = (Rm * Rs) + Rn
    uint32_t op1 = parentCPU.R()[rm];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t acc = parentCPU.R()[rn];
    parentCPU.R()[rd] = (op1 * op2) + acc;

    if (rd != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction);
        if (set_flags) updateFlagsLogical(parentCPU.R()[rd], 0);
    }
}

void ARMCPU::exec_arm_umull(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_umull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
          
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)(result >> 32);
    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_umlal(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_umlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
          
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);

    // Get the accumulator value from RdHi/RdLo
    uint64_t acc = ((uint64_t)parentCPU.R()[rdHi] << 32) | (uint64_t)parentCPU.R()[rdLo];
    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs] + acc;
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)(result >> 32);

    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_smull(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_smull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));

    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);

    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_smlal(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_smlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
      
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);

    // Get the accumulator value from RdHi/RdLo (unsigned 64-bit)
    int64_t acc = ((uint64_t)parentCPU.R()[rdHi] << 32) | (uint32_t)parentCPU.R()[rdLo];
    int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs] + acc;
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);

    if (rdHi != 15 && rdLo != 15) {
        parentCPU.R()[15] += 4; // Increment PC for next instruction
        bool set_flags = bits<20,20>(instruction); 
        if (set_flags) updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}
