#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>
#include <bit> // For std::popcount

void ARMCPU::exec_arm_ldm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    uint8_t addressing_mode = bits<23,22>(instruction);
    bool writeback = bits<21,21>(instruction);

    uint32_t base = parentCPU.R()[rn];
    // int offset = 0;
    // Calculate offset direction and order based on addressing_mode
    bool up = (addressing_mode & 0x2) != 0;
    bool pre = (addressing_mode & 0x1) != 0;

    int reg_count = std::popcount(reg_list);
    int addr = base;
    if (up) {
        addr += pre ? 4 : 0;
    } else {
        addr -= pre ? 4 : 0;
    }
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(addr);
            addr += up ? 4 : -4;
            if (i == 15) r15_updated = true;
        }
    }
    if (writeback) {
        parentCPU.R()[rn] = up ? base + reg_count * 4 : base - reg_count * 4;
    }
    if (!r15_updated) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_stm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    uint8_t addressing_mode = bits<23,22>(instruction);
    bool writeback = bits<21,21>(instruction);

    uint32_t base = parentCPU.R()[rn];
    // int offset = 0;
    // Calculate offset direction and order based on addressing_mode
    bool up = (addressing_mode & 0x2) != 0;
    bool pre = (addressing_mode & 0x1) != 0;
   
    int reg_count = std::popcount(reg_list);
    int addr = base;
    if (up) {
        addr += pre ? 4 : 0;
    } else {
        addr -= pre ? 4 : 0;
    }
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            parentCPU.getMemory().write32(addr, parentCPU.R()[i]);
            addr += up ? 4 : -4;
            if (i == 15) r15_updated = true;
        }
    }
    if (writeback) {
        parentCPU.R()[rn] = up ? base + reg_count * 4 : base - reg_count * 4;
    }
    if (!r15_updated) parentCPU.R()[15] += 4; // Increment PC for next instruction
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
      
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

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
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_swpb(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_swpb: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
      
    if (bits<6,5>(instruction) != 0) [[unlikely]] {
        exec_arm_further_decode(instruction);
        return;
    }

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
    if (rd != 15) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_undefined(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_undefined: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // Trigger ARM undefined instruction exception
    handleException(0x04, 0x1B, true, false); // Vector 0x04, mode 0x1B (Undefined), disable IRQ
}

void ARMCPU::exec_arm_software_interrupt(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_software_interrupt: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint32_t swi_imm = bits<23,0>(instruction);
    // Handle software interrupt (SWI) here. Triggers Supervisor exception.
    DEBUG_ERROR(std::string("SWI executed: immediate=0x") + DEBUG_TO_HEX_STRING(swi_imm, 8) + ", pc=0x" + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8));
    handleException(0x08, 0x13, true, false); // Vector 0x08, mode 0x13 (SVC), disable IRQ
}

void ARMCPU::exec_arm_ldc_imm(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_ldc_imm: Coprocessor LDC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor LDC (imm) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldc_reg(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_ldc_reg: Coprocessor LDC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor LDC (reg) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stc_imm(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_stc_imm: Coprocessor STC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor STC (imm) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stc_reg(uint32_t instruction) {
    DEBUG_ERROR(std::string("exec_arm_stc_reg: Coprocessor STC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor STC (reg) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}