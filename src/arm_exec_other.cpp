// Reference: STM addressing mode and writeback logic
// For STM <cond> <amode> <Rn>!, <reglist>
//
// Addressing mode table:
//  IA (Increment After):      start = base,         end = base + 4 * n,  writeback = base + 4 * n
//  IB (Increment Before):     start = base + 4,     end = base + 4 * (n+1), writeback = base + 4 * n
//  DA (Decrement After):      start = base,         end = base - 4 * n,  writeback = base - 4 * n
//  DB (Decrement Before):     start = base - 4,     end = base - 4 * (n+1), writeback = base - 4 * n
//
// P/U bits:
//  P=0, U=1: IA (Increment After)
//  P=1, U=1: IB (Increment Before)
//  P=0, U=0: DA (Decrement After)
//  P=1, U=0: DB (Decrement Before)
//
// Pseudocode for STM:
//   uint32_t orig_base = Rn;
//   uint32_t addr = orig_base;
//   if (P) addr += (U ? 4 : -4);
//   for (int i = 0; i < 16; ++i) {
//     if (reglist & (1 << i)) {
//       memory.write32(addr, R[i]);
//       addr += (U ? 4 : -4);
//     }
//   }
//   if (W) Rn = orig_base + (U ? 4 : -4) * n_regs;
//
// For DB (P=1, U=0):
//   addr = base - 4;
//   for each reg in reglist (lowest to highest):
//     memory.write32(addr, R[reg]);
//     addr -= 4;
//   writeback = base - 4 * n_regs;
//
// For DA (P=0, U=0):
//   addr = base;
//   for each reg in reglist (lowest to highest):
//     memory.write32(addr, R[reg]);
//     addr -= 4;
//   writeback = base - 4 * n_regs;
//
// For IB (P=1, U=1):
//   addr = base + 4;
//   for each reg in reglist (lowest to highest):
//     memory.write32(addr, R[reg]);
//     addr += 4;
//   writeback = base + 4 * n_regs;
//
// For IA (P=0, U=1):
//   addr = base;
//   for each reg in reglist (lowest to highest):
//     memory.write32(addr, R[reg]);
//     addr += 4;
//   writeback = base + 4 * n_regs;
//
// Note: The order of register writes is always from lowest to highest bit in reglist.
//
// If base register is in reglist, ARM writes the original value, and writeback is implementation-defined.
#include "arm_cpu.h"
#include "debug.h"
#include <cstdio>
#include <bit> // For std::popcount

void ARMCPU::exec_arm_ldm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_ldm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    bool pre = bits<24,24>(instruction);
    bool up  = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);
    int reg_count = std::popcount(reg_list);

    uint32_t base = parentCPU.R()[rn];
    uint32_t addr;

    // Main register load loop, always reset addr for the actual loads
    if (up && pre) addr = base + 4;         // IB
    else if (!up && pre) addr = base - 4;   // DB
    else addr = base;                       // IA/DA
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            parentCPU.R()[i] = parentCPU.getMemory().read32(addr);
            if (i == 15) {
                r15_updated = true;
            }
            if (!up && !pre) addr -= 4; // DA
            else if (!up && pre) addr -= 4; // DB
            else addr += 4;
        }
    }
    if (writeback && reg_count > 0 && !(reg_list & (1 << rn))) {
        uint32_t new_base = up ? base + reg_count * 4 : base - reg_count * 4;
        parentCPU.R()[rn] = new_base;
    }
    if (!r15_updated) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stm(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_stm: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint8_t rn = bits<19,16>(instruction);
    uint16_t reg_list = bits<15,0>(instruction);
    bool pre = bits<24,24>(instruction);
    bool up  = bits<23,23>(instruction);
    bool writeback = bits<21,21>(instruction);

    uint32_t base = parentCPU.R()[rn];
    int reg_count = std::popcount(reg_list);
    uint32_t addr;
    // ARM STM address calculation per mode
    if (up && pre) addr = base + 4;         // IB
    else if (!up && pre) addr = base - 4;   // DB
    else addr = base;                       // IA/DA
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            uint32_t value = parentCPU.R()[i];
            if (i == 15) value += 8; // ARM pipeline effect for PC
            parentCPU.getMemory().write32(addr, value);
            // For DA, decrement after each write
            if (!up && !pre) addr -= 4; // DA
            // For DB, decrement after each write
            else if (!up && pre) addr -= 4; // DB
            // For IB/IA, increment after each write
            else addr += 4;
            if (i == 15) r15_updated = true;
        }
    }
    if (writeback && reg_count > 0) {
        uint32_t new_base = up ? base + reg_count * 4 : base - reg_count * 4;
        parentCPU.R()[rn] = new_base;
    }
    if (!r15_updated) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_b(uint32_t instruction) {
    uint32_t pc_before = parentCPU.R()[15];
    int32_t offset = bits<23,0>(instruction);
    DEBUG_LOG(std::string("[B] pc_before=0x") + DEBUG_TO_HEX_STRING(pc_before, 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    int32_t branch_offset = (offset << 2) + 8;
    DEBUG_LOG(std::string("[B] offset=") + std::to_string(offset) + ", branch_offset=" + std::to_string(branch_offset));
    parentCPU.R()[15] += branch_offset;
    DEBUG_LOG(std::string("[B] pc_after=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8));
}

void ARMCPU::exec_arm_bl(uint32_t instruction) {
    uint32_t pc_before = parentCPU.R()[15];
    int32_t offset = bits<23,0>(instruction);
    DEBUG_LOG(std::string("[BL] pc_before=0x") + DEBUG_TO_HEX_STRING(pc_before, 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    int32_t branch_offset = (offset << 2) + 8;
    DEBUG_LOG(std::string("[BL] offset=") + std::to_string(offset) + ", branch_offset=" + std::to_string(branch_offset));
    parentCPU.R()[14] = parentCPU.R()[15] + 4;
    parentCPU.R()[15] += branch_offset;
    DEBUG_LOG(std::string("[BL] pc_after=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8));
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

    // SWP is only defined for word-aligned addresses; mask to word alignment
    uint32_t addr = parentCPU.R()[rn] & ~0x3;
    DEBUG_LOG(std::string("SWP: masked address = 0x") + DEBUG_TO_HEX_STRING(addr, 8));
    // Read word from memory address in rn (word-aligned)
    uint32_t mem_val = parentCPU.getMemory().read32(addr);
    // Write value from rm to memory (word-aligned)
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

// Handler for BX (possible) region: checks for BX, MRS, MSR, else undefined
void ARMCPU::exec_arm_bx_possible(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_bx_possible: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // BX encoding: bits 27-4 == 0001 0010 1111 1111 1111 0001 (0x012FFF10)
    if ((instruction & 0x0FFFFFF0) == 0x012FFF10) {
        // BX: Branch and Exchange
        uint32_t rm = instruction & 0xF;
        uint32_t target = parentCPU.R()[rm];
        bool thumb = target & 1;
        parentCPU.R()[15] = target & ~1u;
        if (thumb) {
            parentCPU.setFlag(CPU::FLAG_T);
        } else {
            parentCPU.clearFlag(CPU::FLAG_T);
        }
        DEBUG_LOG(std::string("[BX] to=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + (thumb ? " (Thumb)" : " (ARM)"));
        return;
    }
    
    // MSR (register): bits 27-20 == 0x12, bits 19-16 == 0xF, bits 15-12 == 0x0, bit 25 == 0
    if ((instruction & 0x0FBFF000) == 0x012FF000) {
        exec_arm_msr_reg(instruction);
        return;
    }
    // Otherwise, undefined
    exec_arm_undefined(instruction);
}