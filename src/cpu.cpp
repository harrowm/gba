#include "cpu.h"
#include "debug.h" // Use macro-based debug system
#include "thumb_cpu.h" // Include complete definition
#include "arm_cpu.h"   // Include complete definition
#include "scheduler.h"

CPU::CPU(Memory& mem, InterruptController& ic) : memory(mem), interruptController(ic), scheduler(nullptr), halted(false) {
    thumbCPU = new ThumbCPU(*this); // Pass itself as the parent reference
    armCPU = new ARMCPU(*this);     // Pass itself as the parent reference
    std::fill(std::begin(registers), std::end(registers), 0); // Reset all registers to zero using std::fill
    DEBUG_LOG("Thumb and ARM instances created, CPU registers initialized to zero");
    
    // Match mGBA's initial state for easier trace comparison
    // Note: BIOS will set these properly around instruction 100-200, but matching mGBA's
    // initial values eliminates early spurious differences in trace comparisons
    cpsr = 0x1F; // System mode (0x1F), ARM mode, IRQ/FIQ enabled
    registers[13] = 0x03007F00; // SP initialized to top of IWRAM (mGBA default)

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

// Like mGBA's readCPSR hook: re-check pending interrupts after CPSR is
// restored from SPSR (e.g. SUBS PC,LR / LDMFD SP!,{...,PC}^).
// This ensures VBlank IRQs raised while CPSR I=1 get delivered promptly
// once the previous IRQ handler returns and clears I.
//
// When CPSR I-flag clears (e.g. returning from IRQ handler via SUBS PC,LR,#4),
// re-check for pending interrupts.  We cancel any pending IRQ_TRIGGER event
// and reschedule with 0 delay so the interrupt fires at the next instruction
// boundary.  This is slightly faster than mGBA's model (which always uses 7
// cycles via GBATestIRQNoDelay → GBATestIRQ(0)), but compensates for other
// timing differences in our execution model.  The cancel ensures we deliver
// immediately on CPSR restore rather than waiting for a stale retry event.
void CPU::onCPSRWrite() {
    if (!(cpsr & 0x80)) {  // I flag clear — interrupts enabled
        if (scheduler && scheduler->hasEventsOfType(EventType::IRQ_TRIGGER)) {
            scheduler->cancelEventsOfType(EventType::IRQ_TRIGGER);
        }
        interruptController.scheduleIRQCheck(0);
    }
}

// ============================================================================
// Single Instruction Execution
// ============================================================================

void CPU::executeOneInstruction() {
    // Trace instruction state BEFORE execution (like mGBA's GDB trace)
    // Tracers use readDirectIO internally now, so no need to disable wait cycles
    if (tracer.isEnabled()) {
        tracer.traceInstruction(registers.data(), cpsr);
    }
    if (memoryTracer.isEnabled()) {
        uint64_t current_cycle = scheduler ? scheduler->getCurrentCycle() : 0;
        memoryTracer.traceInstruction(registers.data(), cpsr, current_cycle);
    }
    
    // Debug (DISABLED FOR PERFORMANCE)
    // static uint32_t last_pc = 0;
    // if (exec_count <= 5 || exec_count % 50000 == 0 || 
    //     (last_pc == 0x18 || registers[15] == 0x18 || registers[15] < 0x100)) {
    //     printf("[CPU::executeOneInstruction #%llu] PC=0x%08X CPSR=0x%08X T=%d\n",
    //            exec_count, registers[15], cpsr, getFlag(FLAG_T));
    // }
    // last_pc = registers[15];
    
    // NOTE: Interrupts are now handled via scheduler events with IRQ_LATENCY_CYCLES delay
    // We no longer check for interrupts before every instruction - this prevents nested IRQs
    // Instead, when IF is set, the interrupt controller schedules an IRQ_TRIGGER event
    // After 7 cycles, the event fires and checks IME and I flag before calling handleInterrupt()
    
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
    // Check if interrupts are enabled (IME bit and I flag in CPSR)
    bool ime = interruptController.isIMESet();
    bool irqDisabled = (cpsr & 0x80) != 0; // I flag in CPSR bit 7
    
    // Also check if we're already in IRQ mode - don't allow nested interrupts!
    Mode current_mode = static_cast<Mode>(cpsr & 0x1F);
    bool in_irq_mode = (current_mode == IRQ);
    
    if (!ime || irqDisabled || in_irq_mode) {
        return false;
    }
    
    // Check if any interrupts are pending
    bool has_pending = interruptController.hasPendingInterrupt();
    if (has_pending) {
        static int check_count = 0;
        check_count++;
        // Print every 1000th IRQ to avoid spam, but show details
        if (check_count % 1000 == 0) {
            uint16_t ie = memory.readDirectIO16(0x04000200);
            uint16_t ifReg = memory.readDirectIO16(0x04000202);
            LOG_IRQ("[CHECK_IRQ #%d] IE=0x%04X IF=0x%04X (VBlank=%d HBlank=%d Timer0=%d)\n",
                   check_count, ie, ifReg, (ifReg & 0x01), (ifReg & 0x02) >> 1, (ifReg & 0x08) >> 3);
        }
    }
    return has_pending;
}

void CPU::handleInterrupt() {
    // Wake up from HALT state — any interrupt wakes the CPU regardless
    // of whether the interrupt is enabled or not. The I-flag check below
    // will still gate actual IRQ processing.
    halted = false;
    
    // Early exit if I flag is set (like mGBA's ARMRaiseIRQ)
    // This prevents nested interrupts
    if (cpsr & 0x80) {
        return;  // Interrupts disabled, don't process
    }
    
    DEBUG_INFO("CPU: Handling interrupt");
    
    // Save current CPSR before mode switch
    uint32_t old_cpsr = cpsr;
    
    // Calculate return address for IRQ
    // After IRQ completes, "SUBS PC, LR, #4" returns to resume execution
    // 
    // Our architecture:
    // - Step increments PC by instructionWidth BEFORE execution
    // - After normal instruction at addr: R[15] = addr + 2 (Thumb) or addr + 4 (ARM)
    // - After PC-modifying instruction (POP, BX, etc.): R[15] = target (not incremented)
    //
    // For both cases, we want LR = R[15] + 4 so SUBS PC, LR, #4 gives R[15]
    // - Normal: Return = (addr+2) + 4 - 4 = addr+2 (next instruction) ✓
    // - PC-mod: Return = target + 4 - 4 = target (correct destination) ✓
    uint32_t returnAddress = registers[15] + 4;
    
    // Switch to IRQ mode (this handles register banking)
    setMode(IRQ);
    
    // Save old CPSR to SPSR_irq
    SPSR() = old_cpsr;
    
    // Set LR_irq to return address
    registers[14] = returnAddress;
    
    // Disable further interrupts (set I flag in CPSR)
    cpsr |= 0x80; // Set I flag (bit 7)
    
    // Switch to ARM mode (clear T flag in CPSR)
    cpsr &= ~FLAG_T;
    
    // Set PC to IRQ vector (0x00000018)
    registers[15] = 0x00000018;
    
    // Pipeline refill: jumping to the IRQ vector requires refilling the
    // ARM 3-stage pipeline at the target address. This costs 1N + 1S
    // fetch cycles at the target (BIOS ROM at 0x18). Matches mGBA's
    // ARM_WRITE_PC in ARMRaiseIRQ which deducts
    //   activeNonseqCycles32 + activeSeqCycles32
    // from cpu->cycles.
    if (scheduler) {
        uint32_t irqRefillCycles = memory.getNonseqWaitCycles32(0x18)
                                 + memory.getSeqWaitCycles32(0x18);
        // Add base 1+1 = 2 cycles for the two fetches (1N + 1S pipeline refill)
        irqRefillCycles += 2;
        scheduler->advanceCycles(irqRefillCycles);
    }
    
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
    
    // Match mGBA's initial state for easier trace comparison
    // Note: Real ARM7TDMI hardware starts in SVC mode (0xD3), but mGBA starts in System mode (0x1F)
    // BIOS will set proper modes around instruction 100-200, but matching mGBA's initial
    // values eliminates early spurious differences in trace comparisons
    cpsr = 0x0000001F; // System mode (0x1F), ARM mode, IRQ/FIQ enabled (to match mGBA)
    registers[13] = 0x03007F00; // SP initialized to top of IWRAM (matches mGBA default)
    
    // Set PC to reset vector (0x00000000)
    // This is the BIOS entry point
    registers[15] = 0x00000000;
    
    // ARM7TDMI 3-stage pipeline must be filled on startup
    // NOTE: mGBA does NOT count the initial 2 pipeline fill cycles at reset.
    // They start at cycle 0 for the first instruction execution.
    // We previously added 2 cycles here, but commenting out to match mGBA's behavior.
    // This is the "correct" behavior according to mGBA's reference implementation.
    /*
    if (scheduler) {
        scheduler->advanceCycles(2);
        DEBUG_INFO("CPU: Added 2 cycles for pipeline fill at reset");
    }
    */
    
    DEBUG_INFO("CPU: Reset complete in Supervisor mode, PC=0x00000000");
}
