// test_arm_other.cpp
#include <gtest/gtest.h>
#include "memory.h"
#include "interrupt.h"
#include "cpu.h"
#include "arm_cpu.h"

extern "C" {
#include <keystone/keystone.h>
}

class ARMOtherTest : public ::testing::Test {
protected:
    Memory memory;
    InterruptController interrupts;
    CPU cpu;
    ARMCPU arm_cpu;
    ks_engine* ks; // Keystone handle

    ARMOtherTest() : memory(true), cpu(memory, interrupts), arm_cpu(cpu) {}

    void SetUp() override {
        for (int i = 0; i < 16; ++i) cpu.R()[i] = 0;
        cpu.CPSR() = 0x10; // User mode, no flags set
        
        if (ks) ks_close(ks);
        if (ks_open(KS_ARCH_ARM, KS_MODE_ARM, &ks) != KS_ERR_OK) {
            FAIL() << "Failed to initialize Keystone for ARM mode";
        }
    }

    void TearDown() override {
        if (ks) {
            ks_close(ks);
            ks = nullptr;
        }
    }

    // Helper: assemble ARM instruction and write to memory
    bool assemble_and_write(const std::string& asm_code, uint32_t addr, std::vector<uint8_t>* out_bytes = nullptr) {
        unsigned char* encode = nullptr;
        size_t size, count;
        int err = ks_asm(ks, asm_code.c_str(), addr, &encode, &size, &count);
        if ((ks_err)err != KS_ERR_OK) {
            fprintf(stderr, "Keystone error: %s\n", ks_strerror((ks_err)err));
            return false;
        }
        for (size_t i = 0; i < size; ++i)
            memory.write8(addr + i, encode[i]);
        if (out_bytes) out_bytes->assign(encode, encode + size);
        ks_free(encode);
        return true;
    }
};

// LDM/STM basic test
TEST_F(ARMOtherTest, LDM_STM_Basic) {
    // Store values in registers
    cpu.R()[0] = 0x11111111;
    cpu.R()[1] = 0x22222222;
    cpu.R()[2] = 0x33333333;
    cpu.R()[3] = 0x44444444;
    cpu.R()[4] = 0x100; // Base address
    // STMIA R4!, {R0-R3}
    cpu.R()[15] = 0x00000000;
    assemble_and_write("stmia r4!, {r0-r3}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x100), 0x11111111u);
    EXPECT_EQ(memory.read32(0x104), 0x22222222u);
    EXPECT_EQ(memory.read32(0x108), 0x33333333u);
    EXPECT_EQ(memory.read32(0x10C), 0x44444444u);
    EXPECT_EQ(cpu.R()[4], 0x110u); // Writeback
    // LDMIA R4!, {R0-R3}
    cpu.R()[4] = 0x100; // Reset base address
    cpu.R()[0] = cpu.R()[1] = cpu.R()[2] = cpu.R()[3] = 0;
    cpu.R()[15] = 0x00000004;
    assemble_and_write("ldmia r4!, {r0-r3}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x11111111u);
    EXPECT_EQ(cpu.R()[1], 0x22222222u);
    EXPECT_EQ(cpu.R()[2], 0x33333333u);
    EXPECT_EQ(cpu.R()[3], 0x44444444u);
    EXPECT_EQ(cpu.R()[4], 0x110u); // Writeback
}

// Branch (B, BL) test
TEST_F(ARMOtherTest, Branch_B_BL) {
    cpu.R()[15] = 0x00000000;
    // B +8 (offset = 2)
    assemble_and_write("b #0x10", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000010);
    // BL +12 (offset = 3)
    cpu.R()[15] = 0x00000010;
    assemble_and_write("bl #0x24", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], (uint32_t)0x00000024);
    EXPECT_EQ(cpu.R()[14], (uint32_t)0x00000014);
}

// SWP/SWPB test
TEST_F(ARMOtherTest, SWP_SWPB) {
    cpu.R()[1] = 0x200; // Address
    cpu.R()[2] = 0xDEADBEEF; // Value to store
    memory.write32(0x200, 0xCAFEBABE);
    // SWP R0, R2, [R1]
    cpu.R()[15] = 0x00000000;
    assemble_and_write("swp r0, r2, [r1]", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xCAFEBABEu);
    EXPECT_EQ(memory.read32(0x200), 0xDEADBEEFu);
    // SWPB R3, R2, [R1]
    memory.write8(0x200, 0xAA);
    cpu.R()[2] = 0xBB;
    cpu.R()[15] = 0x00000004;
    assemble_and_write("swpb r3, r2, [r1]", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFF, 0xAAu);
    EXPECT_EQ(memory.read8(0x200), 0xBBu);
}

// Undefined and SWI test
TEST_F(ARMOtherTest, UndefinedAndSWI) {
    cpu.R()[15] = 0x00000000;
    // Undefined instruction (should branch to 0x04 and set mode)
    assemble_and_write("mov r0, pc", cpu.R()[15]); // This should trigger undefined behavior
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x04u);
    EXPECT_EQ(cpu.CPSR() & 0x1F, 0x1Bu); // Mode = Undefined
    // SWI (should branch to 0x08 and set mode)
    cpu.R()[15] = 0x00000010;
    assemble_and_write("swi #0x11", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x08u);
    EXPECT_EQ(cpu.CPSR() & 0x1F, 0x13u); // Mode = SVC
}

// Extra LDM/STM edge case tests
TEST_F(ARMOtherTest, LDM_STM_EmptyRegisterList) {
    cpu.R()[4] = 0x200;
    // STMIA R4!, {} (empty list)
    cpu.R()[15] = 0x00000020;
    // NOTE: Keystone cannot assemble empty register lists ({}) syntax
    // Using hardcoded instruction bytes
    memory.write32(cpu.R()[15], 0xE8A40000); // STMIA R4!, {}
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0x200u); // No writeback
    // LDMIA R4!, {} (empty list)
    cpu.R()[15] = 0x00000024;
    // NOTE: Keystone cannot assemble empty register lists ({}) syntax
    // Using hardcoded instruction bytes
    memory.write32(cpu.R()[15], 0xE8B40000); // LDMIA R4!, {}
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0x200u); // No writeback
}

TEST_F(ARMOtherTest, LDM_STM_BaseInList) {
    cpu.R()[0] = 0x11111111;
    cpu.R()[1] = 0x22222222;
    cpu.R()[2] = 0x33333333;
    cpu.R()[3] = 0x44444444;
    cpu.R()[4] = 0x100; // Base address
    // STMIA R4!, {R0-R4} (base in list)
    cpu.R()[15] = 0x00000028;
    assemble_and_write("stmia r4!, {r0-r4}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x100), 0x11111111u);
    EXPECT_EQ(memory.read32(0x104), 0x22222222u);
    EXPECT_EQ(memory.read32(0x108), 0x33333333u);
    EXPECT_EQ(memory.read32(0x10C), 0x44444444u);
    // R4 value written is the original value (0x100)
    EXPECT_EQ(memory.read32(0x110), 0x100u);
    // Writeback: ARM writes old base + 4*(n_regs), but if base in list, result is unpredictable (implementation defined)
    // We'll just check that R4 is not 0x100 anymore
    EXPECT_NE(cpu.R()[4], 0x100u);
}

TEST_F(ARMOtherTest, LDM_STM_PCInList) {
    cpu.R()[0] = 0x11111111;
    cpu.R()[1] = 0x22222222;
    cpu.R()[2] = 0x33333333;
    cpu.R()[3] = 0x44444444;
    cpu.R()[15] = 0x100; // PC
    cpu.R()[4] = 0x200; // Base
    // STMIA R4!, {R0,R3,PC}
    cpu.R()[15] = 0x0000002C;
    assemble_and_write("stmia r4!, {r0,r3,pc}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x200), 0x11111111u);
    EXPECT_EQ(memory.read32(0x204), 0x44444444u);
    EXPECT_EQ(memory.read32(0x208), 0x34u); // PC value
    // LDMIA R4!, {R0,R3,PC}
    cpu.R()[0] = cpu.R()[3] = 0;
    cpu.R()[15] = 0;
    cpu.R()[4] = 0x200;
    cpu.R()[15] = 0x00000030;
    assemble_and_write("ldmia r4!, {r0,r3,pc}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x11111111u);
    EXPECT_EQ(cpu.R()[3], 0x44444444u);
    EXPECT_EQ(cpu.R()[15], 0x34u);
}

TEST_F(ARMOtherTest, LDM_STM_AlternateAddressingModes) {
    // IB: Increment before
    cpu.R()[0] = 0xAAAA5555;
    cpu.R()[4] = 0x300;
    cpu.R()[15] = 0x00000034;
    assemble_and_write("stmib r4!, {r0}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x304), 0xAAAA5555u);
    EXPECT_EQ(cpu.R()[4], 0x304u);
    // DA: Decrement after
    cpu.R()[0] = 0x12345678;
    cpu.R()[4] = 0x400;
    cpu.R()[15] = 0x00000038;
    assemble_and_write("stmda r4!, {r0}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x400), 0x12345678u);
    EXPECT_EQ(cpu.R()[4], 0x3FCu);
    // DB: Decrement before
    cpu.R()[0] = 0xCAFEBABE;
    cpu.R()[4] = 0x500;
    cpu.R()[15] = 0x0000003C;
    assemble_and_write("stmdb r4!, {r0}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x4FC), 0xCAFEBABEu);
    EXPECT_EQ(cpu.R()[4], 0x4FCu);
}

// Extra branch edge case tests
TEST_F(ARMOtherTest, Branch_NegativeOffset) {
    // Place a branch at 0x100 that jumps back to 0x0F8
    cpu.R()[15] = 0x100;
    assemble_and_write("b #0xF8", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0xF8u);
}

TEST_F(ARMOtherTest, Branch_ConditionCodes) {
    // BNE (should not branch if Z=1)
    cpu.R()[15] = 0x200;
    cpu.CPSR() |= (1 << 30); // Set Z flag
    assemble_and_write("bne #0x210", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x200u + 4); // No branch, PC advances by 4
    // BEQ (should branch if Z=1)
    cpu.R()[15] = 0x210;
    assemble_and_write("beq #0x220", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x220u); // Branch to 0x220
}

TEST_F(ARMOtherTest, Branch_UnalignedTarget) {
    // B to unaligned address (should align to 4)
    cpu.R()[15] = 0x300;
    assemble_and_write("b #0x30C", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0x30Cu); // Should be aligned
}

// Extra SWP/SWPB edge case tests
TEST_F(ARMOtherTest, SWP_UnalignedAddress) {
    // SWP with unaligned address (should align to 4)
    cpu.R()[1] = 0x203; // Unaligned address
    cpu.R()[2] = 0xAABBCCDD;
    memory.write32(0x200, 0x11223344);
    cpu.R()[15] = 0x400;
    assemble_and_write("swp r0, r2, [r1]", cpu.R()[15]);
    arm_cpu.execute(1);
    DEBUG_LOG("r0=0x" + DEBUG_TO_HEX_STRING(cpu.R()[0], 8));
    EXPECT_EQ(cpu.R()[0], 0x11223344u);
    EXPECT_EQ(memory.read32(0x200), 0xAABBCCDDu); 
}

TEST_F(ARMOtherTest, SWPB_UnalignedAddress) {
    // SWPB with unaligned address (should work, byte access)
    cpu.R()[1] = 0x205; // Unaligned address
    cpu.R()[2] = 0x77;
    memory.write8(0x205, 0x99);
    cpu.R()[15] = 0x404;
    assemble_and_write("swpb r3, r2, [r1]", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFF, 0x99u);
    EXPECT_EQ(memory.read8(0x205), 0x77u);
}

TEST_F(ARMOtherTest, SWP_SameRegister) {
    // SWP with same register for dest and src
    cpu.R()[0] = 0x12345678;
    cpu.R()[1] = 0x208;
    memory.write32(0x208, 0xCAFEBABE);
    cpu.R()[15] = 0x408;
    assemble_and_write("swp r0, r0, [r1]", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0xCAFEBABEu);
    EXPECT_EQ(memory.read32(0x208), 0x12345678u);
}

TEST_F(ARMOtherTest, SWPB_SameRegister) {
    // SWPB with same register for dest and src
    cpu.R()[3] = 0x55;
    cpu.R()[1] = 0x209;
    memory.write8(0x209, 0xAA);
    cpu.R()[15] = 0x40C;
    assemble_and_write("swpb r3, r3, [r1]", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[3] & 0xFF, 0xAAu);
    EXPECT_EQ(memory.read8(0x209), 0x55u);
}

// Extra LDM/STM advanced edge case tests
TEST_F(ARMOtherTest, LDM_STM_SBitUserSystem) {
    // LDM with S bit set (should transfer user/system registers if supported)
    // This is a stub: actual effect depends on CPU model, but should not crash
    cpu.R()[0] = 0x11111111;
    cpu.R()[4] = 0x600;
    // Initialize memory for LDM
    memory.write32(0x600, 0x11111111);
    memory.write32(0x604, 0xCAFEBABE);
    cpu.R()[15] = 0x500;
    // NOTE: Keystone cannot assemble S-bit (^) syntax for LDM/STM instructions
    // Using hardcoded instruction bytes
    memory.write32(cpu.R()[15], 0xE8B40011); // LDMIA R4!, {R0,R4}^ (S bit set)
    arm_cpu.execute(1);
    // Just check that R0 and R4 are loaded, and no crash
    EXPECT_EQ(cpu.R()[0], 0x11111111u);
    EXPECT_EQ(cpu.R()[4], 0xCAFEBABEu);
}

TEST_F(ARMOtherTest, LDM_BaseInListWriteback) {
    // LDM with base in list and writeback: base should be loaded last
    cpu.R()[0] = 0x12345678;
    cpu.R()[4] = 0x700;
    memory.write32(0x700, 0xDEADBEEF);
    memory.write32(0x704, 0xCAFEBABE);
    cpu.R()[15] = 0x504;
    assemble_and_write("ldmia r4!, {r0,r4}", cpu.R()[15]);
    arm_cpu.execute(1);
    // R0 loaded from 0x700, R4 loaded from 0x704, writeback uses loaded R4
    EXPECT_EQ(cpu.R()[0], 0xDEADBEEFu);
    EXPECT_EQ(cpu.R()[4], 0xCAFEBABEu);
}

TEST_F(ARMOtherTest, LDM_PCInList_SBit) {
    // LDM with PC in list and S bit set (should restore SPSR if available)
    cpu.R()[0] = 0x11111111;
    cpu.R()[15] = 0x88888888;
    cpu.R()[4] = 0x800;
    memory.write32(0x800, 0x11111111);
    memory.write32(0x804, 0x12345678);
    cpu.R()[15] = 0x508;
    // NOTE: Keystone cannot assemble S-bit (^) syntax for LDM/STM instructions
    // Using hardcoded instruction bytes
    memory.write32(cpu.R()[15], 0xE8F48001); // LDMIA R4!, {R0,PC}^ (S bit set)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[0], 0x11111111u);
    EXPECT_EQ(cpu.R()[15], 0x12345678u);
    EXPECT_EQ(cpu.R()[4], 0x808u); // Writeback: R4 updated to point after last loaded register
    // SPSR should be restored if available (not tested here, not implemented)
}

TEST_F(ARMOtherTest, LDM_STM_OverlappingRegisters) {
    // Overlapping registers in list (e.g., R2 and R2)
    cpu.R()[2] = 0xA5A5A5A5;
    cpu.R()[4] = 0x900;
    memory.write32(0x900, 0xDEADBEEF);
    cpu.R()[15] = 0x510;
    assemble_and_write("ldmia r4!, {r2}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[2], 0xDEADBEEFu);
    // STM with overlapping base and data register
    cpu.R()[4] = 0x904;
    cpu.R()[2] = 0xCAFEBABE;
    cpu.R()[15] = 0x514;
    assemble_and_write("stmia r4!, {r2,r4}", cpu.R()[15]);
    arm_cpu.execute(1);
    EXPECT_EQ(memory.read32(0x904), 0xCAFEBABEu);
    // R4 value written is the original base
    EXPECT_EQ(memory.read32(0x908), 0x904u);
}

// Extra B/BL advanced edge case tests
TEST_F(ARMOtherTest, Branch_AllConditionCodes) {
    // Test all ARM condition codes for B (only a few shown for brevity)
    struct { uint32_t instr; uint32_t cpsr; bool should_branch; } cases[] = {
        {0x0A000001, 1 << 30, true},   // BEQ, Z=1 (should branch)
        {0x0A000001, 0, false},        // BEQ, Z=0 (should NOT branch)
        {0x1A000001, 0, true},         // BNE, Z=0 (should branch)
        {0x2A000001, 1 << 29, true},   // BCS, C=1
        {0x3A000001, 0, true},         // BCC, C=0
        {0xAA000001, 0, true},         // BAL (always)
    };
    for (int i = 0; i < 6; ++i) {
        uint32_t orig_pc = 0xA00 + i * 0x10;
        cpu.R()[15] = orig_pc;
        cpu.CPSR() = 0x10 | cases[i].cpsr;
        memory.write32(cpu.R()[15], cases[i].instr);
        arm_cpu.execute(1);
        if (cases[i].should_branch) {
            int32_t offset = (cases[i].instr & 0x00FFFFFF);
            if (offset & 0x00800000) offset |= 0xFF000000; // sign extend
            uint32_t expected = orig_pc + 8 + (offset << 2);
            DEBUG_INFO("Branch i: " + std::to_string(i));
            EXPECT_EQ(cpu.R()[15], expected);
        } else {
            DEBUG_INFO("No branch i: " + std::to_string(i));
            EXPECT_EQ(cpu.R()[15], orig_pc + 4);
        }
    }
}

TEST_F(ARMOtherTest, Branch_ToSelf) {
    // B to current PC (infinite loop)
    cpu.R()[15] = 0xB00;
    uint32_t b_self = 0xEAFFFFFE; // B 0 (offset = -2)
    memory.write32(cpu.R()[15], b_self);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0xB00u);
}

TEST_F(ARMOtherTest, Branch_MaxOffset) {
    // B with largest positive and negative offsets
    cpu.R()[15] = 0xC00;
    uint32_t b_max_pos = 0xEA7FFFFF; // B +0x1FFFFFC
    memory.write32(cpu.R()[15], b_max_pos);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0xC00u + 8 + 0x1FFFFFC);
    cpu.R()[15] = 0xC10;
    uint32_t b_max_neg = 0xEA800000; // B -0x2000000
    memory.write32(cpu.R()[15], b_max_neg);
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[15], 0xC10u + 8 - 0x2000000);
}

// BLX is not supported in ARMv4T, but if your CPU supports it, add a test here

// Extra SWP/SWPB advanced edge case tests
TEST_F(ARMOtherTest, SWP_PCRegister) {
    // SWP with PC as destination or source (should be unpredictable, but test for no crash)
    cpu.R()[1] = 0xD00;
    cpu.R()[2] = 0x12345678;
    memory.write32(0xD00, 0xCAFEBABE);
    uint32_t swp_pc_dest = 0xE10F0092; // SWP PC, R2, [R1]
    cpu.R()[15] = 0xD00;
    memory.write32(cpu.R()[15], swp_pc_dest);
    arm_cpu.execute(1);
    // Can't check PC value, just ensure no crash
    uint32_t swp_pc_src = 0xE1010F92; // SWP R0, PC, [R1]
    cpu.R()[15] = 0xD04;
    memory.write32(cpu.R()[15], swp_pc_src);
    arm_cpu.execute(1);
    // Can't check result, just ensure no crash
}

TEST_F(ARMOtherTest, SWP_MemoryFault) {
    // SWP to invalid memory (simulate fault, if memory model supports it)
    cpu.R()[1] = 0xFFFFFFF0; // Likely invalid
    cpu.R()[2] = 0xDEADBEEF;
    // No actual memory at this address, but should not crash emulator
    cpu.R()[15] = 0xE00;
    assemble_and_write("swp r0, r2, [r1]", cpu.R()[15]);
    // If your memory model throws, catch and ignore
    try { arm_cpu.execute(1); } catch (...) {}
}

TEST_F(ARMOtherTest, LDM_STM_EmptyList_SBit) {
    // STMIA R4!, {}^ (S bit set)
    cpu.R()[4] = 0x1000;
    // NOTE: Keystone cannot assemble empty register lists ({}) or S-bit (^) syntax
    // Using hardcoded instruction bytes constructed with bitwise operations
    cpu.R()[15] = 0xF00;
    memory.write32(cpu.R()[15], 0xE8A40000 | (1 << 22)); // STMIA R4!, {}^ (S bit)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0x1000u); // No writeback
    // LDMIA R4!, {}^ (S bit set)
    cpu.R()[15] = 0xF04;
    memory.write32(cpu.R()[15], 0xE8B40000 | (1 << 22)); // LDMIA R4!, {}^ (S bit)
    arm_cpu.execute(1);
    EXPECT_EQ(cpu.R()[4], 0x1000u); // No writeback
}

