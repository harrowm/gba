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

// Interrupt latency in cycles (like mGBA's GBA_IRQ_DELAY)
constexpr uint32_t IRQ_LATENCY_CYCLES = 7;

class Memory;  // Forward declaration
class Scheduler;  // Forward declaration

class InterruptController {
private:
    Memory* memory;
    Scheduler* scheduler;
    std::function<void()> irqCallback;  // Callback to CPU when interrupt fires

public:
    InterruptController() : memory(nullptr), scheduler(nullptr) {}
    
    void setMemory(Memory* mem) { memory = mem; }
    void setScheduler(Scheduler* sched) { scheduler = sched; }
    void setIRQCallback(std::function<void()> callback) { irqCallback = callback; }
    
    // Request an interrupt (sets IF bit)
    void requestInterrupt(uint16_t irqFlag);
    
    // Schedule an IRQ check event if pending interrupts exist.
    // Self-retries every IRQ_LATENCY_CYCLES until CPSR I=0 allows handling.
    // Called from requestInterrupt and should be called when CPSR I changes 1→0.
    // latency: cycles before the check fires. Use 0 for immediate (CPSR restore),
    //          IRQ_LATENCY_CYCLES for hardware-raised interrupts.
    void scheduleIRQCheck(uint32_t latency = IRQ_LATENCY_CYCLES);
    
    // Check if any enabled interrupts are pending
    bool hasPendingInterrupt() const;
    
    // Check if IME (Interrupt Master Enable) is set
    bool isIMESet() const;
    
    // Specific interrupt triggers
    void triggerVBlank() { requestInterrupt(IRQ_VBLANK); }
    void triggerHBlank() { requestInterrupt(IRQ_HBLANK); }

};

#endif
