#include "cpu.h"
#include "debug.h" // Use macro-based debug system
#include "thumb_cpu.h" // Include complete definition
#include "arm_cpu.h"   // Include complete definition
#include "timing.h"
#include "scheduler.h"

CPU::CPU(Memory& mem, InterruptController& ic) : memory(mem), interruptController(ic), scheduler(nullptr) {
    thumbCPU = new ThumbCPU(*this); // Pass itself as the parent reference
    armCPU = new ARMCPU(*this);     // Pass itself as the parent reference
    std::fill(std::begin(registers), std::end(registers), 0); // Reset all registers to zero using std::fill
    DEBUG_LOG("Thumb and ARM instances created, CPU registers initialized to zero");
    cpsr = 0; // Sets us up in ARM mode and little endian by default

    // Initialize all banked registers to zero
    banked_r13_fiq = banked_r14_fiq = 0;
    banked_r13_svc = banked_r14_svc = 0;
    banked_r13_abt = banked_r14_abt = 0;
    banked_r13_irq = banked_r14_irq = 0;
    banked_r13_und = banked_r14_und = 0;
    banked_r13_usr = banked_r14_usr = 0;

    // Initialize timing system
    timing_init(&timing);
    DEBUG_LOG("Timing system initialized");
}

CPU::CPUState CPU::getCPUState() const {
    DEBUG_INFO("Getting CPU state");
    CPUState state;
    for (int i = 0; i < 16; ++i) {
        state.registers[i] = registers[i];
    }
    state.cpsr = cpsr;
    return state;
}

void CPU::execute(uint32_t cycles) {
    // Use macro-based debug system
    DEBUG_INFO("Executing CPU for " + std::to_string(cycles) + " cycles");

    if (getFlag(FLAG_T)) {
        DEBUG_LOG("Executing Thumb instructions");
        thumbCPU->execute(cycles); // Use pointer
    } else {
        DEBUG_LOG("Executing ARM instructions");
        armCPU->execute(cycles); // Use pointer
    }
}

// New timing-aware execution method
void CPU::executeWithTiming(uint32_t cycles) {
    // Use macro-based debug system
    DEBUG_INFO("Executing CPU with timing for " + std::to_string(cycles) + " cycles");
    
    if (getFlag(FLAG_T)) {
        DEBUG_LOG("Executing Thumb instructions with timing");
        thumbCPU->executeWithTiming(cycles, &timing);
    } else {
        DEBUG_LOG("Executing ARM instructions with timing");
        armCPU->executeWithTiming(cycles, &timing);
    }
}

void CPU::printCPUState() const {
    DEBUG_INFO("Printing CPU state");

    std::string stateStr = "\nCPU State:\n";
    for (int i = 0; i < 16; ++i) {
        stateStr += "R" + std::to_string(i) + (i < 10 ? " : " : ": ") + debug_to_hex_string(registers[i], 8) + "\t";
        if ((i + 1) % 4 == 0) {
            stateStr += "\n"; // New line every 4 registers
        }
    }

    stateStr += "CPSR: " + debug_to_hex_string(cpsr, 8);

    stateStr += " Flags: Z:" + std::to_string(getFlag(FLAG_Z)) + " N:" + std::to_string(getFlag(FLAG_N)) + 
              " V:" + std::to_string(getFlag(FLAG_V)) + " C:" + std::to_string(getFlag(FLAG_C)) + 
              " T:" + std::to_string(getFlag(FLAG_T)) + " E:" + std::to_string(getFlag(FLAG_E)) + "\n";

    DEBUG_INFO(stateStr);
}

// Destructor
CPU::~CPU() {
    delete thumbCPU;
    delete armCPU;
}

// ============================================================================
// Scheduler Integration
// ============================================================================

void CPU::advanceCycles(uint32_t cycles) {
    if (scheduler) {
        // Just advance the cycle counter without processing events
        // Events will be processed by runFrame() when the full frame completes
        scheduler->advanceCycles(cycles);
    }
}

// ============================================================================
// Single Instruction Execution
// ============================================================================

void CPU::executeOneInstruction() {
    static uint64_t exec_count = 0;
    exec_count++;
    
    // Debug: Print first few calls to see if we're even getting here
    if (exec_count <= 5 || exec_count % 50000 == 0) {
        printf("[CPU::executeOneInstruction #%llu] PC=0x%08X CPSR=0x%08X T=%d\n",
               exec_count, registers[15], cpsr, getFlag(FLAG_T));
    }
    
    // Check for pending interrupts before executing
    if (checkPendingInterrupts()) {
        handleInterrupt();
        return;
    }
    
    // Use the existing executeOneInstruction methods from ARM/Thumb CPUs
    if (getFlag(FLAG_T)) {
        // Thumb mode
        thumbCPU->executeOneInstruction();
    } else {
        // ARM mode  
        armCPU->executeOneInstruction();
    }
}

// ============================================================================
// Interrupt Handling
// ============================================================================

bool CPU::checkPendingInterrupts() {
    // Check if interrupts are enabled (IME bit and I flag in CPSR)
    bool ime = interruptController.isIMESet();
    bool irqDisabled = (cpsr & 0x80) != 0; // I flag in CPSR bit 7
    
    if (!ime || irqDisabled) {
        return false;
    }
    
    // Check if any interrupts are pending
    return interruptController.hasPendingInterrupt();
}

void CPU::handleInterrupt() {
    DEBUG_INFO("CPU: Handling interrupt");
    
    // Save current PC+4 to LR_irq
    // In ARM mode, PC is current instruction + 8
    // In Thumb mode, PC is current instruction + 4
    uint32_t returnAddress = registers[15] + (getFlag(FLAG_T) ? 0 : 4);
    
    // Switch to IRQ mode
    setMode(IRQ);
    
    // Now that we're in IRQ mode, set LR_irq
    registers[14] = returnAddress; // LR_irq
    
    // Save old CPSR to SPSR_irq (not implemented yet, would need SPSR banking)
    // TODO: Implement SPSR (Saved Program Status Register) banking
    
    // Disable further interrupts (set I flag)
    cpsr |= 0x80; // Set I flag (bit 7)
    
    // Switch to ARM mode (clear T flag)
    cpsr &= ~FLAG_T;
    
    // Set PC to IRQ vector (0x00000018)
    registers[15] = 0x00000018;
    
    DEBUG_INFO("CPU: Interrupt handled, jumped to 0x00000018");
}

// ============================================================================
// Reset and Initialization
// ============================================================================

void CPU::reset() {
    DEBUG_INFO("CPU: Resetting");
    
    // Reset all registers
    std::fill(std::begin(registers), std::end(registers), 0);
    
    // Reset banked registers
    banked_r13_fiq = banked_r14_fiq = 0;
    banked_r13_svc = banked_r14_svc = 0;
    banked_r13_abt = banked_r14_abt = 0;
    banked_r13_irq = banked_r14_irq = 0;
    banked_r13_und = banked_r14_und = 0;
    banked_r13_usr = banked_r14_usr = 0;
    
    // Initialize stack pointers for different modes
    // BIOS will set these up properly, but we initialize to safe values
    setMode(IRQ);
    registers[13] = 0x03007FA0; // IRQ stack
    
    setMode(SVC);
    registers[13] = 0x03007FE0; // Supervisor stack
    
    setMode(SYS);
    registers[13] = 0x03007F00; // System/User stack
    
    // Start in Supervisor mode with interrupts disabled
    cpsr = 0x000000D3; // SVC mode (0x13) | IRQ disabled (bit 7) | FIQ disabled (bit 6)
    
    // Set PC to reset vector (0x00000000)
    // In a real GBA, the BIOS starts here
    registers[15] = 0x00000000;
    
    DEBUG_INFO("CPU: Reset complete, PC=0x00000000");
}
