// Memory class implementation for GBA emulator (region pointer table version)
#include "memory.h"
#include "scheduler.h"
#include "timer_controller.h"
#include "dma.h"
#include "debug.h"
#include <cstring>
#include <cstdint>
#include <cstdlib>

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

        // --- WRAM: 256KB at 0x02000000 ---
        wram = (uint8_t*)std::malloc(256 * 1024);
        for (uint32_t addr = 0x02000000; addr < 0x02040000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = wram + (addr - 0x02000000);

        // --- IWRAM: 32KB at 0x03000000, mirrored throughout 0x03000000-0x03FFFFFF ---
        iwram = (uint8_t*)std::malloc(32 * 1024);
        // Mirror IWRAM every 64KB across the entire 16MB range
        // Each 64KB block in the 0x03000000-0x03FFFFFF range should point to IWRAM
        // but the actual IWRAM is only 32KB, so we need special handling in the read/write functions
        for (uint32_t addr = 0x03000000; addr < 0x04000000; addr += BLOCK_SIZE) {
            regionTable[addr / BLOCK_SIZE] = iwram;
        }

        // --- I/O: 1KB at 0x04000000 ---
        io = (uint8_t*)std::malloc(1 * 1024);
        memset(io, 0, 1 * 1024);  // Zero-initialize IO memory
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

        // --- Palette RAM: 1KB at 0x05000000 ---
        palette = (uint8_t*)std::malloc(1 * 1024);
        regionTable[0x05000000 / BLOCK_SIZE] = palette;

        // --- VRAM: 96KB at 0x06000000, mirrored in 128KB ---
        vram = (uint8_t*)std::malloc(96 * 1024);
        // Map first 64KB block (0x06000000-0x0600FFFF)
        regionTable[0x06000000 / BLOCK_SIZE] = vram;
        // Map second 64KB block (0x06010000-0x0601FFFF) - only first 32KB is real VRAM
        regionTable[0x06010000 / BLOCK_SIZE] = vram + 64 * 1024;  // Points to second half of VRAM

        // --- OAM: 1KB at 0x07000000, mirrored in 8KB ---
        oam = (uint8_t*)std::malloc(1 * 1024);
        for (uint32_t addr = 0x07000000; addr < 0x07002000; addr += BLOCK_SIZE)
            regionTable[(addr & 0x0FFFFFFF) / BLOCK_SIZE] = oam;

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
    // IWRAM mirroring: 32KB at 0x03000000, mirrored every 32KB in 0x03000000-0x03FFFFFF
    if (address >= 0x03000000 && address < 0x04000000 && base) {
        offset = (address - 0x03000000) % 0x8000; // Mirror every 32KB
    }
    // VRAM mirroring is handled by the base pointer mapping in the regionTable
    // Don't remap offset here - it causes out-of-bounds access
    // OAM mirroring: 0x07000000–0x07001FFF, 1KB mirrored in 8KB
    if (address >= 0x07000000 && address < 0x07002000 && base) {
        offset = (address - 0x07000000) % 1024;
    }
    return base;
}

uint8_t Memory::read8(uint32_t address) const {
    addWaitCycles(address, 8);
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFF;
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
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    // Feature detection: OAM writes (sprite data)
    static bool oam_logged = false;
    if (address >= 0x07000000 && address < 0x07000400 && !oam_logged) {
        printf("[FEATURE] ROM writing to OAM (sprite attribute memory) at 0x%08X\n", address);
        oam_logged = true;
    }
    
    // Debug: Track VRAM writes (commented out - verified working)
    // if (address >= 0x06000000 && address < 0x06018000) {
    //     printf("[VRAM Write8] Address: 0x%08X, Value: 0x%02X\n", address, value);
    // }
    
    // Debug: Track POSTFLG writes
    if (address == 0x04000300) {
        printf("[Memory::write8] POSTFLG write: 0x%02X (was 0x%02X)\n", value, base[offset]);
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
                printf("[DMA%d] Read Word Count: 0x%04X\n", channelID, count);
                return count;
            } else if (regOffset == 10) {  // Control (DMAxCNT_H)
                uint16_t ctrl = dmaController->readControl(channelID);
                printf("[DMA%d] Read Control: 0x%04X (Enable=%d)\n", channelID, ctrl, (ctrl >> 15) & 1);
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
    
    uint32_t rot = (address & 1) * 8;
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), address, offset);
    if (!base) return 0xFFFF;
    uint16_t val = base[offset] | (base[(offset + 1) % Memory::BLOCK_SIZE] << 8);
    return (val >> rot) | (val << (16 - rot));
}

void Memory::write16(uint32_t address, uint16_t value) {
    addWaitCycles(address, 16);
    
    // Handle DMA register writes (word count and control only)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 8) {  // Word count (DMAxCNT_L)
                printf("[DMA%d] Write Word Count: 0x%04X (%d transfers)\n", channelID, value, value ? value : 65536);
                dmaController->writeWordCount(channelID, value);
                return;  // Don't write to memory
            } else if (regOffset == 10) {  // Control (DMAxCNT_H)
                printf("[DMA%d] Write Control: 0x%04X (Enable=%d, Mode=%d, 32bit=%d)\n", 
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
    
    // Debug palette writes
    if (address >= 0x05000000 && address < 0x05000400) {
        static int pal_count = 0;
        if (pal_count++ < 10) {
            uint32_t index = (address - 0x05000000) / 2;
            printf("[Palette Write #%d] Index %d (addr=0x%08X): 0x%04X (R=%d, G=%d, B=%d)\n", 
                   pal_count, index, address, value, value & 0x1F, (value >> 5) & 0x1F, (value >> 10) & 0x1F);
        }
    }
    
    uint32_t rot = (address & 1) * 8;
    uint16_t val = (value << rot) | (value >> (16 - rot));
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    // Log writes to IE register (Interrupt Enable)
    if (address == 0x04000200) {  // REG_IE
        printf("[REG Write] IE (Interrupt Enable) = 0x%04X\n", val);
    }
    
    // Log writes to IME register (Interrupt Master Enable)
    if (address == 0x04000208) {  // REG_IME
        printf("[REG Write] IME (Interrupt Master Enable) = 0x%04X\n", val);
    }
    
    // Special handling for IF register (write 1 to clear)
    if (address == 0x04000202) {  // REG_IF
        uint16_t currentIF = base[offset] | (base[(offset + 1) % Memory::BLOCK_SIZE] << 8);
        uint16_t newIF = currentIF & ~val;  // Clear bits where value has 1
        base[offset] = newIF & 0xFF;
        base[(offset + 1) % Memory::BLOCK_SIZE] = (newIF >> 8) & 0xFF;
        static int if_write_count = 0;
        if (if_write_count++ < 10) {
            printf("[REG Write #%d] IF acknowledge = 0x%04X, currentIF was = 0x%04X, new IF = 0x%04X\n", 
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
        bool bg0 = (value >> 8) & 1;
        bool bg1 = (value >> 9) & 1;
        bool bg2 = (value >> 10) & 1;
        bool bg3 = (value >> 11) & 1;
        bool obj = (value >> 12) & 1;
        bool win0 = (value >> 13) & 1;
        bool win1 = (value >> 14) & 1;
        bool objWin = (value >> 15) & 1;
        
        printf("[DISPCNT] Write 0x%04X: Mode=%d BG0=%d BG1=%d BG2=%d BG3=%d OBJ=%d Win0=%d Win1=%d ObjWin=%d\n",
               value, mode, bg0, bg1, bg2, bg3, obj, win0, win1, objWin);
        
        // Log features being used
        if (mode > 0 && !feature_logged[0]) {
            printf("[FEATURE] ROM using video Mode %d (bitmap/affine modes)\n", mode);
            feature_logged[0] = true;
        }
        if (bg1 && !feature_logged[1]) {
            printf("[FEATURE] ROM enabling BG1 (text/affine background layer)\n");
            feature_logged[1] = true;
        }
        if (bg2 && !feature_logged[2]) {
            printf("[FEATURE] ROM enabling BG2 (text/affine background layer)\n");
            feature_logged[2] = true;
        }
        if (bg3 && !feature_logged[3]) {
            printf("[FEATURE] ROM enabling BG3 (text/affine background layer)\n");
            feature_logged[3] = true;
        }
        if (obj && !feature_logged[4]) {
            printf("[FEATURE] ROM enabling OBJ (sprites/objects)\n");
            feature_logged[4] = true;
        }
        if ((win0 || win1 || objWin) && !feature_logged[5]) {
            printf("[FEATURE] ROM enabling Windows (win0=%d win1=%d objWin=%d)\n", win0, win1, objWin);
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
        
        printf("[BG%dCNT] Write 0x%04X: Priority=%d CharBase=%d Mosaic=%d Colors=%s ScreenBase=%d Wrap=%d Size=%d\n",
               bgNum, value, priority, charBase, mosaic, colors ? "256" : "16", screenBase, wraparound, screenSize);
        feature_logged[10 + bgNum] = true;
    }
    
    // Track affine parameters (BG2/BG3 rotation/scaling)
    if (address >= 0x04000020 && address <= 0x0400003F && !feature_logged[20]) {
        printf("[FEATURE] ROM writing BG affine parameters (rotation/scaling) at 0x%08X = 0x%04X\n", address, value);
        feature_logged[20] = true;
    }
    
    // Track blending registers
    if (address >= 0x04000050 && address <= 0x04000054 && !feature_logged[21]) {
        printf("[FEATURE] ROM enabling color blending/effects at 0x%08X = 0x%04X\n", address, value);
        feature_logged[21] = true;
    }
    
    // Track mosaic
    if (address == 0x0400004C && !feature_logged[22]) {
        printf("[FEATURE] ROM enabling mosaic effect = 0x%04X\n", value);
        feature_logged[22] = true;
    }
    
    base[offset] = val & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (val >> 8) & 0xFF;
}

void Memory::writeDirectIO(uint32_t address, uint16_t value) {
    // Direct write to I/O registers (bypasses write-to-clear and other special handling)
    // Used by hardware components to set registers
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, address, offset);
    if (!base) return;
    
    base[offset] = value & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (value >> 8) & 0xFF;
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
    
    // ARM7TDMI Misaligned Word Load (LDR):
    // On ARM7TDMI, misaligned word loads:
    // 1. Align address down to 4-byte boundary
    // 2. Read the word from aligned address
    // 3. Rotate the value RIGHT by (misalignment * 8) bits
    // Example: LDR from 0x1003 (misaligned by 3):
    //   - Reads from 0x1000
    //   - Rotates result right by 24 bits
    uint32_t aligned_address = address & ~3u;  // Align to 4-byte boundary
    uint32_t misalignment = address & 3u;       // Get misalignment (0-3)
    uint32_t offset;
    uint8_t* base = get_region_base(const_cast<uint8_t* const*>(this->regionTable), aligned_address, offset);
    if (!base) return 0xFFFFFFFF;
    uint32_t val = base[offset]
        | (base[offset + 1] << 8)
        | (base[offset + 2] << 16)
        | (base[offset + 3] << 24);
    
    // Rotate right by misalignment * 8 bits
    if (misalignment != 0) {
        uint32_t rotate_amount = misalignment * 8;
        val = (val >> rotate_amount) | (val << (32 - rotate_amount));
    }
    // Debug: Log I/O register reads during BIOS loop
    if (address >= 0x04000000 && address < 0x04000400) {
        static int ioReadCount = 0;
        if (ioReadCount < 20) {
            printf("[I/O Read32] Address=0x%08X Value=0x%08X\n", address, val);
            ioReadCount++;
        }
    }
    // Debug: Print reads from entry point
    if (address == 0x080000B4) {
        printf("[Memory::read32] Read from 0x%08X: 0x%08X\n", address, val);
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
    
    // Handle DMA register writes (source and dest addresses)
    if (dmaController) {
        if (address >= 0x040000B0 && address <= 0x040000DE) {
            int channelID = (address - 0x040000B0) / 12;
            int regOffset = (address - 0x040000B0) % 12;
            
            if (regOffset == 0) {  // Source address (DMAxSAD)
                printf("[DMA%d] Write Source Address: 0x%08X\n", channelID, value);
                dmaController->writeSourceAddress(channelID, value);
                return;  // Don't write to memory
            } else if (regOffset == 4) {  // Dest address (DMAxDAD)
                printf("[DMA%d] Write Dest Address: 0x%08X\n", channelID, value);
                dmaController->writeDestAddress(channelID, value);
                return;  // Don't write to memory
            }
        }
    }
    
    // ARM7TDMI Misaligned Word Store (STR):
    // On ARM7TDMI, misaligned word stores:
    // 1. Align address down to 4-byte boundary
    // 2. Rotate value LEFT by (misalignment * 8) bits
    // 3. Write rotated value to aligned address
    // Example: STR 0xAABBCCDD to 0x1003 (misaligned by 3):
    //   - Rotates left by 24 bits: 0xDDAABBCC
    //   - Writes to aligned 0x1000
    uint32_t aligned_address = address & ~3u;  // Align to 4-byte boundary
    uint32_t misalignment = address & 3u;       // Get misalignment (0-3)
    uint32_t val = value;
    
    // Rotate left by misalignment * 8 bits
    if (misalignment != 0) {
        uint32_t rotate_amount = misalignment * 8;
        val = (val << rotate_amount) | (val >> (32 - rotate_amount));
    }
    
    uint32_t offset;
    uint8_t* base = get_region_base(this->regionTable, aligned_address, offset);
    if (!base) return;
    
    // Log writes that overlap IE/IF/IME registers
    if (aligned_address >= 0x04000200 && aligned_address <= 0x04000208) {
        printf("[REG Write32] Address=0x%08X Value=0x%08X (may write IE/IF/IME)\n", aligned_address, val);
    }
    
    // Log writes to IRQ handler pointer area
    // The BIOS IRQ dispatcher at 0x128 reads from [0x04000000-4] = 0x03FFFFFC
    // BIOS also uses 0x03007FF0-0x03007FFC area during initialization
    if (aligned_address == 0x03FFFFFC || (aligned_address >= 0x03007FF0 && aligned_address <= 0x03007FFC)) {
        // Get current PC for debugging (requires CPU context)
        printf("[IRQ HANDLER] Write32 to 0x%08X: value=0x%08X (IRQ handler pointer area)\n", aligned_address, val);
        if (aligned_address == 0x03FFFFFC && (val < 0x02000000 || val > 0x0FFFFFFF)) {
            printf("[IRQ HANDLER ERROR] Suspicious IRQ handler address 0x%08X written to 0x%08X!\n", val, aligned_address);
        }
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
        
        printf("[DISPCNT32] Write 0x%04X: Mode=%d BG0=%d BG1=%d BG2=%d BG3=%d OBJ=%d\n",
               dispcnt_value, mode, bg0, bg1, bg2, bg3, obj);
        
        if (mode > 0 && !feature_logged32[0]) {
            printf("[FEATURE] ROM using video Mode %d (detected in write32)\n", mode);
            feature_logged32[0] = true;
        }
        if (obj && !feature_logged32[10]) {
            printf("[FEATURE] ROM enabling sprites/OBJ (detected in write32)\n");
            feature_logged32[10] = true;
        }
    }
    
    // Track OAM writes (32-bit)
    if (aligned_address >= 0x07000000 && aligned_address < 0x07000400 && !feature_logged32[30]) {
        printf("[FEATURE] ROM writing to OAM via write32 at 0x%08X\n", aligned_address);
        feature_logged32[30] = true;
    }
    
    // Debug: Track VRAM writes
    if (address >= 0x06000000 && address < 0x06018000) {
        static int vramWriteCount = 0;
        if (vramWriteCount < 10) {
            printf("[VRAM Write32] Address: 0x%08X, Value: 0x%08X, base=%p, vram=%p, offset=%u\n", 
                   address, value, (void*)base, (void*)vram, offset);
            vramWriteCount++;
        }
    }
    
    base[offset] = val & 0xFF;
    base[(offset + 1) % Memory::BLOCK_SIZE] = (val >> 8) & 0xFF;
    base[(offset + 2) % Memory::BLOCK_SIZE] = (val >> 16) & 0xFF;
    base[(offset + 3) % Memory::BLOCK_SIZE] = (val >> 24) & 0xFF;
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
        // mGBA's cycle model: data_access_cycles = nonseq_cycles - seq_cycles
        // This accounts for the ARM7TDMI pipeline where:
        // - The instruction prefetch already costs 1 + seq_cycles
        // - Data accesses add only the EXTRA time beyond sequential access
        //
        // Examples:
        // - BIOS: nonseq=1, seq=1 → 1-1 = 0 extra cycles (access is "free")
        // - IWRAM: nonseq=1, seq=1 → 1-1 = 0 extra cycles
        // - EWRAM 32-bit: nonseq=6, seq=6 → 6-6 = 0 extra cycles (wait states same for seq/nonseq)
        // - ROM 32-bit: nonseq=5, seq=3 → 5-3 = 2 extra cycles
        //
        // This matches mGBA exactly and gives us proper cycle-accurate timing!
        uint32_t nonseq = getNonseqWaitStates(address, accessWidth);
        uint32_t seq = getSeqWaitStates(address, accessWidth);
        int32_t extraCycles = nonseq - seq;
        
        if (extraCycles > 0) {
            scheduler->advanceCycles(extraCycles);
        }
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

uint32_t Memory::getSeqWaitStates(uint32_t address, uint32_t accessWidth) const {
    uint8_t region = (address >> 24) & 0xFF;
    return (accessWidth == 32) ? waitstatesSeq32[region] : waitstatesSeq16[region];
}
