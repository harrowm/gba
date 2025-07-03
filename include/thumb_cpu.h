#ifndef THUMB_CPU_H
#define THUMB_CPU_H

#include <cstdint>
#include "cpu.h"
class CPU; // Forward declaration

class ThumbCPU {
private:
    CPU& parentCPU; // Reference to the parent CPU
    //  Array of function pointers for Thumb instruction handlers
    void (ThumbCPU::*thumb_instruction_table[256])(uint16_t);

    // Array of function pointers for ALU operations
    void (ThumbCPU::*thumb_alu_operations_table[16])(uint8_t rd, uint8_t rs);

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
    void handle_add_hi(uint16_t instruction);
    void handle_cmp_hi(uint16_t instruction);
    void handle_mov_hi(uint16_t instruction);
    void handle_bx_hi(uint16_t instruction);
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

    void initializeInstructionTable();
    void thumb_init_alu_operations_table();
    
    void decodeAndExecute(uint16_t instruction);

public:
    explicit ThumbCPU(CPU& cpu);
    ~ThumbCPU();

    void execute(uint32_t cycles);    
};

#endif
