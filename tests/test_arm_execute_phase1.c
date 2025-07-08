#include <stdio.h>
#include <assert.h>
#include "arm_execute_phase1.h"

// Mock memory interface functions
uint32_t mock_read32(void* context, uint32_t address) {
    printf("Mock memory read32 from 0x%08X\n", address);
    return 0xDEADBEEF; // Placeholder value
}

void mock_write32(void* context, uint32_t address, uint32_t value) {
    printf("Mock memory write32 to 0x%08X: 0x%08X\n", address, value);
}

uint16_t mock_read16(void* context, uint32_t address) {
    printf("Mock memory read16 from 0x%08X\n", address);
    return 0xBEEF;
}

void mock_write16(void* context, uint32_t address, uint16_t value) {
    printf("Mock memory write16 to 0x%08X: 0x%04X\n", address, value);
}

uint8_t mock_read8(void* context, uint32_t address) {
    printf("Mock memory read8 from 0x%08X\n", address);
    return 0xEF;
}

void mock_write8(void* context, uint32_t address, uint8_t value) {
    printf("Mock memory write8 to 0x%08X: 0x%02X\n", address, value);
}

// Test helpers
void setup_test_cpu(ArmCPUState* state) {
    for (int i = 0; i < 16; i++) {
        state->registers[i] = 0;
    }
    state->cpsr = 0x00000000; // ARM mode, all flags clear
}

// Basic condition code testing
void test_arm_condition_checking() {
    printf("Testing ARM condition code checking...\n");
    
    ArmCPUState state;
    setup_test_cpu(&state);
    
    // Create memory interface
    ArmMemoryInterface memory_interface = {
        .context = NULL,
        .read32 = mock_read32,
        .write32 = mock_write32,
        .read16 = mock_read16,
        .write16 = mock_write16,
        .read8 = mock_read8,
        .write8 = mock_write8
    };
    
    // Test with EQ condition (Z=1)
    state.cpsr = ARM_FLAG_Z; // Set Z flag
    uint32_t eq_instruction = 0x00000000 | (ARM_COND_EQ << 28); // EQ condition
    assert(arm_execute_check_condition(ARM_GET_CONDITION(eq_instruction), state.cpsr));
    printf("✓ EQ condition passed when Z=1\n");
    
    // Test with NE condition (Z=0)
    state.cpsr = 0; // Clear Z flag
    uint32_t ne_instruction = 0x00000000 | (ARM_COND_NE << 28); // NE condition
    assert(arm_execute_check_condition(ARM_GET_CONDITION(ne_instruction), state.cpsr));
    printf("✓ NE condition passed when Z=0\n");
    
    // Test with AL condition (always)
    state.cpsr = 0;
    uint32_t al_instruction = 0x00000000 | (ARM_COND_AL << 28); // AL condition
    assert(arm_execute_check_condition(ARM_GET_CONDITION(al_instruction), state.cpsr));
    printf("✓ AL condition always passes\n");
    
    printf("ARM condition code tests passed!\n\n");
}

// Basic instruction execution test
void test_arm_execute_basic() {
    printf("Testing basic ARM instruction execution...\n");
    
    ArmCPUState state;
    setup_test_cpu(&state);
    
    // Create memory interface
    ArmMemoryInterface memory_interface = {
        .context = NULL,
        .read32 = mock_read32,
        .write32 = mock_write32,
        .read16 = mock_read16,
        .write16 = mock_write16,
        .read8 = mock_read8,
        .write8 = mock_write8
    };
    
    // Test a simple data processing instruction (MOV R0, #1)
    state.registers[0] = 0;
    uint32_t mov_instruction = 0xE3A00001; // MOV R0, #1 (AL condition)
    
    // At this stage, we're just checking that our function handles the instruction format correctly
    bool pc_modified = arm_execute_instruction(&state, mov_instruction, &memory_interface);
    
    // In our initial implementation, we only check if PC could be modified
    assert(pc_modified == false); // R0 is destination, not PC (R15)
    printf("✓ ARM MOV R0, #1 correctly identified as not modifying PC\n");
    
    // Test instruction that writes to PC (MOV PC, #1)
    uint32_t mov_pc_instruction = 0xE3A0F001; // MOV PC, #1 (AL condition)
    pc_modified = arm_execute_instruction(&state, mov_pc_instruction, &memory_interface);
    
    // This should detect potential PC modification
    assert(pc_modified == true);
    printf("✓ ARM MOV PC, #1 correctly identified as potentially modifying PC\n");
    
    printf("Basic ARM instruction execution tests passed!\n\n");
}

// Test data processing instructions
void test_arm_data_processing() {
    printf("Testing ARM data processing instructions...\n");
    
    ArmCPUState state;
    setup_test_cpu(&state);
    
    // Create memory interface
    ArmMemoryInterface memory_interface = {
        .context = NULL,
        .read32 = mock_read32,
        .write32 = mock_write32,
        .read16 = mock_read16,
        .write16 = mock_write16,
        .read8 = mock_read8,
        .write8 = mock_write8
    };
    
    // Test MOV R0, #42 (0xE3A0002A)
    state.registers[0] = 0;
    uint32_t mov_instruction = 0xE3A0002A; // MOV R0, #42
    bool pc_modified = arm_execute_instruction(&state, mov_instruction, &memory_interface);
    assert(state.registers[0] == 42);
    assert(pc_modified == false); // R0 is not PC
    printf("✓ MOV R0, #42 executed correctly\n");
    
    // Test ADD R1, R0, #10 (0xE280100A)
    state.registers[0] = 42;
    state.registers[1] = 0;
    uint32_t add_instruction = 0xE280100A; // ADD R1, R0, #10
    pc_modified = arm_execute_instruction(&state, add_instruction, &memory_interface);
    assert(state.registers[1] == 52);
    assert(pc_modified == false);
    printf("✓ ADD R1, R0, #10 executed correctly\n");
    
    // Test SUB R2, R1, R0 (0xE0412000)
    state.registers[1] = 52;
    state.registers[0] = 42;
    state.registers[2] = 0;
    uint32_t sub_instruction = 0xE0412000; // SUB R2, R1, R0
    pc_modified = arm_execute_instruction(&state, sub_instruction, &memory_interface);
    assert(state.registers[2] == 10);
    assert(pc_modified == false);
    printf("✓ SUB R2, R1, R0 executed correctly\n");
    
    // Test CMP R1, R0 (0xE1510000) - should set flags
    state.registers[1] = 42;
    state.registers[0] = 42;
    state.cpsr = 0; // Clear all flags
    uint32_t cmp_instruction = 0xE1510000; // CMP R1, R0
    pc_modified = arm_execute_instruction(&state, cmp_instruction, &memory_interface);
    assert((state.cpsr & ARM_FLAG_Z) != 0); // Should set Z flag (equal)
    assert((state.cpsr & ARM_FLAG_C) != 0); // Should set C flag (no borrow)
    assert(pc_modified == false);
    printf("✓ CMP R1, R0 executed correctly and set flags\n");
    
    // Test MOV PC, #0x1000 (0xE3A0FA01) - should modify PC
    state.registers[15] = 0;
    uint32_t mov_pc_instruction = 0xE3A0FA01; // MOV PC, #0x1000
    pc_modified = arm_execute_instruction(&state, mov_pc_instruction, &memory_interface);
    assert(state.registers[15] == 0x1000);
    assert(pc_modified == true);
    printf("✓ MOV PC, #0x1000 executed correctly and modified PC\n");
    
    printf("ARM data processing tests passed!\n\n");
}

// Test multiply instructions
void test_arm_multiply() {
    printf("Testing ARM multiply instructions...\n");
    
    ArmCPUState state;
    setup_test_cpu(&state);
    
    // Create memory interface
    ArmMemoryInterface memory_interface = {
        .context = NULL,
        .read32 = mock_read32,
        .write32 = mock_write32,
        .read16 = mock_read16,
        .write16 = mock_write16,
        .read8 = mock_read8,
        .write8 = mock_write8
    };
    
    // Test MUL R0, R1, R2 (0xE0000291)
    state.registers[0] = 0; // Rd
    state.registers[1] = 5; // Rm
    state.registers[2] = 6; // Rs
    state.cpsr = 0; // Clear all flags
    uint32_t mul_instruction = 0xE0000291; // MUL R0, R1, R2
    bool pc_modified = arm_execute_instruction(&state, mul_instruction, &memory_interface);
    assert(state.registers[0] == 30); // 5 * 6 = 30
    assert(pc_modified == false);
    printf("✓ MUL R0, R1, R2 executed correctly\n");
    
    // Test MLA R3, R1, R2, R0 (manually construct)
    state.registers[0] = 10; // Rn (accumulator)
    state.registers[1] = 5;  // Rm
    state.registers[2] = 6;  // Rs
    state.registers[3] = 0;  // Rd
    // MLA format: cond(31-28) | 000000(27-22) | A(21) | S(20) | Rd(19-16) | Rn(15-12) | Rs(11-8) | 1001(7-4) | Rm(3-0)
    // E(1110) | 000000 | 1 | 0 | 0011(3) | 0000(0) | 0010(2) | 1001 | 0001(1)
    uint32_t mla_instruction = 0xE0000090 | (1 << 21) | (3 << 16) | (0 << 12) | (2 << 8) | 1; // MLA R3, R1, R2, R0
    pc_modified = arm_execute_instruction(&state, mla_instruction, &memory_interface);
    assert(state.registers[3] == 40); // 5 * 6 + 10 = 40
    assert(pc_modified == false);
    printf("✓ MLA R3, R1, R2, R0 executed correctly\n");
    
    // Test MULS R0, R1, R2 with flags (0xE0100291)
    state.registers[0] = 0; // Rd
    state.registers[1] = 0; // Rm (zero)
    state.registers[2] = 6; // Rs
    state.cpsr = 0; // Clear all flags
    uint32_t muls_instruction = 0xE0100291; // MULS R0, R1, R2
    pc_modified = arm_execute_instruction(&state, muls_instruction, &memory_interface);
    assert(state.registers[0] == 0); // 0 * 6 = 0
    assert((state.cpsr & FLAG_Z) != 0); // Should set Z flag
    assert(pc_modified == false);
    printf("✓ MULS R0, R1, R2 executed correctly and set Z flag\n");
    
    printf("ARM multiply tests passed!\n\n");
}

// Test block data transfer instructions
void test_arm_block_data_transfer() {
    printf("Testing ARM block data transfer instructions...\n");
    
    ArmCPUState state;
    setup_test_cpu(&state);
    
    // Create memory interface
    ArmMemoryInterface memory_interface = {
        .context = NULL,
        .read32 = mock_read32,
        .write32 = mock_write32,
        .read16 = mock_read16,
        .write16 = mock_write16,
        .read8 = mock_read8,
        .write8 = mock_write8
    };
    
    // Test STMIA R0!, {R1, R2, R3} (0xE8A0000E)
    state.registers[0] = 0x1000; // Base address
    state.registers[1] = 0x11111111;
    state.registers[2] = 0x22222222;
    state.registers[3] = 0x33333333;
    uint32_t stmia_instruction = 0xE8A0000E; // STMIA R0!, {R1, R2, R3}
    bool pc_modified = arm_execute_instruction(&state, stmia_instruction, &memory_interface);
    assert(state.registers[0] == 0x100C); // Base updated (3 registers * 4 bytes)
    assert(pc_modified == false);
    printf("✓ STMIA R0!, {R1, R2, R3} executed correctly\n");
    
    // Test LDMIA R0!, {R4, R5, R6} (0xE8B00070)
    state.registers[0] = 0x2000; // Base address
    state.registers[4] = 0;
    state.registers[5] = 0;
    state.registers[6] = 0;
    uint32_t ldmia_instruction = 0xE8B00070; // LDMIA R0!, {R4, R5, R6}
    pc_modified = arm_execute_instruction(&state, ldmia_instruction, &memory_interface);
    assert(state.registers[0] == 0x200C); // Base updated
    assert(state.registers[4] == 0xDEADBEEF); // Mock read value
    assert(state.registers[5] == 0xDEADBEEF); // Mock read value
    assert(state.registers[6] == 0xDEADBEEF); // Mock read value
    assert(pc_modified == false);
    printf("✓ LDMIA R0!, {R4, R5, R6} executed correctly\n");
    
    // Test LDMIA R0, {PC} (0xE8908000) - should modify PC
    state.registers[0] = 0x3000; // Base address
    state.registers[15] = 0x1000; // Current PC
    uint32_t ldmia_pc_instruction = 0xE8908000; // LDMIA R0, {PC}
    pc_modified = arm_execute_instruction(&state, ldmia_pc_instruction, &memory_interface);
    assert(state.registers[15] == 0xDEADBEEF); // PC loaded from memory
    assert(pc_modified == true);
    printf("✓ LDMIA R0, {PC} executed correctly and modified PC\n");
    
    printf("ARM block data transfer tests passed!\n\n");
}

// Test branch instructions
void test_arm_branch() {
    printf("Testing ARM branch instructions...\n");
    
    ArmCPUState state;
    setup_test_cpu(&state);
    
    // Create memory interface
    ArmMemoryInterface memory_interface = {
        .context = NULL,
        .read32 = mock_read32,
        .write32 = mock_write32,
        .read16 = mock_read16,
        .write16 = mock_write16,
        .read8 = mock_read8,
        .write8 = mock_write8
    };
    
    // Test B #0x100 (0xEA000040)
    state.registers[15] = 0x1000; // Current PC
    uint32_t b_instruction = 0xEA000040; // B #0x100
    bool pc_modified = arm_execute_instruction(&state, b_instruction, &memory_interface);
    assert(state.registers[15] == 0x1000 + 0x100 + 8); // PC + offset + 8
    assert(pc_modified == true);
    printf("✓ B #0x100 executed correctly\n");
    
    // Test BL #0x200 (0xEB000080)
    state.registers[15] = 0x2000; // Current PC
    state.registers[14] = 0; // Clear LR
    uint32_t bl_instruction = 0xEB000080; // BL #0x200
    pc_modified = arm_execute_instruction(&state, bl_instruction, &memory_interface);
    assert(state.registers[15] == 0x2000 + 0x200 + 8); // PC + offset + 8
    assert(state.registers[14] == 0x2000 + 4); // LR = PC + 4
    assert(pc_modified == true);
    printf("✓ BL #0x200 executed correctly and set LR\n");
    
    // Test negative branch B #-0x80 words = -0x200 bytes (0xEAFFFF80)
    state.registers[15] = 0x3000; // Current PC
    uint32_t b_neg_instruction = 0xEAFFFF80; // B #-0x80 words
    pc_modified = arm_execute_instruction(&state, b_neg_instruction, &memory_interface);
    assert(state.registers[15] == 0x3000 - 0x200 + 8); // PC - offset + 8 (offset in bytes)
    assert(pc_modified == true);
    printf("✓ B #-0x80 executed correctly\n");
    
    printf("ARM branch tests passed!\n\n");
}

// Test software interrupt instructions
void test_arm_software_interrupt() {
    printf("Testing ARM software interrupt instructions...\n");
    
    ArmCPUState state;
    setup_test_cpu(&state);
    
    // Create memory interface
    ArmMemoryInterface memory_interface = {
        .context = NULL,
        .read32 = mock_read32,
        .write32 = mock_write32,
        .read16 = mock_read16,
        .write16 = mock_write16,
        .read8 = mock_read8,
        .write8 = mock_write8
    };
    
    // Test SWI #0x123456 (0xEF123456)
    state.registers[15] = 0x1000; // Current PC
    state.registers[14] = 0; // Clear LR
    state.cpsr = 0x10; // User mode
    uint32_t swi_instruction = 0xEF123456; // SWI #0x123456
    bool pc_modified = arm_execute_instruction(&state, swi_instruction, &memory_interface);
    assert(state.registers[15] == 0x08); // PC set to SWI vector
    assert(state.registers[14] == 0x1000 + 4); // LR = PC + 4
    assert((state.cpsr & 0x1F) == 0x13); // SVC mode
    assert((state.cpsr & 0x80) != 0); // IRQ disabled
    assert(pc_modified == true);
    printf("✓ SWI #0x123456 executed correctly\n");
    
    printf("ARM software interrupt tests passed!\n\n");
}

int main() {
    printf("=== ARM Execute Phase 1 Tests ===\n\n");
    
    test_arm_condition_checking();
    test_arm_execute_basic();
    test_arm_data_processing();
    test_arm_multiply();
    test_arm_block_data_transfer();
    test_arm_branch();
    test_arm_software_interrupt();
    
    printf("All tests passed!\n");
    return 0;
}
