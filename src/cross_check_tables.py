import re

# Cross-reference mapping: mgba handler name -> your handler name
mgba_to_yours = {
    # Data processing
    "AND_LSL": "exec_arm_and_reg",
    "AND_LSR": "exec_arm_and_reg",
    "AND_ASR": "exec_arm_and_reg",
    "AND_ROR": "exec_arm_and_reg",
    "ANDS_LSL": "exec_arm_and_reg",
    "ANDS_LSR": "exec_arm_and_reg",
    "ANDS_ASR": "exec_arm_and_reg",
    "ANDS_ROR": "exec_arm_and_reg",
    "EOR_LSL": "exec_arm_eor_reg",
    "EOR_LSR": "exec_arm_eor_reg",
    "EOR_ASR": "exec_arm_eor_reg",
    "EOR_ROR": "exec_arm_eor_reg",
    "EORS_LSL": "exec_arm_eor_reg",
    "EORS_LSR": "exec_arm_eor_reg",
    "EORS_ASR": "exec_arm_eor_reg",
    "EORS_ROR": "exec_arm_eor_reg",
    "SUB_LSL": "exec_arm_sub_reg",
    "SUB_LSR": "exec_arm_sub_reg",
    "SUB_ASR": "exec_arm_sub_reg",
    "SUB_ROR": "exec_arm_sub_reg",
    "SUBS_LSL": "exec_arm_sub_reg",
    "SUBS_LSR": "exec_arm_sub_reg",
    "SUBS_ASR": "exec_arm_sub_reg",
    "SUBS_ROR": "exec_arm_sub_reg",
    "RSB_LSL": "exec_arm_rsb_reg",
    "RSB_LSR": "exec_arm_rsb_reg",
    "RSB_ASR": "exec_arm_rsb_reg",
    "RSB_ROR": "exec_arm_rsb_reg",
    "RSBS_LSL": "exec_arm_rsb_reg",
    "RSBS_LSR": "exec_arm_rsb_reg",
    "RSBS_ASR": "exec_arm_rsb_reg",
    "RSBS_ROR": "exec_arm_rsb_reg",
    "ADD_LSL": "exec_arm_add_reg",
    "ADD_LSR": "exec_arm_add_reg",
    "ADD_ASR": "exec_arm_add_reg",
    "ADD_ROR": "exec_arm_add_reg",
    "ADDS_LSL": "exec_arm_add_reg",
    "ADDS_LSR": "exec_arm_add_reg",
    "ADDS_ASR": "exec_arm_add_reg",
    "ADDS_ROR": "exec_arm_add_reg",
    "ADC_LSL": "exec_arm_adc_reg",
    "ADC_LSR": "exec_arm_adc_reg",
    "ADC_ASR": "exec_arm_adc_reg",
    "ADC_ROR": "exec_arm_adc_reg",
    "ADCS_LSL": "exec_arm_adc_reg",
    "ADCS_LSR": "exec_arm_adc_reg",
    "ADCS_ASR": "exec_arm_adc_reg",
    "ADCS_ROR": "exec_arm_adc_reg",
    "SBC_LSL": "exec_arm_sbc_reg",
    "SBC_LSR": "exec_arm_sbc_reg",
    "SBC_ASR": "exec_arm_sbc_reg",
    "SBC_ROR": "exec_arm_sbc_reg",
    "SBCS_LSL": "exec_arm_sbc_reg",
    "SBCS_LSR": "exec_arm_sbc_reg",
    "SBCS_ASR": "exec_arm_sbc_reg",
    "SBCS_ROR": "exec_arm_sbc_reg",
    "RSC_LSL": "exec_arm_rsc_reg",
    "RSC_LSR": "exec_arm_rsc_reg",
    "RSC_ASR": "exec_arm_rsc_reg",
    "RSC_ROR": "exec_arm_rsc_reg",
    "RSCS_LSL": "exec_arm_rsc_reg",
    "RSCS_LSR": "exec_arm_rsc_reg",
    "RSCS_ASR": "exec_arm_rsc_reg",
    "RSCS_ROR": "exec_arm_rsc_reg",
    # Multiply
    "MUL": "exec_arm_mul",
    "MULS": "exec_arm_mul",
    "MLA": "exec_arm_mla",
    "MLAS": "exec_arm_mla",
    "UMULL": "exec_arm_umull",
    "UMULLS": "exec_arm_umull",
    "UMLAL": "exec_arm_umlal",
    "UMLALS": "exec_arm_umlal",
    "SMULL": "exec_arm_smull",
    "SMULLS": "exec_arm_smull",
    "SMLAL": "exec_arm_smlal",
    "SMLALS": "exec_arm_smlal",
    # Swap
    "SWP": "exec_arm_swp",
    "SWPB": "exec_arm_swpb",
    # PSR transfer
    "MRS": "exec_arm_mrs",
    "MSR": "exec_arm_msr",
    "MRSR": "exec_arm_undefined",
    "MSRR": "exec_arm_undefined",
    # Test/compare
    "TST_LSL": "exec_arm_tst_reg",
    "TST_LSR": "exec_arm_tst_reg",
    "TST_ASR": "exec_arm_tst_reg",
    "TST_ROR": "exec_arm_tst_reg",
    "TEQ_LSL": "exec_arm_teq_reg",
    "TEQ_LSR": "exec_arm_teq_reg",
    "TEQ_ASR": "exec_arm_teq_reg",
    "TEQ_ROR": "exec_arm_teq_reg",
    "CMP_LSL": "exec_arm_cmp_reg",
    "CMP_LSR": "exec_arm_cmp_reg",
    "CMP_ASR": "exec_arm_cmp_reg",
    "CMP_ROR": "exec_arm_cmp_reg",
    "CMN_LSL": "exec_arm_cmn_reg",
    "CMN_LSR": "exec_arm_cmn_reg",
    "CMN_ASR": "exec_arm_cmn_reg",
    "CMN_ROR": "exec_arm_cmn_reg",
    # Move/orr/bic/mvn
    "ORR_LSL": "exec_arm_orr_reg",
    "ORR_LSR": "exec_arm_orr_reg",
    "ORR_ASR": "exec_arm_orr_reg",
    "ORR_ROR": "exec_arm_orr_reg",
    "ORRS_LSL": "exec_arm_orr_reg",
    "ORRS_LSR": "exec_arm_orr_reg",
    "ORRS_ASR": "exec_arm_orr_reg",
    "ORRS_ROR": "exec_arm_orr_reg",
    "MOV_LSL": "exec_arm_mov_reg",
    "MOV_LSR": "exec_arm_mov_reg",
    "MOV_ASR": "exec_arm_mov_reg",
    "MOV_ROR": "exec_arm_mov_reg",
    "MOVS_LSL": "exec_arm_mov_reg",
    "MOVS_LSR": "exec_arm_mov_reg",
    "MOVS_ASR": "exec_arm_mov_reg",
    "MOVS_ROR": "exec_arm_mov_reg",
    "BIC_LSL": "exec_arm_bic_reg",
    "BIC_LSR": "exec_arm_bic_reg",
    "BIC_ASR": "exec_arm_bic_reg",
    "BIC_ROR": "exec_arm_bic_reg",
    "BICS_LSL": "exec_arm_bic_reg",
    "BICS_LSR": "exec_arm_bic_reg",
    "BICS_ASR": "exec_arm_bic_reg",
    "BICS_ROR": "exec_arm_bic_reg",
    "MVN_LSL": "exec_arm_mvn_reg",
    "MVN_LSR": "exec_arm_mvn_reg",
    "MVN_ASR": "exec_arm_mvn_reg",
    "MVN_ROR": "exec_arm_mvn_reg",
    "MVNS_LSL": "exec_arm_mvn_reg",
    "MVNS_LSR": "exec_arm_mvn_reg",
    "MVNS_ASR": "exec_arm_mvn_reg",
    "MVNS_ROR": "exec_arm_mvn_reg",
    # Immediate variants (guessing)
    "ANDI": "exec_arm_and_imm",
    "ANDSI": "exec_arm_and_imm",
    "EORI": "exec_arm_eor_imm",
    "EORSI": "exec_arm_eor_imm",
    "SUBI": "exec_arm_sub_imm",
    "SUBSI": "exec_arm_sub_imm",
    "RSBI": "exec_arm_rsb_imm",
    "RSBSI": "exec_arm_rsb_imm",
    "ADDI": "exec_arm_add_imm",
    "ADDSI": "exec_arm_add_imm",
    "ADCI": "exec_arm_adc_imm",
    "ADCSI": "exec_arm_adc_imm",
    "SBCI": "exec_arm_sbc_imm",
    "SBCSI": "exec_arm_sbc_imm",
    "RSCI": "exec_arm_rsc_imm",
    "RSCSI": "exec_arm_rsc_imm",
    "TSTI": "exec_arm_tst_imm",
    "TEQI": "exec_arm_teq_imm",
    "CMPI": "exec_arm_cmp_imm",
    "CMNI": "exec_arm_cmn_imm",
    "ORRI": "exec_arm_orr_imm",
    "ORRSI": "exec_arm_orr_imm",
    "MOVI": "exec_arm_mov_imm",
    "MOVSI": "exec_arm_mov_imm",
    "BICI": "exec_arm_bic_imm",
    "BICSI": "exec_arm_bic_imm",
    "MVNI": "exec_arm_mvn_imm",
    "MVNSI": "exec_arm_mvn_imm",
    # Single data transfer (guessing)
    "STRI": "exec_arm_str_imm_pre_nowb",
    "LDRI": "exec_arm_ldr_imm_pre_nowb",
    "STRTI": "exec_arm_str_imm_pre_wb",
    "LDRTI": "exec_arm_ldr_imm_pre_wb",
    "STRBI": "exec_arm_strb_imm_pre_nowb",
    "LDRBI": "exec_arm_ldrb_imm_pre_nowb",
    "STRBTI": "exec_arm_strb_imm_pre_wb",
    "LDRBTI": "exec_arm_ldrb_imm_pre_wb",
    "STRIU": "exec_arm_str_imm_post_wb",
    "LDRIU": "exec_arm_ldr_imm_post_wb",
    "STRTIU": "exec_arm_str_imm_post_wb",
    "LDRTIU": "exec_arm_ldr_imm_post_wb",
    "STRBIU": "exec_arm_strb_imm_post_wb",
    "LDRBIU": "exec_arm_ldrb_imm_post_wb",
    "STRBTIU": "exec_arm_strb_imm_post_wb",
    "LDRBTIU": "exec_arm_ldrb_imm_post_wb",
    # Block data transfer
    "STMDA": "exec_arm_stm",
    "LDMDA": "exec_arm_ldm",
    "STMDAW": "exec_arm_stm",
    "LDMDAW": "exec_arm_ldm",
    "STMSDA": "exec_arm_stm",
    "LDMSDA": "exec_arm_ldm",
    "STMSDAW": "exec_arm_stm",
    "LDMSDAW": "exec_arm_ldm",
    "STMIA": "exec_arm_stm",
    "LDMIA": "exec_arm_ldm",
    "STMIAW": "exec_arm_stm",
    "LDMIAW": "exec_arm_ldm",
    "STMSIA": "exec_arm_stm",
    "LDMSIA": "exec_arm_ldm",
    "STMSIAW": "exec_arm_stm",
    "LDMSIAW": "exec_arm_ldm",
    "STMDB": "exec_arm_stm",
    "LDMDB": "exec_arm_ldm",
    "STMDBW": "exec_arm_stm",
    "LDMDBW": "exec_arm_ldm",
    "STMSDB": "exec_arm_stm",
    "LDMSDB": "exec_arm_ldm",
    "STMSDBW": "exec_arm_stm",
    "LDMSDBW": "exec_arm_ldm",
    "STMIB": "exec_arm_stm",
    "LDMIB": "exec_arm_ldm",
    "STMIBW": "exec_arm_stm",
    "LDMIBW": "exec_arm_ldm",
    "STMSIB": "exec_arm_stm",
    "LDMSIB": "exec_arm_ldm",
    "STMSIBW": "exec_arm_stm",
    "LDMSIBW": "exec_arm_ldm",
    # Branch
    "B": "exec_arm_b",
    "BL": "exec_arm_bl",
    # Coprocessor
    "STC": "exec_arm_stc_imm",
    "LDC": "exec_arm_ldc_imm",
    "CDP": "exec_arm_undefined",
    "MCR": "exec_arm_undefined",
    "MRC": "exec_arm_undefined",
    # Software interrupt
    "SWI": "exec_arm_software_interrupt",
    # Undefined
    "ILL": "exec_arm_undefined",
}

def parse_mgba_table(path):
    """Parse mgba_table.c and return {key: handler_name}"""
    table = {}
    with open(path, 'r') as f:
        for line in f:
            m = re.match(r'\s*\[(0x[0-9A-Fa-f]+)\]\s*=\s*([a-zA-Z0-9_]+),', line)
            if m:
                key = int(m.group(1), 16)
                handler = m.group(2)
                table[key] = handler
    return table

def parse_your_table(path):
    """Parse include/inst_table.inc and return {key: handler_name}"""
    table = {}
    with open(path, 'r') as f:
        for idx, line in enumerate(f):
            m = re.match(r'\s*ARM_FN\(([^)]+)\)', line)
            if m:
                handler = m.group(1)
                table[idx] = handler
    return table

def main():
    mgba_table = parse_mgba_table("src/mgba_table.c")
    your_table = parse_your_table("include/inst_table.inc")
    all_keys = set(mgba_table.keys()) | set(your_table.keys())
    mismatches = 0
    for key in sorted(all_keys):
        mgba_handler = mgba_table.get(key, "ILL")
        your_handler = your_table.get(key, "exec_arm_undefined")
        mapped_handler = mgba_to_yours.get(mgba_handler, "(unmapped)")
        if mapped_handler != your_handler:
            print(f"Mismatch at key 0x{key:03X}: mgba={mgba_handler} (maps to {mapped_handler}), yours={your_handler}")
            mismatches += 1
    print(f"\nTotal mismatches: {mismatches}")

if __name__ == "__main__":
    main()
