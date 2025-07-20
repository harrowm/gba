#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>

// ARM Data Processing decoders (AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC, TST, TEQ, CMP, CMN, ORR, BIC, MVN) split between IMM and REG variants
// Use macros to generate these    
#define ARM_DP_DECODER_IMM(name) \
void ARMCPU::decode_arm_##name##_imm(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name "_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.rotate = bits<11,8>(decoded.instruction) * 2; \
    decoded.imm = bits<7,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name##_imm; \
}

#define ARM_DP_DECODER_REG(name) \
void ARMCPU::decode_arm_##name##_reg(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name "_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.rs = bits<11,8>(decoded.instruction); \
    decoded.shift_type = bits<6,5>(decoded.instruction); \
    decoded.reg_shift = bits<4,4>(decoded.instruction); \
    decoded.rm = bits<3,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name##_reg; \
}

ARM_DP_DECODER_IMM(and)
ARM_DP_DECODER_REG(and)
ARM_DP_DECODER_IMM(eor)
ARM_DP_DECODER_REG(eor)
ARM_DP_DECODER_IMM(sub)
ARM_DP_DECODER_REG(sub)
ARM_DP_DECODER_IMM(rsb)
ARM_DP_DECODER_REG(rsb)
ARM_DP_DECODER_IMM(add)
ARM_DP_DECODER_REG(add)
ARM_DP_DECODER_IMM(adc)
ARM_DP_DECODER_REG(adc)
ARM_DP_DECODER_IMM(sbc)
ARM_DP_DECODER_REG(sbc)
ARM_DP_DECODER_IMM(rsc)
ARM_DP_DECODER_REG(rsc)
ARM_DP_DECODER_IMM(tst)
ARM_DP_DECODER_REG(tst)
ARM_DP_DECODER_IMM(teq)
ARM_DP_DECODER_REG(teq)
ARM_DP_DECODER_IMM(cmp)
ARM_DP_DECODER_REG(cmp)
ARM_DP_DECODER_IMM(cmn)
ARM_DP_DECODER_REG(cmn)
ARM_DP_DECODER_IMM(orr)
ARM_DP_DECODER_REG(orr)
ARM_DP_DECODER_IMM(bic)
ARM_DP_DECODER_REG(bic)
ARM_DP_DECODER_IMM(mvn)
ARM_DP_DECODER_REG(mvn)

// Macro for ARM single data transfer decoders (STR/LDR/STRB/LDRB, IMM/REG)
#define ARM_SDT_DECODER_IMM(name) \
void ARMCPU::decode_arm_##name##_imm(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name "_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.imm = bits<11,0>(decoded.instruction); \
    decoded.pre_index = bits<24,24>(decoded.instruction); \
    decoded.up = bits<23,23>(decoded.instruction); \
    decoded.writeback = bits<21,21>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name##_imm; \
}

#define ARM_SDT_DECODER_REG(name) \
void ARMCPU::decode_arm_##name##_reg(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name "_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rm = bits<3,0>(decoded.instruction); \
    decoded.offset_type = bits<6,5>(decoded.instruction); \
    decoded.pre_index = bits<24,24>(decoded.instruction); \
    decoded.up = bits<23,23>(decoded.instruction); \
    decoded.writeback = bits<21,21>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name##_reg; \
}

ARM_SDT_DECODER_IMM(strb)
ARM_SDT_DECODER_REG(strb)
ARM_SDT_DECODER_IMM(ldrb)
ARM_SDT_DECODER_REG(ldrb)
ARM_SDT_DECODER_IMM(str)
ARM_SDT_DECODER_REG(str)
ARM_SDT_DECODER_IMM(ldr)
ARM_SDT_DECODER_REG(ldr)


void ARMCPU::exec_arm_ldm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    uint8_t addressing_mode = bits<23,22>(instruction);
    bool writeback = bits<21,21>(instruction);

    uint32_t base = parentCPU.R()[rn];
    int offset = 0;
    // Calculate offset direction and order based on addressing_mode
    bool up = (addressing_mode & 0x2) != 0;
    bool pre = (addressing_mode & 0x1) != 0;
    int reg_count = 0;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) ++reg_count;
    }
    int addr = base;
    if (up) {
        addr += pre ? 4 : 0;
    } else {
        addr -= pre ? 4 : 0;
    }
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(addr);
            addr += up ? 4 : -4;
        }
    }
    if (writeback) {
        parentCPU.R()[rn] = up ? base + reg_count * 4 : base - reg_count * 4;
    }
}

void ARMCPU::exec_arm_stm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_stm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    uint8_t addressing_mode = bits<23,22>(instruction);
    bool writeback = bits<21,21>(instruction);

    uint32_t base = parentCPU.R()[rn];
    int offset = 0;
    // Calculate offset direction and order based on addressing_mode
    bool up = (addressing_mode & 0x2) != 0;
    bool pre = (addressing_mode & 0x1) != 0;
    int reg_count = 0;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) ++reg_count;
    }
    int addr = base;
    if (up) {
        addr += pre ? 4 : 0;
    } else {
        addr -= pre ? 4 : 0;
    }
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            parentCPU.getMemory().write32(addr, parentCPU.R()[i]);
            addr += up ? 4 : -4;
        }
    }
    if (writeback) {
        parentCPU.R()[rn] = up ? base + reg_count * 4 : base - reg_count * 4;
    }
}
void ARMCPU::exec_arm_b(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_b: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    int32_t offset = bits<23,0>(instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    uint32_t branch_offset = (offset << 2) + 8;
    // Branch to target address
    parentCPU.R()[15] += branch_offset;
}

void ARMCPU::exec_arm_bl(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_bl: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    int32_t offset = bits<23,0>(instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    uint32_t branch_offset = (offset << 2) + 8;
    // Set LR to return address (current PC + 4)
    parentCPU.R()[14] = parentCPU.R()[15] + 4;
    // Branch to target address
    parentCPU.R()[15] += branch_offset;
}

void ARMCPU::exec_arm_swp(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_swp: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);

    // Read word from memory address in rn
    uint32_t addr = parentCPU.R()[rn];
    uint32_t mem_val = parentCPU.getMemory().read32(addr);
    // Write value from rm to memory
    parentCPU.getMemory().write32(addr, parentCPU.R()[rm]);
    // Store original memory value in rd
    parentCPU.R()[rd] = mem_val;
}

void ARMCPU::exec_arm_swpb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_swpb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<15,12>(instruction);
    uint8_t rn = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);

    // Read byte from memory address in rn
    uint32_t addr = parentCPU.R()[rn];
    uint8_t mem_val = parentCPU.getMemory().read8(addr);
    // Write value from rm to memory
    parentCPU.getMemory().write8(addr, parentCPU.R()[rm] & 0xFF);
    // Store original memory value in rd
    parentCPU.R()[rd] = mem_val;
}

void ARMCPU::exec_arm_mul(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mul: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    bool    set_flags = bits<20,20>(instruction);
    
    // Multiply two registers and store the result
    uint32_t op1 = parentCPU.R()[rm];
    uint32_t op2 = parentCPU.R()[rs];
    parentCPU.R()[rd] = op1 * op2;
    
    if (set_flags && rd != 15) {
        // Update flags based on the result
        updateFlagsLogical(parentCPU.R()[rd], 0); // No carry for multiplication
    }
}

void ARMCPU::exec_arm_mla(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_mla: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rd = bits<19,16>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    uint8_t rn = bits<15,12>(instruction);
    bool set_flags = bits<20,20>(instruction);

    // Multiply and accumulate: Rd = (Rm * Rs) + Rn
    uint32_t op1 = parentCPU.R()[rm];
    uint32_t op2 = parentCPU.R()[rs];
    uint32_t acc = parentCPU.R()[rn];
    parentCPU.R()[rd] = (op1 * op2) + acc;

    if (set_flags && rd != 15) {
        // Update flags based on the result
        updateFlagsLogical(parentCPU.R()[rd], 0); // No carry for MLA
    }
}

void ARMCPU::exec_arm_umull(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_umull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    bool set_flags = bits<20,20>(instruction);

    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)(result >> 32);

    if (set_flags && rdHi != 15 && rdLo != 15) {
        // Update flags based on the result (N and Z)
        updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_umlal(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_umlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    bool set_flags = bits<20,20>(instruction);

    // Get the accumulator value from RdHi/RdLo
    uint64_t acc = ((uint64_t)parentCPU.R()[rdHi] << 32) | (uint64_t)parentCPU.R()[rdLo];
    uint64_t result = (uint64_t)parentCPU.R()[rm] * (uint64_t)parentCPU.R()[rs] + acc;
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)(result >> 32);

    if (set_flags && rdHi != 15 && rdLo != 15) {
        // Update flags based on the result (N and Z)
        updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::exec_arm_smull(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_smull: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rdHi = bits<19,16>(instruction);
    uint8_t rdLo = bits<15,12>(instruction);
    uint8_t rm = bits<3,0>(instruction);
    uint8_t rs = bits<11,8>(instruction);
    bool set_flags = bits<20,20>(instruction);

    int64_t result = (int64_t)(int32_t)parentCPU.R()[rm] * (int64_t)(int32_t)parentCPU.R()[rs];
    parentCPU.R()[rdLo] = (uint32_t)(result & 0xFFFFFFFF);
    parentCPU.R()[rdHi] = (uint32_t)((result >> 32) & 0xFFFFFFFF);

    if (set_flags && rdHi != 15 && rdLo != 15) {
        // Update flags based on the result (N and Z)
        updateFlagsLogical(parentCPU.R()[rdHi], parentCPU.R()[rdLo]);
    }
}

void ARMCPU::decode_arm_smlal(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_smlal: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rdHi = bits<19,16>(decoded.instruction);
    decoded.rdLo = bits<15,12>(decoded.instruction);
    decoded.rm = bits<3,0>(decoded.instruction);
    decoded.rs = bits<11,8>(decoded.instruction);
    decoded.signed_op = true;
    decoded.accumulate = true;
    decoded.execute_func = &ARMCPU::execute_arm_smlal;
}

// LDRH/STRH/LDRSB/LDRSH decoders
#define ARM_HALFWORD_DECODER(name) \
void ARMCPU::decode_arm_##name(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name ": pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rd = bits<15,12>(decoded.instruction); \
    decoded.rn = bits<19,16>(decoded.instruction); \
    decoded.rm = bits<3,0>(decoded.instruction); \
    decoded.execute_func = &ARMCPU::execute_arm_##name; \
}

ARM_HALFWORD_DECODER(ldrh)
ARM_HALFWORD_DECODER(strh)
ARM_HALFWORD_DECODER(ldrsb)
ARM_HALFWORD_DECODER(ldrsh)

// Undefined decoder
void ARMCPU::decode_arm_undefined(ARMCachedInstruction& decoded) {
    DEBUG_ERROR(std::string("decode_arm_undefined: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.execute_func = &ARMCPU::execute_arm_undefined; 
}

void ARMCPU::decode_arm_mov_imm(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_mov_imm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rotate = bits<11,8>(decoded.instruction) * 2; 
    decoded.imm = bits<7,0>(decoded.instruction);
    
    decoded.execute_func = &ARMCPU::execute_arm_mov_imm;
}

void ARMCPU::decode_arm_mov_reg(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_mov_reg: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    decoded.rn = bits<19,16>(decoded.instruction);
    decoded.rd = bits<15,12>(decoded.instruction);
    decoded.rs = bits<11,7>(decoded.instruction); // could be rs or imm depending on reg_shift
    decoded.shift_type = bits<6,5>(decoded.instruction);
    decoded.reg_shift = bits<4,4>(decoded.instruction); 
    decoded.rm = bits<3,0>(decoded.instruction);

    decoded.execute_func = &ARMCPU::execute_arm_mov_reg;
}

void ARMCPU::decode_arm_software_interrupt(ARMCachedInstruction& decoded) {
    DEBUG_LOG(std::string("decode_arm_software_interrupt: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
    // SWI immediate is bits 23-0
    decoded.imm = bits<23,0>(decoded.instruction);
    decoded.execute_func = &ARMCPU::execute_arm_undefined;
    DEBUG_ERROR(std::string("decode_arm_software_interrupt: SWI instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8));
}


// Load/Store to/from Coprocessor
#define ARM_COPROCESSOR_DECODER(name, type) \
void ARMCPU::decode_arm_##name##_##type(ARMCachedInstruction& decoded) { \
    DEBUG_LOG(std::string("decode_arm_" #name "_" #type ": pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
    decoded.rs = bits<11,8>(decoded.instruction); /* Coprocessor number */ \
    decoded.execute_func = &ARMCPU::execute_arm_undefined; \
    DEBUG_ERROR(std::string("decode_arm_" #name "_" #type ": Coprocessor " #name " (" #type ") instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(decoded.instruction, 8)); \
}

ARM_COPROCESSOR_DECODER(ldc, imm)
ARM_COPROCESSOR_DECODER(ldc, reg)
ARM_COPROCESSOR_DECODER(stc, imm)
ARM_COPROCESSOR_DECODER(stc, reg)
