#ifndef THUMB_CPU_H
#define THUMB_CPU_H

#include <cstdint>
#include "cpu.h"
class CPU; // Forward declaration

class ThumbCPU {
private:
    CPU& parentCPU; // Reference to the parent CPU

    // Instruction handlers
    void handle_thumb_lsl(uint16_t instruction);
    void handle_thumb_lsr(uint16_t instruction);
    void handle_thumb_asr(uint16_t instruction);
    void handle_thumb_add_register(uint16_t instruction);
    void handle_thumb_add_offset(uint16_t instruction);
    void handle_thumb_sub_register(uint16_t instruction);
    void handle_thumb_sub_offset(uint16_t instruction);
    void handle_thumb_mov_imm(uint16_t instruction);
    void handle_thumb_cmp_imm(uint16_t instruction);
    void handle_thumb_add_imm(uint16_t instruction);
    void handle_thumb_sub_imm(uint16_t instruction);
    void handle_thumb_alu_operations(uint16_t instruction);
    void handle_format5(uint16_t instruction);
    void handle_thumb_ldr(uint16_t instruction);
    void handle_thumb_str_word(uint16_t instruction);
    void handle_thumb_str_byte(uint16_t instruction);
    void handle_thumb_ldr_word(uint16_t instruction);
    void handle_thumb_ldr_byte(uint16_t instruction);
    void handle_thumb_strh(uint16_t instruction);
    void handle_thumb_ldsb(uint16_t instruction);
    void handle_thumb_ldrh(uint16_t instruction);
    void handle_thumb_ldsh(uint16_t instruction);
    void handle_thumb_str_immediate_offset(uint16_t instruction);
    void handle_thumb_ldr_immediate_offset(uint16_t instruction);
    void handle_thumb_str_immediate_offset_byte(uint16_t instruction);
    void handle_thumb_ldr_immediate_offset_byte(uint16_t instruction);
    void handle_thumb_strh_imm(uint16_t instruction);
    void handle_thumb_ldrh_imm(uint16_t instruction);
    void handle_thumb_str_sp_rel(uint16_t instruction);
    void handle_thumb_ldr_sp_rel(uint16_t instruction);
    void handle_thumb_ldr_pc_rel(uint16_t instruction);
    void handle_thumb_ldr_address_pc(uint16_t instruction);
    void handle_thumb_ldr_address_sp(uint16_t instruction);
    void handle_thumb_add_sub_offset_to_stack_pointer(uint16_t instruction);
    void handle_thumb_push_registers(uint16_t instruction);
    void handle_thumb_push_registers_and_lr(uint16_t instruction);
    void handle_thumb_pop_registers(uint16_t instruction);
    void handle_thumb_pop_registers_and_pc(uint16_t instruction);
    void handle_thumb_stmia(uint16_t instruction);
    void handle_thumb_ldmia(uint16_t instruction);
    void handle_thumb_beq(uint16_t instruction);
    void handle_thumb_bne(uint16_t instruction);
    void handle_thumb_bcs(uint16_t instruction);
    void handle_thumb_bcc(uint16_t instruction);
    void handle_thumb_bmi(uint16_t instruction);
    void handle_thumb_bpl(uint16_t instruction);
    void handle_thumb_bvs(uint16_t instruction);
    void handle_thumb_bvc(uint16_t instruction);
    void handle_thumb_bhi(uint16_t instruction);
    void handle_thumb_bls(uint16_t instruction);
    void handle_thumb_bge(uint16_t instruction);
    void handle_thumb_blt(uint16_t instruction);
    void handle_thumb_bgt(uint16_t instruction);
    void handle_thumb_ble(uint16_t instruction);
    void handle_thumb_swi(uint16_t instruction);
    void handle_thumb_b(uint16_t instruction);
    void handle_thumb_bl(uint16_t instruction);

    void thumb_alu_and(uint8_t rd, uint8_t rs);
    void thumb_alu_eor(uint8_t rd, uint8_t rs);
    void thumb_alu_lsl(uint8_t rd, uint8_t rs);
    void thumb_alu_lsr(uint8_t rd, uint8_t rs);
    void thumb_alu_asr(uint8_t rd, uint8_t rs);
    void thumb_alu_adc(uint8_t rd, uint8_t rs);
    void thumb_alu_sbc(uint8_t rd, uint8_t rs);
    void thumb_alu_ror(uint8_t rd, uint8_t rs);
    void thumb_alu_tst(uint8_t rd, uint8_t rs);
    void thumb_alu_neg(uint8_t rd, uint8_t rs);
    void thumb_alu_cmp(uint8_t rd, uint8_t rs);
    void thumb_alu_cmn(uint8_t rd, uint8_t rs);
    void thumb_alu_orr(uint8_t rd, uint8_t rs);
    void thumb_alu_mul(uint8_t rd, uint8_t rs);
    void thumb_alu_bic(uint8_t rd, uint8_t rs);
    void thumb_alu_mvn(uint8_t rd, uint8_t rs);

    // Static compile-time array of function pointers for ALU operations
    static constexpr void (ThumbCPU::*thumb_alu_operations_table[16])(uint8_t rd, uint8_t rs) = {
        &ThumbCPU::thumb_alu_and,  // 0x0
        &ThumbCPU::thumb_alu_eor,  // 0x1
        &ThumbCPU::thumb_alu_lsl,  // 0x2
        &ThumbCPU::thumb_alu_lsr,  // 0x3
        &ThumbCPU::thumb_alu_asr,  // 0x4
        &ThumbCPU::thumb_alu_adc,  // 0x5
        &ThumbCPU::thumb_alu_sbc,  // 0x6
        &ThumbCPU::thumb_alu_ror,  // 0x7
        &ThumbCPU::thumb_alu_tst,  // 0x8
        &ThumbCPU::thumb_alu_neg,  // 0x9
        &ThumbCPU::thumb_alu_cmp,  // 0xA
        &ThumbCPU::thumb_alu_cmn,  // 0xB
        &ThumbCPU::thumb_alu_orr,  // 0xC
        &ThumbCPU::thumb_alu_mul,  // 0xD
        &ThumbCPU::thumb_alu_bic,  // 0xE
        &ThumbCPU::thumb_alu_mvn   // 0xF
    };

    //  Static compile-time array of function pointers for Thumb instruction handlers
    static constexpr void (ThumbCPU::*thumb_instruction_table[256])(uint16_t) = {
        // 0x00-0x07: Format 1 - LSL
        &ThumbCPU::handle_thumb_lsl, &ThumbCPU::handle_thumb_lsl, &ThumbCPU::handle_thumb_lsl, &ThumbCPU::handle_thumb_lsl,
        &ThumbCPU::handle_thumb_lsl, &ThumbCPU::handle_thumb_lsl, &ThumbCPU::handle_thumb_lsl, &ThumbCPU::handle_thumb_lsl,
        
        // 0x08-0x0F: Format 1 - LSR  
        &ThumbCPU::handle_thumb_lsr, &ThumbCPU::handle_thumb_lsr, &ThumbCPU::handle_thumb_lsr, &ThumbCPU::handle_thumb_lsr,
        &ThumbCPU::handle_thumb_lsr, &ThumbCPU::handle_thumb_lsr, &ThumbCPU::handle_thumb_lsr, &ThumbCPU::handle_thumb_lsr,
        
        // 0x10-0x17: Format 1 - ASR
        &ThumbCPU::handle_thumb_asr, &ThumbCPU::handle_thumb_asr, &ThumbCPU::handle_thumb_asr, &ThumbCPU::handle_thumb_asr,
        &ThumbCPU::handle_thumb_asr, &ThumbCPU::handle_thumb_asr, &ThumbCPU::handle_thumb_asr, &ThumbCPU::handle_thumb_asr,
        
        // 0x18-0x1F: Format 2 - add/subtract
        &ThumbCPU::handle_thumb_add_register, &ThumbCPU::handle_thumb_add_register, &ThumbCPU::handle_thumb_sub_register, &ThumbCPU::handle_thumb_sub_register,
        &ThumbCPU::handle_thumb_add_offset, &ThumbCPU::handle_thumb_add_offset, &ThumbCPU::handle_thumb_sub_offset, &ThumbCPU::handle_thumb_sub_offset,
        
        // 0x20-0x27: Format 3 - MOV immediate
        &ThumbCPU::handle_thumb_mov_imm, &ThumbCPU::handle_thumb_mov_imm, &ThumbCPU::handle_thumb_mov_imm, &ThumbCPU::handle_thumb_mov_imm,
        &ThumbCPU::handle_thumb_mov_imm, &ThumbCPU::handle_thumb_mov_imm, &ThumbCPU::handle_thumb_mov_imm, &ThumbCPU::handle_thumb_mov_imm,
        
        // 0x28-0x2F: Format 3 - CMP immediate
        &ThumbCPU::handle_thumb_cmp_imm, &ThumbCPU::handle_thumb_cmp_imm, &ThumbCPU::handle_thumb_cmp_imm, &ThumbCPU::handle_thumb_cmp_imm,
        &ThumbCPU::handle_thumb_cmp_imm, &ThumbCPU::handle_thumb_cmp_imm, &ThumbCPU::handle_thumb_cmp_imm, &ThumbCPU::handle_thumb_cmp_imm,
        
        // 0x30-0x37: Format 3 - ADD immediate
        &ThumbCPU::handle_thumb_add_imm, &ThumbCPU::handle_thumb_add_imm, &ThumbCPU::handle_thumb_add_imm, &ThumbCPU::handle_thumb_add_imm,
        &ThumbCPU::handle_thumb_add_imm, &ThumbCPU::handle_thumb_add_imm, &ThumbCPU::handle_thumb_add_imm, &ThumbCPU::handle_thumb_add_imm,
        
        // 0x38-0x3F: Format 3 - SUB immediate
        &ThumbCPU::handle_thumb_sub_imm, &ThumbCPU::handle_thumb_sub_imm, &ThumbCPU::handle_thumb_sub_imm, &ThumbCPU::handle_thumb_sub_imm,
        &ThumbCPU::handle_thumb_sub_imm, &ThumbCPU::handle_thumb_sub_imm, &ThumbCPU::handle_thumb_sub_imm, &ThumbCPU::handle_thumb_sub_imm,
        
        // 0x40-0x43: Format 4 - ALU operations
        &ThumbCPU::handle_thumb_alu_operations, &ThumbCPU::handle_thumb_alu_operations, &ThumbCPU::handle_thumb_alu_operations, &ThumbCPU::handle_thumb_alu_operations,
        
        // 0x44-0x47: Format 5 - Hi register operations/branch exchange
        &ThumbCPU::handle_format5, &ThumbCPU::handle_format5, &ThumbCPU::handle_format5, &ThumbCPU::handle_format5,
        
        // 0x48-0x4F: Format 6 - PC relative load
        &ThumbCPU::handle_thumb_ldr_pc_rel, &ThumbCPU::handle_thumb_ldr_pc_rel, &ThumbCPU::handle_thumb_ldr_pc_rel, &ThumbCPU::handle_thumb_ldr_pc_rel,
        &ThumbCPU::handle_thumb_ldr_pc_rel, &ThumbCPU::handle_thumb_ldr_pc_rel, &ThumbCPU::handle_thumb_ldr_pc_rel, &ThumbCPU::handle_thumb_ldr_pc_rel,
        
        // 0x50-0x5F: Format 7 & 8 - load/store with register offset and sign-extended
        &ThumbCPU::handle_thumb_str_word, &ThumbCPU::handle_thumb_str_word, &ThumbCPU::handle_thumb_strh, &ThumbCPU::handle_thumb_strh,
        &ThumbCPU::handle_thumb_str_byte, &ThumbCPU::handle_thumb_str_byte, &ThumbCPU::handle_thumb_ldsb, &ThumbCPU::handle_thumb_ldsb,
        &ThumbCPU::handle_thumb_ldr_word, &ThumbCPU::handle_thumb_ldr_word, &ThumbCPU::handle_thumb_ldrh, &ThumbCPU::handle_thumb_ldrh,
        &ThumbCPU::handle_thumb_ldr_byte, &ThumbCPU::handle_thumb_ldr_byte, &ThumbCPU::handle_thumb_ldsh, &ThumbCPU::handle_thumb_ldsh,
        
        // 0x60-0x67: Format 9 - STR immediate offset
        &ThumbCPU::handle_thumb_str_immediate_offset, &ThumbCPU::handle_thumb_str_immediate_offset, &ThumbCPU::handle_thumb_str_immediate_offset, &ThumbCPU::handle_thumb_str_immediate_offset,
        &ThumbCPU::handle_thumb_str_immediate_offset, &ThumbCPU::handle_thumb_str_immediate_offset, &ThumbCPU::handle_thumb_str_immediate_offset, &ThumbCPU::handle_thumb_str_immediate_offset,
        
        // 0x68-0x6F: Format 9 - LDR immediate offset
        &ThumbCPU::handle_thumb_ldr_immediate_offset, &ThumbCPU::handle_thumb_ldr_immediate_offset, &ThumbCPU::handle_thumb_ldr_immediate_offset, &ThumbCPU::handle_thumb_ldr_immediate_offset,
        &ThumbCPU::handle_thumb_ldr_immediate_offset, &ThumbCPU::handle_thumb_ldr_immediate_offset, &ThumbCPU::handle_thumb_ldr_immediate_offset, &ThumbCPU::handle_thumb_ldr_immediate_offset,
        
        // 0x70-0x77: Format 9 - STRB immediate offset
        &ThumbCPU::handle_thumb_str_immediate_offset_byte, &ThumbCPU::handle_thumb_str_immediate_offset_byte, &ThumbCPU::handle_thumb_str_immediate_offset_byte, &ThumbCPU::handle_thumb_str_immediate_offset_byte,
        &ThumbCPU::handle_thumb_str_immediate_offset_byte, &ThumbCPU::handle_thumb_str_immediate_offset_byte, &ThumbCPU::handle_thumb_str_immediate_offset_byte, &ThumbCPU::handle_thumb_str_immediate_offset_byte,
        
        // 0x78-0x7F: Format 9 - LDRB immediate offset
        &ThumbCPU::handle_thumb_ldr_immediate_offset_byte, &ThumbCPU::handle_thumb_ldr_immediate_offset_byte, &ThumbCPU::handle_thumb_ldr_immediate_offset_byte, &ThumbCPU::handle_thumb_ldr_immediate_offset_byte,
        &ThumbCPU::handle_thumb_ldr_immediate_offset_byte, &ThumbCPU::handle_thumb_ldr_immediate_offset_byte, &ThumbCPU::handle_thumb_ldr_immediate_offset_byte, &ThumbCPU::handle_thumb_ldr_immediate_offset_byte,
        
        // 0x80-0x87: Format 10 - STRH immediate
        &ThumbCPU::handle_thumb_strh_imm, &ThumbCPU::handle_thumb_strh_imm, &ThumbCPU::handle_thumb_strh_imm, &ThumbCPU::handle_thumb_strh_imm,
        &ThumbCPU::handle_thumb_strh_imm, &ThumbCPU::handle_thumb_strh_imm, &ThumbCPU::handle_thumb_strh_imm, &ThumbCPU::handle_thumb_strh_imm,
        
        // 0x88-0x8F: Format 10 - LDRH immediate
        &ThumbCPU::handle_thumb_ldrh_imm, &ThumbCPU::handle_thumb_ldrh_imm, &ThumbCPU::handle_thumb_ldrh_imm, &ThumbCPU::handle_thumb_ldrh_imm,
        &ThumbCPU::handle_thumb_ldrh_imm, &ThumbCPU::handle_thumb_ldrh_imm, &ThumbCPU::handle_thumb_ldrh_imm, &ThumbCPU::handle_thumb_ldrh_imm,
        
        // 0x90-0x97: Format 11 - STR SP relative
        &ThumbCPU::handle_thumb_str_sp_rel, &ThumbCPU::handle_thumb_str_sp_rel, &ThumbCPU::handle_thumb_str_sp_rel, &ThumbCPU::handle_thumb_str_sp_rel,
        &ThumbCPU::handle_thumb_str_sp_rel, &ThumbCPU::handle_thumb_str_sp_rel, &ThumbCPU::handle_thumb_str_sp_rel, &ThumbCPU::handle_thumb_str_sp_rel,
        
        // 0x98-0x9F: Format 11 - LDR SP relative
        &ThumbCPU::handle_thumb_ldr_sp_rel, &ThumbCPU::handle_thumb_ldr_sp_rel, &ThumbCPU::handle_thumb_ldr_sp_rel, &ThumbCPU::handle_thumb_ldr_sp_rel,
        &ThumbCPU::handle_thumb_ldr_sp_rel, &ThumbCPU::handle_thumb_ldr_sp_rel, &ThumbCPU::handle_thumb_ldr_sp_rel, &ThumbCPU::handle_thumb_ldr_sp_rel,
        
        // 0xA0-0xA7: Format 12 - Load address PC
        &ThumbCPU::handle_thumb_ldr_address_pc, &ThumbCPU::handle_thumb_ldr_address_pc, &ThumbCPU::handle_thumb_ldr_address_pc, &ThumbCPU::handle_thumb_ldr_address_pc,
        &ThumbCPU::handle_thumb_ldr_address_pc, &ThumbCPU::handle_thumb_ldr_address_pc, &ThumbCPU::handle_thumb_ldr_address_pc, &ThumbCPU::handle_thumb_ldr_address_pc,
        
        // 0xA8-0xAF: Format 12 - Load address SP
        &ThumbCPU::handle_thumb_ldr_address_sp, &ThumbCPU::handle_thumb_ldr_address_sp, &ThumbCPU::handle_thumb_ldr_address_sp, &ThumbCPU::handle_thumb_ldr_address_sp,
        &ThumbCPU::handle_thumb_ldr_address_sp, &ThumbCPU::handle_thumb_ldr_address_sp, &ThumbCPU::handle_thumb_ldr_address_sp, &ThumbCPU::handle_thumb_ldr_address_sp,
        
        // 0xB0-0xBF: Format 13 & 14 - Stack operations
        &ThumbCPU::handle_thumb_add_sub_offset_to_stack_pointer, nullptr, nullptr, nullptr,
        &ThumbCPU::handle_thumb_push_registers, &ThumbCPU::handle_thumb_push_registers_and_lr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        &ThumbCPU::handle_thumb_pop_registers, &ThumbCPU::handle_thumb_pop_registers_and_pc, nullptr, nullptr,
        
        // 0xC0-0xC7: Format 15 - STMIA
        &ThumbCPU::handle_thumb_stmia, &ThumbCPU::handle_thumb_stmia, &ThumbCPU::handle_thumb_stmia, &ThumbCPU::handle_thumb_stmia,
        &ThumbCPU::handle_thumb_stmia, &ThumbCPU::handle_thumb_stmia, &ThumbCPU::handle_thumb_stmia, &ThumbCPU::handle_thumb_stmia,
        
        // 0xC8-0xCF: Format 15 - LDMIA
        &ThumbCPU::handle_thumb_ldmia, &ThumbCPU::handle_thumb_ldmia, &ThumbCPU::handle_thumb_ldmia, &ThumbCPU::handle_thumb_ldmia,
        &ThumbCPU::handle_thumb_ldmia, &ThumbCPU::handle_thumb_ldmia, &ThumbCPU::handle_thumb_ldmia, &ThumbCPU::handle_thumb_ldmia,
        
        // 0xD0-0xDF: Format 16 & 17 - Conditional branch and SWI
        &ThumbCPU::handle_thumb_beq, &ThumbCPU::handle_thumb_bne, &ThumbCPU::handle_thumb_bcs, &ThumbCPU::handle_thumb_bcc,
        &ThumbCPU::handle_thumb_bmi, &ThumbCPU::handle_thumb_bpl, &ThumbCPU::handle_thumb_bvs, &ThumbCPU::handle_thumb_bvc,
        &ThumbCPU::handle_thumb_bhi, &ThumbCPU::handle_thumb_bls, &ThumbCPU::handle_thumb_bge, &ThumbCPU::handle_thumb_blt,
        &ThumbCPU::handle_thumb_bgt, &ThumbCPU::handle_thumb_ble, nullptr, &ThumbCPU::handle_thumb_swi,
        
        // 0xE0-0xE7: Format 18 - Unconditional branch
        &ThumbCPU::handle_thumb_b, &ThumbCPU::handle_thumb_b, &ThumbCPU::handle_thumb_b, &ThumbCPU::handle_thumb_b,
        &ThumbCPU::handle_thumb_b, &ThumbCPU::handle_thumb_b, &ThumbCPU::handle_thumb_b, &ThumbCPU::handle_thumb_b,
        
        // 0xE8-0xEF: Undefined
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        
        // 0xF0-0xFF: Format 19 - Long branch with link
        &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl,
        &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl,
        &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl,
        &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl, &ThumbCPU::handle_thumb_bl
    };

    constexpr uint8_t bits10to8(uint16_t instruction) { return (uint8_t)((instruction >> 8) & 0x07); } 
    constexpr uint8_t bits7to0(uint16_t instruction) { return (uint8_t)(instruction & 0xFF); }
    constexpr uint8_t bits2to0(uint16_t instruction) { return (uint8_t)(instruction & 0x07); }
    constexpr uint8_t bits5to3(uint16_t instruction) { return (uint8_t)((instruction >> 3) & 0x07); }
    constexpr uint8_t bits10to6(uint16_t instruction) { return (uint8_t)((instruction >> 6) & 0x1F); }
    constexpr uint8_t bits8to6(uint16_t instruction) { return (uint8_t)((instruction >> 6) & 0x07); }
    constexpr uint8_t bits9to6(uint16_t instruction) { return (uint8_t)((instruction >> 6) & 0x0F); }
    
public:
    explicit ThumbCPU(CPU& cpu);
    ~ThumbCPU();

    void execute(uint32_t cycles);    
};

#endif
