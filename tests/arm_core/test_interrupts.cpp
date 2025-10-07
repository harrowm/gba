#include <gtest/gtest.h>
#include "cpu.h"
#include "memory.h"
#include "interrupt.h"
#include "timer_controller.h"
#include "scheduler.h"
#include "gba.h"

class InterruptTest : public ::testing::Test {
protected:
    void SetUp() override {
        gba = new GBA(true);  // Test mode
    }

    void TearDown() override {
        delete gba;
    }

    GBA* gba;
};

// Test 1: Basic interrupt handling
TEST_F(InterruptTest, CPUHandlesVBlankInterrupt) {
    CPU& cpu = gba->getCPU();
    Memory& memory = gba->getMemory();
    
    // Enable interrupts globally
    memory.write16(REG_IME, 1);  // IME = 1 (interrupts enabled)
    memory.write16(REG_IE, IRQ_VBLANK);  // Enable V-Blank interrupt
    
    // Set up CPU in System mode
    cpu.setMode(CPU::SYS);
    cpu.R()[15] = 0x08000000;  // PC at ROM
    cpu.CPSR() = 0x0000001F;  // System mode, interrupts enabled (I flag clear)
    
    // Trigger V-Blank interrupt
    memory.write16(REG_IF, IRQ_VBLANK);  // Request V-Blank interrupt
    
    // Check for pending interrupt
    EXPECT_TRUE(cpu.checkPendingInterrupts());
    
    // Handle the interrupt
    cpu.handleInterrupt();
    
    // Verify CPU switched to IRQ mode
    EXPECT_EQ(cpu.CPSR() & 0x1F, CPU::IRQ);
    
    // Verify PC jumped to IRQ vector
    EXPECT_EQ(cpu.R()[15], 0x00000018);
    
    // Verify I flag is set (interrupts disabled)
    EXPECT_NE(cpu.CPSR() & 0x80, 0);
    
    // Verify return address was saved to LR_irq
    EXPECT_EQ(cpu.LR(), 0x08000000);  // Should be old PC value
}

// Test 2: Timer initialization
TEST_F(InterruptTest, TimerInitialization) {
    Memory& memory = gba->getMemory();
    
    // Write to timer 0 reload register
    memory.write16(TM0CNT_L, 0x1000);
    
    // Read back
    uint16_t reload = memory.read16(TM0CNT_L);
    EXPECT_EQ(reload, 0x1000);
    
    // Write to timer 0 control
    memory.write16(TM0CNT_H, TIMER_ENABLE);
    
    // Read back
    uint16_t control = memory.read16(TM0CNT_H);
    EXPECT_EQ(control, TIMER_ENABLE);
}

// Test 3: Timer overflow and interrupt
TEST_F(InterruptTest, TimerOverflowInterrupt) {
    Memory& memory = gba->getMemory();
    Scheduler& scheduler = gba->getScheduler();
    
    // Enable interrupts
    memory.write16(REG_IME, 1);
    memory.write16(REG_IE, IRQ_TIMER0);
    
    // Set timer 0 to overflow quickly
    // Start from 0xFFFE, will overflow after 2 ticks
    memory.write16(TM0CNT_L, 0xFFFE);
    
    // Enable timer with prescaler /1 and IRQ enabled
    memory.write16(TM0CNT_H, TIMER_ENABLE | TIMER_IRQ_ENABLE);
    
    // Run scheduler for enough cycles to cause overflow
    // 2 ticks * 1 cycle per tick = 2 cycles
    scheduler.runUntil(scheduler.getCurrentCycle() + 10);
    
    // Check if interrupt was triggered
    uint16_t ifReg = memory.read16(REG_IF);
    EXPECT_NE(ifReg & IRQ_TIMER0, 0);
}

// Test 4: SPSR save/restore
TEST_F(InterruptTest, SPSRSaveRestore) {
    CPU& cpu = gba->getCPU();
    Memory& memory = gba->getMemory();
    
    // Enable interrupts
    memory.write16(REG_IME, 1);
    memory.write16(REG_IE, IRQ_VBLANK);
    
    // Set up initial CPU state
    cpu.setMode(CPU::SYS);
    cpu.R()[15] = 0x08000000;
    cpu.CPSR() = 0x60000010;  // System mode with some flags set
    
    uint32_t original_cpsr = cpu.CPSR();
    
    // Trigger interrupt
    memory.write16(REG_IF, IRQ_VBLANK);
    cpu.handleInterrupt();
    
    // Check we're in IRQ mode
    EXPECT_EQ(cpu.CPSR() & 0x1F, CPU::IRQ);
    
    // Check SPSR_irq contains original CPSR
    EXPECT_EQ(cpu.SPSR(), original_cpsr);
}

// Test 5: Cascaded timers
TEST_F(InterruptTest, CascadedTimers) {
    Memory& memory = gba->getMemory();
    Scheduler& scheduler = gba->getScheduler();
    
    // Set timer 0 to overflow after 2 ticks
    memory.write16(TM0CNT_L, 0xFFFE);
    memory.write16(TM0CNT_H, TIMER_ENABLE);  // No IRQ, just counting
    
    // Set timer 1 in cascade mode (counts timer 0 overflows)
    memory.write16(TM1CNT_L, 0xFFFE);  // Start from 0xFFFE
    memory.write16(TM1CNT_H, TIMER_ENABLE | TIMER_COUNT_UP | TIMER_IRQ_ENABLE);
    
    // Enable Timer 1 interrupt
    memory.write16(REG_IME, 1);
    memory.write16(REG_IE, IRQ_TIMER1);
    
    // Run for multiple timer 0 overflows
    // Timer 0 overflows every 2 cycles
    // Timer 1 needs 2 overflows of timer 0 to overflow
    // So we need at least 4 cycles
    scheduler.runUntil(scheduler.getCurrentCycle() + 10);
    
    // Check if Timer 1 interrupt triggered
    uint16_t ifReg = memory.read16(REG_IF);
    EXPECT_NE(ifReg & IRQ_TIMER1, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
