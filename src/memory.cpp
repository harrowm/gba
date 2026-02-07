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

Memory::Memory(bool testMode) {
    if (testMode) {
        // Only allocate and map test RAM at 0x00000000 (32KB)
        test_ram = (uint8_t*)std::malloc(32 * 1024);
        
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
        io = (uint8_t*)std::malloc(1 * 1024);
        memset(io, 0, 1 * 1024);  // Zero-initialize
        regionTable[0x04000000 / BLOCK_SIZE] = io;
        
        // Allocate palette RAM in test mode (GPU needs it for rendering)
        palette = (uint8_t*)std::malloc(1 * 1024);
        memset(palette, 0, 1 * 1024);  // Zero-initialize  
        for (uint32_t addr = 0x05000000; addr < 0x05000400; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = palette;
        
        // All other region pointers remain null
        bios = wram = iwram = vram = oam = rom = sram = nullptr;

    } else {
        // --- BIOS: 16KB at 0x00000000 ---
        bios = (uint8_t*)std::malloc(16 * 1024);
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
        wram = (uint8_t*)std::calloc(256 * 1024, 1);
        for (uint32_t addr = 0x02000000; addr < 0x03000000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = wram + ((addr - 0x02000000) % (256 * 1024));

        // --- IWRAM: 32KB at 0x03000000, mirrored throughout 0x03000000-0x03FFFFFF ---
        iwram = (uint8_t*)std::calloc(32 * 1024, 1);
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
        io = (uint8_t*)std::malloc(BLOCK_SIZE);
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
        palette = (uint8_t*)std::malloc(BLOCK_SIZE);
        memset(palette, 0, BLOCK_SIZE);
        for (uint32_t addr = 0x05000000; addr < 0x06000000; addr += BLOCK_SIZE)
            regionTable[addr / BLOCK_SIZE] = palette;

        // --- VRAM: 96KB at 0x06000000, mirrored in 128KB ---
        vram = (uint8_t*)std::malloc(96 * 1024);
        // Map first 64KB block (0x06000000-0x0600FFFF)
        regionTable[0x06000000 / BLOCK_SIZE] = vram;
        // Map second 64KB block (0x06010000-0x0601FFFF) - only first 32KB is real VRAM
        regionTable[0x06010000 / BLOCK_SIZE] = vram + 64 * 1024;  // Points to second half of VRAM

        // --- OAM: 1KB at 0x07000000, mirrored throughout 0x07000000-0x07FFFFFF ---
        // Allocate full block to prevent overflow (get_region_base handles 1KB mirroring)
        oam = (uint8_t*)std::malloc(BLOCK_SIZE);
        memset(oam, 0, BLOCK_SIZE);
        for (uint32_t addr = 0x07000000; addr < 0x08000000; addr += BLOCK_SIZE)
            regionTable[addr / BLOCK_SIZE] = oam;

        // --- Game Pak ROM: up to 32MB at 0x08000000 ---
        rom = (uint8_t*)std::malloc(32 * 1024 * 1024);
        memset(rom, 0, 32 * 1024 * 1024);  // Initialize with zeros
        for (uint32_t addr = 0x08000000; addr < 0x0A000000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = rom + (addr - 0x08000000);

        // --- Game Pak SRAM: 64KB at 0x0E000000 ---
        sram = (uint8_t*)std::malloc(64 * 1024);
        regionTable[0x0E000000 / BLOCK_SIZE] = sram;

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
    // VRAM mirroring is handled by the base pointer mapping in the regionTable
    // Don't remap offset here - it causes out-of-bounds access
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
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) {
        // ROM open bus: out-of-bounds ROM reads return halfword address reflection
        uint32_t region = address >> 24;
        if (region >= 0x08 && region <= 0x0D) {
            uint16_t openbus = (address >> 1);
            return (address & 1) ? (openbus >> 8) : (openbus & 0xFF);
        }
        return 0xFF;
    }
    // Debug: Print reads from logo and entry point
    // if (address == 0x0800009C || address == 0x080000B4) {
    //     printf("[Memory::read8] Read from 0x%08X: 0x%02X\n", address, base[offset]);
    // }
    // Debug: Print POSTFLG reads
    // if (address == 0x04000300) {
    //     printf("[Memory::read8] POSTFLG read: 0x%02X\n", base[offset]);
    // }
    return base[offset];
}

void Memory::write8(uint32_t address, uint8_t value) {
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
    addWaitCycles(address, 16);
    
    // Handle DMA register reads (word count and control only)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 8) {  // Word count (DMAxCNT_L)
                uint16_t count = dmaController->readWordCount(channelID);
                LOG_DMA("[DMA%d] Read Word Count: 0x%04X\n", channelID, count);
                return count;
            } else if (regOffset == 10) {  // Control (DMAxCNT_H)
                uint16_t ctrl = dmaController->readControl(channelID);
                LOG_DMA("[DMA%d] Read Control: 0x%04X (Enable=%d)\n", channelID, ctrl, (ctrl >> 15) & 1);
                return ctrl;
            }
        }
    }
    
    // Handle timer register reads
    if (timerController) {
        if (address >= 0x04000100 && address <= 0x0400010E) {
            int timerID = (address - 0x04000100) / 4;
            bool isControl = ((address - 0x04000100) % 4) == 2;
            
            if (isControl) {
                return timerController->readControl(timerID);
            } else {
                return timerController->readCounter(timerID);
            }
        }
    }
    
    // GBA LDRH/STRH force-align: bit 0 of address is ignored by the bus
    address &= ~1u;
    
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) {
        // ROM open bus: out-of-bounds ROM reads return halfword address reflection
        uint32_t region = address >> 24;
        if (region >= 0x08 && region <= 0x0D) {
            return (address >> 1) & 0xFFFF;
        }
        return 0xFFFF;
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
                LOG_VRAM("[OBJ VRAM Write #%d] addr=0x%08X (tile %d, offset 0x%06X) val=0x%04X\n",
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
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return 0xFF;
    return base[offset];
}

uint16_t Memory::readDirectIO16(uint32_t address) const {
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
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
    uint8_t* base = get_region_base(this->regionTable, address, offset);
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
               bios18_read_count, offset, b0, b1, b2, b3, val, (void*)base);
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
    addWaitCycles(address, 32);
    
    // Debug: trace reads from IRQ handler pointer
    if (address == 0x03FFFFFC || address == 0x03007FFC) {
        static int irq_ptr_read_count = 0;
        if (irq_ptr_read_count++ < 100) {
            LOG_IRQ("[IRQ PTR READ #%d] Reading from 0x%08X\n", irq_ptr_read_count, address);
        }
    }
    
    // Handle DMA register reads (source and dest addresses)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 0) {  // Source address (DMAxSAD)
                return dmaController->readSourceAddress(channelID);
            } else if (regOffset == 4) {  // Dest address (DMAxDAD)
                return dmaController->readDestAddress(channelID);
            }
        }
    }
    
    // ARM7TDMI bus: force-align to word boundary, return aligned word.
    // NOTE: The LDR instruction applies rotation for misaligned addresses;
    // LDM, DMA, and other bus masters just get the aligned word.
    uint32_t aligned_address = address & ~3u;  // Align to 4-byte boundary
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), aligned_address, offset);
    if (!base) {
        // ROM open bus: out-of-bounds ROM reads return halfword address reflection
        uint32_t region = aligned_address >> 24;
        if (region >= 0x08 && region <= 0x0D) {
            uint16_t lo = (aligned_address >> 1) & 0xFFFF;
            uint16_t hi = ((aligned_address + 2) >> 1) & 0xFFFF;
            return lo | ((uint32_t)hi << 16);
        }
        return 0xFFFFFFFF;
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
    addWaitCycles(address, 32);
    
    // GBA read-only regions: writes are silently ignored
    // BIOS ROM (0x00) and Game Pak ROM (0x08-0x0D) are not writable
    uint32_t region = address >> 24;
    if (region == 0x00 || (region >= 0x08 && region <= 0x0D)) return;
    
    // Handle DMA register writes (source and dest addresses)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 0) {  // Source address (DMAxSAD)
                LOG_DMA("[DMA%d] Write Source Address: 0x%08X\n", channelID, value);
                dmaController->writeSourceAddress(channelID, value);
                return;  // Don't write to memory
            } else if (regOffset == 4) {  // Dest address (DMAxDAD)
                LOG_DMA("[DMA%d] Write Dest Address: 0x%08X\n", channelID, value);
                dmaController->writeDestAddress(channelID, value);
                return;  // Don't write to memory
            } else if (regOffset == 8) {  // Word count + Control (DMAxCNT as 32-bit write)
                uint16_t word_count = value & 0xFFFF;
                uint16_t control = (value >> 16) & 0xFFFF;
                LOG_DMA("[DMA%d] Write32 CNT: WordCount=0x%04X Control=0x%04X (Enable=%d, Mode=%d, 32bit=%d)\n", 
                       channelID, word_count, control, 
                       (control >> 15) & 1, (control >> 12) & 3, (control >> 10) & 1);
                dmaController->writeWordCount(channelID, word_count);
                dmaController->writeControl(channelID, control);
                return;  // Don't write to memory
            }
        }
    }
    
    // Handle sound FIFO writes (0x040000A0 = FIFO_A, 0x040000A4 = FIFO_B)
    if (apu) {
        if (address == 0x040000A0) {
            apu->writeFIFO_A(value);
            return;  // FIFO is write-only, don't store in memory
        } else if (address == 0x040000A4) {
            apu->writeFIFO_B(value);
            return;
        }
        // Handle other sound register 32-bit writes
        if (address >= 0x04000060 && address <= 0x040000A6) {
            apu->write32(address, value);
            // Also write to memory for debug reads
        }
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
                   address, value, (void*)base, (void*)vram, offset);
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
        case 0x0D:
            // Default: 5 cycles for first access (non-sequential)
            // TODO: Make configurable via WAITCNT register
            // 16-bit bus: 5 cycles for 8/16-bit, 8 cycles for 32-bit (5+3 sequential)
            return (accessWidth == 32) ? 8 : 5;
            
        case 0x0E: // Game Pak SRAM
            // 8-bit bus: 5 cycles for any access
            // TODO: Make configurable via WAITCNT register
            return 5;
            
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
    
    if (scheduler) {
        // ARM7TDMI memory access timing:
        // Data accesses (LDR/STR) cost 1I + 1N cycles where:
        // - 1I = internal cycle (counted in arm_timing.c base cycles)
        // - 1N = nonsequential memory access cycle (counted here)
        //
        // The instruction prefetch happens in parallel and doesn't add cycles.
        // We charge the full nonsequential wait state for data accesses.
        //
        // Examples:
        // - BIOS/I/O: nonseq=1 → +1 cycle (total 2 with base internal)
        // - IWRAM: nonseq=1 → +1 cycle (total 2)
        // - ROM 32-bit: nonseq=5 → +5 cycles (total 6)
        uint32_t waitCycles = getNonseqWaitStates(address, accessWidth);
        scheduler->advanceCycles(waitCycles);
    }
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
    
    // Zero-fill remaining ROM space
    if (read < 32 * 1024 * 1024) {
        memset(rom + read, 0, 32 * 1024 * 1024 - read);
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

// Initialize wait state tables based on GBA memory regions
// This matches mGBA's wait state model for cycle-accurate timing
void Memory::initWaitStateTables() {
    // Based on ARM7TDMI specifications and GBA hardware:
    // - BIOS (0x00): 1 cycle for any access (0 wait states)
    // - EWRAM (0x02): 3 cycles for 8/16-bit, 6 cycles for 32-bit (2/5 wait states)
    // - IWRAM (0x03): 1 cycle for any access (0 wait states, fastest)
    // - I/O (0x04): 1 cycle for any access
    // - Palette (0x05): 1 cycle for 16-bit, 2 cycles for 32-bit
    // - VRAM (0x06): 1 cycle for 16-bit, 2 cycles for 32-bit
    // - OAM (0x07): 1 cycle for any access
    // - ROM (0x08-0x0D): Configurable via WAITCNT, default 4+1 cycles
    // - SRAM (0x0E): 5 cycles (4 wait states)
    
    // Initialize all to 1 cycle by default
    for (int i = 0; i < 256; i++) {
        waitstatesNonseq32[i] = 1;
        waitstatesNonseq16[i] = 1;
        waitstatesSeq32[i] = 1;
        waitstatesSeq16[i] = 1;
    }
    
    // BIOS (0x00): 1 cycle, same for seq and nonseq
    waitstatesNonseq32[0x00] = 1;
    waitstatesNonseq16[0x00] = 1;
    waitstatesSeq32[0x00] = 1;
    waitstatesSeq16[0x00] = 1;
    
    // EWRAM (0x02): 16-bit bus, slower
    waitstatesNonseq32[0x02] = 6;  // 32-bit: two 16-bit accesses
    waitstatesNonseq16[0x02] = 3;  // 16-bit: base cost
    waitstatesSeq32[0x02] = 6;     // Sequential same as nonseq for EWRAM
    waitstatesSeq16[0x02] = 3;
    
    // IWRAM (0x03): 32-bit bus, fastest (1 cycle)
    waitstatesNonseq32[0x03] = 1;
    waitstatesNonseq16[0x03] = 1;
    waitstatesSeq32[0x03] = 1;
    waitstatesSeq16[0x03] = 1;
    
    // I/O (0x04): 1 cycle
    waitstatesNonseq32[0x04] = 1;
    waitstatesNonseq16[0x04] = 1;
    waitstatesSeq32[0x04] = 1;
    waitstatesSeq16[0x04] = 1;
    
    // Palette (0x05): 16-bit bus
    waitstatesNonseq32[0x05] = 2;  // 32-bit: two 16-bit accesses
    waitstatesNonseq16[0x05] = 1;
    waitstatesSeq32[0x05] = 2;
    waitstatesSeq16[0x05] = 1;
    
    // VRAM (0x06): 16-bit bus
    waitstatesNonseq32[0x06] = 2;
    waitstatesNonseq16[0x06] = 1;
    waitstatesSeq32[0x06] = 2;
    waitstatesSeq16[0x06] = 1;
    
    // OAM (0x07): 32-bit bus, fast
    waitstatesNonseq32[0x07] = 1;
    waitstatesNonseq16[0x07] = 1;
    waitstatesSeq32[0x07] = 1;
    waitstatesSeq16[0x07] = 1;
    
    // ROM (0x08-0x0D): Default to 4+1 cycles (will be configurable via WAITCNT later)
    // For now, use conservative defaults matching real hardware power-on state
    for (int region = 0x08; region <= 0x0D; region++) {
        waitstatesNonseq32[region] = 5;  // 4 wait + 1 cycle
        waitstatesNonseq16[region] = 5;
        waitstatesSeq32[region] = 3;     // Sequential accesses are faster
        waitstatesSeq16[region] = 3;
    }
    
    // SRAM (0x0E): Very slow, 5 cycles
    waitstatesNonseq32[0x0E] = 5;
    waitstatesNonseq16[0x0E] = 5;
    waitstatesSeq32[0x0E] = 5;
    waitstatesSeq16[0x0E] = 5;
}

uint32_t Memory::getNonseqWaitStates(uint32_t address, uint32_t accessWidth) const {
    uint8_t region = (address >> 24) & 0xFF;
    return (accessWidth == 32) ? waitstatesNonseq32[region] : waitstatesNonseq16[region];
}
