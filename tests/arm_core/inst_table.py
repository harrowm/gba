def classify(bits_27_20, mul_swp_bit):
    # bits 27-20
    b27 = (bits_27_20 >> 7) & 1
    b26 = (bits_27_20 >> 6) & 1
    b25 = (bits_27_20 >> 5) & 1  # 0=reg, 1=imm
    b24 = (bits_27_20 >> 4) & 1
    b23 = (bits_27_20 >> 3) & 1
    b22 = (bits_27_20 >> 2) & 1
    b21 = (bits_27_20 >> 1) & 1
    b20 = bits_27_20 & 1

    # MUL/MLA: bits 27-22=000000, mul_swp_bit=1
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 0 and b23 == 0 and b22 == 0 and mul_swp_bit == 1:
        if b21 == 0 and b20 == 0:
            return "MUL"
        elif b21 == 1 and b20 == 0:
            return "MLA"

    # UMULL/UMLAL/SMULL/SMLAL: bits 27-23=00001x, mul_swp_bit=1
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 0 and b23 == 1 and mul_swp_bit == 1:
        if b22 == 0 and b21 == 0:
            return "UMULL"
        elif b22 == 0 and b21 == 1:
            return "UMLAL"
        elif b22 == 1 and b21 == 0:
            return "SMULL"
        elif b22 == 1 and b21 == 1:
            return "SMLAL"

    # SWP/SWPB: bits 27-23=00010x, mul_swp_bit=1
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 1 and b23 == 0 and mul_swp_bit == 1:
        if b22 == 0:
            return "SWP"
        elif b22 == 1:
            return "SWPB"

    # Data Processing: bits 27-26=00, mul_swp_bit=0
    if b27 == 0 and b26 == 0 and mul_swp_bit == 0:
        opcode = (bits_27_20 >> 1) & 0xF  # bits 24-21
        suffix = "_IMM" if b25 == 1 else "_REG"
        data_proc_map = {
            0x0: "AND",
            0x1: "EOR",
            0x2: "SUB",
            0x3: "RSB",
            0x4: "ADD",
            0x5: "ADC",
            0x6: "SBC",
            0x7: "RSC",
            0x8: "TST",
            0x9: "TEQ",
            0xA: "CMP",
            0xB: "CMN",
            0xC: "ORR",
            0xD: "MOV",
            0xE: "BIC",
            0xF: "MVN"
        }
        # TST, TEQ, CMP, CMN only have register form
        if opcode in [0x8, 0x9, 0xA, 0xB]:
            return f"{data_proc_map[opcode]}_REG"
        return f"{data_proc_map.get(opcode, 'Other DP')}{suffix}"

    # Single Data Transfer: b27=0, b26=1, mul_swp_bit=0
    if b27 == 0 and b26 == 1 and mul_swp_bit == 0:
        if b20 == 0 and b22 == 0 and b25 == 1:
            return "STR_IMM"
        elif b20 == 0 and b22 == 0 and b25 == 0:
            return "STR_REG"
        elif b20 == 0 and b22 == 1 and b25 == 1:
            return "STRB_IMM"
        elif b20 == 0 and b22 == 1 and b25 == 0:
            return "STRB_REG"
        elif b20 == 1 and b22 == 0 and b25 == 1:
            return "LDR_IMM"
        elif b20 == 1 and b22 == 0 and b25 == 0:
            return "LDR_REG"
        elif b20 == 1 and b22 == 1 and b25 == 1:
            return "LDRB_IMM"
        elif b20 == 1 and b22 == 1 and b25 == 0:
            return "LDRB_REG"
        else:
            return "Other SDT"
        
    # Branch instructions: bits 27-25 = 101
    if b27 == 1 and b26 == 0 and b25 == 1:
        if b24 == 0:
            return "B"
        elif b24 == 1:
            return "BL"
        
    # Block Data Transfer: bits 27-25 = 100
    if b27 == 1 and b26 == 0 and b25 == 0:
        if b20 == 1:
            return "LDM"
        else:
            return "STM"    
        
    # Software Interrupt: bits 27-24 = 1111
    if b27 == 1 and b26 == 1 and b25 == 1 and b24 == 1:
        return "SWI"

    # Coprocessor instructions: bits 27-24 = 1110
    if b27 == 1 and b26 == 1 and b25 == 1 and b24 == 0:
        if b22 == 1 and b20 == 1:
            return "LDC_IMM"
        elif b22 == 0 and b20 == 1:
            return "LDC_REG"
        elif b22 == 1 and b20 == 0:
            return "STC_IMM"
        elif b22 == 0 and b20 == 0:
            return "STC_REG"
        elif b22 == 1:
            return "MRC"
        elif b22 == 0:
            return "MCR"    
    
    # BX (Branch and Exchange): bits 27-4 = 000100101111111111110001 (not fully covered by this index, but we can mark possible region)
    # For this table, BX is not uniquely identified, but you can add a placeholder:
    if b27 == 0 and b26 == 0 and b25 == 1 and b24 == 0 and b23 == 0 and b22 == 1 and b21 == 0 and b20 == 0 and mul_swp_bit == 0:
        return "BX (possible)"

    # Undefined/Reserved regions
    # Mark anything not matched above as "Undefined/Reserved"
    return "Undefined/Reserved"

type_to_handler = {
    "MUL": "decode_arm_mul",
    "MLA": "decode_arm_mla",
    "UMULL": "decode_arm_umull",
    "UMLAL": "decode_arm_umlal",
    "SMULL": "decode_arm_smull",
    "SMLAL": "decode_arm_smlal",
    "SWP": "decode_arm_swp",
    "SWPB": "decode_arm_swpb",
    "AND_REG": "decode_arm_and_reg",
    "AND_IMM": "decode_arm_and_imm",
    "EOR_REG": "decode_arm_eor_reg",
    "EOR_IMM": "decode_arm_eor_imm",
    "SUB_REG": "decode_arm_sub_reg",
    "SUB_IMM": "decode_arm_sub_imm",
    "RSB_REG": "decode_arm_rsb_reg",
    "RSB_IMM": "decode_arm_rsb_imm",
    "ADD_REG": "decode_arm_add_reg",
    "ADD_IMM": "decode_arm_add_imm",
    "ADC_REG": "decode_arm_adc_reg",
    "ADC_IMM": "decode_arm_adc_imm",
    "SBC_REG": "decode_arm_sbc_reg",
    "SBC_IMM": "decode_arm_sbc_imm",
    "RSC_REG": "decode_arm_rsc_reg",
    "RSC_IMM": "decode_arm_rsc_imm",
    "TST_REG": "decode_arm_tst_reg",
    "TST_IMM": "decode_arm_tst_imm",
    "TEQ_REG": "decode_arm_teq_reg",
    "TEQ_IMM": "decode_arm_teq_imm",
    "CMP_REG": "decode_arm_cmp_reg",
    "CMP_IMM": "decode_arm_cmp_imm",
    "CMN_REG": "decode_arm_cmn_reg",
    "CMN_IMM": "decode_arm_cmn_imm",
    "ORR_REG": "decode_arm_orr_reg",
    "ORR_IMM": "decode_arm_orr_imm",
    "MOV_REG": "decode_arm_mov_reg",
    "MOV_IMM": "decode_arm_mov_imm",
    "BIC_REG": "decode_arm_bic_reg",
    "BIC_IMM": "decode_arm_bic_imm",
    "MVN_REG": "decode_arm_mvn_reg",
    "MVN_IMM": "decode_arm_mvn_imm",
    "STR_IMM": "decode_arm_str_imm",
    "STR_REG": "decode_arm_str_reg",
    "STRB_IMM": "decode_arm_strb_imm",
    "STRB_REG": "decode_arm_strb_reg",
    "LDR_IMM": "decode_arm_ldr_imm",
    "LDR_REG": "decode_arm_ldr_reg",
    "LDRB_IMM": "decode_arm_ldrb_imm",
    "LDRB_REG": "decode_arm_ldrb_reg",
    "LDM": "decode_arm_ldm",
    "STM": "decode_arm_stm",
    "B": "decode_arm_b",
    "BL": "decode_arm_bl",
    "SWI": "decode_arm_software_interrupt",
    "LDC_IMM": "decode_arm_ldc_imm",
    "LDC_REG": "decode_arm_ldc_reg",
    "STC_IMM": "decode_arm_stc_imm",
    "STC_REG": "decode_arm_stc_reg",
    "MRC": "decode_arm_mrc",
    "MCR": "decode_arm_mcr",
    "BX (possible)": "decode_arm_bx_possible",
    "Undefined/Reserved": "decode_arm_undefined"
}

# Build the handler array for all 512 indices
handler_array = [None] * 512
for idx in range(256):
    for mul_swp_bit in (0, 1):
        new_idx = (idx << 1) | mul_swp_bit
        mnemonic = classify(idx, mul_swp_bit)
        handler_array[new_idx] = type_to_handler.get(mnemonic, "decode_arm_undefined")

boilerplate = """// The following code is generated by inst_table.py
// Helper macros for cleaner instruction table initialization
#define ARM_HANDLER(func) &ARMCPU::func
#define REPEAT_2(handler) handler, handler
#define REPEAT_4(handler) handler, handler, handler, handler
#define REPEAT_8(handler) handler, handler, handler, handler, handler, handler, handler, handler
#define REPEAT_16(handler) REPEAT_8(handler), REPEAT_8(handler)
#define REPEAT_32(handler) REPEAT_16(handler), REPEAT_16(handler)
#define REPEAT_64(handler) REPEAT_32(handler), REPEAT_32(handler)
#define REPEAT_128(handler) REPEAT_64(handler), REPEAT_64(handler)

// Table of 512 entries indexed by bits 27-19 (9 bits)
static constexpr void (ARMCPU::*arm_decode_table[512])(ARMCachedInstruction& decoded) = {
"""

cleanup_boilerplate = """};

// Cleanup macros to avoid polluting the namespace
#undef ARM_HANDLER
#undef REPEAT_2
#undef REPEAT_4
#undef REPEAT_8
#undef REPEAT_16
#undef REPEAT_32
#undef REPEAT_64
#undef REPEAT_128
"""

def emit_repeat_macro(handler, count, start, comment_col=52):
    indent = "    "
    if count == 1:
        code = f"{indent}ARM_HANDLER({handler}),"
    elif count == 2:
        code = f"{indent}REPEAT_2(ARM_HANDLER({handler})),"
    elif count == 4:
        code = f"{indent}REPEAT_4(ARM_HANDLER({handler})),"
    elif count == 8:
        code = f"{indent}REPEAT_8(ARM_HANDLER({handler})),"
    elif count == 16:
        code = f"{indent}REPEAT_16(ARM_HANDLER({handler})),"
    elif count == 32:
        code = f"{indent}REPEAT_32(ARM_HANDLER({handler})),"
    elif count == 64:
        code = f"{indent}REPEAT_64(ARM_HANDLER({handler})),"
    elif count == 128:
        code = f"{indent}REPEAT_128(ARM_HANDLER({handler})),"
    else:
        code = indent + ", ".join([f"ARM_HANDLER({handler})" for _ in range(count)]) + ","
    if count == 1:
        comment = f"0x{start:03X}: {handler}"
    else:
        comment = f"0x{start:03X}-0x{start+count-1:03X}: {handler}"
    pad_len = max(comment_col - len(code), 1)
    return f"{code}{' ' * pad_len}// {comment}"

def generate_cpp_table(handler_array):
    output = []
    i = 0
    while i < len(handler_array):
        handler = handler_array[i]
        start = i
        count = 1
        while i + count < len(handler_array) and handler_array[i + count] == handler:
            count += 1
        macros = [128, 64, 32, 16, 8, 4, 2, 1]
        while count > 0:
            for m in macros:
                if count >= m:
                    output.append(emit_repeat_macro(handler, m, start))
                    start += m
                    count -= m
                    break
        i = start
    return "\n".join(output)

if __name__ == "__main__":
    print(boilerplate)
    print(generate_cpp_table(handler_array))
    print(cleanup_boilerplate)