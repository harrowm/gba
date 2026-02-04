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
    static int check_count = 0;
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
        check_count++;
        // Print every 1000th IRQ to avoid spam, but show details
        if (check_count % 1000 == 0) {
            uint16_t ie = memory.read16(0x04000200);
            uint16_t ifReg = memory.read16(0x04000202);
            LOG_IRQ("[CHECK_IRQ #%d] IE=0x%04X IF=0x%04X (VBlank=%d HBlank=%d Timer0=%d)\n",
                   check_count, ie, ifReg, (ifReg & 0x01), (ifReg & 0x02) >> 1, (ifReg & 0x08) >> 3);
        }
    }
    return has_pending;
}

void CPU::handleInterrupt() {
    // Early exit if I flag is set (like mGBA's ARMRaiseIRQ)
    // This prevents nested interrupts
    if (cpsr & 0x80) {
        return;  // Interrupts disabled, don't process
    }
    
    static int irq_count = 0;
    irq_count++;
    
    DEBUG_INFO("CPU: Handling interrupt");
    
    // Save current CPSR before mode switch
    uint32_t old_cpsr = cpsr;
    
    // PHASE 4: Log IRQ entry with full register state
    static int phase4_count = 0;
    if (phase4_count < 20) {
        fprintf(stderr, "[IRQ ENTRY #%d] PC=0x%08X CPSR=0x%08X SP=0x%08X LR=0x%08X\n",
                phase4_count, registers[15], cpsr, registers[13], registers[14]);
        fprintf(stderr, "  R0-R3: %08X %08X %08X %08X  R4-R7: %08X %08X %08X %08X\n",
                registers[0], registers[1], registers[2], registers[3],
                registers[4], registers[5], registers[6], registers[7]);
        phase4_count++;
    }
    
    if (irq_count <= 5) {
        LOG_IRQ("[IRQ #%d] handleInterrupt: CPSR before=0x%08X, PC=0x%08X\n", irq_count, cpsr, registers[15]);
        fflush(stdout);
    }
    
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
    uint32_t returnAddress;
    if (getFlag(FLAG_T)) {
        // Thumb mode: LR = PC + 4, so SUBS PC, LR, #4 returns to PC
        returnAddress = registers[15] + 4;
    } else {
        // ARM mode: LR = PC + 4, so SUBS PC, LR, #4 returns to PC
        // (ARM step increments by 4, so same logic applies)
        returnAddress = registers[15] + 4;
    }
    
    if (irq_count <= 5) {
        LOG_IRQ("[IRQ #%d] Calculated return address=0x%08X\n", irq_count, returnAddress);
    }
    
    // Switch to IRQ mode (this handles register banking)
    setMode(IRQ);
    
    // Save old CPSR to SPSR_irq
    SPSR() = old_cpsr;
    
    // Set LR_irq to return address
    registers[14] = returnAddress;
    
    // Disable further interrupts (set I flag in CPSR)
    cpsr |= 0x80; // Set I flag (bit 7)
    
    if (irq_count <= 5) {
        LOG_IRQ("[IRQ #%d] Set I flag, CPSR now=0x%08X\n", irq_count, cpsr);
    }
    
    // Switch to ARM mode (clear T flag in CPSR)
    cpsr &= ~FLAG_T;
    
    // Set PC to IRQ vector (0x00000018)
    registers[15] = 0x00000018;
    
    if (irq_count <= 5) {
        LOG_IRQ("[IRQ #%d] Set PC to IRQ vector 0x00000018\n", irq_count);
        fflush(stdout);
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
