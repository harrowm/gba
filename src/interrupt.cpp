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
    memory->setDisableWaitCycles(true);
    uint16_t currentIF = memory->read16(REG_IF);
    memory->setDisableWaitCycles(false);
    
    // Set the interrupt request flag
    currentIF |= irqFlag;
    
    // Write directly to I/O memory (bypass write-to-clear logic)
    // This is an internal write from hardware, not from CPU
    memory->writeDirectIO(REG_IF, currentIF);
    
    DEBUG_INFO("Interrupt requested: 0x" + debug_to_hex_string(irqFlag, 4));
    
    // Schedule IRQ trigger with latency (like mGBA's GBATestIRQ)
    // Only schedule if interrupts are enabled and not already scheduled
    if (scheduler && hasPendingInterrupt()) {
        if (!scheduler->hasEventsOfType(EventType::IRQ_TRIGGER)) {
            static int schedule_count = 0;
            schedule_count++;
            if (schedule_count <= 10) {
                printf("[IRQ SCHEDULE #%d] Scheduling IRQ_TRIGGER event with %d cycle delay\n", 
                       schedule_count, IRQ_LATENCY_CYCLES);
            }
            scheduler->schedule(IRQ_LATENCY_CYCLES, [this]() {
                static int trigger_count = 0;
                trigger_count++;
                if (trigger_count <= 10) {
                    printf("[IRQ TRIGGER #%d] Event fired, checking pending interrupts\n", trigger_count);
                }
                // At trigger time, check IME and I flag, then call CPU
                if (hasPendingInterrupt() && irqCallback) {
                    if (trigger_count <= 10) {
                        printf("[IRQ TRIGGER #%d] Calling irqCallback\n", trigger_count);
                    }
                    irqCallback();
                }
            }, EventType::IRQ_TRIGGER);
        }
    }
}

bool InterruptController::isIMESet() const {
    if (!memory) {
        return false;
    }
    
    // Disable wait cycles for interrupt register read (like mGBA - these are instant)
    memory->setDisableWaitCycles(true);
    uint16_t imeVal = memory->read16(REG_IME);
    memory->setDisableWaitCycles(false);
    
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
    
    // Disable wait cycles for interrupt register reads
    memory->setDisableWaitCycles(true);
    uint16_t ieVal = memory->read16(REG_IE);
    uint16_t ifVal = memory->read16(REG_IF);
    memory->setDisableWaitCycles(false);
    
    // Check if any enabled interrupts are pending
    return (ieVal & ifVal) != 0;
}
