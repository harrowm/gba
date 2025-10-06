# ARM Data Processing PC+8 Pipeline Offset Fixes

## Overview
ARM7TDMI has a 3-stage pipeline (fetch/decode/execute). When R15 (PC) is used as an operand in data processing instructions, the value read must be PC+8 to account for the pipeline.

## Status Legend
- ✅ Fixed and tested
- ⏳ Todo
- 🧪 Test written, awaiting fix

## Progress: 6/32 Complete (18.75%)

---

## Category 1: Two-Operand Instructions - Register Versions (9 functions)
**Need to fix: Both Rn and Rm operands**

| Function | Rn | Rm | Status | Test | Notes |
|----------|----|----|--------|------|-------|
| exec_arm_add_reg | ✅ | ✅ | ✅ | ✅ | Already fixed - BIOS boot fix |
| exec_arm_sub_reg | ✅ | ✅ | ✅ | ✅ | Fixed - tests passing |
| exec_arm_rsb_reg | ✅ | ✅ | ✅ | ✅ | Fixed - tests passing |
| exec_arm_and_reg | ⏳ | ⏳ | ⏳ | ⏳ | |
| exec_arm_eor_reg | ⏳ | ⏳ | ⏳ | ⏳ | Exclusive OR |
| exec_arm_orr_reg | ⏳ | ⏳ | ⏳ | ⏳ | Logical OR |
| exec_arm_bic_reg | ⏳ | ⏳ | ⏳ | ⏳ | Bit clear |
| exec_arm_adc_reg | ⏳ | ⏳ | ⏳ | ⏳ | Add with carry |
| exec_arm_sbc_reg | ⏳ | ⏳ | ⏳ | ⏳ | Subtract with carry |
| exec_arm_rsc_reg | ⏳ | ⏳ | ⏳ | ⏳ | Reverse subtract with carry |

---

## Category 2: Two-Operand Instructions - Immediate Versions (9 functions)
**Need to fix: Only Rn operand**

| Function | Rn | Status | Test | Notes |
|----------|----| -------|------|-------|
| exec_arm_add_imm | ✅ | ✅ | ✅ | Already fixed - BIOS boot fix (ADR instruction) |
| exec_arm_sub_imm | ✅ | ✅ | ✅ | Fixed - tests passing |
| exec_arm_rsb_imm | ✅ | ✅ | ✅ | Fixed - tests passing |
| exec_arm_and_imm | ⏳ | ⏳ | ⏳ | |
| exec_arm_eor_imm | ⏳ | ⏳ | ⏳ | Exclusive OR |
| exec_arm_orr_imm | ⏳ | ⏳ | ⏳ | Logical OR |
| exec_arm_bic_imm | ⏳ | ⏳ | ⏳ | Bit clear |
| exec_arm_adc_imm | ⏳ | ⏳ | ⏳ | Add with carry |
| exec_arm_sbc_imm | ⏳ | ⏳ | ⏳ | Subtract with carry |
| exec_arm_rsc_imm | ⏳ | ⏳ | ⏳ | Reverse subtract with carry |

---

## Category 3: Comparison/Test Instructions (8 functions)
**Need to fix: Only Rn operand (these don't write results, only set flags)**

| Function | Rn | Status | Test | Notes |
|----------|----| -------|------|-------|
| exec_arm_cmp_imm | ⏳ | ⏳ | ⏳ | Compare: sets flags like SUB but doesn't store |
| exec_arm_cmp_reg | ⏳ | ⏳ | ⏳ | |
| exec_arm_cmn_imm | ⏳ | ⏳ | ⏳ | Compare negative: sets flags like ADD |
| exec_arm_cmn_reg | ⏳ | ⏳ | ⏳ | |
| exec_arm_tst_imm | ⏳ | ⏳ | ⏳ | Test: sets flags like AND |
| exec_arm_tst_reg | ⏳ | ⏳ | ⏳ | |
| exec_arm_teq_imm | ⏳ | ⏳ | ⏳ | Test equivalence: sets flags like EOR |
| exec_arm_teq_reg | ⏳ | ⏳ | ⏳ | |

---

## Category 4: Move Instructions (4 functions)
**Need to fix: Only Rm operand (or none for immediate versions)**

| Function | Rm | Status | Test | Notes |
|----------|----| -------|------|-------|
| exec_arm_mov_imm | N/A | ⏳ | ⏳ | No register operand - may not need fix |
| exec_arm_mov_reg | ⏳ | ⏳ | ⏳ | Move register |
| exec_arm_mvn_imm | N/A | ⏳ | ⏳ | Move NOT - no register operand |
| exec_arm_mvn_reg | ⏳ | ⏳ | ⏳ | Move NOT register |

---

## Fix Pattern Reference

### For Register Versions (two operands):
```cpp
// Before:
uint32_t value = parentCPU.R()[rm];
...
uint32_t result = parentCPU.R()[rn] op shifted.value;

// After:
uint32_t value = readOperand(rm);  // Apply PC+8 if rm==15
...
uint32_t op1 = readOperand(rn);    // Apply PC+8 if rn==15
uint32_t result = op1 op shifted.value;
```

### For Immediate Versions (one operand):
```cpp
// Before:
uint32_t result = parentCPU.R()[rn] op value;

// After:
uint32_t op1 = readOperand(rn);    // Apply PC+8 if rn==15
uint32_t result = op1 op value;
```

### For Move Register (one operand):
```cpp
// Before:
uint32_t value = parentCPU.R()[rm];

// After:
uint32_t value = readOperand(rm);  // Apply PC+8 if rm==15
```

---

## Test File Location
`tests/arm_core/test_arm_data_processing_pc.cpp`

## Test Pattern
For each instruction, test:
1. PC as Rn operand (if applicable)
2. PC as Rm operand (if applicable)
3. Verify result equals expected value with PC+8 offset applied

Example:
```cpp
TEST(ARMDataProcessing, SUB_REG_WithPC_AsRn) {
    // SUB R0, PC, R1
    // Expected: R0 = (PC+8) - R1
}
```

---

## Related Files
- Implementation: `src/arm_exec_data_processing.cpp`
- Header: `include/arm_cpu.h` (helper function)
- Tests: `tests/arm_core/test_arm_data_processing_pc.cpp`

## References
- ARM7TDMI Technical Reference Manual Section 4.5.5
- GBA BIOS uses PC-relative addressing extensively (e.g., ADR at 0x114)
