#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif
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
    bool s_bit = bits<22,22>(instruction);  // S bit - restore SPSR to CPSR if R15 in list
    bool writeback = bits<21,21>(instruction);
    int reg_count = std::popcount(reg_list);
    bool r15_in_list = (reg_list & (1 << 15)) != 0;

    uint32_t base = parentCPU.R()[rn];
    uint32_t addr;

    // Main register load loop, always reset addr for the actual loads
    if (up && pre) addr = base + 4;         // IB
    else if (!up && pre) addr = base - 4;   // DB
    else addr = base;                       // IA/DA
    
    // Debug logging for BIOS IRQ handler - DISABLED for performance
    // static int ldm_count = 0;
    // if (ldm_count < 10 && parentCPU.R()[15] < 0x4000) {
    //     const char* mode = (up && pre) ? "IB" : (!up && pre) ? "DB" : up ? "IA" : "DA";
    //     printf("[LDM #%d @0x%08X] Mode=%s, R%d=0x%08X, reglist=0x%04X, start_addr=0x%08X, WB=%d\n",
    //            ldm_count++, parentCPU.R()[15], mode, rn, base, reg_list, addr, writeback);
    // }
    
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            // DISABLED debug logging
            // if (ldm_count <= 10 && parentCPU.R()[15] < 0x4000) {
            //     printf("    Loading R%d from 0x%08X...", i, addr);
            //     fflush(stdout);
            // }
            parentCPU.R()[i] = parentCPU.getMemory().read32(addr);
            if (i == 15) {
                r15_updated = true;
            }
            if (!up && !pre) addr -= 4; // DA
            else if (!up && pre) addr -= 4; // DB
            else addr += 4;
        }
    }
    
    // S-bit handling: When S=1 and R15 is loaded, restore CPSR from SPSR
    // This is used for returning from exception handlers (IRQ, SWI, etc.)
    if (s_bit && r15_in_list) {
        uint32_t spsr = parentCPU.SPSR();
        // PHASE 4: Log IRQ exit with full register state
        static int irq_exit_count = 0;
        if (irq_exit_count < 20) {
            fprintf(stderr, "[IRQ EXIT #%d] PC=0x%08X->0x%08X CPSR=0x%08X->0x%08X SP=0x%08X LR=0x%08X\n",
                    irq_exit_count, parentCPU.R()[15], parentCPU.R()[15], parentCPU.CPSR(), spsr, 
                    parentCPU.R()[13], parentCPU.R()[14]);
            fprintf(stderr, "  R0-R3: %08X %08X %08X %08X  R4-R7: %08X %08X %08X %08X\n",
                    parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
                    parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7]);
            irq_exit_count++;
        }
        // Debug: Log the CPSR restoration
        static int spsr_restore_count = 0;
        if (spsr_restore_count++ < 10) {
            LOG_IRQ("[LDM ^] Restoring CPSR from SPSR: 0x%08X -> 0x%08X, new PC=0x%08X\n",
                   parentCPU.CPSR(), spsr, parentCPU.R()[15]);
        }
        parentCPU.setCPSR(spsr);
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
    // For DB: start at lowest address (base - reg_count*4)
    // Registers stored in ascending order go to ascending addresses
    if (up && pre) addr = base + 4;                    // IB
    else if (!up && pre) addr = base - (reg_count * 4); // DB: start at lowest address
    else addr = base;                                  // IA/DA
    
    bool r15_updated = false;
    for (int i = 0; i < 16; ++i) {
        if (reg_list & (1 << i)) {
            uint32_t value = parentCPU.R()[i];
            if (i == 15) value += 8; // ARM pipeline effect for PC
            parentCPU.getMemory().write32(addr, value);
            // For DA, decrement after each write
            if (!up && !pre) addr -= 4; // DA
            // For DB, INCREMENT after each write (we started at lowest address)
            else if (!up && pre) addr += 4; // DB
            // For IB/IA, increment after each write
            else addr += 4;
            if (i == 15) r15_updated = true;
        }
    }
    if (writeback && reg_count > 0) {
        uint32_t new_base = up ? base + reg_count * 4 : base - reg_count * 4;
        
        // Debug: Log writeback for VRAM addresses
        if (base >= 0x06000000 && base < 0x06018000) {
            static int vram_stm_count = 0;
            if (vram_stm_count < 20) {
                LOG_VRAM("[STM VRAM #%d @PC=0x%08X] R%d writeback: 0x%08X -> 0x%08X (delta=%d)\n",
                       vram_stm_count++, parentCPU.R()[15], rn, base, new_base, (int32_t)(new_base - base));
            }
        }
        
        parentCPU.R()[rn] = new_base;
    }
    if (!r15_updated) parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_b(uint32_t instruction) {
    uint32_t pc_before = parentCPU.R()[15]; UNUSED(pc_before);
    int32_t offset = bits<23,0>(instruction);
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    int32_t branch_offset = (offset << 2) + 8;
    
    // Debug: Trace ALL branches from BIOS
    if (pc_before < 0x4000) {
        static int bios_b_count = 0;
        bios_b_count++;
        uint32_t target = pc_before + branch_offset;
        LOG_BIOS("[BIOS B #%d] @0x%08X: offset=0x%06X, branch_offset=%d -> target=0x%08X CPSR=0x%08X\n",
               bios_b_count, pc_before, offset & 0xFFFFFF, branch_offset, target, parentCPU.CPSR());
    }
    
    parentCPU.R()[15] += branch_offset;
    // printf("[B] pc_after=0x%08X\n", parentCPU.R()[15]);
}

void ARMCPU::exec_arm_bl(uint32_t instruction) {
    uint32_t pc_before = parentCPU.R()[15]; UNUSED(pc_before);
    int32_t offset = bits<23,0>(instruction);
    DEBUG_LOG(std::string("[BL] pc_before=0x") + DEBUG_TO_HEX_STRING(pc_before, 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    if (offset & 0x800000) offset |= 0xFF000000; // sign extend
    int32_t branch_offset = (offset << 2) + 8;
    DEBUG_LOG(std::string("[BL] offset=") + std::to_string(offset) + ", branch_offset=" + std::to_string(branch_offset));
    
    // Debug: Print mode and old LR before setting
    uint32_t old_lr = parentCPU.R()[14]; UNUSED(old_lr);
    uint32_t mode = parentCPU.CPSR() & 0x1F; UNUSED(mode);
    
    parentCPU.R()[14] = parentCPU.R()[15] + 4;
    
    // printf("[BL @0x%08X] Mode=0x%02X, Old LR=0x%08X, New LR=0x%08X\n", 
    //        pc_before, mode, old_lr, parentCPU.R()[14]);
    
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
    UNUSED(instruction);
    DEBUG_ERROR(std::string("exec_arm_undefined: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    
    static int undef_count = 0;
    undef_count++;
    if (undef_count <= 20) {
        LOG_CRASH("[UNDEFINED INSTRUCTION #%d] PC=0x%08X Instr=0x%08X\n", undef_count, parentCPU.R()[15], instruction);
        LOG_CRASH("  CPSR=0x%08X Mode=0x%02X T=%d\n", parentCPU.CPSR(), parentCPU.CPSR() & 0x1F, (parentCPU.CPSR() >> 5) & 1);
        LOG_CRASH("  R0-R3: %08X %08X %08X %08X\n", parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
        LOG_CRASH("  LR=%08X SP=%08X\n", parentCPU.R()[14], parentCPU.R()[13]);
        fflush(stdout);
    }
    
    // Trigger ARM undefined instruction exception
    handleException(0x04, 0x1B, true, false); // Vector 0x04, mode 0x1B (Undefined), disable IRQ
}

void ARMCPU::exec_arm_software_interrupt(uint32_t instruction) {
    DEBUG_LOG(std::string("exec_arm_software_interrupt: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    uint32_t swi_imm = bits<23,0>(instruction); UNUSED(swi_imm);
    // Handle software interrupt (SWI) here. Triggers Supervisor exception.
    // DEBUG_ERROR(std::string("SWI executed: immediate=0x") + DEBUG_TO_HEX_STRING(swi_imm, 8) + ", pc=0x" + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8));
    handleException(0x08, 0x13, true, false); // Vector 0x08, mode 0x13 (SVC), disable IRQ
}

void ARMCPU::exec_arm_ldc_imm(uint32_t instruction) {
    UNUSED(instruction);
    DEBUG_ERROR(std::string("exec_arm_ldc_imm: Coprocessor LDC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor LDC (imm) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_ldc_reg(uint32_t instruction) {
    UNUSED(instruction);
    DEBUG_ERROR(std::string("exec_arm_ldc_reg: Coprocessor LDC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor LDC (reg) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stc_imm(uint32_t instruction) {
    UNUSED(instruction);
    DEBUG_ERROR(std::string("exec_arm_stc_imm: Coprocessor STC (imm) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor STC (imm) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

void ARMCPU::exec_arm_stc_reg(uint32_t instruction) {
    UNUSED(instruction);
    DEBUG_ERROR(std::string("exec_arm_stc_reg: Coprocessor STC (reg) instruction not implemented, pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // TODO: Implement coprocessor STC (reg) logic if needed
    parentCPU.R()[15] += 4; // Increment PC for next instruction
}

// Handler for BX (possible) region: checks for BX, MRS, MSR, else undefined
void ARMCPU::exec_arm_bx_possible(uint32_t instruction) {
    static int call_count = 0;
    if (call_count < 5) {
        LOG_TRACE_CAT("[exec_arm_bx_possible #%d] PC=0x%08X, Instr=0x%08X\n", 
               call_count++, parentCPU.R()[15], instruction);
        LOG_TRACE_CAT("  BX check: (instr & 0x0FFFFFF0) = 0x%08X (vs 0x012FFF10)\n", instruction & 0x0FFFFFF0);
        LOG_TRACE_CAT("  MSR check: (instr & 0x0FF00FF0) = 0x%08X (vs 0x01200F00)\n", instruction & 0x0FF00FF0);
    }
    DEBUG_LOG(std::string("exec_arm_bx_possible: pc=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + ", instr=0x" + DEBUG_TO_HEX_STRING(instruction, 8));
    // BX encoding: bits 27-4 == 0001 0010 1111 1111 1111 0001 (0x012FFF10)
    if ((instruction & 0x0FFFFFF0) == 0x012FFF10) {
        // BX: Branch and Exchange
        uint32_t rm = instruction & 0xF;
        uint32_t target = parentCPU.R()[rm];
        bool thumb = target & 1;
        
        if (thumb) {
            // THUMB mode: clear bit 0 (halfword align)
            uint32_t new_pc = target & ~1u;
            parentCPU.R()[15] = new_pc;
            parentCPU.setFlag(CPU::FLAG_T);
        } else {
            // ARM mode: clear bits 0 and 1 (word align to 4 bytes)
            parentCPU.R()[15] = target & ~3u;
            parentCPU.clearFlag(CPU::FLAG_T);
        }
        DEBUG_LOG(std::string("[BX] to=0x") + DEBUG_TO_HEX_STRING(parentCPU.R()[15], 8) + (thumb ? " (Thumb)" : " (ARM)"));
        return;
    }
    
    // MSR (register): bits 27-21 == 0001001 (0x12), bit 20 == 0, bits 15-12 == 1111, bits 11-4 == 00000000
    // bits 19-16 are the field mask and can be any value
    if ((instruction & 0x0FF0FFF0) == 0x0120F000) {
        exec_arm_msr_reg(instruction);
        return;
    }
    // Otherwise, undefined
    exec_arm_undefined(instruction);
}