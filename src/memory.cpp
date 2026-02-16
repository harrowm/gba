// Memory class implementation for GBA emulator (region pointer table version)
#include "memory.h"
#include "cpu.h"
#include "scheduler.h"
#include "timer_controller.h"
#include "dma.h"
#include "apu.h"
#include "interrupt.h"
#include "debug.h"
#include <cstring>
#include <cstdint>
#include <cstdlib>

// External globals for watchpoint logging
extern uint32_t g_current_frame;
extern uint64_t g_total_instruction_count;
extern uint32_t g_cpu_pc; // For debug tracking - set by CPU before each instruction

// Watchpoint configuration
#define WATCHPOINT_ADDR 0x03007EA0
#define WATCHPOINT_ENABLED 0

// Helper: allocate memory or abort on failure (emulator cannot recover from OOM)
static uint8_t* checked_malloc(size_t size) {
    auto* ptr = static_cast<uint8_t*>(std::malloc(size));
    if (!ptr) {
        fprintf(stderr, "FATAL: Memory allocation failed (%zu bytes)\n", size);
        std::abort();
    }
    return ptr;
}

static uint8_t* checked_calloc(size_t count, size_t size) {
    auto* ptr = static_cast<uint8_t*>(std::calloc(count, size));
    if (!ptr) {
        fprintf(stderr, "FATAL: Memory allocation failed (%zu * %zu bytes)\n", count, size);
        std::abort();
    }
    return ptr;
}

Memory::Memory(bool testMode) {
    if (testMode) {
        // Only allocate and map test RAM at 0x00000000 (32KB)
        test_ram = checked_malloc(32 * 1024);
        
        // Initialize with infinite loop instruction (ARM: B #-8, opcode 0xEAFFFFFE)
        // This provides safe default code if no test instructions are loaded
        for (uint32_t i = 0; i < 32 * 1024; i += 4) {
            test_ram[i + 0] = 0xFE;  // Little-endian 0xEAFFFFFE
            test_ram[i + 1] = 0xFF;
            test_ram[i + 2] = 0xFF;
            test_ram[i + 3] = 0xEA;
        }
        
        for (uint32_t addr = 0x00000000; addr < 0x00008000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = test_ram + (addr - 0x00000000);
        
        // Allocate IO region even in test mode (needed for video timing tests)
        io = checked_malloc(1 * 1024);
        memset(io, 0, 1 * 1024);  // Zero-initialize
        regionTable[0x04000000 / BLOCK_SIZE] = io;
        
        // Allocate palette RAM in test mode (GPU needs it for rendering)
        palette = checked_malloc(1 * 1024);
        memset(palette, 0, 1 * 1024);  // Zero-initialize  
        for (uint32_t addr = 0x05000000; addr < 0x05000400; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = palette;
        
        // All other region pointers remain null
        bios = wram = iwram = vram = oam = rom = sram = nullptr;

    } else {
        // --- BIOS: 16KB at 0x00000000 ---
        bios = checked_malloc(16 * 1024);
        regionTable[0x00000000 / BLOCK_SIZE] = bios;
        // Load BIOS file - try relative path first, then absolute path
        {
            FILE* biosFile = fopen("assets/bios.bin", "rb");
            if (!biosFile) {
                biosFile = fopen("/Users/malcolm/gba/assets/bios.bin", "rb");
            }
            if (biosFile) {
                size_t read = fread(bios, 1, 16 * 1024, biosFile);
                fclose(biosFile);
                if (read < 16 * 1024) {
                    fprintf(stderr, "WARNING: BIOS file too small (%zu bytes), padding with zeros\n", read);
                    memset(bios + read, 0, 16 * 1024 - read);
                }
                printf("✓ BIOS loaded successfully: %zu bytes (testMode=false)\n", read);
                printf("✓ First BIOS instruction: 0x%02X%02X%02X%02X\n", 
                       bios[0], bios[1], bios[2], bios[3]);
            } else {
                fprintf(stderr, "ERROR: Failed to open BIOS file: assets/bios.bin\n");
                fprintf(stderr, "ERROR: BIOS is required for proper ROM execution\n");
                fprintf(stderr, "       Filling BIOS area with dummy data\n");
                memset(bios, 0x1, 16 * 1024);
            }
        }

        // --- WRAM: 256KB at 0x02000000, mirrored every 256KB throughout 0x02000000-0x02FFFFFF ---
        wram = checked_calloc(256 * 1024, 1);
        for (uint32_t addr = 0x02000000; addr < 0x03000000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = wram + ((addr - 0x02000000) % (256 * 1024));

        // --- IWRAM: 32KB at 0x03000000, mirrored throughout 0x03000000-0x03FFFFFF ---
        iwram = checked_calloc(32 * 1024, 1);
        // Mirror IWRAM every 64KB across the entire 16MB range
        // Each 64KB block in the 0x03000000-0x03FFFFFF range should point to IWRAM
        // but the actual IWRAM is only 32KB, so we need special handling in the read/write functions
        for (uint32_t addr = 0x03000000; addr < 0x04000000; addr += BLOCK_SIZE) {
            regionTable[addr / BLOCK_SIZE] = iwram;
        }

        // --- I/O: Allocate full block size (64KB) to match regionTable block granularity.
        // The GBA I/O register space is 0x04000000-0x04000301, but games may
        // access undocumented registers (e.g. 0x04000410, 0x04000800) and the
        // regionTable maps a full 64KB block to this pointer.
        io = checked_malloc(BLOCK_SIZE);
        memset(io, 0, BLOCK_SIZE);  // Zero-initialize IO memory
        regionTable[0x04000000 / BLOCK_SIZE] = io;
        // Initialize critical boot-related registers for clean BIOS boot
        io[0x000] = 0x80; // DISPCNT low byte: Forced blank (bit 7) set on power-on
        io[0x001] = 0x00; // DISPCNT high byte
        io[0x300] = 0x00; // POSTFLG: Boot Flag (0=First boot from power-on, 1=Further boot/reset)
        io[0x301] = 0x00; // HALTCNT: Power Down Control
        io[0x204] = 0x00; // WAITCNT: Game Pak Waitstate Control
        io[0x200] = 0x00; // IE: Interrupt Enable Register
        io[0x202] = 0x00; // IF: Interrupt Request Flags
        io[0x208] = 0x00; // IME: Interrupt Master Enable
        io[0x130] = 0xFF; // KEYINPUT low byte: All buttons unpressed
        io[0x131] = 0x03; // KEYINPUT high byte: All buttons unpressed (0x03FF)

        // --- Palette RAM: 1KB at 0x05000000, mirrored throughout 0x05000000-0x05FFFFFF ---
        // Allocate full block to prevent overflow from offset calculations
        palette = checked_malloc(BLOCK_SIZE);
        memset(palette, 0, BLOCK_SIZE);
        for (uint32_t addr = 0x05000000; addr < 0x06000000; addr += BLOCK_SIZE)
            regionTable[addr / BLOCK_SIZE] = palette;

        // --- VRAM: 96KB at 0x06000000, mirrored in 128KB period throughout 0x06000000-0x06FFFFFF ---
        // 128KB period: first 64KB = BG VRAM, second 64KB = OBJ VRAM (32KB real + 32KB mirror)
        // get_region_base handles the 32KB fold-back within each OBJ block
        vram = checked_malloc(96 * 1024);
        memset(vram, 0, 96 * 1024);
        for (uint32_t addr = 0x06000000; addr < 0x07000000; addr += BLOCK_SIZE) {
            uint32_t periodOffset = (addr - 0x06000000) % 0x20000; // 128KB period
            if (periodOffset < 0x10000)
                regionTable[addr / BLOCK_SIZE] = vram;              // BG VRAM block
            else
                regionTable[addr / BLOCK_SIZE] = vram + 0x10000;    // OBJ VRAM block
        }

        // --- OAM: 1KB at 0x07000000, mirrored throughout 0x07000000-0x07FFFFFF ---
        // Allocate full block to prevent overflow (get_region_base handles 1KB mirroring)
        oam = checked_malloc(BLOCK_SIZE);
        memset(oam, 0, BLOCK_SIZE);
        for (uint32_t addr = 0x07000000; addr < 0x08000000; addr += BLOCK_SIZE)
            regionTable[addr / BLOCK_SIZE] = oam;

        // --- Game Pak ROM: up to 32MB at 0x08000000 ---
        rom = checked_malloc(32 * 1024 * 1024);
        memset(rom, 0, 32 * 1024 * 1024);  // Initialize with zeros
        for (uint32_t addr = 0x08000000; addr < 0x0A000000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = rom + (addr - 0x08000000);

        // --- Game Pak SRAM: 64KB at 0x0E000000, mirrored throughout 0x0E-0x0F ---
        // SRAM has an 8-bit bus; read16/read32 handle byte replication
        sram = checked_malloc(64 * 1024);
        memset(sram, 0, 64 * 1024);
        for (uint32_t addr = 0x0E000000; addr < 0x10000000; addr += BLOCK_SIZE)
            regionTable[addr / BLOCK_SIZE] = sram;

        // Test RAM not used in normal mode
        test_ram = nullptr;
    }
    
    // Initialize wait state tables (matching mGBA's model)
    initWaitStateTables();
}

Memory::~Memory() {
    std::free(bios);
    std::free(wram);
    std::free(iwram);
    std::free(io);
    std::free(palette);
    std::free(vram);
    std::free(oam);
    std::free(rom);
    std::free(sram);
    std::free(test_ram);
}

// Helper: get region base and offset, handling VRAM/OAM mirroring

inline uint8_t* get_region_base(uint8_t* const* regionTable, uint32_t address, uint32_t& offset) {
    uint32_t block = (address & 0x0FFFFFFF) / Memory::BLOCK_SIZE;
    uint8_t* base = regionTable[block];
    offset = address % Memory::BLOCK_SIZE;
    uint32_t original_offset = offset;
    
    // IWRAM mirroring: 32KB at 0x03000000, mirrored every 32KB in 0x03000000-0x03FFFFFF
    if (address >= 0x03000000 && address < 0x04000000 && base) {
        offset = (address - 0x03000000) % 0x8000; // Mirror every 32KB
        
        // Debug: Log mirroring for IntrWait check address
        static int mirror_log_count = 0;
        if ((address == 0x03FFFFF8 || (address >= 0x03007FF8 && address <= 0x03007FFC)) && mirror_log_count++ < 20) {
            LOG_TRACE_CAT("[IWRAM MIRROR] addr=0x%08X → block=%u orig_offset=0x%X final_offset=0x%X (maps to 0x%08X)\n",
                   address, block, original_offset, offset, 0x03000000 + offset);
        }
        // Bounds check for IWRAM
        if (offset > 0x7FFC) {
            LOG_CRASH("[IWRAM OOB] addr=0x%08X offset=0x%X (max 0x7FFF)\n", address, offset);
        }
    }
    // VRAM OBJ 32KB mirroring: within each OBJ block (second 64KB of 128KB period),
    // offsets 0x8000-0xFFFF fold back to 0x0000-0x7FFF (32KB real OBJ VRAM)
    if (address >= 0x06000000 && address < 0x07000000 && base) {
        uint32_t vramPeriodOffset = (address - 0x06000000) % 0x20000;
        if (vramPeriodOffset >= 0x10000) {
            offset = offset & 0x7FFF; // Fold to 32KB OBJ area
        }
    }
    // Palette RAM mirroring: 1KB at 0x05000000, mirrored across entire 0x05 range
    if (address >= 0x05000000 && address < 0x06000000 && base) {
        offset = (address - 0x05000000) & 0x3FF; // 1KB mirror
    }
    // OAM mirroring: 1KB at 0x07000000, mirrored across entire 0x07 range
    if (address >= 0x07000000 && address < 0x08000000 && base) {
        offset = (address - 0x07000000) & 0x3FF; // 1KB mirror
    }
    return base;
}

uint8_t Memory::read8(uint32_t address) const {
    addWaitCycles(address, 8);

    // --- BIOS region protection (0x00000000 - 0x00FFFFFF) ---
    if (bios && (address >> 24) == 0x00) {
        if (address >= 0x4000) {
            uint32_t ob = cpu ? cpu->openBusPrefetch : 0;
            return (ob >> ((address & 3) * 8)) & 0xFF;
        }
        if (!cpuInBios) {
            return (biosPrefetch >> ((address & 3) * 8)) & 0xFF;
        }
    }

    // --- I/O register region: derive from ioRead16 ---
    if ((address >> 24) == 0x04 && (address & 0x00FF0000) == 0) {
        uint16_t offset = address & 0xFFFE;  // halfword-aligned offset within 64KB I/O block
        uint16_t val16 = ioRead16(offset);
        uint8_t result = (address & 1) ? (val16 >> 8) : (val16 & 0xFF);
        return result;
    }

    uint32_t offset;
    const uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) {
        // ROM open bus: out-of-bounds ROM reads return halfword address reflection
        uint32_t region = address >> 24;
        if (region >= 0x08 && region <= 0x0D) {
            uint16_t openbus = (address >> 1);
            return (address & 1) ? (openbus >> 8) : (openbus & 0xFF);
        }
        // General open bus: return byte from CPU prefetch pipeline
        uint32_t ob = cpu ? cpu->openBusPrefetch : 0;
        return (ob >> ((address & 3) * 8)) & 0xFF;
    }
    // Debug: Print reads from logo and entry point
    // if (address == 0x0800009C || address == 0x080000B4) {
    //     printf("[Memory::read8] Read from 0x%08X: 0x%02X\n", address, base[offset]);
    // }
    return base[offset];
}

void Memory::write8(uint32_t address, uint8_t value) {
    // mGBA debug interface: byte writes to debug string buffer (no wait cycles)
    if ((address & 0xFFFFF000) == 0x04FFF000) {
        uint32_t off = address & 0xFFF;
        if (off >= 0x600 && off < 0x700) {
            uint32_t idx = off - 0x600;
            if (idx < 255) {
                mgbaDebugString[idx] = value;
            }
        }
        return;
    }
    addWaitCycles(address, 8);
    
    // GBA read-only regions: writes are silently ignored
    // BIOS ROM (0x00) and Game Pak ROM (0x08-0x0D) are not writable
    uint32_t region = address >> 24;
    if (region == 0x00 || (region >= 0x08 && region <= 0x0D)) return;
    
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
#if WATCHPOINT_ENABLED
    // Watchpoint: Monitor ALL write8 to 0x03007EA0-0x03007EA7
    if (address >= 0x03000000 && address < 0x04000000) {
        uint32_t iwram_off = address & 0x7FFF;
        if (iwram_off >= 0x7EA0 && iwram_off <= 0x7EA7) {
            fprintf(stderr, "[WATCH8] frame=%u instr=%llu addr=0x%08X val=0x%02X\n",
                    g_current_frame, (unsigned long long)g_total_instruction_count, address, value);
        }
    }
#endif
    
    // Feature detection: OAM writes (sprite data)
    static bool oam_logged = false;
    if (address >= 0x07000000 && address < 0x07000400 && !oam_logged) {
        LOG_FEATURE("[FEATURE] ROM writing to OAM (sprite attribute memory) at 0x%08X\n", address);
        oam_logged = true;
    }
    
    // Debug: Track VRAM writes (commented out - verified working)
    // if (address >= 0x06000000 && address < 0x06018000) {
    //     printf("[VRAM Write8] Address: 0x%08X, Value: 0x%02X\n", address, value);
    // }
    
    // Debug: Track POSTFLG writes
    if (address == 0x04000300) {
        LOG_FEATURE("[Memory::write8] POSTFLG write: 0x%02X (was 0x%02X)\n", value, base[offset]);
    }
    
    // HALTCNT (0x04000301): CPU power-down control
    // Bit 7: 0 = HALT (stop CPU until interrupt), 1 = STOP (deep sleep)
    // The BIOS writes here during SWI 0x02 (Halt), SWI 0x04 (IntrWait),
    // SWI 0x05 (VBlankIntrWait). On real hardware this stops the CPU clock
    // until an enabled interrupt fires.
    // IME (0x04000208): Interrupt Master Enable
    // The BIOS IntrWait uses strb to toggle IME. When IME transitions
    // to 1, we must schedule an IRQ check for any pending IE & IF match.
    if (address == 0x04000208) {
        base[offset] = value;
        if ((value & 1) && interruptController) {
            interruptController->scheduleIRQCheck();
        }
        return;
    }

    // IF register (0x04000202-0x04000203): write-1-to-clear semantics for byte writes.
    // The BIOS IRQ handler uses STRB to acknowledge interrupts (e.g. STRB R1, [R3, #0x202]).
    // Without this, the interrupt flag is never cleared and IRQs fire endlessly.
    if (address == 0x04000202 || address == 0x04000203) {
        base[offset] = base[offset] & ~value;
        return;
    }

    if (address == 0x04000301) {
        if (cpu) {
            if ((value & 0x80) == 0) {
                // HALT mode: stop CPU until interrupt
                cpu->halt();
            }
            // STOP mode (bit 7 set) is not commonly used by games
        }
        base[offset] = value;
        return;
    }
    
    // KEYINPUT (0x04000130-0x04000131) is READ-ONLY - ignore writes
    if (address == 0x04000130 || address == 0x04000131) {
        return;  // Silently ignore writes to KEYINPUT
    }
    
    // GBA bus width restrictions for byte writes:
    // VRAM, Palette RAM, and OAM have 16-bit buses - no 8-bit write support.
    // VRAM BG area: byte write duplicates to aligned halfword (val | val<<8)
    // VRAM OBJ area: byte writes are IGNORED
    // Palette RAM: byte write duplicates to aligned halfword
    // OAM: byte writes are IGNORED
    
    // VRAM (0x06000000-0x06FFFFFF)
    if (address >= 0x06000000 && address < 0x07000000) {
        // Determine if BG or OBJ VRAM after mirroring (128KB period)
        uint32_t vramOffset = (address - 0x06000000) % 0x20000;
        if (vramOffset >= 0x10000) {
            return; // OBJ VRAM: byte writes ignored
        }
        // BG VRAM: duplicate byte to aligned halfword
        uint32_t alignedOffset = offset & ~1u;
        base[alignedOffset] = value;
        base[alignedOffset + 1] = value;
        return;
    }
    
    // Palette RAM (0x05000000-0x05FFFFFF)
    if (address >= 0x05000000 && address < 0x06000000) {
        // Duplicate byte to aligned halfword
        uint32_t alignedOffset = offset & ~1u;
        base[alignedOffset] = value;
        base[alignedOffset + 1] = value;
        return;
    }
    
    // OAM (0x07000000-0x07FFFFFF)
    if (address >= 0x07000000 && address < 0x08000000) {
        return; // OAM: byte writes ignored
    }
    
    base[offset] = value;
}

uint16_t Memory::read16(uint32_t address) const {
    // mGBA debug interface reads (no wait cycles - virtual registers)
    if ((address & 0xFFFFF000) == 0x04FFF000) {
        uint32_t off = address & 0xFFF;
        if (off == 0x780) {
            return mgbaDebugEnabled ? 0x1DEA : 0x0000;
        }
        return 0; // other debug regs read as 0
    }
    addWaitCycles(address, 16);
    
    // --- I/O register region: use ioRead16 handler ---
    if ((address >> 24) == 0x04 && (address & 0x00FF0000) == 0) {
        uint16_t offset = address & 0xFFFE;  // halfword-aligned offset within 64KB I/O block
        return ioRead16(offset);
    }
    
    // SRAM (0x0E-0x0F) has 8-bit bus: reads return single byte duplicated.
    // Check BEFORE force-alignment so CPU LDRH from unaligned SRAM address
    // reads the byte at the exact address (DMA provides pre-aligned addresses).
    if ((address >> 24) >= 0x0E) {
        uint32_t sramOffset;
        const uint8_t* sramBase = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, sramOffset);
        if (!sramBase) return 0xFFFF;
        uint8_t byte = sramBase[sramOffset];
        return byte | ((uint16_t)byte << 8);
    }
    
    // --- BIOS region protection (0x00000000 - 0x00FFFFFF) ---
    if (bios && (address >> 24) == 0x00) {
        if (address >= 0x4000) {
            uint32_t ob = cpu ? cpu->openBusPrefetch : 0;
            return (ob >> (((address & ~1u) & 2) * 8)) & 0xFFFF;
        }
        if (!cpuInBios) {
            uint32_t aligned = address & ~1u;
            return (biosPrefetch >> ((aligned & 2) * 8)) & 0xFFFF;
        }
    }

    // GBA LDRH/STRH force-align: bit 0 of address is ignored by the bus
    address &= ~1u;
    
    uint32_t offset;
    const uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) {
        // ROM open bus: out-of-bounds ROM reads return halfword address reflection
        uint32_t region = address >> 24;
        if (region >= 0x08 && region <= 0x0D) {
            return (address >> 1) & 0xFFFF;
        }
        // General open bus: return halfword from CPU prefetch pipeline
        uint32_t ob = cpu ? cpu->openBusPrefetch : 0;
        return (ob >> ((address & 2) * 8)) & 0xFFFF;
    }
    // IWRAM is only 32KB (0x8000 bytes), not 64KB like BLOCK_SIZE
    uint32_t wrapSize = (address >= 0x03000000 && address < 0x04000000) ? 0x8000 : Memory::BLOCK_SIZE;
    uint16_t val = base[offset] | (base[(offset + 1) % wrapSize] << 8);
    
    // Log IntrWait's read from 0x03FFFFF8 (mirrors to 0x03007FF8)
    if (address == 0x03FFFFF8 || address == 0x03007FF8) {
        static int intrwait_read_count = 0;
        if (intrwait_read_count++ < 50) {
            LOG_IRQ("[INTRWAIT READ16 #%d] addr=0x%08X → offset=0x%X val=0x%04X\n",
                   intrwait_read_count, address, offset, val);
        }
    }
    
    return val;
}

void Memory::write16(uint32_t address, uint16_t value) {
    // mGBA debug interface: 0x04FFF600-0x04FFF7FF (no wait cycles - virtual registers)
    if ((address & 0xFFFFF000) == 0x04FFF000) {
        uint32_t off = address & 0xFFF;
        if (off >= 0x600 && off < 0x700) {
            // REG_DEBUG_STRING: write into 256-byte debug buffer
            uint32_t idx = off - 0x600;
            if (idx < 254) {
                mgbaDebugString[idx]     = value & 0xFF;
                mgbaDebugString[idx + 1] = (value >> 8) & 0xFF;
            }
            return;
        }
        if (off == 0x700) {
            // REG_DEBUG_FLAGS: bit 8 = send flag
            if (value & 0x100) {
                int level = value & 0x7;
                mgbaDebugString[255] = '\0';  // ensure null-termination
                const char* levelStr = "FATAL";
                switch (level) {
                    case 0: levelStr = "FATAL"; break;
                    case 1: levelStr = "ERROR"; break;
                    case 2: levelStr = "WARN";  break;
                    case 3: levelStr = "INFO";  break;
                    case 4: levelStr = "DEBUG"; break;
                    default: levelStr = "LOG";  break;
                }
                fprintf(stderr, "[mGBA %s] %s\n", levelStr, mgbaDebugString);
                // Check for skip-on-text match (crashing suites)
                for (int i = 0; i < skipOnTextCount; i++) {
                    if (strstr(mgbaDebugString, skipOnTexts[i])) {
                        skipSuiteTriggered = true;
                        break;
                    }
                }
                // Check for exit-on-text match
                if (exitOnText && strstr(mgbaDebugString, exitOnText)) {
                    if (!skipSuiteTriggered) {
                        exitOnTextTriggered = true;
                    }
                }
                memset(mgbaDebugString, 0, sizeof(mgbaDebugString));
            }
            return;
        }
        if (off == 0x780) {
            // REG_DEBUG_ENABLE: write 0xC0DE to enable
            mgbaDebugEnabled = (value == 0xC0DE);
            return;
        }
        return; // ignore other writes in debug range
    }
    addWaitCycles(address, 16);
    
    // GBA read-only regions: writes are silently ignored
    // BIOS ROM (0x00) and Game Pak ROM (0x08-0x0D) are not writable
    uint32_t region = address >> 24;
    if (region == 0x00 || (region >= 0x08 && region <= 0x0D)) return;
    
    // Log writes to BIOS work RAM for interrupt tracking
    if (address >= 0x03007F00 && address <= 0x03007FFC) {
        LOG_IRQ("[IRQ HANDLER] Write16 to 0x%08X: value=0x%04X (BIOS work area)\n", address, value);
    }
    
    // Handle DMA register writes (word count and control only)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            // Handle 16-bit writes to source/dest addresses (low and high halves)
            if (regOffset == 0) {  // Source address low (DMAxSAD_L)
                uint32_t current = dmaController->readSourceAddress(channelID);
                uint32_t newAddr = (current & 0xFFFF0000) | value;
                dmaController->writeSourceAddress(channelID, newAddr);
                return;
            } else if (regOffset == 2) {  // Source address high (DMAxSAD_H)
                uint32_t current = dmaController->readSourceAddress(channelID);
                uint32_t newAddr = (current & 0x0000FFFF) | (static_cast<uint32_t>(value) << 16);
                dmaController->writeSourceAddress(channelID, newAddr);
                return;
            } else if (regOffset == 4) {  // Dest address low (DMAxDAD_L)
                uint32_t current = dmaController->readDestAddress(channelID);
                uint32_t newAddr = (current & 0xFFFF0000) | value;
                dmaController->writeDestAddress(channelID, newAddr);
                return;
            } else if (regOffset == 6) {  // Dest address high (DMAxDAD_H)
                uint32_t current = dmaController->readDestAddress(channelID);
                uint32_t newAddr = (current & 0x0000FFFF) | (static_cast<uint32_t>(value) << 16);
                dmaController->writeDestAddress(channelID, newAddr);
                return;
            } else if (regOffset == 8) {  // Word count (DMAxCNT_L)
                LOG_DMA("[DMA%d] Write Word Count: 0x%04X (%d transfers)\n", channelID, value, value ? value : 65536);
                dmaController->writeWordCount(channelID, value);
                return;  // Don't write to memory
            } else if (regOffset == 10) {  // Control (DMAxCNT_H)
                LOG_DMA("[DMA%d] Write Control: 0x%04X (Enable=%d, Mode=%d, 32bit=%d)\n", 
                       channelID, value, (value >> 15) & 1, (value >> 12) & 3, (value >> 10) & 1);
                dmaController->writeControl(channelID, value);
                return;  // Don't write to memory
            }
        }
    }
    
    // Handle timer register writes
    if (timerController) {
        if (address >= 0x04000100 && address <= 0x0400010E) {
            int timerID = (address - 0x04000100) / 4;
            bool isControl = ((address - 0x04000100) % 4) == 2;
            
            if (isControl) {
                timerController->writeControl(timerID, value);
            } else {
                timerController->writeReload(timerID, value);
            }
            // Don't return - also write to memory for debugging
        }
    }
    
    // Handle sound register writes (0x04000060 - 0x040000A7)
    if (apu) {
        if (address >= 0x04000060 && address <= 0x040000A6) {
            apu->write16(address, value);
            // Also write to memory for debug reads
        }
    }
    
    // Handle WAITCNT register write (0x04000204)
    if (address == 0x04000204) {
        uint16_t masked = value & 0x5FFF; // Only bits 0-12, 14 are writable (bit 15 and 13 are unused)
        updateWaitstates(masked);
        // Store masked value to memory and return
        io[0x204] = masked & 0xFF;
        io[0x205] = (masked >> 8) & 0xFF;
        return;
    }
    
    // SRAM has 8-bit bus: STRH writes only the LSB to the exact byte address
    // (no force-alignment, no second byte). This matches real GBA hardware where
    // only STRB truly works for SRAM, but STRH/STR write via the 8-bit bus.
    uint32_t sramRegion = address >> 24;
    if (sramRegion >= 0x0E) {
        uint32_t sramOffset;
        uint8_t* sramBase = get_region_base(this->regionTable, address, sramOffset);
        if (sramBase) {
            sramBase[sramOffset] = value & 0xFF;
        }
        return;
    }
    
    // GBA LDRH/STRH force-align: bit 0 of address is ignored by the bus
    address &= ~1u;
    
    uint16_t val = value;
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    // IWRAM is only 32KB (0x8000 bytes), not 64KB like BLOCK_SIZE
    uint32_t wrapSize = (address >= 0x03000000 && address < 0x04000000) ? 0x8000 : Memory::BLOCK_SIZE;
    
#if WATCHPOINT_ENABLED
    // Watchpoint: Monitor ALL write16 to 0x03007EA0-0x03007EA6
    if (address >= 0x03000000 && address < 0x04000000) {
        uint32_t iwram_off = address & 0x7FFF;
        if (iwram_off >= 0x7EA0 && iwram_off <= 0x7EA6) {
            fprintf(stderr, "[WATCH16] frame=%u instr=%llu addr=0x%08X val=0x%04X\n",
                    g_current_frame, (unsigned long long)g_total_instruction_count, address, val);
        }
    }
#endif
    
    // Log writes to OBJ VRAM and OBJ Palette (Nintendo logo investigation)
    if (address >= 0x06000000 && address < 0x06018000) {
        static int vram_writes = 0;
        if (vram_writes++ < 200) {
            if (address >= 0x06010000) {
                uint32_t tileNum = (address - 0x06010000) / 32;
                LOG_VRAM("[OBJ VRAM Write #%d] addr=0x%08X (tile %u, offset 0x%06X) val=0x%04X\n",
                       vram_writes, address, tileNum, address - 0x06010000, val);
            } else {
                LOG_VRAM("[BG VRAM Write #%d] addr=0x%08X (offset 0x%06X) val=0x%04X\n",
                       vram_writes, address, address - 0x06000000, val);
            }
        }
    }
    
    // Log writes to IE register (Interrupt Enable)
    if (address == 0x04000200) {  // REG_IE
        LOG_REG("[REG Write] IE (Interrupt Enable) = 0x%04X\n", val);
    }
    
    // Log writes to IME register (Interrupt Master Enable)
    if (address == 0x04000208) {  // REG_IME
        LOG_REG("[REG Write] IME (Interrupt Master Enable) = 0x%04X\n", val);
    }
    
    // Log writes to IntrWait check location at 0x03007FF8 (can be written via 0x03FFFFF8)
    if (address == 0x03007FF8 || address == 0x03FFFFF8) {
        static int intrwait_write_count = 0;
        if (intrwait_write_count++ < 50) {
            LOG_IRQ("[INTRWAIT WRITE16 #%d] addr=0x%08X val=0x%04X (BIOS should write IF copy here)\n",
                   intrwait_write_count, address, val);
        }
    }
    
    // Special handling for IF register (write 1 to clear)
    if (address == 0x04000202) {  // REG_IF
        uint16_t currentIF = base[offset] | (base[(offset + 1) % wrapSize] << 8);
        uint16_t newIF = currentIF & ~val;  // Clear bits where value has 1
        base[offset] = newIF & 0xFF;
        base[(offset + 1) % wrapSize] = (newIF >> 8) & 0xFF;
        static int if_write_count = 0;
        if (if_write_count++ < 10) {
            LOG_REG("[REG Write #%d] IF acknowledge = 0x%04X, currentIF was = 0x%04X, new IF = 0x%04X\n", 
                   if_write_count, val, currentIF, newIF);
        }
        return;
    }
    
    // Debug: Track VRAM writes (commented out - verified working)
    // if (address >= 0x06000000 && address < 0x06018000) {
    //     printf("[VRAM Write16] Address: 0x%08X, Value: 0x%04X (RGB555)\n", address, value);
    // }
    
    // Feature detection: Track what display features ROM is trying to use
    static bool feature_logged[256] = {false};  // Track which features we've logged
    
    if (address == 0x04000000) { // REG_DISPCNT
        int mode = value & 0x7;
        bool forcedBlank = (value >> 7) & 1;
        bool bg0 = (value >> 8) & 1;
        bool bg1 = (value >> 9) & 1;
        bool bg2 = (value >> 10) & 1;
        bool bg3 = (value >> 11) & 1;
        bool obj = (value >> 12) & 1;
        bool win0 = (value >> 13) & 1;
        bool win1 = (value >> 14) & 1;
        bool objWin = (value >> 15) & 1;
        
        LOG_REG("[DISPCNT] Write 0x%04X: Mode=%d Blank=%d BG0=%d BG1=%d BG2=%d BG3=%d OBJ=%d Win0=%d Win1=%d ObjWin=%d\n",
               value, mode, forcedBlank, bg0, bg1, bg2, bg3, obj, win0, win1, objWin);
        

        
        // Log features being used
        if (mode > 0 && !feature_logged[0]) {
            LOG_FEATURE("[FEATURE] ROM using video Mode %d (bitmap/affine modes)\n", mode);
            feature_logged[0] = true;
        }
        if (bg1 && !feature_logged[1]) {
            LOG_FEATURE("[FEATURE] ROM enabling BG1 (text/affine background layer)\n");
            feature_logged[1] = true;
        }
        if (bg2 && !feature_logged[2]) {
            LOG_FEATURE("[FEATURE] ROM enabling BG2 (text/affine background layer)\n");
            feature_logged[2] = true;
        }
        if (bg3 && !feature_logged[3]) {
            LOG_FEATURE("[FEATURE] ROM enabling BG3 (text/affine background layer)\n");
            feature_logged[3] = true;
        }
        if (obj && !feature_logged[4]) {
            LOG_FEATURE("[FEATURE] ROM enabling OBJ (sprites/objects)\n");
            feature_logged[4] = true;
        }
        if ((win0 || win1 || objWin) && !feature_logged[5]) {
            LOG_FEATURE("[FEATURE] ROM enabling Windows (win0=%d win1=%d objWin=%d)\n", win0, win1, objWin);
            feature_logged[5] = true;
        }
    }
    
    // Track background control registers (BG0CNT-BG3CNT)
    if (address >= 0x04000008 && address <= 0x0400000E && !feature_logged[10 + (address - 0x04000008)/2]) {
        int bgNum = (address - 0x04000008) / 2;
        int priority = value & 0x3;
        int charBase = (value >> 2) & 0x3;
        int mosaic = (value >> 6) & 0x1;
        int colors = (value >> 7) & 0x1;  // 0=16 colors, 1=256 colors
        int screenBase = (value >> 8) & 0x1F;
        int wraparound = (value >> 13) & 0x1;
        int screenSize = (value >> 14) & 0x3;
        
        LOG_REG("[BG%dCNT] Write 0x%04X: Priority=%d CharBase=%d Mosaic=%d Colors=%s ScreenBase=%d Wrap=%d Size=%d\n",
               bgNum, value, priority, charBase, mosaic, colors ? "256" : "16", screenBase, wraparound, screenSize);
        feature_logged[10 + bgNum] = true;
    }
    
    // Track affine parameters (BG2/BG3 rotation/scaling)
    if (address >= 0x04000020 && address <= 0x0400003F && !feature_logged[20]) {
        LOG_FEATURE("[FEATURE] ROM writing BG affine parameters (rotation/scaling) at 0x%08X = 0x%04X\n", address, value);
        feature_logged[20] = true;
    }
    
    // Track blending registers
    if (address >= 0x04000050 && address <= 0x04000054 && !feature_logged[21]) {
        LOG_FEATURE("[FEATURE] ROM enabling color blending/effects at 0x%08X = 0x%04X\n", address, value);
        feature_logged[21] = true;
    }
    
    // Track mosaic
    if (address == 0x0400004C && !feature_logged[22]) {
        LOG_FEATURE("[FEATURE] ROM enabling mosaic effect = 0x%04X\n", value);
        feature_logged[22] = true;
    }
    
    // KEYINPUT (0x04000130) is READ-ONLY - ignore writes
    // This register reflects physical button state, not software-writeable
    if (address == 0x04000130) {
        return;  // Silently ignore writes to KEYINPUT
    }
    
    // STACK CORRUPTION WATCH: Watch for writes to stack region
    if (address >= 0x03000000 && address < 0x04000000) {
        uint32_t iwram_offset = address & 0x7FFF;  // IWRAM 32KB mask
        if (iwram_offset >= 0x7E80 && iwram_offset <= 0x7EA0) {
            LOG_STACK("[STACK WATCH] Write16 to 0x%08X (IWRAM+0x%04X): value=0x%04X\n",
                   address, iwram_offset, val);
        }
    }
    
    base[offset] = val & 0xFF;
    base[(offset + 1) % wrapSize] = (val >> 8) & 0xFF;
    
    // Like mGBA's GBATestIRQ: after writing IE or IME, re-check for pending IRQs.
    // Must happen AFTER the store so scheduleIRQCheck reads the new value.
    if ((address == 0x04000200 || address == 0x04000208) && interruptController) {
        interruptController->scheduleIRQCheck();
    }
}

// ============================================================================
// I/O Register Read Handler
// ============================================================================
// GBA I/O registers have three categories:
// 1. Readable: return the stored value, optionally masked for unused bits
// 2. Write-only: return CPU open bus (the instruction prefetch pipeline value)
// 3. Unused gaps: return 0 or open bus depending on the specific address
//
// This matches mGBA's GBAIORead() behavior.  The test suite writes 0xFFFF
// then reads back, expecting: mask for readable, 0xDEAD for write-only
// (because the test asm places 0xDEADDEAD as a literal after the ldrh),
// or 0 for unused-zero gaps.
// ============================================================================
uint16_t Memory::ioRead16(uint16_t offset) const {
    // Helper: read 16-bit from io buffer
    auto ioVal = [&](uint16_t off) -> uint16_t {
        return io[off] | (io[off + 1] << 8);
    };

    // Helper: open bus value (halfword from CPU prefetch)
    auto openBus = [&]() -> uint16_t {
        uint32_t ob = cpu ? cpu->openBusPrefetch : 0;
        return (ob >> ((offset & 2) * 8)) & 0xFFFF;
    };

    // --- Timer registers (via timerController for live counter) ---
    if (timerController && offset >= 0x100 && offset <= 0x10E) {
        int timerID = (offset - 0x100) / 4;
        bool isControl = ((offset - 0x100) % 4) == 2;
        if (isControl) return timerController->readControl(timerID);
        return timerController->readCounter(timerID);
    }

    // --- DMA registers ---
    if (offset >= 0x0B0 && offset <= 0x0DE) {
        int ch = (offset - 0x0B0) / 12;
        int reg = (offset - 0x0B0) % 12;
        if (reg == 10) {
            // CNT_HI: readable with mask (bits 0-4 always 0)
            uint16_t ctrl = dmaController ? dmaController->readControl(ch) : 0;
            // DMA0-2: bit 11 (game pak DRQ) not present → mask 0xF7E0
            // DMA3: bit 11 present → mask 0xFFE0
            uint16_t mask = (ch == 3) ? 0xFFE0 : 0xF7E0;
            return ctrl & mask;
        }
        if (reg == 8) return 0;     // CNT_LO (word count): always reads 0
        return openBus();            // SAD / DAD: write-only → open bus
    }

    switch (offset) {
    // ----- Video: readable registers -----
    case 0x000: return ioVal(offset);                       // DISPCNT
    case 0x002: return ioVal(offset);                       // Green Swap / STEREOCNT
    case 0x004: return ioVal(offset);                       // DISPSTAT
    case 0x006: return ioVal(offset);                       // VCOUNT
    case 0x008: return ioVal(offset) & 0xDFFF;              // BG0CNT (bit 13 not readable)
    case 0x00A: return ioVal(offset) & 0xDFFF;              // BG1CNT
    case 0x00C: return ioVal(offset);                       // BG2CNT
    case 0x00E: return ioVal(offset);                       // BG3CNT

    // ----- Video: write-only registers → open bus -----
    case 0x010: case 0x012: case 0x014: case 0x016:         // BGxHOFS/VOFS
    case 0x018: case 0x01A: case 0x01C: case 0x01E:
    case 0x020: case 0x022: case 0x024: case 0x026:         // BG2 affine params
    case 0x028: case 0x02A: case 0x02C: case 0x02E:         // BG2 ref point
    case 0x030: case 0x032: case 0x034: case 0x036:         // BG3 affine params
    case 0x038: case 0x03A: case 0x03C: case 0x03E:         // BG3 ref point
    case 0x040: case 0x042: case 0x044: case 0x046:         // WINxH/V
    case 0x04C:                                              // MOSAIC
    case 0x054:                                              // BLDY
        return openBus();

    // ----- Video: readable with masks -----
    case 0x048: return ioVal(offset) & 0x3F3F;              // WININ
    case 0x04A: return ioVal(offset) & 0x3F3F;              // WINOUT
    case 0x050: return ioVal(offset) & 0x3FFF;              // BLDCNT
    case 0x052: return ioVal(offset) & 0x1F1F;              // BLDALPHA

    // ----- Sound: readable with masks -----
    case 0x060: return ioVal(offset) & 0x007F;              // SOUND1CNT_LO
    case 0x062: return ioVal(offset) & 0xFFC0;              // SOUND1CNT_HI
    case 0x064: return ioVal(offset) & 0x4000;              // SOUND1CNT_X
    case 0x068: return ioVal(offset) & 0xFFC0;              // SOUND2CNT_LO
    case 0x06C: return ioVal(offset) & 0x4000;              // SOUND2CNT_HI
    case 0x070: return ioVal(offset) & 0x00E0;              // SOUND3CNT_LO
    case 0x072: return ioVal(offset) & 0xE000;              // SOUND3CNT_HI
    case 0x074: return ioVal(offset) & 0x4000;              // SOUND3CNT_X
    case 0x078: return ioVal(offset) & 0xFF00;              // SOUND4CNT_LO
    case 0x07C: return ioVal(offset) & 0x40FF;              // SOUND4CNT_HI
    case 0x080: return ioVal(offset) & 0xFF77;              // SOUNDCNT_LO
    case 0x082: return ioVal(offset) & 0x770F;              // SOUNDCNT_HI
    case 0x084: return ioVal(offset) & 0x0080;              // SOUNDCNT_X (only master enable readable; ch flags are hw-managed)
    case 0x088: return ioVal(offset);                        // SOUNDBIAS

    // ----- Sound: unused gaps → 0 -----
    case 0x066: case 0x06A: case 0x06E:
    case 0x076: case 0x07A: case 0x07E:
    case 0x086: case 0x08A:
        return 0;

    // ----- WAVE_RAM: readable -----
    case 0x090: case 0x092: case 0x094: case 0x096:
    case 0x098: case 0x09A: case 0x09C: case 0x09E:
        return ioVal(offset);

    // ----- FIFO: write-only → open bus -----
    case 0x0A0: case 0x0A2: case 0x0A4: case 0x0A6:
        return openBus();

    // ----- DMA: source/dest are write-only → open bus, control is readable -----
    case 0x0B0: case 0x0B2: case 0x0B4: case 0x0B6:         // DMA0 SAD/DAD
    case 0x0B8:                                              // DMA0 word count (write-only)
    case 0x0BC: case 0x0BE: case 0x0C0: case 0x0C2:         // DMA1 SAD/DAD
    case 0x0C4:                                              // DMA1 word count
    case 0x0C8: case 0x0CA: case 0x0CC: case 0x0CE:         // DMA2 SAD/DAD
    case 0x0D0:                                              // DMA2 word count
    case 0x0D4: case 0x0D6: case 0x0D8: case 0x0DA:         // DMA3 SAD/DAD
    case 0x0DC:                                              // DMA3 word count
        return openBus();
    case 0x0BA: return ioVal(offset);                        // DMA0 CNT_H (readable)
    case 0x0C6: return ioVal(offset);                        // DMA1 CNT_H
    case 0x0D2: return ioVal(offset);                        // DMA2 CNT_H
    case 0x0DE: return ioVal(offset);                        // DMA3 CNT_H

    // ----- Timer registers -----
    // Timer counter reads must be computed on-the-fly from scheduler cycles.
    // The io[] array holds the reload value, not the running counter.
    case 0x100:  // TM0CNT_L (counter)
        if (timerController) return timerController->readCounter(0);
        return ioVal(offset);
    case 0x102: return ioVal(offset);                        // TM0CNT_H (control)
    case 0x104:  // TM1CNT_L
        if (timerController) return timerController->readCounter(1);
        return ioVal(offset);
    case 0x106: return ioVal(offset);                        // TM1CNT_H
    case 0x108:  // TM2CNT_L
        if (timerController) return timerController->readCounter(2);
        return ioVal(offset);
    case 0x10A: return ioVal(offset);                        // TM2CNT_H
    case 0x10C:  // TM3CNT_L
        if (timerController) return timerController->readCounter(3);
        return ioVal(offset);
    case 0x10E: return ioVal(offset);                        // TM3CNT_H

    // ----- Keypad -----
    case 0x130: return ioVal(offset);                        // KEYINPUT
    case 0x132: return ioVal(offset);                        // KEYCNT

    // ----- Serial I/O (stubs) -----
    case 0x120: case 0x122: case 0x124: case 0x126:         // SIO multi
    case 0x128: case 0x12A:                                  // SIOCNT / SIO send
    case 0x134:                                              // RCNT
    case 0x140:                                              // JOYCNT
    case 0x150: case 0x152:                                  // JOY_RECV
    case 0x154: case 0x156:                                  // JOY_TRANS
    case 0x158:                                              // JOYSTAT
        return ioVal(offset);

    // ----- Interrupts / system -----
    case 0x200: return ioVal(offset);                        // IE
    case 0x202: return ioVal(offset);                        // IF
    case 0x204: return ioVal(offset);                        // WAITCNT
    case 0x208: return ioVal(offset);                        // IME
    case 0x300: return ioVal(offset);                        // POSTFLG

    // ----- Unused addresses that return 0 -----
    case 0x136: case 0x142: case 0x15A:
    case 0x206: case 0x20A: case 0x302:
        return 0;

    default:
        // Unknown / unmapped I/O → open bus
        return openBus();
    }
}

void Memory::writeDirectIO(uint32_t address, uint16_t value) {
    // Direct write to I/O registers (bypasses write-to-clear and other special handling)
    // Used by hardware components to set registers
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    // IWRAM is only 32KB (0x8000 bytes), not 64KB like BLOCK_SIZE
    uint32_t wrapSize = (address >= 0x03000000 && address < 0x04000000) ? 0x8000 : Memory::BLOCK_SIZE;
    base[offset] = value & 0xFF;
    base[(offset + 1) % wrapSize] = (value >> 8) & 0xFF;
}

void Memory::setKeyState(uint16_t keyState) {
    // Directly set KEYINPUT register (0x04000130) - bypasses read-only protection
    // This is called from Display::handleEvents() to update key state from SDL
    if (io) {
        io[0x130] = keyState & 0xFF;
        io[0x131] = (keyState >> 8) & 0xFF;
    }
}

// Direct I/O reads (bypass wait cycle counting)
// Used by tracer and debugging code that shouldn't affect timing
uint8_t Memory::readDirectIO8(uint32_t address) const {
    uint32_t offset;
    const uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return 0xFF;
    return base[offset];
}

uint16_t Memory::readDirectIO16(uint32_t address) const {
    uint32_t offset;
    const uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return 0xFFFF;
    
    // For IWRAM, we need to properly wrap around at 32KB (0x8000)
    // For other regions, use BLOCK_SIZE (64KB)
    uint32_t wrapSize = (address >= 0x03000000 && address < 0x04000000) ? 0x8000 : Memory::BLOCK_SIZE;
    
    uint8_t low = base[offset];
    uint8_t high = base[(offset + 1) % wrapSize];
    return low | (high << 8);
}

uint32_t Memory::readDirectIO32(uint32_t address) const {
    uint32_t offset;
    const uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return 0xFFFFFFFF;
    
    // For IWRAM, we need to properly wrap around at 32KB (0x8000)
    // For other regions, use BLOCK_SIZE (64KB)
    uint32_t wrapSize = (address >= 0x03000000 && address < 0x04000000) ? 0x8000 : Memory::BLOCK_SIZE;
    
    uint8_t b0 = base[offset];
    uint8_t b1 = base[(offset + 1) % wrapSize];
    uint8_t b2 = base[(offset + 2) % wrapSize];
    uint8_t b3 = base[(offset + 3) % wrapSize];
    uint32_t val = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    
    // Debug: Trace BIOS reads at 0x18 (IRQ vector)
    if (address == 0x00000018) {
        static int bios18_read_count = 0;
        bios18_read_count++;
        LOG_BIOS("[BIOS 0x18 DirectIO READ #%d] offset=%u, bytes: %02X %02X %02X %02X = value 0x%08X (base=%p)\n",
               bios18_read_count, offset, b0, b1, b2, b3, val, static_cast<const void*>(base));
    }
    
    return val;
}

// ARM7TDMI Unaligned Word Access (LDR)
// Reference: ARM7TDMI Technical Reference Manual, Section 6.3 (Memory Interface)
// https://developer.arm.com/documentation/ddi0210/c/
//
// When a word load (LDR) is performed with an unaligned address:
// 1. Address is aligned down to 4-byte boundary (address & ~3)
// 2. Word is read from the aligned address
// 3. Result is rotated RIGHT by (address & 3) * 8 bits
//
// Example: LDR from address 0x1003 (unaligned by 3 bytes)
//   - Reads word from 0x1000 (aligned)
//   - Rotates result right by 24 bits (3 * 8)
//   - This effectively moves byte at 0x1003 to LSB position
//
// This behavior allows unaligned loads to work predictably, with the
// byte at the requested address ending up in the LSB of the result.
uint32_t Memory::read32(uint32_t address) const {
    // mGBA debug interface reads (no wait cycles - virtual registers)
    if ((address & 0xFFFFF000) == 0x04FFF000) {
        uint32_t off = address & 0xFFF;
        if (off == 0x780) {
            uint16_t val = mgbaDebugEnabled ? 0x1DEA : 0x0000;
            return val | ((uint32_t)val << 16);
        }
        return 0;
    }
    addWaitCycles(address, 32);
    
    // Debug: trace reads from IRQ handler pointer
    if (address == 0x03FFFFFC || address == 0x03007FFC) {
        static int irq_ptr_read_count = 0;
        if (irq_ptr_read_count++ < 100) {
            LOG_IRQ("[IRQ PTR READ #%d] Reading from 0x%08X\n", irq_ptr_read_count, address);
        }
    }
    
    // --- I/O register region: combine two ioRead16 calls ---
    if ((address >> 24) == 0x04 && (address & 0x00FF0000) == 0) {
        uint16_t offset = address & 0xFFFC;  // word-aligned offset within 64KB I/O block
        uint16_t lo = ioRead16(offset);
        uint16_t hi = ioRead16(offset + 2);
        return lo | ((uint32_t)hi << 16);
    }
    
    // SRAM (0x0E-0x0F) has 8-bit bus: reads return single byte replicated to 32 bits.
    // Check BEFORE force-alignment so CPU LDR from unaligned SRAM address
    // reads the byte at the exact address (DMA provides pre-aligned addresses).
    if ((address >> 24) >= 0x0E) {
        uint32_t sramOffset;
        const uint8_t* sramBase = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, sramOffset);
        if (!sramBase) return 0xFFFFFFFF;
        uint8_t byte = sramBase[sramOffset];
        return byte * 0x01010101u;
    }
    
    // --- BIOS region protection (0x00000000 - 0x00FFFFFF) ---
    if (bios && (address >> 24) == 0x00) {
        if (address >= 0x4000) {
            // Past BIOS bounds: open bus
            return cpu ? cpu->openBusPrefetch : 0;
        }
        if (!cpuInBios) {
            // BIOS protection: return latched prefetch
            return biosPrefetch;
        }
        // CPU inside BIOS — fall through to normal read
    }

    // ARM7TDMI bus: force-align to word boundary, return aligned word.
    // NOTE: The LDR instruction applies rotation for misaligned addresses;
    // LDM, DMA, and other bus masters just get the aligned word.
    uint32_t aligned_address = address & ~3u;  // Align to 4-byte boundary
    uint32_t offset;
    const uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), aligned_address, offset);
    if (!base) {
        // ROM open bus: out-of-bounds ROM reads return halfword address reflection
        uint32_t region = aligned_address >> 24;
        if (region >= 0x08 && region <= 0x0D) {
            uint16_t lo = (aligned_address >> 1) & 0xFFFF;
            uint16_t hi = ((aligned_address + 2) >> 1) & 0xFFFF;
            return lo | ((uint32_t)hi << 16);
        }
        // General open bus: return CPU prefetch pipeline value
        return cpu ? cpu->openBusPrefetch : 0;
    }
    // Debug: Check for unexpected offset values for IWRAM
    if (aligned_address >= 0x03000000 && aligned_address < 0x04000000 && offset >= 0x8000) {
        printf("[READ32 OOB] IWRAM address 0x%08X mapped to offset 0x%X (>= 32KB!)\n", address, offset);
    }
    // IWRAM is only 32KB (0x8000 bytes), not 64KB like BLOCK_SIZE
    // Must wrap within actual buffer size to prevent buffer overflow
    uint32_t wrapSize = (aligned_address >= 0x03000000 && aligned_address < 0x04000000) ? 0x8000 : Memory::BLOCK_SIZE;
    uint32_t val = base[offset]
        | (base[(offset + 1) % wrapSize] << 8)
        | (base[(offset + 2) % wrapSize] << 16)
        | (base[(offset + 3) % wrapSize] << 24);
    
    // Debug: Trace BIOS reads at 0x18 (IRQ vector)
    if (aligned_address == 0x00000018) {
        static int bios18_read_count = 0;
        bios18_read_count++;
        LOG_BIOS("[BIOS 0x18 READ #%d] offset=%u, bytes: %02X %02X %02X %02X = value 0x%08X\n",
               bios18_read_count, offset, base[offset], base[offset+1], base[offset+2], base[offset+3], val);
    }
    
    // Debug: Log I/O register reads during BIOS loop
    if (address >= 0x04000000 && address < 0x04000400) {
        static int ioReadCount = 0;
        if (ioReadCount < 20) {
            LOG_REG("[I/O Read32] Address=0x%08X Value=0x%08X\n", address, val);
            ioReadCount++;
        }
    }
    // Debug: Print reads from entry point
    if (address == 0x080000B4) {
        printf("[Memory::read32] Read from 0x%08X: 0x%08X\n", address, val);
    }
    
    // Debug: trace result from IRQ handler pointer read
    if (address == 0x03FFFFFC || address == 0x03007FFC) {
        static int irq_ptr_result_count = 0;
        if (irq_ptr_result_count++ < 100) {
            LOG_IRQ("[IRQ PTR VALUE #%d] Read from 0x%08X = 0x%08X (offset=0x%X)\n", 
                   irq_ptr_result_count, address, val, offset);
        }
    }
    
    return val;  // No rotation on ARM7TDMI
}

// ARM7TDMI Unaligned Word Access (STR)
// Reference: ARM7TDMI Technical Reference Manual, Section 6.3 (Memory Interface)
// https://developer.arm.com/documentation/ddi0210/c/
//
// When a word store (STR) is performed with an unaligned address:
// 1. Address is aligned down to 4-byte boundary (address & ~3)
// 2. Value is rotated LEFT by (address & 3) * 8 bits
// 3. Rotated value is written to the aligned address
//
// Example: STR 0xAABBCCDD to address 0x1003 (unaligned by 3 bytes)
//   - Rotates value left by 24 bits: 0xDDAABBCC
//   - Writes to aligned address 0x1000
//   - Memory at 0x1000: 0xCC, 0x1001: 0xBB, 0x1002: 0xAA, 0x1003: 0xDD
//
// This matches the read behavior: if you STR then LDR at the same
// unaligned address, you get back the original value (rotations cancel out).
//
// Note: This is standard ARM7TDMI behavior, not a GBA-specific quirk.
void Memory::write32(uint32_t address, uint32_t value) {
    // mGBA debug interface: 32-bit writes decompose to two 16-bit writes (no wait cycles)
    if ((address & 0xFFFFF000) == 0x04FFF000) {
        write16(address, value & 0xFFFF);
        write16(address + 2, (value >> 16) & 0xFFFF);
        return;
    }
    addWaitCycles(address, 32);
    
    // GBA read-only regions: writes are silently ignored
    // BIOS ROM (0x00) and Game Pak ROM (0x08-0x0D) are not writable
    uint32_t region = address >> 24;
    if (region == 0x00 || (region >= 0x08 && region <= 0x0D)) return;
    
    // I/O region (0x04000000 - 0x040003FF): decompose into two 16-bit writes
    // so that all special side-effect handlers fire (timers, DMA, WAITCNT, 
    // interrupts, sound, etc.). Exception: FIFO writes need 32-bit semantics.
    if (region == 0x04 && (address & 0x00FF0000) == 0) {
        // FIFO writes must stay 32-bit (push 4 bytes at once)
        if (apu) {
            if (address == 0x040000A0) { apu->writeFIFO_A(value); return; }
            if (address == 0x040000A4) { apu->writeFIFO_B(value); return; }
        }
        // Decompose: write low halfword first, then high halfword
        uint32_t aligned = address & ~3u;
        disableWaitCycles = true;  // Already charged 32-bit wait above
        write16(aligned, value & 0xFFFF);
        write16(aligned + 2, (value >> 16) & 0xFFFF);
        disableWaitCycles = false;
        return;
    }
    
    // SRAM has 8-bit bus: STR writes only the LSB to the exact byte address
    // (no force-alignment, no multi-byte write). Same as STRH — only one byte goes through.
    if (region >= 0x0E) {
        uint32_t sramOffset;
        uint8_t* sramBase = get_region_base(this->regionTable, address, sramOffset);
        if (sramBase) {
            sramBase[sramOffset] = value & 0xFF;
        }
        return;
    }
    
    // ARM7TDMI Word Store (STR):
    // The bus force-aligns the address (bits [1:0] ignored).
    // Unlike LDR, STR does NOT rotate the value — it writes as-is to the
    // aligned address. Only LDR applies rotation for misaligned addresses.
    uint32_t aligned_address = address & ~3u;  // Align to 4-byte boundary
    uint32_t val = value;
    
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, aligned_address, offset);
    if (!base) return;
    
    // Log writes that overlap IE/IF/IME registers
    if (aligned_address >= 0x04000200 && aligned_address <= 0x04000208) {
        LOG_REG("[REG Write32] Address=0x%08X Value=0x%08X (may write IE/IF/IME)\n", aligned_address, val);
    }
    
    // KEYINPUT (0x04000130) is READ-ONLY - ignore writes that would affect it
    if (aligned_address == 0x04000130) {
        return;  // Silently ignore writes to KEYINPUT
    }
    
    // Log writes to IRQ handler pointer area AND interrupt acknowledge flags
    // The BIOS IRQ dispatcher at 0x128 reads from [0x04000000-4] = 0x03FFFFFC
    // BIOS also uses 0x03007FF0-0x03007FFC area during initialization
    // IMPORTANT: 0x03007FF8 = IntrCheck flag (BIOS writes interrupt bits here)
    // Log ALL writes to IRQ handler location
    if (aligned_address == 0x03FFFFFC || aligned_address == 0x03007FFC) {
        static int irq_write_count = 0;
        if (irq_write_count++ < 50) {
            LOG_IRQ("[IRQ HANDLER WRITE #%d] Writing 0x%08X to 0x%08X\n", 
                   irq_write_count, val, aligned_address);
        }
    }
    
#if WATCHPOINT_ENABLED
    // Watchpoint: Monitor ALL write32 to 0x03007EA0-0x03007EA7
    uint32_t iwram_offset = aligned_address & 0x7FFF; // IWRAM 32KB mask
    if ((aligned_address >= 0x03000000 && aligned_address < 0x04000000) &&
        (iwram_offset >= 0x7EA0 && iwram_offset <= 0x7EA7)) {
        fprintf(stderr, "[WATCH32] frame=%u instr=%llu addr=0x%08X val=0x%08X\n",
                g_current_frame, (unsigned long long)g_total_instruction_count, aligned_address, val);
    }
#endif

    if (aligned_address == 0x03FFFFFC || (aligned_address >= 0x03007F00 && aligned_address <= 0x03007FFC)) {
        // TEMPORARILY DISABLED FOR PHASE 1 TESTING
        #if 0
        // Get current PC for debugging (requires CPU context)
        // Detect corrupted values (addresses outside valid GBA memory)
        bool suspicious = (val >= 0x10000000 && val < 0x80000000) || // Outside valid ranges
                          ((val >> 28) == 0x6) ||  // 0x6xxxxxxx range
                          ((val >> 28) == 0x1);    // 0x1xxxxxxx range
        if (suspicious) {
            LOG_CRASH("[CORRUPTION!] Write32 to 0x%08X: value=0x%08X (suspicious value in IRQ/BIOS work area)\n", 
                   aligned_address, val);
        }
        if (aligned_address == 0x03FFFFFC && (val < 0x02000000 || val > 0x0FFFFFFF)) {
            LOG_CRASH("[IRQ HANDLER ERROR] Suspicious IRQ handler address 0x%08X written to 0x%08X!\n", val, aligned_address);
        }
        #endif
    }
    
    // Feature detection: Track display register writes in write32
    static bool feature_logged32[256] = {false};
    
    if (aligned_address == 0x04000000) { // REG_DISPCNT (32-bit write)
        uint16_t dispcnt_value = val & 0xFFFF;
        int mode = dispcnt_value & 0x7;
        bool bg0 = (dispcnt_value >> 8) & 1;
        bool bg1 = (dispcnt_value >> 9) & 1;
        bool bg2 = (dispcnt_value >> 10) & 1;
        bool bg3 = (dispcnt_value >> 11) & 1;
        bool obj = (dispcnt_value >> 12) & 1;
        
        LOG_REG("[DISPCNT32] Write 0x%04X: Mode=%d BG0=%d BG1=%d BG2=%d BG3=%d OBJ=%d\n",
               dispcnt_value, mode, bg0, bg1, bg2, bg3, obj);
        
        if (mode > 0 && !feature_logged32[0]) {
            LOG_FEATURE("[FEATURE] ROM using video Mode %d (detected in write32)\n", mode);
            feature_logged32[0] = true;
        }
        if (obj && !feature_logged32[10]) {
            LOG_FEATURE("[FEATURE] ROM enabling sprites/OBJ (detected in write32)\n");
            feature_logged32[10] = true;
        }
    }
    
    // Track OAM writes (32-bit)
    if (aligned_address >= 0x07000000 && aligned_address < 0x07000400 && !feature_logged32[30]) {
        LOG_FEATURE("[FEATURE] ROM writing to OAM via write32 at 0x%08X\n", aligned_address);
        feature_logged32[30] = true;
    }
    
    // STACK CORRUPTION WATCH: TEMPORARILY DISABLED FOR PHASE 1 TESTING
    // TODO: Re-enable after Phase 1 verification
    #if 0
    // The crash at frame 2236 is caused by BX R3 where R3=0xF4F7FF46 loaded from stack ~0x03007E8C
    uint32_t iwram_offset = aligned_address & 0x7FFF;  // IWRAM 32KB mask
    if (aligned_address >= 0x03000000 && aligned_address < 0x04000000) {
        // Watch for writes to stack region 0x03007E80-0x03007EA0
        if (iwram_offset >= 0x7E80 && iwram_offset <= 0x7EA0) {
            LOG_STACK("[STACK WATCH] Write32 to 0x%08X (IWRAM+0x%04X): value=0x%08X\n",
                   aligned_address, iwram_offset, val);
        }
        // Also catch the specific corrupt value anywhere in IWRAM stack area
        if (val == 0xF4F7FF46 || (val >= 0xF0000000 && val <= 0xFFFFFFFF)) {
            LOG_CRASH("[STACK CORRUPT!] Write32 to 0x%08X: suspicious value 0x%08X\n",
                   aligned_address, val);
        }
    }
    #endif
    
    // Debug: Track VRAM writes
    if (address >= 0x06000000 && address < 0x06018000) {
        static int vramWriteCount = 0;
        if (vramWriteCount < 10) {
            LOG_VRAM("[VRAM Write32] Address: 0x%08X, Value: 0x%08X, base=%p, vram=%p, offset=%u\n", 
                   address, value, static_cast<const void*>(base), static_cast<const void*>(vram), offset);
            vramWriteCount++;
        }
    }
    
    // IWRAM is only 32KB (0x8000 bytes), not 64KB like BLOCK_SIZE
    uint32_t wrapSize = (aligned_address >= 0x03000000 && aligned_address < 0x04000000) ? 0x8000 : Memory::BLOCK_SIZE;
    
    // Special handling: 32-bit write to 0x04000200 overlaps IE (low 16) and IF (high 16).
    // IF uses write-to-clear semantics: writing a 1-bit *clears* that flag.
    if (aligned_address == 0x04000200) {
        // Low 16 bits → IE: normal write
        base[offset] = val & 0xFF;
        base[(offset + 1) % wrapSize] = (val >> 8) & 0xFF;
        // High 16 bits → IF: write-to-clear
        uint16_t currentIF = base[(offset + 2) % wrapSize] | (base[(offset + 3) % wrapSize] << 8);
        uint16_t ifWriteVal = (val >> 16) & 0xFFFF;
        uint16_t newIF = currentIF & ~ifWriteVal;
        base[(offset + 2) % wrapSize] = newIF & 0xFF;
        base[(offset + 3) % wrapSize] = (newIF >> 8) & 0xFF;
    } else {
        base[offset] = val & 0xFF;
        base[(offset + 1) % wrapSize] = (val >> 8) & 0xFF;
        base[(offset + 2) % wrapSize] = (val >> 16) & 0xFF;
        base[(offset + 3) % wrapSize] = (val >> 24) & 0xFF;
    }
    
    // Like mGBA's GBATestIRQ: after writing IE or IME via write32, re-check IRQs.
    // Must happen AFTER the store so scheduleIRQCheck reads the updated values.
    if ((aligned_address == 0x04000200 || aligned_address == 0x04000208) && interruptController) {
        interruptController->scheduleIRQCheck();
    }
    
    // PHASE 4: Verify write to crash addresses - DISABLED FOR SPEED
    // if ((aligned_address >= 0x03000000 && aligned_address < 0x04000000) &&
    //     (offset >= 0x7EA0 && offset <= 0x7EA4)) {
    //     uint32_t readback = base[offset] | (base[(offset+1) % wrapSize] << 8) |
    //                         (base[(offset+2) % wrapSize] << 16) | (base[(offset+3) % wrapSize] << 24);
    //     fprintf(stderr, "[VERIFY WRITE] addr=0x%08X offset=0x%X wrote=0x%08X readback=0x%08X base=%p\n",
    //             aligned_address, offset, val, readback, (void*)base);
    // }
}

// ============================================================================
// Memory Timing (Wait States)
// ============================================================================

uint32_t Memory::calculateWaitStates(uint32_t address, uint32_t accessWidth) const {
    // Mask to 28-bit address space (GBA mirrors upper 4 bits)
    uint32_t addr = address & 0x0FFFFFFF;
    
    // Determine memory region and calculate wait states
    switch (addr >> 24) {
        case 0x00: // BIOS ROM (16KB)
            // 1 cycle for any access
            return 1;
            
        case 0x02: // EWRAM (256KB on-board Work RAM)
            // 16-bit bus: 3 cycles for 8/16-bit, 6 cycles for 32-bit
            return (accessWidth == 32) ? 6 : 3;
            
        case 0x03: // IWRAM (32KB on-chip Work RAM)
            // 32-bit bus: 1 cycle for any access (fastest)
            return 1;
            
        case 0x04: // I/O Registers
            // 1 cycle for any access
            return 1;
            
        case 0x05: // Palette RAM (1KB)
            // 16-bit bus: 1 cycle for 16-bit, 2 cycles for 32-bit
            // +1 if video controller accessing (not implemented yet)
            return (accessWidth == 32) ? 2 : 1;
            
        case 0x06: // VRAM (96KB)
            // 16-bit bus: 1 cycle for 16-bit, 2 cycles for 32-bit
            // +1 if video controller accessing (not implemented yet)
            return (accessWidth == 32) ? 2 : 1;
            
        case 0x07: // OAM (1KB)
            // 32-bit bus: 1 cycle for any access
            // +1 if video controller accessing (not implemented yet)
            return 1;
            
        case 0x08: // Game Pak ROM Wait State 0
        case 0x09:
        case 0x0A: // Game Pak ROM Wait State 1
        case 0x0B:
        case 0x0C: // Game Pak ROM Wait State 2
        case 0x0D: {
            // Use the wait state tables (configured by WAITCNT register)
            uint8_t region = (addr >> 24) & 0xFF;
            return (accessWidth == 32) ? waitstatesNonseq32[region] : waitstatesNonseq16[region];
        }
            
        case 0x0E: // Game Pak SRAM
        case 0x0F: {
            // Use the wait state tables (configured by WAITCNT register)
            return (accessWidth == 32) ? waitstatesNonseq32[0x0E] : waitstatesNonseq16[0x0E];
        }
            
        default:
            // Unmapped memory - no wait states (open bus)
            return 1;
    }
}

void Memory::addWaitCycles(uint32_t address, uint32_t accessWidth) const {
    // Skip wait cycles if disabled (e.g., during tracer reads or instruction fetch)
    if (disableWaitCycles) {
        return;
    }
    
    // ARM7TDMI memory access timing:
    // Data accesses (LDR/STR) cost 1 base cycle + extra wait states.
    // The wait state tables store EXTRA waits beyond the base 1 cycle
    // (matching mGBA's model), so we add 1 here.
    //
    // For block transfers (LDM/STM), the first access is non-sequential
    // and subsequent accesses to the same region are sequential.
    // The nextDataAccessSequential flag is set by LDM/STM code.
    uint32_t waitCycles;
    if (nextDataAccessSequential) {
        waitCycles = 1 + getSeqWaitStates(address, accessWidth);
        nextDataAccessSequential = false;
    } else {
        waitCycles = 1 + getNonseqWaitStates(address, accessWidth);
    }

    // VRAM/Palette/OAM bus contention during HDraw (visible scanlines, not HBlank):
    // The GPU reads these regions during active display, causing +1 wait state
    // for any CPU access.  hDrawActive is set/cleared by the GPU scheduler.
    if (hDrawActive) {
        uint8_t region = (address >> 24) & 0xFF;
        if (region == 0x05 || region == 0x06 || region == 0x07) {
            waitCycles += 1;
        }
    }
    
    // Track whether data accesses target non-ROM addresses (for prefetch buffer)
    if (accumulatingCycles) {
        uint8_t dataRegion = (address >> 24) & 0xFF;
        if (dataRegion < 0x08) {
            hadNonRomDataAccess = true;
            nonRomDataCycles += waitCycles;
        } else if (dataRegion >= 0x08 && dataRegion <= 0x0D) {
            // ROM data access uses the Game Pak bus, blocking prefetch
            hadRomDataAccess = true;
        }
    }
    
    if (accumulatingCycles) {
        // During CPU instruction execution: accumulate for end-of-instruction commit.
        // This prevents timer/other I/O side effects from seeing mid-instruction
        // cycle values (matches mGBA's local currentCycles model).
        pendingDataCycles += waitCycles;
    } else if (scheduler) {
        // Outside instruction execution (DMA, etc.): advance scheduler immediately
        scheduler->advanceCycles(waitCycles);
    }
}

// Game Pak prefetch buffer stall reduction (matches mGBA's GBAMemoryStall).
//
// When the CPU is executing from ROM with prefetch enabled, and a data access
// stalls the CPU on a non-ROM address (e.g. EWRAM, IWRAM), the prefetch unit
// continues fetching sequential halfwords from ROM into its 8-entry buffer.
// When the CPU resumes, those instruction fetches are free.
//
// The model is retroactive: we compute how many S-cycle ROM halfword fetches
// fit in the data stall time, then reduce the wait accordingly.
// The return value CAN be negative — this represents a net cycle credit from
// the prefetch buffer's N→S conversion and absorbed fetches.
int32_t Memory::prefetchStall(uint32_t pc, int32_t waitCycles, bool isThumb) const {
    (void)isThumb;  // mGBA always uses halfword (WORD_SIZE_THUMB=2) for prefetch tracking
    uint8_t pcRegion = (pc >> 24) & 0xFF;
    
    // Only benefits execution from ROM (regions 0x08-0x0D) with prefetch enabled
    if (pcRegion < 0x08 || pcRegion > 0x0D || !prefetchEnabled) {
        return waitCycles;
    }
    
    int32_t previousLoads = 0;
    
    // Don't prefetch too much if we're overlapping with a previous prefetch.
    // Unsigned subtraction intentional: if lastPrefetchedPc < pc, dist wraps
    // to a large value (>= 16), giving previousLoads = 0. Matches mGBA.
    uint32_t dist = (lastPrefetchedPc - pc);
    int32_t maxLoads = 8;  // Buffer holds 8 halfwords
    if (dist < 16) {
        previousLoads = dist >> 1;
        maxLoads -= previousLoads;
    }
    
    // Figure out how many sequential loads we can jam in.
    // First fetch costs s+1, subsequent fetches cost just s (mGBA model).
    int32_t s = waitstatesSeq16[pcRegion];
    int32_t stall = s + 1;
    int32_t loads = 1;
    
    while (stall < waitCycles && loads < maxLoads) {
        stall += s;
        ++loads;
    }
    
    // Update how far ahead we've prefetched (always in halfword units)
    Memory* self = const_cast<Memory*>(this);
    self->lastPrefetchedPc = pc + 2 * (loads + previousLoads - 1);
    
    if (stall > waitCycles) {
        // The wait cannot take less time than the prefetch stalls
        waitCycles = stall;
    }
    
    // This instruction used to have an N, convert it to an S.
    waitCycles -= (int32_t)waitstatesNonseq16[pcRegion] - s;
    
    // The next |loads| S waitstates disappear entirely, so long as
    // they're all in a row
    waitCycles -= stall;
    
    return waitCycles;
}

uint32_t Memory::getWaitStates(uint32_t address, uint32_t accessWidth) const {
    return calculateWaitStates(address, accessWidth);
}

bool Memory::loadROM(const char* filepath) {
    FILE* romFile = fopen(filepath, "rb");
    if (!romFile) {
        fprintf(stderr, "Error: Failed to open ROM file: %s\n", filepath);
        return false;
    }
    
    // Get file size
    fseek(romFile, 0, SEEK_END);
    long fileSize = ftell(romFile);
    fseek(romFile, 0, SEEK_SET);
    
    if (fileSize < 0) {
        fprintf(stderr, "Error: Failed to determine ROM file size\n");
        fclose(romFile);
        return false;
    }
    
    // Validate ROM size (typical GBA ROMs are up to 32MB)
    if (fileSize > 32 * 1024 * 1024) {
        fprintf(stderr, "Warning: ROM file larger than 32MB (%ld bytes), truncating\n", fileSize);
        fileSize = 32 * 1024 * 1024;
    }
    
    // Minimum valid GBA ROM size (must have header)
    if (fileSize < 192) {
        fprintf(stderr, "Error: ROM file too small to be valid GBA ROM (%ld bytes)\n", fileSize);
        fclose(romFile);
        return false;
    }
    
    // Read ROM into buffer
    size_t read = fread(rom, 1, fileSize, romFile);
    fclose(romFile);
    
    if (read != (size_t)fileSize) {
        fprintf(stderr, "Error: Failed to read complete ROM file (read %zu of %ld bytes)\n", read, fileSize);
        return false;
    }
    
    // Store actual ROM size for open bus detection
    romSize = read;
    
    // Fill remaining ROM space with GBA open bus pattern instead of zeros.
    // On real GBA hardware, reading from ROM addresses beyond the loaded ROM
    // returns (address >> 1) per halfword — the "open bus" value.
    // Pre-filling the buffer with this pattern makes all read sizes (8/16/32)
    // and all access paths (CPU, DMA, SWI) return correct values automatically.
    size_t fillStart = (read + 1) & ~(size_t)1; // Round up to halfword boundary
    if (read < fillStart && fillStart <= 32 * 1024 * 1024) {
        // Zero the gap byte if ROM size was odd
        rom[read] = 0;
    }
    for (size_t i = fillStart; i < 32 * 1024 * 1024; i += 2) {
        uint16_t openbus = (uint16_t)(i >> 1);
        rom[i]     = openbus & 0xFF;
        rom[i + 1] = (openbus >> 8) & 0xFF;
    }
    
    printf("ROM loaded: %zu bytes from %s\n", read, filepath);
    
    // Display ROM header info
    char title[13];
    memcpy(title, rom + 0xA0, 12);
    title[12] = '\0';
    printf("ROM Title: %s\n", title);
    printf("Game Code: %.4s\n", rom + 0xAC);
    printf("Maker Code: %.2s\n", rom + 0xB0);
    
    return true;
}

bool Memory::loadBIOS(const char* filepath) {
    FILE* biosFile = fopen(filepath, "rb");
    if (!biosFile) {
        fprintf(stderr, "Error: Failed to open BIOS file: %s\n", filepath);
        return false;
    }
    
    // Get file size
    fseek(biosFile, 0, SEEK_END);
    long fileSize = ftell(biosFile);
    fseek(biosFile, 0, SEEK_SET);
    
    if (fileSize < 0) {
        fprintf(stderr, "Error: Failed to determine BIOS file size\n");
        fclose(biosFile);
        return false;
    }
    
    // GBA BIOS is exactly 16KB
    if (fileSize != 16 * 1024) {
        fprintf(stderr, "Warning: BIOS file is %ld bytes (expected 16384 bytes)\n", fileSize);
        if (fileSize > 16 * 1024) {
            fileSize = 16 * 1024;
        }
    }
    
    // Read BIOS into buffer
    size_t read = fread(bios, 1, fileSize, biosFile);
    fclose(biosFile);
    
    if (read != (size_t)fileSize) {
        fprintf(stderr, "Error: Failed to read complete BIOS file (read %zu of %ld bytes)\n", read, fileSize);
        return false;
    }
    
    // Zero-fill remaining BIOS space if needed
    if (read < 16 * 1024) {
        memset(bios + read, 0, 16 * 1024 - read);
    }
    
    printf("BIOS loaded: %zu bytes from %s\n", read, filepath);
    
    return true;
}

// Initialize wait state tables based on GBA memory regions.
// Values are EXTRA wait states beyond the base 1-cycle access,
// matching mGBA's GBA_BASE_WAITSTATES model exactly.
// The base 1 cycle is added by addWaitCycles() for data accesses
// and by the CPU instruction timing for fetches.
void Memory::initWaitStateTables() {
    // Initialize all to 0 extra waits by default
    for (int i = 0; i < 256; i++) {
        waitstatesNonseq32[i] = 0;
        waitstatesNonseq16[i] = 0;
        waitstatesSeq32[i] = 0;
        waitstatesSeq16[i] = 0;
    }
    
    // mGBA GBA_BASE_WAITSTATES values (extra waits, not total):
    //   BIOS(0)=0  EWRAM(2)=2/5  IWRAM(3)=0  I/O(4)=0
    //   Palette(5)=0/1  VRAM(6)=0/1  OAM(7)=0
    
    // BIOS (0x00): 0 extra wait states
    // (already 0 from initialization)
    
    // EWRAM (0x02): 16-bit bus — 2 extra waits for 16-bit, 5 for 32-bit
    waitstatesNonseq32[0x02] = 5;
    waitstatesNonseq16[0x02] = 2;
    waitstatesSeq32[0x02] = 5;     // Sequential same as nonseq for EWRAM
    waitstatesSeq16[0x02] = 2;
    
    // IWRAM (0x03): 32-bit bus, 0 extra wait states (already 0)
    // I/O (0x04): 0 extra wait states (already 0)
    
    // Palette (0x05): 16-bit bus — 0 extra for 16-bit, 1 extra for 32-bit
    waitstatesNonseq32[0x05] = 1;
    waitstatesSeq32[0x05] = 1;
    
    // VRAM (0x06): 16-bit bus — 0 extra for 16-bit, 1 extra for 32-bit
    waitstatesNonseq32[0x06] = 1;
    waitstatesSeq32[0x06] = 1;
    
    // OAM (0x07): 32-bit bus, 0 extra wait states (already 0)
    
    // ROM and SRAM: initialized via updateWaitstates() below
    // with power-on default WAITCNT = 0x0000
    updateWaitstates(0x0000);
}

// Update wait state tables when WAITCNT register (0x04000204) is written.
// Matches mGBA's GBAAdjustWaitstates() exactly.
void Memory::updateWaitstates(uint16_t waitcnt) {
    // GBA ROM waitstate lookup tables (from GBATEK / mGBA)
    // Non-sequential: WAITCNT bits map to actual wait cycles
    static const uint8_t romWaitstates[] = { 4, 3, 2, 8 };      // N access
    // Sequential: different per wait state region
    static const uint8_t romWaitstatesSeq[] = { 2, 1, 4, 1, 8, 1 }; // S access
    
    // Parse WAITCNT fields
    int sramWait =  waitcnt & 0x0003;
    int ws0    = (waitcnt & 0x000C) >> 2;
    int ws0seq = (waitcnt & 0x0010) >> 4;
    int ws1    = (waitcnt & 0x0060) >> 5;
    int ws1seq = (waitcnt & 0x0080) >> 7;
    int ws2    = (waitcnt & 0x0300) >> 8;
    int ws2seq = (waitcnt & 0x0400) >> 10;
    prefetchEnabled = (waitcnt & 0x4000) != 0;
    
    // SRAM (region 0x0E, mirror 0x0F)
    waitstatesNonseq16[0x0E] = waitstatesNonseq16[0x0F] = romWaitstates[sramWait];
    waitstatesSeq16[0x0E]    = waitstatesSeq16[0x0F]    = romWaitstates[sramWait];
    waitstatesNonseq32[0x0E] = waitstatesNonseq32[0x0F] = 2 * romWaitstates[sramWait] + 1;
    waitstatesSeq32[0x0E]    = waitstatesSeq32[0x0F]    = 2 * romWaitstates[sramWait] + 1;
    
    // ROM Wait State 0 (regions 0x08, 0x09) - 16-bit non-sequential
    waitstatesNonseq16[0x08] = waitstatesNonseq16[0x09] = romWaitstates[ws0];
    // ROM Wait State 1 (regions 0x0A, 0x0B)
    waitstatesNonseq16[0x0A] = waitstatesNonseq16[0x0B] = romWaitstates[ws1];
    // ROM Wait State 2 (regions 0x0C, 0x0D)
    waitstatesNonseq16[0x0C] = waitstatesNonseq16[0x0D] = romWaitstates[ws2];
    
    // ROM sequential 16-bit
    waitstatesSeq16[0x08] = waitstatesSeq16[0x09] = romWaitstatesSeq[ws0seq];
    waitstatesSeq16[0x0A] = waitstatesSeq16[0x0B] = romWaitstatesSeq[ws1seq + 2];
    waitstatesSeq16[0x0C] = waitstatesSeq16[0x0D] = romWaitstatesSeq[ws2seq + 4];
    
    // ROM 32-bit = N16 + 1 + S16 (non-sequential: one N + one S halfword access)
    waitstatesNonseq32[0x08] = waitstatesNonseq32[0x09] = waitstatesNonseq16[0x08] + 1 + waitstatesSeq16[0x08];
    waitstatesNonseq32[0x0A] = waitstatesNonseq32[0x0B] = waitstatesNonseq16[0x0A] + 1 + waitstatesSeq16[0x0A];
    waitstatesNonseq32[0x0C] = waitstatesNonseq32[0x0D] = waitstatesNonseq16[0x0C] + 1 + waitstatesSeq16[0x0C];
    
    // ROM 32-bit sequential = 2 * S16 + 1
    waitstatesSeq32[0x08] = waitstatesSeq32[0x09] = 2 * waitstatesSeq16[0x08] + 1;
    waitstatesSeq32[0x0A] = waitstatesSeq32[0x0B] = 2 * waitstatesSeq16[0x0A] + 1;
    waitstatesSeq32[0x0C] = waitstatesSeq32[0x0D] = 2 * waitstatesSeq16[0x0C] + 1;
}

uint32_t Memory::getNonseqWaitStates(uint32_t address, uint32_t accessWidth) const {
    uint8_t region = (address >> 24) & 0xFF;
    return (accessWidth == 32) ? waitstatesNonseq32[region] : waitstatesNonseq16[region];
}

uint32_t Memory::getSeqWaitStates(uint32_t address, uint32_t accessWidth) const {
    uint8_t region = (address >> 24) & 0xFF;
    return (accessWidth == 32) ? waitstatesSeq32[region] : waitstatesSeq16[region];
}
