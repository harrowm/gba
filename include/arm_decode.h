#ifndef ARM_DECODE_H
#define ARM_DECODE_H

#include <cstdint>
#include <cstddef>

// ARM7TDMI instruction decode table using bits 27-19 (9 bits, providing finer granularity)
// Each entry is a function pointer or nullptr if not a valid instruction.
// For groups or ambiguous cases, a group name is used in the comment.
// Many ARM instructions share the same decode group (e.g., data processing, load/store, branch, etc.)
// This table is for illustration and should be filled with actual function pointers in implementation.

// Example function pointer type for instruction handlers
using ArmInstrHandler = void(*)(uint32_t instruction);

// Forward declarations for handler functions (fill in as needed)
void arm_and(uint32_t);
void arm_eor(uint32_t);
void arm_sub(uint32_t);
void arm_rsb(uint32_t);
void arm_add(uint32_t);
void arm_adc(uint32_t);
void arm_sbc(uint32_t);
void arm_rsc(uint32_t);
void arm_tst(uint32_t);
void arm_teq(uint32_t);
void arm_cmp(uint32_t);
void arm_cmn(uint32_t);
void arm_orr(uint32_t);
void arm_mov(uint32_t);
void arm_bic(uint32_t);
void arm_mvn(uint32_t);
void arm_data_processing(uint32_t);
void arm_load_store(uint32_t);
void arm_branch(uint32_t);
void arm_load_store_multiple(uint32_t);
void arm_mul(uint32_t);
void arm_mla(uint32_t);
void arm_umull(uint32_t);
void arm_umlal(uint32_t);
void arm_smull(uint32_t);
void arm_smlal(uint32_t);
void arm_swp(uint32_t);
void arm_swpb(uint32_t);
void arm_ldrh(uint32_t);
void arm_strh(uint32_t);
void arm_ldrsb(uint32_t);
void arm_ldrsh(uint32_t);
void arm_halfword_transfer(uint32_t); // fallback for ambiguous/undefined
void arm_mul_group(uint32_t); // fallback for ambiguous/undefined
void arm_mull_group(uint32_t); // fallback for ambiguous/undefined
void arm_swap_group(uint32_t); // fallback for ambiguous/undefined
void arm_stm(uint32_t);
void arm_ldm(uint32_t);
void arm_undefined(uint32_t);
void arm_coprocessor(uint32_t);
void arm_software_interrupt(uint32_t);
void arm_ldr(uint32_t);
void arm_str(uint32_t);
void arm_ldrb(uint32_t);
void arm_strb(uint32_t);
void arm_ldm(uint32_t);
void arm_stm(uint32_t);
void arm_b(uint32_t);
void arm_bl(uint32_t);
void arm_cdp(uint32_t);
void arm_mrc(uint32_t);
void arm_mcr(uint32_t);

// Table of 512 entries indexed by bits 27-19 (9 bits)
static constexpr ArmInstrHandler arm_decode_table[512] = {
    // 0x000 - 0x07F: Data Processing (now with specific handlers)
    // Each group of 8 covers one opcode (bits 24-21)
    // 0x000-0x007: AND
    arm_and, arm_and, arm_and, arm_and, arm_and, arm_and, arm_and, arm_and,
    // 0x008-0x00F: EOR
    arm_eor, arm_eor, arm_eor, arm_eor, arm_eor, arm_eor, arm_eor, arm_eor,
    // 0x010-0x017: SUB
    arm_sub, arm_sub, arm_sub, arm_sub, arm_sub, arm_sub, arm_sub, arm_sub,
    // 0x018-0x01F: RSB
    arm_rsb, arm_rsb, arm_rsb, arm_rsb, arm_rsb, arm_rsb, arm_rsb, arm_rsb,
    // 0x020-0x027: ADD
    arm_add, arm_add, arm_add, arm_add, arm_add, arm_add, arm_add, arm_add,
    // 0x028-0x02F: ADC
    arm_adc, arm_adc, arm_adc, arm_adc, arm_adc, arm_adc, arm_adc, arm_adc,
    // 0x030-0x037: SBC
    arm_sbc, arm_sbc, arm_sbc, arm_sbc, arm_sbc, arm_sbc, arm_sbc, arm_sbc,
    // 0x038-0x03F: RSC
    arm_rsc, arm_rsc, arm_rsc, arm_rsc, arm_rsc, arm_rsc, arm_rsc, arm_rsc,
    // 0x040-0x047: TST
    arm_tst, arm_tst, arm_tst, arm_tst, arm_tst, arm_tst, arm_tst, arm_tst,
    // 0x048-0x04F: TEQ
    arm_teq, arm_teq, arm_teq, arm_teq, arm_teq, arm_teq, arm_teq, arm_teq,
    // 0x050-0x057: CMP
    arm_cmp, arm_cmp, arm_cmp, arm_cmp, arm_cmp, arm_cmp, arm_cmp, arm_cmp,
    // 0x058-0x05F: CMN
    arm_cmn, arm_cmn, arm_cmn, arm_cmn, arm_cmn, arm_cmn, arm_cmn, arm_cmn,
    // 0x060-0x067: ORR
    arm_orr, arm_orr, arm_orr, arm_orr, arm_orr, arm_orr, arm_orr, arm_orr,
    // 0x068-0x06F: MOV
    arm_mov, arm_mov, arm_mov, arm_mov, arm_mov, arm_mov, arm_mov, arm_mov,
    // 0x070-0x077: BIC
    arm_bic, arm_bic, arm_bic, arm_bic, arm_bic, arm_bic, arm_bic, arm_bic,
    // 0x078-0x07F: MVN
    arm_mvn, arm_mvn, arm_mvn, arm_mvn, arm_mvn, arm_mvn, arm_mvn, arm_mvn,
    // 0x080 - 0x083: MUL, MLA
    arm_mul, arm_mul, arm_mla, arm_mla,
    // 0x084 - 0x087: UMULL, UMLAL, SMULL, SMLAL
    arm_umull, arm_umlal, arm_smull, arm_smlal,
    // 0x088 - 0x089: SWP, SWPB
    arm_swp, arm_swpb,
    // 0x08A - 0x08B: Reserved/undefined (swap group fallback)
    arm_swap_group, arm_swap_group,
    // 0x08C - 0x08F: LDRH, LDRSB, LDRSH, STRH
    arm_ldrh, arm_ldrsb, arm_ldrsh, arm_strh,
    // 0x090 - 0x0FF: Undefined or reserved
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    // 0x100 - 0x13F: STR (store word)
    arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str,
    arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str,
    arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str,
    arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str, arm_str,
    // 0x140 - 0x17F: LDR (load word)
    arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr,
    arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr,
    arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr,
    arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr, arm_ldr,
    // 0x180 - 0x19F: STRB (store byte)
    arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb,
    arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb,
    arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb,
    arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb, arm_strb,
    // 0x1A0 - 0x1DF: LDRB (load byte)
    arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb,
    arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb,
    arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb,
    arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb, arm_ldrb,
    // 0x1E0 - 0x1FF: Reserved/undefined
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    // 0x180 - 0x187: STM
    arm_stm, arm_stm, arm_stm, arm_stm, arm_stm, arm_stm, arm_stm, arm_stm,
    // 0x188 - 0x18F: LDM
    arm_ldm, arm_ldm, arm_ldm, arm_ldm, arm_ldm, arm_ldm, arm_ldm, arm_ldm,
    // 0x1C0 - 0x1DF: Reserved/undefined
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined, arm_undefined,
    // 0x1E0 - 0x1FF: Branch (B, BL)
    arm_b, arm_b, arm_b, arm_b, arm_b, arm_b, arm_b, arm_b,
    arm_bl, arm_bl, arm_bl, arm_bl, arm_bl, arm_bl, arm_bl, arm_bl,
    // 0x200 - 0x2FF: Coprocessor (CDP, MRC, MCR)
    arm_cdp, arm_cdp, arm_cdp, arm_cdp, arm_cdp, arm_cdp, arm_cdp, arm_cdp,
    arm_mrc, arm_mrc, arm_mrc, arm_mrc, arm_mrc, arm_mrc, arm_mrc, arm_mrc,
    arm_mcr, arm_mcr, arm_mcr, arm_mcr, arm_mcr, arm_mcr, arm_mcr, arm_mcr,
    // 0x300 - 0x3FF: Software Interrupt
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt,
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt,
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt,
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt,
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt,
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt,
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt,
    arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt, arm_software_interrupt
};

/*
Notes:
- Data processing instructions (arm_data_processing) cover most of 0x000-0x07F.
- Load/store (arm_load_store) covers 0x100-0x17F.
- Halfword transfer, swap, multiply, multiply long, undefined, coprocessor, and software interrupt are grouped as shown.
- Many entries are grouped (e.g., all data processing, all load/store, etc.).
- For entries not used by ARM7TDMI, use arm_undefined or nullptr.
- If you want to distinguish more finely, expand the table and handler set.
*/

#endif // ARM_DECODE_H
