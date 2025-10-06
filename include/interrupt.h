#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <cstdint>
#include <functional>

// Interrupt bit flags (REG_IE and REG_IF)
constexpr uint16_t IRQ_VBLANK  = 0x0001;
constexpr uint16_t IRQ_HBLANK  = 0x0002;
constexpr uint16_t IRQ_VCOUNT  = 0x0004;
constexpr uint16_t IRQ_TIMER0  = 0x0008;
constexpr uint16_t IRQ_TIMER1  = 0x0010;
constexpr uint16_t IRQ_TIMER2  = 0x0020;
constexpr uint16_t IRQ_TIMER3  = 0x0040;
constexpr uint16_t IRQ_SERIAL  = 0x0080;
constexpr uint16_t IRQ_DMA0    = 0x0100;
constexpr uint16_t IRQ_DMA1    = 0x0200;
constexpr uint16_t IRQ_DMA2    = 0x0400;
constexpr uint16_t IRQ_DMA3    = 0x0800;
constexpr uint16_t IRQ_KEYPAD  = 0x1000;
constexpr uint16_t IRQ_GAMEPAK = 0x2000;

// I/O Register addresses
constexpr uint32_t REG_IE  = 0x04000200;  // Interrupt Enable
constexpr uint32_t REG_IF  = 0x04000202;  // Interrupt Request/Acknowledge
constexpr uint32_t REG_IME = 0x04000208;  // Interrupt Master Enable

class Memory;  // Forward declaration

class InterruptController {
private:
    Memory* memory;
    std::function<void()> irqCallback;  // Callback to CPU when interrupt fires

public:
    InterruptController() : memory(nullptr) {}
    
    void setMemory(Memory* mem) { memory = mem; }
    void setIRQCallback(std::function<void()> callback) { irqCallback = callback; }
    
    // Request an interrupt (sets IF bit)
    void requestInterrupt(uint16_t irqFlag);
    
    // Check if any enabled interrupts are pending
    bool hasPendingInterrupt() const;
    
    // Specific interrupt triggers
    void triggerVBlank() { requestInterrupt(IRQ_VBLANK); }
    void triggerHBlank() { requestInterrupt(IRQ_HBLANK); }
    void triggerVCount() { requestInterrupt(IRQ_VCOUNT); }
    
    // Legacy method for compatibility
    void triggerInterrupt() { requestInterrupt(IRQ_VBLANK); }
};

#endif
