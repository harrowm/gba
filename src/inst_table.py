# There is a lot of confusion over bit 25 in the instruction format.  The arm manual states it is
# always 1 - I think this is a typo and it should be I.
# There is also confusion over what 0 and 1 mean, in summary, 0=reg, 1=imm.

import re


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

    # MRS / MSR: these are definite matches, some edge cases fall through to TST TEQ CMN and CMP and 
    # these will be handled in those decode functions.  The MSR/MRS instructions were added late to the ARM instruction set
    # and reuse TST TEQ CMN and CMP with rn==15
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 1 and b23 == 0 and b20 == 0 and mul_swp_bit == 0:
        if b21 == 0:
            return "MRS"
        else:
            return "MSR_REG"
    # MSR (immediate): b27==0, b26==0, b25==1, b24==1, b23==0, b20==0, mul_swp_bit==0
    if b27 == 0 and b26 == 0 and b25 == 1 and b24 == 1 and b23 == 0 and b20 == 0 and mul_swp_bit == 0:
        return "MSR_IMM"

    # MUL/MLA: bits 27-22=000000, mul_swp_bit=1
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 0 and b23 == 0 and b22 == 0 and mul_swp_bit == 1:
        if b21 == 0:
            return "MULS" if b20 == 1 else "MUL"
        elif b21 == 1:
            return "MLAS" if b20 == 1 else "MLA"

    # UMULL/UMLAL/SMULL/SMLAL: bits 27-23=00001x, mul_swp_bit=1
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 0 and b23 == 1 and mul_swp_bit == 1:
        if b22 == 0 and b21 == 0:
            return "UMULLS" if b20 == 1 else "UMULL"
        elif b22 == 0 and b21 == 1:
            return "UMLALS" if b20 == 1 else "UMLAL"
        elif b22 == 1 and b21 == 0:
            return "SMULLS" if b20 == 1 else "SMULL"
        elif b22 == 1 and b21 == 1:
            return "SMLALS" if b20 == 1 else "SMLAL"

    # SWP/SWPB: bits 27-23=00010x, mul_swp_bit=1
    if b27 == 0 and b26 == 0 and b25 == 0 and b24 == 1 and b23 == 0 and mul_swp_bit == 1:
        if b22 == 0:
            return "SWP"
        elif b22 == 1:
            return "SWPB"

    # Data Processing: bits 27-26=00, mul_swp_bit=0
    if b27 == 0 and b26 == 0 and mul_swp_bit == 0:
        opcode = (bits_27_20 >> 1) & 0xF  # bits 24-21
        suffix = "_IMM" if b25 == 1 else "_REG" # 0=reg, 1=imm, this ternary python syntax is confusing ..
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
        # TST, TEQ, CMP, CMN: generate both _REG and _IMM variants
        if opcode in [0x8, 0x9, 0xA, 0xB]:
            return f"{data_proc_map[opcode]}{suffix}"
        return f"{data_proc_map.get(opcode, 'Other DP')}{suffix}"

    # Single Data Transfer: b27=0, b26=1, mul_swp_bit=0
    if b27 == 0 and b26 == 1 and mul_swp_bit == 0:
        # POST index always writebacks as per ARM spec
        # STR/LDR/STRB/LDRB immediate: distinguish pre/post by b24
        # STR_IMM
        if b20 == 0 and b22 == 0 and b25 == 0:
            if b24 == 0:
                return "STR_IMM_POST_WB"
            return "STR_IMM_PRE_WB" if b21 == 1 else "STR_IMM_PRE_NOWB"    
        # STR_REG
        elif b20 == 0 and b22 == 0 and b25 == 1:
            if b24 == 0:
                return "STR_REG_POST_WB"
            return "STR_REG_PRE_WB" if b21 == 1 else "STR_REG_PRE_NOWB"    
        # STRB_IMM
        elif b20 == 0 and b22 == 1 and b25 == 0:
            if b24 == 0:
                return "STRB_IMM_POST_WB"
            return "STRB_IMM_PRE_WB" if b21 == 1 else "STRB_IMM_PRE_NOWB"
        # STRB_REG
        elif b20 == 0 and b22 == 1 and b25 == 1:
            if b24 == 0:
                return "STRB_REG_POST_WB"
            return "STRB_REG_PRE_WB" if b21 == 1 else "STRB_REG_PRE_NOWB"    
        # LDR_IMM
        elif b20 == 1 and b22 == 0 and b25 == 0:
            if b24 == 0:
                return "LDR_IMM_POST_WB"
            return "LDR_IMM_PRE_WB" if b21 == 1 else "LDR_IMM_PRE_NOWB"
        # LDR_REG
        elif b20 == 1 and b22 == 0 and b25 == 1:
            if b24 == 0:
                return "LDR_REG_POST_WB"
            return "LDR_REG_PRE_WB" if b21 == 1 else "LDR_REG_PRE_NOWB"
        # LDRB_IMM
        elif b20 == 1 and b22 == 1 and b25 == 0:
            if b24 == 0:
                return "LDRB_IMM_POST_WB"
            return "LDRB_IMM_PRE_WB" if b21 == 1 else "LDRB_IMM_PRE_NOWB"
        # LDRB_REG
        elif b20 == 1 and b22 == 1 and b25 == 1:
            if b24 == 0:
                return "LDRB_REG_POST_WB"
            return "LDRB_REG_PRE_WB" if b21 == 1 else "LDRB_REG_PRE_NOWB"
        else:
            return "Other SDT"

    # Branch instructions: bits 27-25 = 101
    if b27 == 1 and b26 == 0 and b25 == 1:
        if b24 == 0:
            return "B"
        elif b24 == 1:
            return "BL"
        
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
    if b27 == 1 and b26 == 1 and b24 == 0:
        # LDC (Load to Coprocessor from Memory)
        if b22 == 1 and b20 == 1:
            return "LDC_IMM" if b25 == 0 else "LDC_REG"
        # LDC (alternate encoding, some ARM docs show b22==0 for LDC)
        elif b22 == 0 and b20 == 1:
            return "LDC_IMM" if b25 == 0 else "LDC_REG"
        # STC (Store from Coprocessor to Memory)
        elif b22 == 1 and b20 == 0:
            return "STC_IMM" if b25 == 0 else "STC_REG"
        # STC (alternate encoding, some ARM docs show b22==0 for STC)
        elif b22 == 0 and b20 == 0:
            return "STC_IMM" if b25 == 0 else "STC_REG"
        # MRC/MCR (Move to/from Coprocessor)
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
    # Data Transfer (LDR/STR/LDRB/STRB)
    "LDR_IMM_PRE_WB": "exec_arm_ldr_imm_pre_wb",
    "LDR_IMM_PRE_NOWB": "exec_arm_ldr_imm_pre_nowb",
    "LDR_IMM_POST_WB": "exec_arm_ldr_imm_post_wb",
    "LDR_IMM_POST_NOWB": "exec_arm_ldr_imm_post_nowb",
    "LDR_REG_PRE_WB": "exec_arm_ldr_reg_pre_wb",
    "LDR_REG_PRE_NOWB": "exec_arm_ldr_reg_pre_nowb",
    "LDR_REG_POST_WB": "exec_arm_ldr_reg_post_wb",
    "LDR_REG_POST_NOWB": "exec_arm_ldr_reg_post_nowb",
    "LDRB_IMM_PRE_WB": "exec_arm_ldrb_imm_pre_wb",
    "LDRB_IMM_PRE_NOWB": "exec_arm_ldrb_imm_pre_nowb",
    "LDRB_IMM_POST_WB": "exec_arm_ldrb_imm_post_wb",
    "LDRB_IMM_POST_NOWB": "exec_arm_ldrb_imm_post_nowb",
    "LDRB_REG_PRE_WB": "exec_arm_ldrb_reg_pre_wb",
    "LDRB_REG_PRE_NOWB": "exec_arm_ldrb_reg_pre_nowb",
    "LDRB_REG_POST_WB": "exec_arm_ldrb_reg_post_wb",
    "LDRB_REG_POST_NOWB": "exec_arm_ldrb_reg_post_nowb",
    "STR_IMM_PRE_WB": "exec_arm_str_imm_pre_wb",
    "STR_IMM_PRE_NOWB": "exec_arm_str_imm_pre_nowb",
    "STR_IMM_POST_WB": "exec_arm_str_imm_post_wb",
    "STR_IMM_POST_NOWB": "exec_arm_str_imm_post_nowb",
    "STR_REG_PRE_WB": "exec_arm_str_reg_pre_wb",
    "STR_REG_PRE_NOWB": "exec_arm_str_reg_pre_nowb",
    "STR_REG_POST_WB": "exec_arm_str_reg_post_wb",
    "STR_REG_POST_NOWB": "exec_arm_str_reg_post_nowb",
    "STRB_IMM_PRE_WB": "exec_arm_strb_imm_pre_wb",
    "STRB_IMM_PRE_NOWB": "exec_arm_strb_imm_pre_nowb",
    "STRB_IMM_POST_WB": "exec_arm_strb_imm_post_wb",
    "STRB_IMM_POST_NOWB": "exec_arm_strb_imm_post_nowb",
    "STRB_REG_PRE_WB": "exec_arm_strb_reg_pre_wb",
    "STRB_REG_PRE_NOWB": "exec_arm_strb_reg_pre_nowb",
    "STRB_REG_POST_WB": "exec_arm_strb_reg_post_wb",
    "STRB_REG_POST_NOWB": "exec_arm_strb_reg_post_nowb",

    # Multiply/Swap
    "MLA": "exec_arm_mla",
    "MLAS": "exec_arm_mla",
    "MUL": "exec_arm_mul",
    "MULS": "exec_arm_mul",
    "SMLAL": "exec_arm_smlal",
    "SMLALS": "exec_arm_smlal",
    "SMULL": "exec_arm_smull",
    "SMULLS": "exec_arm_smull",
    "SWP": "exec_arm_swp",
    "SWPB": "exec_arm_swpb",
    "UMLAL": "exec_arm_umlal",
    "UMLALS": "exec_arm_umlal",
    "UMULL": "exec_arm_umull",
    "UMULLS": "exec_arm_umull",

    # Data Processing
    "ADC_IMM": "exec_arm_adc_imm",
    "ADC_REG": "exec_arm_adc_reg",
    "ADD_IMM": "exec_arm_add_imm",
    "ADD_REG": "exec_arm_add_reg",
    "AND_IMM": "exec_arm_and_imm",
    "AND_REG": "exec_arm_and_reg",
    "BIC_IMM": "exec_arm_bic_imm",
    "BIC_REG": "exec_arm_bic_reg",
    "CMN_IMM": "exec_arm_cmn_imm",
    "CMN_REG": "exec_arm_cmn_reg",
    "CMP_IMM": "exec_arm_cmp_imm",
    "CMP_REG": "exec_arm_cmp_reg",
    "EOR_IMM": "exec_arm_eor_imm",
    "EOR_REG": "exec_arm_eor_reg",
    "MOV_IMM": "exec_arm_mov_imm",
    "MOV_REG": "exec_arm_mov_reg",
    "MVN_IMM": "exec_arm_mvn_imm",
    "MVN_REG": "exec_arm_mvn_reg",
    "ORR_IMM": "exec_arm_orr_imm",
    "ORR_REG": "exec_arm_orr_reg",
    "RSB_IMM": "exec_arm_rsb_imm",
    "RSB_REG": "exec_arm_rsb_reg",
    "RSC_IMM": "exec_arm_rsc_imm",
    "RSC_REG": "exec_arm_rsc_reg",
    "SBC_IMM": "exec_arm_sbc_imm",
    "SBC_REG": "exec_arm_sbc_reg",
    "SUB_IMM": "exec_arm_sub_imm",
    "SUB_REG": "exec_arm_sub_reg",
    "TEQ_IMM": "exec_arm_teq_imm",
    "TEQ_REG": "exec_arm_teq_reg",
    "TST_IMM": "exec_arm_tst_imm",
    "TST_REG": "exec_arm_tst_reg",

    # Branch/Block Transfer
    "B": "exec_arm_b",
    "BL": "exec_arm_bl",
    "LDM": "exec_arm_ldm",
    "STM": "exec_arm_stm",

    # Software Interrupt
    "SWI": "exec_arm_software_interrupt",

    # Coprocessor
    "LDC_IMM": "exec_arm_ldc_imm",
    "LDC_REG": "exec_arm_ldc_reg",
    "MCR": "exec_arm_mcr",
    "MRC": "exec_arm_mrc",
    "STC_IMM": "exec_arm_stc_imm",
    "STC_REG": "exec_arm_stc_reg",

    # Miscellaneous
    "BX (possible)": "exec_arm_bx_possible",
    "MRS": "exec_arm_mrs",
    "MSR_REG": "exec_arm_msr_reg",
    "MSR_IMM": "exec_arm_msr_imm",

    # Undefined
    "Undefined/Reserved": "exec_arm_undefined"
}

# Build the handler array for all 512 indices
handler_array = [None] * 512
for idx in range(256):
    for mul_swp_bit in (0, 1):
        new_idx = (idx << 1) | mul_swp_bit
        mnemonic = classify(idx, mul_swp_bit)
        handler_array[new_idx] = type_to_handler.get(mnemonic, "exec_arm_undefined")

boilerplate = """    // The following code is generated by inst_table.py
    // Helper macros for cleaner instruction table initialization
    #define ARM_FN(func) &ARMCPU::func
    #define REPEAT_2(handler) handler, handler
    #define REPEAT_4(handler) handler, handler, handler, handler
    #define REPEAT_8(handler) handler, handler, handler, handler, handler, handler, handler, handler
    #define REPEAT_16(handler) REPEAT_8(handler), REPEAT_8(handler)
    #define REPEAT_32(handler) REPEAT_16(handler), REPEAT_16(handler)
    #define REPEAT_64(handler) REPEAT_32(handler), REPEAT_32(handler)
    #define REPEAT_128(handler) REPEAT_64(handler), REPEAT_64(handler)
    #define REPEAT_ALT(a, b) a, b, a, b
    #define REPEAT_2ALT(a, b) a, a, b, b, a, a, b, b

    // Table of 512 entries indexed by bits 27-19 (9 bits)
    static constexpr void (ARMCPU::*arm_exec_table[512])(uint32_t instruction) = {"""

cleanup_boilerplate = """    };

    // Cleanup macros to avoid polluting the namespace
    #undef ARM_FN
    #undef REPEAT_2
    #undef REPEAT_4
    #undef REPEAT_8
    #undef REPEAT_16
    #undef REPEAT_32
    #undef REPEAT_64
    #undef REPEAT_128
    #undef REPEAT_ALT
    #undef REPEAT_2ALT
"""

def emit_repeat_macro(handler, count, start, comment_col=52):
    indent = "        "
    # Support for alternating patterns
    if isinstance(handler, tuple) and len(handler) == 2:
        a, b = handler
        if count == 4:
            code = f"{indent}REPEAT_ALT(ARM_FN({a}), ARM_FN({b})),"
            comment = f"0x{start:03X}-0x{start+count-1:03X}: {a}/{b} ABAB"
        elif count == 4:  # AABBAABB
            code = f"{indent}REPEAT_2ALT(ARM_FN({a}), ARM_FN({b})),"
            comment = f"0x{start:03X}-0x{start+count-1:03X}: {a}/{b} AABBAABB"
        else:
            # fallback to explicit listing for other counts
            code = indent + ", ".join([f"ARM_FN({a})" if (i % 2 == 0) else f"ARM_FN({b})" for i in range(count)]) + ","
            comment = f"0x{start:03X}-0x{start+count-1:03X}: {a}/{b} ALT"
        pad_len = max(comment_col - len(code), 1)
        return f"{code}{' ' * pad_len}// {comment}"
    # ...existing code for single handler...
    if count == 1:
        code = f"{indent}ARM_FN({handler}),"
    elif count == 2:
        code = f"{indent}REPEAT_2(ARM_FN({handler})),"
    elif count == 4:
        code = f"{indent}REPEAT_4(ARM_FN({handler})),"
    elif count == 8:
        code = f"{indent}REPEAT_8(ARM_FN({handler})),"
    elif count == 16:
        code = f"{indent}REPEAT_16(ARM_FN({handler})),"
    elif count == 32:
        code = f"{indent}REPEAT_32(ARM_FN({handler})),"
    elif count == 64:
        code = f"{indent}REPEAT_64(ARM_FN({handler})),"
    elif count == 128:
        code = f"{indent}REPEAT_128(ARM_FN({handler})),"
    else:
        code = indent + ", ".join([f"ARM_FN({handler})" for _ in range(count)]) + ","
    if count == 1:
        comment = f"0x{start:03X}: {handler}"
    else:
        comment = f"0x{start:03X}-0x{start+count-1:03X}: {handler}"
    pad_len = max(comment_col - len(code), 1)
    return f"{code}{' ' * pad_len}// {comment}"

def generate_cpp_table(handler_array):
    output = []
    i = 0
    n = len(handler_array)
    macros = [128, 64, 32, 16, 8, 4, 2, 1]
    while i < n:
        # Check for ABAB pattern (REPEAT_ALT, 4 entries)
        if i + 3 < n:
            a = handler_array[i]
            b = handler_array[i+1]
            ababab = True
            for j in range(4):
                expected = a if j % 2 == 0 else b
                if handler_array[i+j] != expected:
                    ababab = False
                    break
            if ababab and a != b:
                output.append(emit_repeat_macro((a, b), 4, i))
                i += 4
                continue
        # Check for AABBAABB pattern (REPEAT_2ALT)
        if i + 3 < n:
            a = handler_array[i]
            b = handler_array[i+2]
            aabbaabb = True
            for j in range(4):
                expected = a if j < 2 else b
                if handler_array[i+j] != expected:
                    aabbaabb = False
                    break
            if aabbaabb and a != b:
                output.append(emit_repeat_macro((a, b), 4, i))
                i += 4
                continue
        # Fallback to runs of identical handlers
        handler = handler_array[i]
        start = i
        count = 1
        while i + count < n and handler_array[i + count] == handler:
            count += 1
        while count > 0:
            for m in macros:
                if count >= m:
                    output.append(emit_repeat_macro(handler, m, start))
                    start += m
                    count -= m
                    break
        i = start
    return "\n".join(output)

# Generate synchronized function name array
def generate_cpp_fn_name_table(handler_array):
    output = []
    output.append("static constexpr const char* arm_exec_fn_names[512] = {")
    for i, handler in enumerate(handler_array):
        output.append(f'    "{handler}",  // 0x{i:03X}')
    output.append("};")
    return "\n".join(output)

if __name__ == "__main__":
    print(boilerplate)
    print(generate_cpp_table(handler_array))
    print(cleanup_boilerplate)
    print(generate_cpp_fn_name_table(handler_array))