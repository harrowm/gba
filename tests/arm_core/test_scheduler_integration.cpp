// Integration tests to verify that CPU execution advances the scheduler correctly

#include <gtest/gtest.h>
#include "cpu.h"
#include "scheduler.h"
#include "memory.h"
#include "interrupt.h"
#include "arm_cpu.h"
#include "thumb_cpu.h"

class CPUSchedulerIntegrationTest : public ::testing::Test {
protected:
    CPU* cpu;
    Scheduler* scheduler;
    Memory* memory;
    InterruptController* interrupt;
    
    void SetUp() override {
        scheduler = new Scheduler();
        memory = new Memory();
        interrupt = new InterruptController();
        cpu = new CPU(*memory, *interrupt);
        
        // Wire everything together
        memory->setScheduler(scheduler);
        cpu->setScheduler(scheduler);
        
        // Start in ARM mode (clear Thumb flag)
        cpu->clearFlag(CPU::FLAG_T);
        cpu->R()[15] = 0x08000000;  // ROM region
    }
    
    void TearDown() override {
        delete cpu;
        delete interrupt;
        delete memory;
        delete scheduler;
    }
};

TEST_F(CPUSchedulerIntegrationTest, SingleInstructionAdvancesCycles) {
    // Set up a simple MOV instruction: MOV R0, #42 (0xE3A0002A)
    memory->write32(0x08000000, 0xE3A0002A);
    
    uint64_t cycles_before = scheduler->getCurrentCycle();
    
    // Execute one ARM instruction
    cpu->getARMCPU().executeOneInstruction();
    
    uint64_t cycles_after = scheduler->getCurrentCycle();
    
    // MOV immediate should take 1S cycle (sequential)
    // In ROM (0x0800xxxx) with 16-bit bus, that's 1 wait state
    // Instruction fetch: 2 cycles (32-bit @ 16-bit bus = 2 accesses)
    // Execution: 1 cycle
    // Total should be > 0
    EXPECT_GT(cycles_after, cycles_before);
    EXPECT_EQ(cpu->R()[0], 42u);  // Verify instruction executed
}

TEST_F(CPUSchedulerIntegrationTest, MultipleInstructionsAccumulateCycles) {
    // Set up multiple MOV instructions
    memory->write32(0x08000000, 0xE3A00001);  // MOV R0, #1
    memory->write32(0x08000004, 0xE3A01002);  // MOV R1, #2
    memory->write32(0x08000008, 0xE3A02003);  // MOV R2, #3
    
    uint64_t cycles_before = scheduler->getCurrentCycle();
    
    // Execute three instructions
    cpu->getARMCPU().executeOneInstruction();
    uint64_t cycles_after_first = scheduler->getCurrentCycle();
    
    cpu->getARMCPU().executeOneInstruction();
    uint64_t cycles_after_second = scheduler->getCurrentCycle();
    
    cpu->getARMCPU().executeOneInstruction();
    uint64_t cycles_after_third = scheduler->getCurrentCycle();
    
    // Each instruction should advance cycles
    EXPECT_GT(cycles_after_first, cycles_before);
    EXPECT_GT(cycles_after_second, cycles_after_first);
    EXPECT_GT(cycles_after_third, cycles_after_second);
    
    // Verify instructions executed correctly
    EXPECT_EQ(cpu->R()[0], 1u);
    EXPECT_EQ(cpu->R()[1], 2u);
    EXPECT_EQ(cpu->R()[2], 3u);
}

TEST_F(CPUSchedulerIntegrationTest, MemoryAccessesAddWaitStates) {
    // LDR R0, [R1]  - Load from memory
    // Set R1 to point to ROM (slow) vs IWRAM (fast)
    
    // Test 1: Load from IWRAM (fast, 0 wait states)
    cpu->R()[1] = 0x03000000;  // IWRAM base
    memory->write32(0x03000000, 0x12345678);
    memory->write32(0x08000000, 0xE5910000);  // LDR R0, [R1]
    cpu->R()[15] = 0x08000000;
    
    uint64_t cycles_before_iwram = scheduler->getCurrentCycle();
    cpu->getARMCPU().executeOneInstruction();
    uint64_t cycles_after_iwram = scheduler->getCurrentCycle();
    uint64_t iwram_cycles = cycles_after_iwram - cycles_before_iwram;
    
    EXPECT_EQ(cpu->R()[0], 0x12345678);
    
    // Test 2: Load from ROM (slow, wait states)
    cpu->R()[1] = 0x08000100;  // ROM
    cpu->R()[0] = 0;  // Reset R0
    memory->write32(0x08000100, 0x87654321);
    memory->write32(0x08000010, 0xE5910000);  // LDR R0, [R1]
    cpu->R()[15] = 0x08000010;
    
    uint64_t cycles_before_rom = scheduler->getCurrentCycle();
    cpu->getARMCPU().executeOneInstruction();
    uint64_t cycles_after_rom = scheduler->getCurrentCycle();
    uint64_t rom_cycles = cycles_after_rom - cycles_before_rom;
    
    EXPECT_EQ(cpu->R()[0], 0x87654321);
    
    // ROM access should take more cycles than IWRAM due to wait states
    EXPECT_GT(rom_cycles, iwram_cycles);
}

TEST_F(CPUSchedulerIntegrationTest, MultiplyInstructionTakesMultipleCycles) {
    // MUL takes variable cycles depending on operand value
    // MUL R0, R1, R2 (0xE0000291)
    
    cpu->R()[1] = 0x00000002;  // Small value
    cpu->R()[2] = 0x00000003;
    memory->write32(0x08000000, 0xE0000291);
    cpu->R()[15] = 0x08000000;
    
    uint64_t cycles_before = scheduler->getCurrentCycle();
    cpu->getARMCPU().executeOneInstruction();
    uint64_t cycles_after = scheduler->getCurrentCycle();
    
    // Multiply should take more than 1 cycle
    EXPECT_GT(cycles_after - cycles_before, 1ull);
    EXPECT_EQ(cpu->R()[0], 6u);
}

TEST_F(CPUSchedulerIntegrationTest, ThumbInstructionExecution) {
    // Switch to Thumb mode
    cpu->setFlag(CPU::FLAG_T);
    cpu->R()[15] = 0x08000000;
    
    // MOVS R0, #42 (0x202A in Thumb)
    memory->write16(0x08000000, 0x202A);
    
    uint64_t cycles_before = scheduler->getCurrentCycle();
    cpu->getThumbCPU().executeOneInstruction();
    uint64_t cycles_after = scheduler->getCurrentCycle();
    
    EXPECT_GT(cycles_after, cycles_before);
    EXPECT_EQ(cpu->R()[0], 42u);
}

TEST_F(CPUSchedulerIntegrationTest, PCAdvancesCorrectly) {
    // Verify that PC advances in addition to cycles
    memory->write32(0x08000000, 0xE3A00001);  // MOV R0, #1
    cpu->R()[15] = 0x08000000;
    
    cpu->getARMCPU().executeOneInstruction();
    
    // In ARM mode, PC should advance by 4
    EXPECT_EQ(cpu->R()[15], 0x08000004);
}
