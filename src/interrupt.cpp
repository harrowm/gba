#include "interrupt.h"
#include "memory.h"
#include "scheduler.h"
#include "debug.h"

void InterruptController::requestInterrupt(uint16_t irqFlag) {
    if (!memory) {
        DEBUG_ERROR("InterruptController: memory not set");
        return;
    }
    
    // Read current IF register (without wait cycles - internal operation)
    uint16_t currentIF = memory->readDirectIO16(REG_IF);
    uint16_t ieVal = memory->readDirectIO16(REG_IE);
    uint16_t imeVal = memory->readDirectIO32(REG_IME);
    
    // Set the interrupt request flag
    currentIF |= irqFlag;
    
    // Write directly to I/O memory (bypass write-to-clear logic)
    // This is an internal write from hardware, not from CPU
    memory->writeDirectIO(REG_IF, currentIF);
    
    // Log interrupt raising (like mGBA)
    static const char* irq_names[] = {"VBLANK", "HBLANK", "VCOUNTER", "TIMER0", "TIMER1", "TIMER2", "TIMER3", "SIO", "DMA0", "DMA1", "DMA2", "DMA3", "KEYPAD", "GAMEPAK"};
    int irq_bit = -1;
    for (int i = 0; i < 14; i++) {
        if (irqFlag & (1 << i)) {
            irq_bit = i;
            break;
        }
    }
    uint64_t currentCycle = scheduler ? scheduler->getCurrentCycle() : 0;
    // Disable HBlank spam, only log VBlank
    if (irq_bit == 0) {  // VBlank only
        LOG_IRQ("[GBA IRQ] %s raised at cycle %llu, IF=0x%04X, IE=0x%04X, IME=0x%04X\n",
                irq_bit >= 0 ? irq_names[irq_bit] : "UNKNOWN",
                currentCycle,
                currentIF,
                ieVal,
                imeVal);
    }
    
    DEBUG_INFO("Interrupt requested: 0x" + debug_to_hex_string(irqFlag, 4));
    
    // Schedule IRQ trigger with latency (like mGBA's GBATestIRQ)
    // Schedule whenever IE & IF match, regardless of IME.
    // On real hardware, HALT wakes on any IE & IF match (IME is irrelevant
    // for wake-up). IME only gates whether the IRQ is actually *taken* by
    // the CPU, which the irqCallback already handles.
    uint16_t ieIF = ieVal & currentIF;
    if (scheduler && ieIF) {
        scheduleIRQCheck();
    }
}

// Schedule an IRQ_TRIGGER event if none is already pending.
// The callback unhalts the CPU and (if CPSR I=0) takes the interrupt.
// No self-retry: on real hardware the CPU checks pending IRQs each
// instruction cycle once I is cleared, so a new requestInterrupt()
// (e.g. from the next HBlank/timer) will naturally re-schedule.
void InterruptController::scheduleIRQCheck() {
    if (!scheduler || !memory) return;
    if (scheduler->hasEventsOfType(EventType::IRQ_TRIGGER)) return;

    uint16_t ie = memory->readDirectIO16(REG_IE);
    uint16_t ifr = memory->readDirectIO16(REG_IF);
    if (!(ie & ifr)) return;

    scheduler->schedule(IRQ_LATENCY_CYCLES, [this]() {
        uint16_t ie2 = memory->readDirectIO16(REG_IE);
        uint16_t ifr2 = memory->readDirectIO16(REG_IF);
        if ((ie2 & ifr2) && irqCallback) {
            irqCallback();
        }
        // No self-retry — avoids tight 7-cycle scheduler loop while CPSR I=1
    }, EventType::IRQ_TRIGGER);
}

bool InterruptController::isIMESet() const {
    if (!memory) {
        return false;
    }
    
    // Direct read without wait cycles (like mGBA - these are instant)
    uint16_t imeVal = memory->readDirectIO16(REG_IME);
    
    return (imeVal & 0x0001) != 0;
}

bool InterruptController::hasPendingInterrupt() const {
    if (!memory) {
        return false;
    }
    
    // Check using memory reads with wait cycles disabled (like mGBA - interrupt checks are instant)
    if (!isIMESet()) {
        return false;  // Master interrupt disabled
    }
    
    // Direct read without wait cycles
    uint16_t ieVal = memory->readDirectIO16(REG_IE);
    uint16_t ifVal = memory->readDirectIO16(REG_IF);
    
    // Check if any enabled interrupts are pending
    return (ieVal & ifVal) != 0;
}
