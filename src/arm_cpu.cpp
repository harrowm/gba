
#include <cstdint>
#include <cassert>
#include <capstone/capstone.h>
#include "arm_cpu.h"
#include "debug.h"
#include "arm_timing.h"
#include "scheduler.h"

// Global flag for BIOS tracing
bool g_trace_bios = false;
// Global flag for tracing all instructions (not just BIOS)
bool g_trace_all = false;
uint32_t g_trace_max_instructions = 50000;

// External PC tracker for debug output from memory.cpp
extern uint32_t g_cpu_pc;


// Secondary decode function for ambiguous region (data processing/MUL/MLA overlap)
// Phase 1: New entry point for ambiguous region

ARMCPU::ARMCPU(CPU& cpu) : parentCPU(cpu), capstone_handle(0) {
    DEBUG_INFO("Initializing ARMCPU with parent CPU");
    if (cs_open(CS_ARCH_ARM, CS_MODE_ARM, &capstone_handle) != CS_ERR_OK) {
        DEBUG_ERROR("Failed to initialize Capstone for ARM mode");
    } else {
        cs_option(capstone_handle, CS_OPT_DETAIL, CS_OPT_ON);
    }
}

ARMCPU::~ARMCPU() {
    if (capstone_handle) {
        cs_close(&capstone_handle);
    }
}

// HACK - exception taken looks like a bodge ..
void ARMCPU::execute(uint32_t cycles) {
    exception_taken = false;
    while (cycles > 0) {
        // Check if we're still in ARM mode - if not, break out early
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            DEBUG_INFO("Mode switched to Thumb during execution, breaking out of ARM execution");
            break;
        }

        uint32_t pc = parentCPU.R()[15]; // Get current PC
        
        // Track invalid PC values
        if (pc >= 0x10000000) {
            uint64_t current_cycle = parentCPU.getScheduler() ? parentCPU.getScheduler()->getCurrentCycle() : 0;
            LOG_CRASH("[ARM ERROR] Invalid PC=0x%08X detected! Cycle=%llu\n", pc, current_cycle);
            LOG_CRASH("  R0-R7: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                   parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
                   parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7]);
            LOG_CRASH("  R8-R15: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                   parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11],
                   parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[15]);
            LOG_CRASH("  CPSR: %08X\n", parentCPU.CPSR());
            // Only log first occurrence
            static bool logged_invalid = false;
            if (!logged_invalid) {
                logged_invalid = true;
            } else {
                exit(1);
            }
        }
        
        // Log when we enter ROM space
        static bool in_rom = false;
        if (!in_rom && pc >= 0x08000000 && pc < 0x0E000000) {
            LOG_REGION("\n*** ENTERING ROM CODE at PC=0x%08X ***\n\n", pc);
            in_rom = true;
        }
        
        // Fetch instruction without charging wait cycles
        // ARM7TDMI has 3-stage pipeline: Fetch happens in parallel with previous instruction's execution
        // Only the execute stage costs cycles
        uint32_t instruction = parentCPU.getMemory().readDirectIO32(pc);

        // Debug IntrWait function (0x330-0x378)
        if (pc >= 0x330 && pc <= 0x378) {
            LOG_IRQ("\n[IntrWait TRACE] PC=0x%08X, instruction=0x%08X\n", pc, instruction);
            LOG_IRQ("  R0=0x%08X R1=0x%08X R2=0x%08X R3=0x%08X\n",
                   parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
            LOG_IRQ("  R4=0x%08X R12=0x%08X LR=0x%08X PC=0x%08X\n",
                   parentCPU.R()[4], parentCPU.R()[12], parentCPU.R()[14], parentCPU.R()[15]);
            LOG_IRQ("  CPSR=0x%08X (Z=%d N=%d C=%d V=%d)\n",
                   parentCPU.CPSR(),
                   parentCPU.getFlag(CPU::FLAG_Z), parentCPU.getFlag(CPU::FLAG_N),
                   parentCPU.getFlag(CPU::FLAG_C), parentCPU.getFlag(CPU::FLAG_V));
            uint16_t ie = parentCPU.getMemory().readDirectIO16(0x04000200);
            uint16_t if_reg = parentCPU.getMemory().readDirectIO16(0x04000202);
            uint8_t ime = parentCPU.getMemory().readDirectIO8(0x04000208);
            LOG_IRQ("  IE=0x%04X IF=0x%04X IME=0x%02X\n", ie, if_reg, ime);
        }

        g_cpu_pc = pc; // Track PC for debug output in memory.cpp
        executeInstruction(pc, instruction);
        if (exception_taken) {
            break;
        }
        
        DEBUG_INFO("PC now: 0x" + debug_to_hex_string(parentCPU.R()[15], 8));
        cycles -= 1; // Placeholder for cycle deduction
    }
}

// Calculate cycles for next instruction without executing it
uint32_t ARMCPU::calculateInstructionCycles(uint32_t instruction) {
    // Convert CPU registers to array format for the C function
    uint32_t registers[16];
    for (int i = 0; i < 16; i++) {
        registers[i] = parentCPU.R()[i];
    }
    
    uint32_t pc = parentCPU.R()[15];
    uint32_t cpsr = parentCPU.CPSR();
    return arm_calculate_instruction_cycles(instruction, pc, registers, cpsr);
}

const ARMCPU::CondFunc ARMCPU::condTable[16] = {
    &ARMCPU::cond_eq, // 0: EQ
    &ARMCPU::cond_ne, // 1: NE
    &ARMCPU::cond_cs, // 2: CS
    &ARMCPU::cond_cc, // 3: CC
    &ARMCPU::cond_mi, // 4: MI
    &ARMCPU::cond_pl, // 5: PL
    &ARMCPU::cond_vs, // 6: VS
    &ARMCPU::cond_vc, // 7: VC
    &ARMCPU::cond_hi, // 8: HI
    &ARMCPU::cond_ls, // 9: LS
    &ARMCPU::cond_ge, // 10: GE
    &ARMCPU::cond_lt, // 11: LT
    &ARMCPU::cond_gt, // 12: GT
    &ARMCPU::cond_le, // 13: LE
    &ARMCPU::cond_al, // 14: AL
    &ARMCPU::cond_nv  // 15: NV
};

void ARMCPU::executeInstruction(uint32_t pc, uint32_t instruction) {
    // Track PC regions and transitions
    static bool in_rom = false;
    static bool in_bios = true;
    static uint32_t last_pc = 0;
    static uint64_t instruction_count = 0;
    
    instruction_count++;
    
    // SP tracing for comparison with mGBA
    extern void trace_sp(uint32_t pc, uint32_t sp, const char* mode);
    trace_sp(pc, parentCPU.R()[13], "ARM");
    
    // Progress indicator every 100K instructions (DISABLED FOR PERFORMANCE)
    // if (instruction_count % 100000 == 0) {
    //     printf("[Progress] %llu instructions executed, PC=0x%08X\n", instruction_count, pc);
    // }
    
    bool pc_in_rom = (pc >= 0x08000000 && pc < 0x0E000000);
    bool pc_in_bios = (pc < 0x00004000);
    
    // Detailed BIOS tracing
    bool should_trace = (g_trace_bios && pc_in_bios) || (g_trace_all && instruction_count <= g_trace_max_instructions);
    
    if (should_trace) {
        // Read key I/O registers (use direct I/O to avoid affecting timing)
        uint16_t ie = parentCPU.getMemory().readDirectIO16(0x04000200);
        uint16_t irq_flags = parentCPU.getMemory().readDirectIO16(0x04000202);
        uint32_t ime = parentCPU.getMemory().readDirectIO32(0x04000208);
        
        // Print in mGBA-compatible compact format (matches the format from the trace script)
        LOG_TRACE_CAT("PC:%08X R00:%08X R01:%08X R02:%08X R03:%08X R04:%08X R05:%08X R06:%08X R07:%08X R08:%08X R09:%08X R10:%08X R11:%08X R12:%08X R13:%08X R14:%08X R15:%08X CPSR:%08X | IE:%04X IF:%04X IME:%08X\n",
               pc,
               parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
               parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7],
               parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11],
               parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[15],
               parentCPU.CPSR(),
               ie, irq_flags, ime);
        
        // Optional: Add disassembly on second line
        if (capstone_handle) {
            cs_insn* insn;
            size_t count = cs_disasm(capstone_handle,
                                     reinterpret_cast<const uint8_t*>(&instruction),
                                     sizeof(instruction),
                                     pc, 1, &insn);
            if (count > 0) {
                LOG_TRACE_CAT("     ; %s %s\n", insn[0].mnemonic, insn[0].op_str);
                cs_free(insn, count);
            }
        }
    }
    
    // Track BIOS exit
    if (!pc_in_bios && in_bios) {
        LOG_BIOS("\n*** [%llu] PC LEFT BIOS at 0x%08X (last BIOS PC: 0x%08X) ***\n", 
               instruction_count, pc, last_pc);
        in_bios = false;
    }
    
    // Track ROM entry (disabled for debugging crash)
    // static uint64_t rom_instructions = 0;
    // if (pc_in_rom && !in_rom) {
    //     printf("\n*** [%llu] PC ENTERED ROM REGION at 0x%08X (from 0x%08X) ***\n", 
    //            instruction_count, pc, last_pc);
    //     printf("*** First ROM instruction: 0x%08X ***\n\n", instruction);
    //     in_rom = true;
    //     rom_instructions = 0;
    // } else if (!pc_in_rom && in_rom) {
    //     printf("\n*** [%llu] PC LEFT ROM REGION at 0x%08X ***\n\n", instruction_count, pc);
    //     in_rom = false;
    // }
    // 
    // // Debug ROM execution - trace first 20 instructions
    // if (in_rom && rom_instructions < 20) {
    //     rom_instructions++;
    //     printf("[ROM #%llu @0x%08X] Instr: 0x%08X | R0=%08X R1=%08X R2=%08X R3=%08X R4=%08X R5=%08X | SP=%08X LR=%08X\n",
    //            rom_instructions, pc, instruction,
    //            parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
    //            parentCPU.R()[4], parentCPU.R()[5],
    //            parentCPU.R()[13], parentCPU.R()[14]);
    // }
    if (pc_in_rom && !in_rom) in_rom = true;
    else if (!pc_in_rom && in_rom) in_rom = false;
    
    // Debug BIOS execution (DISABLED FOR PERFORMANCE)
    // if ((in_bios && instruction_count <= 200) || (in_bios && pc >= 0x140 && pc <= 0x170)) {
    //     printf("[%3llu] BIOS PC=0x%08X: Instr=0x%08X | R0-R3=0x%08X 0x%08X 0x%08X 0x%08X | SP=0x%08X LR=0x%08X R12=0x%08X\n",
    //            instruction_count, pc, instruction,
    //            parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
    //            parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[12]);
    //     
    //     // Show specific details for critical instructions
    //     if (pc == 0x00000060 || pc == 0x00000064 || pc == 0x0000016C) {
    //         uint32_t sp = parentCPU.R()[13];
    //         printf("     -> Stack contents: [0x%08X]=0x%08X, [0x%08X]=0x%08X\n",
    //                sp, parentCPU.getMemory().read32(sp),
    //                sp+4, parentCPU.getMemory().read32(sp+4));
    //     }
    // }
    
    // TRACE IntrWait (DISABLED FOR PERFORMANCE)
    // static int intrwait_trace_count = 0;
    // if (in_bios && pc >= 0x358 && pc <= 0x374) {
    //     intrwait_trace_count++;
    // }
    
    last_pc = pc;

    // Print CPSR flags before disassembly (commented out - too verbose for normal use)
    // uint32_t cpsr = parentCPU.CPSR();
    // printf("[ARMCPU] CPSR flags before disasm: N:%d Z:%d C:%d V:%d (CPSR=0x%08X)\n",
    //     (cpsr >> 31) & 1, (cpsr >> 30) & 1, (cpsr >> 29) & 1, (cpsr >> 28) & 1, cpsr);

    // Debug: Trace BIOS IRQ handling at 0x18
    if (pc == 0x00000018) {
        static int irq_vec_count = 0;
        irq_vec_count++;
        LOG_IRQ("[BIOS IRQ #%d] PC=0x%08X Instr=0x%08X CPSR=0x%08X\n", 
               irq_vec_count, pc, instruction, parentCPU.CPSR());
        LOG_IRQ("  R0-R3: %08X %08X %08X %08X\n",
               parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
        LOG_IRQ("  LR=%08X SP=%08X\n", parentCPU.R()[14], parentCPU.R()[13]);
    }

    uint32_t index = (bits<27,20>(instruction) << 1) | ((instruction & 0x90) == 0x90); // bit 7 and 4 are set
    DEBUG_INFO("executeInstruction: PC=0x" + debug_to_hex_string(pc, 8) + 
                " Instruction=0x" + debug_to_hex_string(instruction, 8) + " using fn table index: 0x" + debug_to_hex_string(index, 3));

    // Capstone disassembly hook
    if (g_disassemble_enabled && capstone_handle) {
        cs_insn* insn;
        size_t count = cs_disasm(capstone_handle,
                                 reinterpret_cast<const uint8_t*>(&instruction),
                                 sizeof(instruction),
                                 pc, 1, &insn);
        if (count > 0) {
            printf("[DISASM][ARM] 0x%08X: %s %s\n", (unsigned int)pc, insn[0].mnemonic, insn[0].op_str);
            cs_free(insn, count);
        } else {
            printf("[DISASM][ARM] 0x%08X: <failed to disassemble>\n", (unsigned int)pc);
        }
    }

    // Check if condition is met before executing instruction
    uint8_t condition = bits<31, 28>(instruction);
    if ((condition == 0xE) || (ARMCPU::condTable[condition](parentCPU.CPSR() >> 28))) {
        (this->*arm_exec_table[index])(instruction);
    } else {
        DEBUG_INFO("Condition not met, skipping instruction, incrementing PC");
        parentCPU.R()[15] += 4;
    }
}

void ARMCPU::handleException(uint32_t vector_address, uint32_t new_mode, bool disable_irq, bool disable_fiq) {
    DEBUG_LOG("handleException ENTRY: vector=0x" + debug_to_hex_string(vector_address, 8) + ", new_mode=0x" + debug_to_hex_string(new_mode, 2) + ", PC=0x" + debug_to_hex_string(parentCPU.R()[15], 8));
    CPU::Mode old_mode = static_cast<CPU::Mode>(parentCPU.CPSR() & 0x1F);
    CPU::Mode new_mode_enum = static_cast<CPU::Mode>(new_mode & 0x1F);
    
    assert((new_mode & 0x1F) >= 0x10 && "Invalid new_mode in handleException");
    // Save the current CPSR (SPSR not supported in this implementation)
    uint32_t old_cpsr = parentCPU.CPSR(); UNUSED(old_cpsr);

    // Calculate the return address for LR
    // For SWI/SVC: LR = next instruction after SWI
    // - ARM mode: PC wasn't pre-incremented, so LR = PC + 4
    // - Thumb mode: PC was already incremented by 2, so LR = PC (which is already next instruction)
    // For other exceptions (IRQ, FIQ, etc.): LR = PC + 4 (to re-execute the interrupted instruction after adjustment)
    bool is_thumb = (old_cpsr & 0x20) != 0;
    uint32_t return_address;
    if (vector_address == 0x08) { // SWI vector
        // SWI return: LR points to instruction after SWI
        if (is_thumb) {
            return_address = parentCPU.R()[15]; // Already points to next instruction
        } else {
            return_address = parentCPU.R()[15] + 4; // ARM mode needs +4
        }
    } else {
        // Other exceptions: PC + 4 (standard)
        return_address = parentCPU.R()[15] + 4;
    }
    if (old_mode == CPU::USER || old_mode == CPU::SYS) {
        parentCPU.bankedLRUser() = parentCPU.R()[14]; // save User LR
    } else {
        parentCPU.bankedLR(old_mode) = parentCPU.R()[14]; // save banked LR for privileged mode
    }
    
    // Set LR for the NEW mode BEFORE swapping
    if (new_mode_enum == CPU::USER || new_mode_enum == CPU::SYS) {
        // Do not overwrite banked_r14_usr on exception entry
    } else if (new_mode_enum == CPU::SVC) {
        parentCPU.bankedLR(CPU::SVC) = return_address;
    } else if (new_mode_enum == CPU::UND) {
        parentCPU.bankedLR(CPU::UND) = return_address;
    } else if (new_mode_enum == CPU::IRQ) {
        parentCPU.bankedLR(CPU::IRQ) = return_address;
    } else if (new_mode_enum == CPU::FIQ) {
        parentCPU.bankedLR(CPU::FIQ) = return_address;
    } else if (new_mode_enum == CPU::ABT) {
        parentCPU.bankedLR(CPU::ABT) = return_address;
    } else {
        parentCPU.R()[14] = return_address;
    }

    // Now swap to the new mode
    parentCPU.setMode(new_mode_enum);
    
    // Save old CPSR to SPSR of the NEW mode (must be done AFTER mode switch)
    // This is critical for exception handlers to return correctly
    parentCPU.SPSR() = old_cpsr;
    DEBUG_LOG("Saved CPSR 0x" + debug_to_hex_string(old_cpsr, 8) + " to SPSR of mode 0x" + debug_to_hex_string(new_mode, 2));
    
    // Set IRQ/FIQ disable bits in CPSR and clear T bit (after mode switch)
    // ARM7TDMI: Exception handling ALWAYS switches to ARM state
    uint32_t new_cpsr = parentCPU.CPSR();
    new_cpsr &= ~0x20; // Clear T bit (bit 5) - switch to ARM state
    if (disable_irq) {
        new_cpsr |= 0x80; // Set I bit
    }
    if (disable_fiq) {
        new_cpsr |= 0x40; // Set F bit
    }
    parentCPU.CPSR() = new_cpsr;

    // Set the PC to the exception vector
    parentCPU.R()[15] = vector_address;
}

// ============================================================================
// Scheduler-Integrated Execution
// ============================================================================

void ARMCPU::executeOneInstruction() {
    // Check if we're still in ARM mode
    if (parentCPU.getFlag(CPU::FLAG_T)) {
        DEBUG_INFO("CPU switched to Thumb mode");
        return;
    }
    
    uint32_t pc = parentCPU.R()[15];
    
    // --- Open bus / BIOS prefetch tracking (ARM mode) ---
    // ARM7TDMI 3-stage pipeline: when instruction at A executes, the fetch
    // stage is reading A+8.  prefetch[1] in mGBA terms = instruction at A+8.
    // This is what appears on the data bus for open-bus reads.
    Memory& mem = parentCPU.getMemory();
    parentCPU.openBusPrefetch = mem.readDirectIO32(pc + 8);

    // Track whether the CPU is executing from the BIOS region.
    // When the CPU leaves BIOS, save the current prefetch as the BIOS latch.
    bool inBios = (pc < 0x4000);
    if (inBios) {
        mem.biosPrefetch = parentCPU.openBusPrefetch;
    }
    mem.cpuInBios = inBios;

    // Fetch instruction without charging wait cycles (pipelined - happens during previous instruction's execution)
    uint32_t instruction = mem.readDirectIO32(pc);

    // Targeted trace around Sonic branch to UNKNOWN region
    if (pc >= 0x08097240 && pc <= 0x08097260) {
        static int trace_count = 0;
        if (trace_count++ < 20) {
            printf("[TRACE ROM] PC=0x%08X instr=0x%08X CPSR=0x%08X R0=0x%08X R1=0x%08X R2=0x%08X R3=0x%08X LR=0x%08X\n",
                   pc, instruction, parentCPU.CPSR(), parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3], parentCPU.R()[14]);
        }
    }
    
    // Detect infinite loop (b . or 0xEAFFFFFE) - used by test ROMs to indicate completion
    static bool infinite_loop_detected = false;
    if (instruction == 0xEAFFFFFE && !infinite_loop_detected) {
        infinite_loop_detected = true;
        printf("\n*** INFINITE LOOP DETECTED at PC=0x%08X ***\n", pc);
        printf("R0=0x%08X R1=0x%08X R2=0x%08X R3=0x%08X\n", 
               parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
        printf("R4=0x%08X R5=0x%08X R6=0x%08X R7=0x%08X\n",
               parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7]);
        printf("R8=0x%08X R9=0x%08X R10=0x%08X R11=0x%08X\n",
               parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11]);
        printf("R12=0x%08X (TEST ID) SP=0x%08X LR=0x%08X PC=0x%08X\n",
               parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[15]);
        printf("CPSR=0x%08X\n\n", parentCPU.CPSR());
    }
    
    // Calculate how many cycles this instruction will take
    uint32_t instruction_cycles = calculateInstructionCycles(instruction);
    
    // Track PC for debug output in memory.cpp
    g_cpu_pc = pc;
    mem.cpuIsThumb = false;
    
    // Debug: Log first few instructions (DISABLED FOR PERFORMANCE)
    // static int debug_count = 0;
    // if (debug_count < 10) {
    //     printf("[ARM EXEC] PC=0x%08X instr=0x%08X cycles=%u\n", pc, instruction, instruction_cycles);
    //     fflush(stdout);
    //     debug_count++;
    // }
    
    // Execute the instruction
    // Bracket with begin/endInstructionCycles so data access wait cycles
    // accumulate in pendingDataCycles rather than advancing the scheduler
    // mid-instruction. This matches mGBA's local currentCycles model:
    // timer enable/read during execution see instruction-boundary cycle values.
    mem.beginInstructionCycles();
    executeInstruction(pc, instruction);
    uint32_t dataCycles = mem.endInstructionCycles();
    
    // Advance scheduler by instruction execution cycles + fetch waits
    // + accumulated data access cycles from read/write operations.
    //
    // After data transfers (LDR/STR/LDM/STM/SWP) and multiplies, the next
    // instruction fetch is non-sequential because the data bus was used,
    // disrupting the prefetch pipeline. For all other instructions, the
    // fetch is sequential. This matches mGBA's POST_BODY macros:
    //   ARM_LOAD_POST_BODY:  += activeNonseqCycles32 - activeSeqCycles32
    //   ARM_STORE_POST_BODY: += activeNonseqCycles32 - activeSeqCycles32
    uint32_t bits27_26 = (instruction >> 26) & 0x3;
    uint32_t bits27_25 = (instruction >> 25) & 0x7;
    bool isDataTransfer = (bits27_26 == 0x01)                     // LDR/STR/LDRB/STRB
                       || (bits27_25 == 0x4)                       // LDM/STM
                       || ((bits27_25 == 0x0) &&                   // Halfword/signed xfer (register encoding only)
                           (instruction & 0x00000090) == 0x00000090 &&
                           (instruction & 0x00000060) != 0x00000000)
                       || ((instruction & 0x0FB00FF0) == 0x01000090)  // SWP
                       || ((instruction & 0x0FC000F0) == 0x00000090)  // MUL/MLA
                       || ((instruction & 0x0F8000F0) == 0x00800090); // Long MUL (uses nonseq like store)
    
    // Compute base fetch cost
    uint32_t fetchCycles = isDataTransfer
        ? mem.getNonseqWaitCycles32(pc)
        : mem.getSeqWaitCycles32(pc);
    
    // Game Pak prefetch buffer: when executing from ROM with prefetch enabled
    // and data accessed non-ROM memory, the prefetch unit fills during the
    // stall. The benefit is applied to fetchCycles (next instruction fetch)
    // since the prefetch buffer handles upcoming instruction reads.
    // dataCycles remain unchanged — the data access still took its full time.
    if (isDataTransfer && mem.prefetchEnabled && mem.hadNonRomAccess()
        && !mem.hadRomAccess()) {
        uint8_t pcRegion = (pc >> 24) & 0xFF;
        if (pcRegion >= 0x08 && pcRegion <= 0x0D) {
            // S16 fetch cost for current ROM region (total = extra + 1 base)
            uint32_t sFetchCost = mem.getSeqWaitCycles16(pc) + 1;
            // How many S16 fetches completed during the NON-ROM data stall?
            // Only non-ROM data cycles allow prefetch (ROM accesses use the bus)
            uint32_t nonRomStall = mem.getNonRomDataCycles();
            uint32_t completedFetches = nonRomStall / sFetchCost;
            // ARM 32-bit fetch = 2 halfword fetches from ROM.
            // Each prefetched halfword replaces one S16 ROM access.
            // Also, first fetch converts from N→S (savings = N16-S16 extra waits).
            if (completedFetches >= 2) {
                // Both halves of the 32-bit fetch are prefetched: fetch is free
                fetchCycles = 0;
            } else if (completedFetches == 1) {
                // One halfword prefetched: save the N→S conversion on first half
                fetchCycles = mem.getSeqWaitCycles32(pc);
            }
            // If 0 fetches completed, no prefetch benefit
        }
    }
    
    // Detect branches: if PC changed non-sequentially, the ARM7TDMI must
    // refill its 3-stage pipeline at the target address.  This costs 1N + 1S
    // fetch cycles at the target (matching mGBA's ARMWritePC/ThumbWritePC).
    // The 2 internal cycles for the refill are already included in
    // instruction_cycles (arm_timing.c returns 3 for B/BL/BX/SWI).
    uint32_t branchRefillCycles = 0;
    uint32_t newPc = parentCPU.R()[15];
    if (newPc != pc + 4) {
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            // BX switched to Thumb mode — refill uses 16-bit fetches
            branchRefillCycles = mem.getNonseqWaitCycles16(newPc)
                               + mem.getSeqWaitCycles16(newPc);
        } else {
            // Still in ARM mode — refill uses 32-bit fetches
            branchRefillCycles = mem.getNonseqWaitCycles32(newPc)
                               + mem.getSeqWaitCycles32(newPc);
        }
        mem.flushPrefetch();
    }
    
    parentCPU.advanceCycles(instruction_cycles + fetchCycles + dataCycles + branchRefillCycles);}