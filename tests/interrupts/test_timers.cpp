#include <gtest/gtest.h>
#include "../../include/timer_controller.h"
#include "../../include/scheduler.h"
#include "../../include/interrupt.h"
#include "../../include/memory.h"

class TimerTest : public ::testing::Test {
protected:
    TimerController* timerController;
    Scheduler* scheduler;
    InterruptController* interruptController;
    Memory* memory;

    void SetUp() override {
        scheduler = new Scheduler();
        memory = new Memory(false);  // Normal mode (needed for I/O registers)
        interruptController = new InterruptController();
        interruptController->setMemory(memory);
        
        timerController = new TimerController();
        timerController->setScheduler(scheduler);
        timerController->setInterruptController(interruptController);
    }

    void TearDown() override {
        delete timerController;
        delete interruptController;
        delete memory;
        delete scheduler;
    }
};

TEST_F(TimerTest, ReloadValue) {
    // Write reload value
    timerController->writeReload(0, 0x1234);
    
    // Counter should also be set (when disabled)
    EXPECT_EQ(timerController->readCounter(0), 0x1234);
}

TEST_F(TimerTest, EnableTimer) {
    // Set reload value
    timerController->writeReload(0, 0x0000);
    
    // Enable timer with prescaler 0 (CPU/1)
    timerController->writeControl(0, TIMER_ENABLE | 0);
    
    // Control register should reflect enabled state
    uint16_t control = timerController->readControl(0);
    EXPECT_TRUE(control & TIMER_ENABLE);
}

TEST_F(TimerTest, TimerOverflow_NoIRQ) {
    // Set reload to near overflow
    timerController->writeReload(0, 0xFFFF);
    
    // Enable timer (prescaler 0 = CPU/1)
    timerController->writeControl(0, TIMER_ENABLE | 0);
    
    // Run scheduler past overflow point (1 tick = 1 cycle at prescaler 0)
    scheduler->runUntil(2);
    
    // Timer should have overflowed and reloaded
    EXPECT_EQ(timerController->readCounter(0), 0xFFFF);
}

TEST_F(TimerTest, TimerOverflow_WithIRQ) {
    // Enable IME and Timer 0 interrupt
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_TIMER0);
    
    // Set reload to near overflow
    timerController->writeReload(0, 0xFFFE);
    
    // Enable timer with IRQ (prescaler 0 = CPU/1)
    timerController->writeControl(0, TIMER_ENABLE | TIMER_IRQ_ENABLE | 0);
    
    // Run scheduler past overflow (2 ticks to overflow)
    scheduler->runUntil(3);
    
    // Should have timer interrupt pending
    uint16_t ifReg = memory->read16(REG_IF);
    EXPECT_TRUE(ifReg & IRQ_TIMER0);
}

TEST_F(TimerTest, Prescaler_1) {
    // Prescaler 0 = CPU/1 (no prescaling)
    timerController->writeReload(0, 0x0000);
    timerController->writeControl(0, TIMER_ENABLE | 0);
    
    // Run for 0x10000 cycles (should overflow once)
    scheduler->runUntil(0x10000);
    
    // Counter should have wrapped to reload value
    EXPECT_EQ(timerController->readCounter(0), 0x0000);
}

TEST_F(TimerTest, Prescaler_64) {
    // Enable IME and Timer 0 interrupt
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_TIMER0);
    
    // Prescaler 1 = CPU/64
    timerController->writeReload(0, 0xFFFF);
    timerController->writeControl(0, TIMER_ENABLE | TIMER_IRQ_ENABLE | 1);
    
    // Run for 64 cycles (should increment once and overflow)
    scheduler->runUntil(65);
    
    // Should have overflowed and triggered interrupt
    uint16_t ifReg = memory->read16(REG_IF);
    EXPECT_TRUE(ifReg & IRQ_TIMER0);
}

TEST_F(TimerTest, Prescaler_256) {
    // Enable IME and Timer 1 interrupt
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_TIMER1);
    
    // Prescaler 2 = CPU/256
    timerController->writeReload(1, 0xFFFF);
    timerController->writeControl(1, TIMER_ENABLE | TIMER_IRQ_ENABLE | 2);
    
    // Run for 256 cycles (should increment once and overflow)
    scheduler->runUntil(257);
    
    // Should have overflowed and triggered interrupt
    uint16_t ifReg = memory->read16(REG_IF);
    EXPECT_TRUE(ifReg & IRQ_TIMER1);
}

TEST_F(TimerTest, Prescaler_1024) {
    // Enable IME and Timer 2 interrupt
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_TIMER2);
    
    // Prescaler 3 = CPU/1024
    timerController->writeReload(2, 0xFFFF);
    timerController->writeControl(2, TIMER_ENABLE | TIMER_IRQ_ENABLE | 3);
    
    // Run for 1024 cycles (should increment once and overflow)
    scheduler->runUntil(1025);
    
    // Should have overflowed and triggered interrupt
    uint16_t ifReg = memory->read16(REG_IF);
    EXPECT_TRUE(ifReg & IRQ_TIMER2);
}

TEST_F(TimerTest, CascadeMode_Timer1FromTimer0) {
    // Enable IME and Timer 1 interrupt
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_TIMER1);
    
    // Timer 0: normal mode, will overflow
    timerController->writeReload(0, 0xFFFF);
    timerController->writeControl(0, TIMER_ENABLE | 0);
    
    // Timer 1: cascade mode (count on Timer 0 overflow)
    timerController->writeReload(1, 0xFFFF);
    timerController->writeControl(1, TIMER_ENABLE | TIMER_IRQ_ENABLE | TIMER_COUNT_UP);
    
    // Run until Timer 0 overflows (1 tick)
    scheduler->runUntil(2);
    
    // Timer 1 should have overflowed (was at 0xFFFF, incremented to 0x0000)
    uint16_t ifReg = memory->read16(REG_IF);
    EXPECT_TRUE(ifReg & IRQ_TIMER1);
}

TEST_F(TimerTest, DisableTimer) {
    // Enable timer
    timerController->writeReload(0, 0x1000);
    timerController->writeControl(0, TIMER_ENABLE | 0);
    
    // Run a bit
    scheduler->runUntil(100);
    
    // Disable timer
    timerController->writeControl(0, 0);
    
    // Run more
    scheduler->runUntil(200);
    
    // Counter should not have changed much (scheduler events cancelled)
    // Just verify it doesn't crash and timer is disabled
    uint16_t control = timerController->readControl(0);
    EXPECT_FALSE(control & TIMER_ENABLE);
}

TEST_F(TimerTest, MultipleTimers_Independent) {
    // Enable all 4 timers with different prescalers
    for (int i = 0; i < 4; i++) {
        timerController->writeReload(i, 0xFFF0 + i);
        timerController->writeControl(i, TIMER_ENABLE | i);
    }
    
    // Run scheduler
    scheduler->runUntil(1000);
    
    // All timers should be running independently
    // Just verify no crashes
    for (int i = 0; i < 4; i++) {
        uint16_t control = timerController->readControl(i);
        EXPECT_TRUE(control & TIMER_ENABLE);
    }
}
