#include "interrupt.h"
#include "memory.h"
#include "debug.h"

void InterruptController::requestInterrupt(uint16_t irqFlag) {
    if (!memory) {
        DEBUG_ERROR("InterruptController: memory not set");
        return;
    }
    
    // Read current IF register
    uint16_t ifReg = memory->read16(REG_IF);
    
    // Set the interrupt request flag
    ifReg |= irqFlag;
    
    // Write directly to I/O memory (bypass write-to-clear logic)
    // This is an internal write from hardware, not from CPU
    memory->writeDirectIO(REG_IF, ifReg);
    
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
    
    // Read IME (Interrupt Master Enable)
    uint16_t ime = memory->read16(REG_IME);
    return (ime & 0x0001) != 0;
}

bool InterruptController::hasPendingInterrupt() const {
    if (!memory) {
        return false;
    }
    
    // Read IME (Interrupt Master Enable)
    if (!isIMESet()) {
        return false;  // Master interrupt disabled
    }
    
    // Read IE (Interrupt Enable) and IF (Interrupt Flags)
    uint16_t ie = memory->read16(REG_IE);
    uint16_t ifReg = memory->read16(REG_IF);
    
    // Check if any enabled interrupts are pending
    return (ie & ifReg) != 0;
}
