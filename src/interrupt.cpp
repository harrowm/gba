#include "interrupt.h"
#include "memory.h"
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
    
    // Check if this interrupt should trigger CPU interrupt
    if (hasPendingInterrupt() && irqCallback) {
        irqCallback();
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
