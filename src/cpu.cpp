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
    banked_r8_fiq = banked_r9_fiq = banked_r10_fiq = banked_r11_fiq = banked_r12_fiq = 0;
    banked_r13_fiq = banked_r14_fiq = 0;
    banked_r13_svc = banked_r14_svc = 0;
    banked_r13_abt = banked_r14_abt = 0;
    banked_r13_irq = banked_r14_irq = 0;
    banked_r13_und = banked_r14_und = 0;
    banked_r8_usr = banked_r9_usr = banked_r10_usr = banked_r11_usr = banked_r12_usr = 0;
    banked_r13_usr = banked_r14_usr = 0;
    
    // Initialize all SPSR registers to System mode (0x1F) to avoid invalid mode issues
    // SPSR should contain a valid CPSR value with a valid mode (0x10-0x1F)
    // Using System mode (0x1F) is safe as a default
    spsr_fiq = 0x1F;  // System mode
    spsr_svc = 0x1F;  // System mode
    spsr_abt = 0x1F;  // System mode
    spsr_irq = 0x1F;  // System mode
    spsr_und = 0x1F;  // System mode

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
    
    // Save PC before execution for debugging
    uint32_t pc_before __attribute__((unused)) = registers[15];
    
    // Use the existing executeOneInstruction methods from ARM/Thumb CPUs
    if (getFlag(FLAG_T)) {
        // Thumb mode
        thumbCPU->executeOneInstruction();
    } else {
        // ARM mode  
        armCPU->executeOneInstruction();
    }
    
    // Debug logging commented out - BIOS loop fixed!
    // uint32_t pc_addr = pc_before & ~1; // Clear thumb bit
    // if (pc_addr >= 0x118 && pc_addr <= 0x124) {
    //     printf("[BIOS LOOP AFTER] PC was 0x%08X, now 0x%08X | r0=0x%08X r1=0x%08X r4=0x%08X CPSR=0x%08X (N=%d)\n",
    //            pc_before, registers[15], registers[0], registers[1], registers[4], cpsr,
    //            (cpsr >> 31) & 1);
    // }
}

// ============================================================================
// Interrupt Handling
// ============================================================================

bool CPU::checkPendingInterrupts() {
    static int check_count = 0;
    // Check if interrupts are enabled (IME bit and I flag in CPSR)
    bool ime = interruptController.isIMESet();
    bool irqDisabled = (cpsr & 0x80) != 0; // I flag in CPSR bit 7
    
    if (!ime || irqDisabled) {
        return false;
    }
    
    // Check if any interrupts are pending
    bool has_pending = interruptController.hasPendingInterrupt();
    if (has_pending && check_count++ < 5) {
        uint16_t ie = memory.read16(0x04000200);
        uint16_t ifReg = memory.read16(0x04000202);
        printf("[CHECK_IRQ #%d] IME=%d, IRQ_disabled=%d, pending=%d IE=0x%04X IF=0x%04X => WILL CALL handleInterrupt!\n",
               check_count, ime, irqDisabled, has_pending, ie, ifReg);
    }
    return has_pending;
}

void CPU::handleInterrupt() {
    DEBUG_INFO("CPU: Handling interrupt");
    
    // Save current CPSR before mode switch
    uint32_t old_cpsr = cpsr;
    printf("[IRQ] handleInterrupt: CPSR before=0x%08X, PC=0x%08X\n", cpsr, registers[15]);
    fflush(stdout);
    
    // Calculate return address
    // In ARM mode, PC+4 (current instruction + 8, then -4 for return)
    // In Thumb mode, PC+4 is sufficient (current instruction + 4)
    uint32_t returnAddress;
    if (getFlag(FLAG_T)) {
        // Thumb mode: PC already points to next instruction + 2
        returnAddress = registers[15];
    } else {
        // ARM mode: PC+4 to return to instruction after current one
        returnAddress = registers[15] + 4;
    }
    
    printf("[IRQ] Calculated return address=0x%08X\n", returnAddress);
    
    // Switch to IRQ mode (this handles register banking)
    setMode(IRQ);
    printf("[IRQ] After setMode(IRQ): CPSR=0x%08X, LR=0x%08X\n", cpsr, registers[14]);
    
    // Save old CPSR to SPSR_irq
    SPSR() = old_cpsr;
    
    // Set LR_irq to return address
    registers[14] = returnAddress;
    printf("[IRQ] After setting LR: LR=0x%08X\n", registers[14]);
    
    // Disable further interrupts (set I flag in CPSR)
    cpsr |= 0x80; // Set I flag (bit 7)
    
    // Switch to ARM mode (clear T flag in CPSR)
    cpsr &= ~FLAG_T;
    
    printf("[IRQ] Final CPSR=0x%08X before jump to 0x18\n", cpsr);
    
    // Set PC to IRQ vector (0x00000018)
    registers[15] = 0x00000018;
    
    DEBUG_INFO("CPU: Interrupt handled, jumped to IRQ vector 0x00000018, return address = 0x" + 
               debug_to_hex_string(returnAddress, 8));
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
