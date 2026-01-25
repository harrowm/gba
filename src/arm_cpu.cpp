
#include <cstdint>
#include <cassert>
#include <capstone/capstone.h>
#include "arm_cpu.h"
#include "debug.h"
#include "timing.h"
#include "arm_timing.h"
#include "scheduler.h"

// Global flag for BIOS tracing
bool g_trace_bios = false;
// Global flag for tracing all instructions (not just BIOS)
bool g_trace_all = false;
uint32_t g_trace_max_instructions = 50000;


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
            printf("[ARM ERROR] Invalid PC=0x%08X detected! Cycle=%llu\n", pc, current_cycle);
            printf("  R0-R7: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                   parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3],
                   parentCPU.R()[4], parentCPU.R()[5], parentCPU.R()[6], parentCPU.R()[7]);
            printf("  R8-R15: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                   parentCPU.R()[8], parentCPU.R()[9], parentCPU.R()[10], parentCPU.R()[11],
                   parentCPU.R()[12], parentCPU.R()[13], parentCPU.R()[14], parentCPU.R()[15]);
            printf("  CPSR: %08X\n", parentCPU.CPSR());
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
            printf("\n*** ENTERING ROM CODE at PC=0x%08X ***\n\n", pc);
            in_rom = true;
        }
        
        // Fetch instruction without charging wait cycles
        // ARM7TDMI has 3-stage pipeline: Fetch happens in parallel with previous instruction's execution
        // Only the execute stage costs cycles
        uint32_t instruction = parentCPU.getMemory().readDirectIO32(pc);

        // Debug IntrWait function (0x330-0x378)
        if (pc >= 0x330 && pc <= 0x378) {
            printf("\n[IntrWait TRACE] PC=0x%08X, instruction=0x%08X\n", pc, instruction);
            printf("  R0=0x%08X R1=0x%08X R2=0x%08X R3=0x%08X\n",
                   parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
            printf("  R4=0x%08X R12=0x%08X LR=0x%08X PC=0x%08X\n",
                   parentCPU.R()[4], parentCPU.R()[12], parentCPU.R()[14], parentCPU.R()[15]);
            printf("  CPSR=0x%08X (Z=%d N=%d C=%d V=%d)\n",
                   parentCPU.CPSR(),
                   parentCPU.getFlag(CPU::FLAG_Z), parentCPU.getFlag(CPU::FLAG_N),
                   parentCPU.getFlag(CPU::FLAG_C), parentCPU.getFlag(CPU::FLAG_V));
            uint16_t ie = parentCPU.getMemory().readDirectIO16(0x04000200);
            uint16_t if_reg = parentCPU.getMemory().readDirectIO16(0x04000202);
            uint8_t ime = parentCPU.getMemory().readDirectIO8(0x04000208);
            printf("  IE=0x%04X IF=0x%04X IME=0x%02X\n", ie, if_reg, ime);
        }

        executeInstruction(pc, instruction);
        if (exception_taken) {
            break;
        }
        
        DEBUG_INFO("PC now: 0x" + debug_to_hex_string(parentCPU.R()[15], 8));
        cycles -= 1; // Placeholder for cycle deduction
    }
}

// New timing-aware execution method
void ARMCPU::executeWithTiming(uint32_t cycles, TimingState* timing) {
    // Use macro-based debug system
    
    while (cycles > 0) {
        exception_taken = false;
        // Check if we're still in ARM mode - if not, break out early
        if (parentCPU.getFlag(CPU::FLAG_T)) {
            DEBUG_INFO("Mode switched to Thumb during timing execution, breaking out of ARM execution");
            break;
        }
        
        // Calculate cycles until next timing event
        uint32_t cycles_until_event = timing_cycles_until_next_event(timing);
        
        // Fetch next instruction to determine its cycle cost
        uint32_t pc = parentCPU.R()[15];
        
        // Fetch without charging wait cycles (pipelined)
        uint32_t instruction = parentCPU.getMemory().readDirectIO32(pc);
        
        uint32_t instruction_cycles = calculateInstructionCycles(instruction);
        
        // Use debug macros for detailed instruction logging
        DEBUG_INFO("Next ARM instruction: 0x" + debug_to_hex_string(instruction, 8) 
                  + " at PC: 0x" + debug_to_hex_string(pc, 8)
                  + " will take " + std::to_string(instruction_cycles) + " cycles");
        DEBUG_INFO("Cycles until next event: " + std::to_string(cycles_until_event)); 
        
        // Check if instruction will complete before next timing event
        if (instruction_cycles <= cycles_until_event) {
            // Execute instruction normally with cache
            executeInstruction(pc, instruction);
        
            // Update timing
            timing_advance(timing, instruction_cycles);
            cycles -= instruction_cycles;
            
        } else {
            // Process timing event first, then continue
            DEBUG_INFO("Processing timing event before executing instruction");
            timing_advance(timing, cycles_until_event);
            timing_process_timer_events(timing);
            timing_process_video_events(timing);
            cycles -= cycles_until_event;
        }
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
    
    // Progress indicator every 100K instructions (DISABLED FOR PERFORMANCE)
    // if (instruction_count % 100000 == 0) {
    //     printf("[Progress] %llu instructions executed, PC=0x%08X\n", instruction_count, pc);
    // }
    
    bool pc_in_rom = (pc >= 0x08000000 && pc < 0x0E000000);
    bool pc_in_bios = (pc < 0x00004000);
    bool pc_in_iwram = (pc >= 0x03000000 && pc < 0x03008000);
    bool pc_in_ewram = (pc >= 0x02000000 && pc < 0x02040000);
    
    // Detailed BIOS tracing
    bool should_trace = (g_trace_bios && pc_in_bios) || (g_trace_all && instruction_count <= g_trace_max_instructions);
    
    if (should_trace) {
        // Read key I/O registers (use direct I/O to avoid affecting timing)
        uint16_t ie = parentCPU.getMemory().readDirectIO16(0x04000200);
        uint16_t irq_flags = parentCPU.getMemory().readDirectIO16(0x04000202);
        uint32_t ime = parentCPU.getMemory().readDirectIO32(0x04000208);
        
        // Print in mGBA-compatible compact format (matches the format from the trace script)
        printf("PC:%08X R00:%08X R01:%08X R02:%08X R03:%08X R04:%08X R05:%08X R06:%08X R07:%08X R08:%08X R09:%08X R10:%08X R11:%08X R12:%08X R13:%08X R14:%08X R15:%08X CPSR:%08X | IE:%04X IF:%04X IME:%08X\n",
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
                printf("     ; %s %s\n", insn[0].mnemonic, insn[0].op_str);
                cs_free(insn, count);
            }
        }
    }
    
    // Track BIOS exit
    if (!pc_in_bios && in_bios) {
        printf("\n*** [%llu] PC LEFT BIOS at 0x%08X (last BIOS PC: 0x%08X) ***\n", 
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
        printf("[BIOS IRQ #%d] PC=0x%08X Instr=0x%08X CPSR=0x%08X\n", 
               irq_vec_count, pc, instruction, parentCPU.CPSR());
        printf("  R0-R3: %08X %08X %08X %08X\n",
               parentCPU.R()[0], parentCPU.R()[1], parentCPU.R()[2], parentCPU.R()[3]);
        printf("  LR=%08X SP=%08X\n", parentCPU.R()[14], parentCPU.R()[13]);
        fflush(stdout);
    }

    uint32_t index = (bits<27,20>(instruction) << 1) | ((instruction & 0x90) == 0x90); // bit 7 and 4 are set
    DEBUG_INFO("executeInstruction: PC=0x" + debug_to_hex_string(pc, 8) + 
                " Instruction=0x" + debug_to_hex_string(instruction, 8) + " using fn table index: 0x" + debug_to_hex_string(index, 3));

    // Capstone disassembly hook
    extern bool g_disassemble_enabled;
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
    
    assert((new_mode & 0x1F) >= 0x10 && (new_mode & 0x1F) <= 0x1F && "Invalid new_mode in handleException");
    // Save the current CPSR (SPSR not supported in this implementation)
    uint32_t old_cpsr = parentCPU.CPSR(); UNUSED(old_cpsr);

    // Calculate the return address for LR
    // NOTE: ARM architectural requirement: LR for exception modes is always set to PC+4 (address of next instruction).
    // Even without a pipeline, exception handlers expect LR = PC+4.
    uint32_t return_address = parentCPU.R()[15] + 4;
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
    
    // Set IRQ/FIQ disable bits in CPSR (after mode switch)
    uint32_t new_cpsr = parentCPU.CPSR();
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
    
    // Fetch instruction without charging wait cycles (pipelined - happens during previous instruction's execution)
    uint32_t instruction = parentCPU.getMemory().readDirectIO32(pc);
    
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
    
    // Debug: Log first few instructions (DISABLED FOR PERFORMANCE)
    // static int debug_count = 0;
    // if (debug_count < 10) {
    //     printf("[ARM EXEC] PC=0x%08X instr=0x%08X cycles=%u\n", pc, instruction, instruction_cycles);
    //     fflush(stdout);
    //     debug_count++;
    // }
    
    // Execute the instruction
    executeInstruction(pc, instruction);
    
    // Advance scheduler by instruction execution cycles
    // Note: Memory access cycles are already handled by memory.cpp addWaitCycles()
    // This adds the CPU execution cycles on top of memory wait states
    parentCPU.advanceCycles(instruction_cycles);
}

