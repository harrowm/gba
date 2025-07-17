# Stage 1: Data Processing, MUL/MLA, and Immediate

def cpp_table_stage1():
    lines = []
    # Data processing register and MUL/MLA region (0x000-0x03F)
    # Each instruction covers 2 indices (IMM/REG split), so we use REPEAT_2 for each
    instrs = [
        ("decode_arm_and_reg", "AND register"),
        ("decode_arm_eor_reg", "EOR register"),
        ("decode_arm_sub_reg", "SUB register"),
        ("decode_arm_rsb_reg", "RSB register"),
        ("decode_arm_add_reg", "ADD register"),
        ("decode_arm_adc_reg", "ADC register"),
        ("decode_arm_sbc_reg", "SBC register"),
        ("decode_arm_rsc_reg", "RSC register"),
        ("decode_arm_tst_reg", "TST register"),
        ("decode_arm_teq_reg", "TEQ register"),
        ("decode_arm_cmp_reg", "CMP register"),
        ("decode_arm_cmn_reg", "CMN register"),
        ("decode_arm_orr_reg", "ORR register"),
        ("decode_arm_mov_reg", "MOV register"),
        ("decode_arm_bic_reg", "BIC register"),
        ("decode_arm_mvn_reg", "MVN register"),
    ]
    # 0x000-0x01F: Data processing register (AND/EOR/SUB/RSB/ADD/ADC/SBC/RSC)
    for i, (func, comment) in enumerate(instrs[:8]):
        lines.append(f"// 0x{(i*2):03X}-0x{(i*2+1):03X}: {comment}")
        lines.append(f"REPEAT_2(ARM_HANDLER({func})),")
    # 0x020-0x02F: Data processing register (TST/TEQ/CMP/CMN)
    for i, (func, comment) in enumerate(instrs[8:12]):
        lines.append(f"// 0x{(16+i)*2:03X}-0x{(16+i)*2+1:03X}: {comment}")
        lines.append(f"REPEAT_2(ARM_HANDLER({func})),")
    # 0x030-0x03F: Data processing register (ORR/MOV/BIC/MVN)
    for i, (func, comment) in enumerate(instrs[12:]):
        lines.append(f"// 0x{(24+i)*2:03X}-0x{(24+i)*2+1:03X}: {comment}")
        lines.append(f"REPEAT_2(ARM_HANDLER({func})),")
    # 0x040-0x05F: Data processing immediate (AND/EOR/SUB/RSB/ADD/ADC/SBC/RSC)
    imm_instrs = [
        ("decode_arm_and_imm", "AND immediate"),
        ("decode_arm_eor_imm", "EOR immediate"),
        ("decode_arm_sub_imm", "SUB immediate"),
        ("decode_arm_rsb_imm", "RSB immediate"),
        ("decode_arm_add_imm", "ADD immediate"),
        ("decode_arm_adc_imm", "ADC immediate"),
        ("decode_arm_sbc_imm", "SBC immediate"),
        ("decode_arm_rsc_imm", "RSC immediate"),
    ]
    for i, (func, comment) in enumerate(imm_instrs):
        lines.append(f"// 0x{0x040+i*2:03X}-0x{0x040+i*2+1:03X}: {comment}")
        lines.append(f"REPEAT_2(ARM_HANDLER({func})),")
    # 0x060-0x06F: Data processing immediate (TST/TEQ/CMP/CMN)
    imm_instrs2 = [
        ("decode_arm_tst_imm", "TST immediate"),
        ("decode_arm_teq_imm", "TEQ immediate"),
        ("decode_arm_cmp_imm", "CMP immediate"),
        ("decode_arm_cmn_imm", "CMN immediate"),
    ]
    for i, (func, comment) in enumerate(imm_instrs2):
        lines.append(f"// 0x{0x060+i*2:03X}-0x{0x060+i*2+1:03X}: {comment}")
        lines.append(f"REPEAT_2(ARM_HANDLER({func})),")
    # 0x070-0x07F: Data processing immediate (ORR/MOV/BIC/MVN)
    imm_instrs3 = [
        ("decode_arm_orr_imm", "ORR immediate"),
        ("decode_arm_mov_imm", "MOV immediate"),
        ("decode_arm_bic_imm", "BIC immediate"),
        ("decode_arm_mvn_imm", "MVN immediate"),
    ]
    for i, (func, comment) in enumerate(imm_instrs3):
        lines.append(f"// 0x{0x070+i*2:03X}-0x{0x070+i*2+1:03X}: {comment}")
        lines.append(f"REPEAT_2(ARM_HANDLER({func})),")
    # 0x080-0x083: MUL/MLA
    lines.append("// 0x080-0x081: MUL")
    lines.append("REPEAT_2(ARM_HANDLER(decode_arm_mul)),")
    lines.append("// 0x082-0x083: MLA")
    lines.append("REPEAT_2(ARM_HANDLER(decode_arm_mla)),")
    # 0x084-0x087: UMULL/UMLAL/SMULL/SMLAL
    lines.append("// 0x084: UMULL")
    lines.append("ARM_HANDLER(decode_arm_umull),")
    lines.append("// 0x085: UMLAL")
    lines.append("ARM_HANDLER(decode_arm_umlal),")
    lines.append("// 0x086: SMULL")
    lines.append("ARM_HANDLER(decode_arm_smull),")
    lines.append("// 0x087: SMLAL")
    lines.append("ARM_HANDLER(decode_arm_smlal),")
    # 0x088-0x089: SWP/SWPB
    lines.append("// 0x088: SWP")
    lines.append("ARM_HANDLER(decode_arm_swp),")
    lines.append("// 0x089: SWPB")
    lines.append("ARM_HANDLER(decode_arm_swpb),")
    # 0x08A-0x08F: Reserved/undefined
    lines.append("// 0x08A-0x08F: Reserved/undefined")
    lines.append("REPEAT_6(ARM_HANDLER(decode_arm_undefined)),")
    return "\n".join(lines)

if __name__ == "__main__":
    print(cpp_table_stage1())
