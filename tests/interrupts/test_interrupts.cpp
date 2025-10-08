#include <gtest/gtest.h>
#include "../../include/cpu.h"
#include "../../include/memory.h"
#include "../../include/interrupt.h"
#include "../../include/scheduler.h"

class InterruptTest : public ::testing::Test {
protected:
    Memory* memory;
    InterruptController* interruptController;
    Scheduler* scheduler;
    CPU* cpu;

    void SetUp() override {
        memory = new Memory(false);  // Normal mode (needed for I/O registers)
        interruptController = new InterruptController();
        scheduler = new Scheduler();
        
        // Wire up components
        interruptController->setMemory(memory);
        memory->setScheduler(scheduler);
        
        cpu = new CPU(*memory, *interruptController);
        cpu->setScheduler(scheduler);
        
        // Set up IRQ callback
        interruptController->setIRQCallback([]() {
            // CPU will check on next instruction
        });
        
        cpu->reset();
    }

    void TearDown() override {
        delete cpu;
        delete scheduler;
        delete interruptController;
        delete memory;
    }
};

TEST_F(InterruptTest, IME_EnableDisable) {
    // Initially IME should be 0
    EXPECT_FALSE(interruptController->isIMESet());
    
    // Enable IME
    memory->write16(REG_IME, 0x0001);
    EXPECT_TRUE(interruptController->isIMESet());
    
    // Disable IME
    memory->write16(REG_IME, 0x0000);
    EXPECT_FALSE(interruptController->isIMESet());
}

TEST_F(InterruptTest, IE_IF_Registers) {
    // Enable master interrupt
    memory->write16(REG_IME, 0x0001);
    
    // Enable V-Blank interrupt
    memory->write16(REG_IE, IRQ_VBLANK);
    EXPECT_EQ(memory->read16(REG_IE), IRQ_VBLANK);
    
    // Request V-Blank interrupt
    interruptController->requestInterrupt(IRQ_VBLANK);
    EXPECT_EQ(memory->read16(REG_IF), IRQ_VBLANK);
    
    // Should have pending interrupt
    EXPECT_TRUE(interruptController->hasPendingInterrupt());
    
    // Acknowledge interrupt (write 1 to clear)
    memory->write16(REG_IF, IRQ_VBLANK);
    EXPECT_EQ(memory->read16(REG_IF), 0);
    
    // Should not have pending interrupt anymore
    EXPECT_FALSE(interruptController->hasPendingInterrupt());
}

TEST_F(InterruptTest, CPU_IRQ_ModeSwitch) {
    // Set up CPU in User mode
    cpu->setMode(CPU::USER);
    cpu->R()[13] = 0x03007F00;  // User SP
    cpu->R()[14] = 0x12345678;  // User LR
    cpu->R()[15] = 0x08000100;  // PC in ROM
    cpu->CPSR() = 0x10;  // User mode, ARM mode, interrupts enabled
    
    // Enable interrupts
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_VBLANK);
    
    // Request interrupt
    interruptController->requestInterrupt(IRQ_VBLANK);
    
    // Check if interrupt should be handled
    EXPECT_TRUE(cpu->checkPendingInterrupts());
    
    // Handle interrupt
    cpu->handleInterrupt();
    
    // Verify CPU switched to IRQ mode
    EXPECT_EQ(cpu->CPSR() & 0x1F, CPU::IRQ);
    
    // Verify PC jumped to IRQ vector
    EXPECT_EQ(cpu->R()[15], 0x00000018u);
    
    // Verify interrupts disabled (I flag set)
    EXPECT_TRUE(cpu->CPSR() & 0x80);
    
    // Verify LR_irq contains return address
    EXPECT_EQ(cpu->R()[14], 0x08000104u);  // PC + 4
    
    // Verify SPSR_irq saved old CPSR
    EXPECT_EQ(cpu->SPSR(), 0x10u);
}

TEST_F(InterruptTest, CPU_InterruptDisabled_By_CPSR) {
    // Set up CPU with I flag set (interrupts disabled in CPSR)
    cpu->setMode(CPU::SYS);
    cpu->R()[15] = 0x08000100;
    cpu->CPSR() = 0x9F;  // System mode, ARM mode, I flag set (bit 7)
    
    // Enable interrupts in interrupt controller
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_VBLANK);
    interruptController->requestInterrupt(IRQ_VBLANK);
    
    // Interrupt controller has pending interrupt
    EXPECT_TRUE(interruptController->hasPendingInterrupt());
    
    // But CPU should not handle it (I flag set)
    EXPECT_FALSE(cpu->checkPendingInterrupts());
}

TEST_F(InterruptTest, MultipleInterrupts_Priority) {
    // Enable interrupts
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_VBLANK | IRQ_HBLANK | IRQ_TIMER0);
    
    // Request multiple interrupts
    interruptController->requestInterrupt(IRQ_TIMER0);
    interruptController->requestInterrupt(IRQ_HBLANK);
    interruptController->requestInterrupt(IRQ_VBLANK);
    
    // Should have all three pending
    uint16_t ifReg = memory->read16(REG_IF);
    EXPECT_TRUE(ifReg & IRQ_VBLANK);
    EXPECT_TRUE(ifReg & IRQ_HBLANK);
    EXPECT_TRUE(ifReg & IRQ_TIMER0);
    
    // All should be pending
    EXPECT_TRUE(interruptController->hasPendingInterrupt());
}

TEST_F(InterruptTest, InterruptDisabled_WhenNotInIE) {
    // Enable master interrupt
    memory->write16(REG_IME, 0x0001);
    
    // Enable only V-Blank
    memory->write16(REG_IE, IRQ_VBLANK);
    
    // Request H-Blank interrupt (not enabled)
    interruptController->requestInterrupt(IRQ_HBLANK);
    
    // Should not have pending interrupt (not enabled)
    EXPECT_FALSE(interruptController->hasPendingInterrupt());
}

TEST_F(InterruptTest, VBlank_Trigger) {
    // Enable interrupts
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_VBLANK);
    
    // Trigger V-Blank
    interruptController->triggerVBlank();
    
    // Should have pending interrupt
    EXPECT_TRUE(interruptController->hasPendingInterrupt());
    EXPECT_TRUE(memory->read16(REG_IF) & IRQ_VBLANK);
}

TEST_F(InterruptTest, HBlank_Trigger) {
    // Enable interrupts
    memory->write16(REG_IME, 0x0001);
    memory->write16(REG_IE, IRQ_HBLANK);
    
    // Trigger H-Blank
    interruptController->triggerHBlank();
    
    // Should have pending interrupt
    EXPECT_TRUE(interruptController->hasPendingInterrupt());
    EXPECT_TRUE(memory->read16(REG_IF) & IRQ_HBLANK);
}
