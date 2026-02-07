#include "dma.h"

extern uint32_t g_current_frame;
extern uint32_t g_cpu_pc;

// DMA debug tracking - when DMA is active, set g_cpu_pc to indicate DMA transfer
int g_current_dma_channel = -1;
uint32_t g_dma_source_addr = 0;
uint32_t g_dma_dest_addr = 0;

// Sound DMA diagnostic: track source addresses and data content
uint32_t g_soundDmaSourceA = 0, g_soundDmaSourceB = 0;
int g_soundDmaZeroWordsA = 0, g_soundDmaNonZeroWordsA = 0;
int g_soundDmaZeroWordsB = 0, g_soundDmaNonZeroWordsB = 0;
uint32_t g_soundDmaFirstWordA = 0, g_soundDmaFirstWordB = 0;
int g_soundDmaClampCountA = 0, g_soundDmaClampCountB = 0;

#include "memory.h"
#include "scheduler.h"
#include "interrupt.h"
#include "debug.h"
#include <cstdio>
#include <cstring>

// DMA Channel Implementation
DMAChannel::DMAChannel() {
    reset();
}

void DMAChannel::reset() {
    sourceAddr = 0;
    destAddr = 0;
    wordCount = 0;
    control = 0;
    internalSource = 0;
    internalDest = 0;
    internalCount = 0;
    active = false;
}

void DMAChannel::setSourceAddress(uint32_t addr) {
    sourceAddr = addr;
}

void DMAChannel::setDestAddress(uint32_t addr) {
    destAddr = addr;
}

void DMAChannel::setWordCount(uint16_t count) {
    wordCount = count;
}

void DMAChannel::setControl(uint16_t ctrl) {
    control = ctrl;
}

// DMA Controller Implementation
DMAController::DMAController() 
    : memory(nullptr), scheduler(nullptr), interruptController(nullptr) {
}

void DMAController::reset() {
    for (int i = 0; i < 4; i++) {
        channels[i].reset();
    }
}

uint32_t DMAController::readSourceAddress(int channelId) const {
    if (channelId < 0 || channelId >= 4) return 0;
    return channels[channelId].getSourceAddress();
}

uint32_t DMAController::readDestAddress(int channelId) const {
    if (channelId < 0 || channelId >= 4) return 0;
    return channels[channelId].getDestAddress();
}

uint16_t DMAController::readWordCount(int channelId) const {
    if (channelId < 0 || channelId >= 4) return 0;
    return channels[channelId].getWordCount();
}

uint16_t DMAController::readControl(int channelId) const {
    if (channelId < 0 || channelId >= 4) return 0;
    return channels[channelId].getControl();
}

void DMAController::writeSourceAddress(int channelId, uint32_t value) {
    if (channelId < 0 || channelId >= 4) return;
    
    // Mask based on channel (different channels have different address ranges)
    const uint32_t masks[4] = {
        0x07FFFFFF,  // DMA0: max 128MB
        0x0FFFFFFF,  // DMA1: max 256MB
        0x0FFFFFFF,  // DMA2: max 256MB
        0x0FFFFFFF   // DMA3: max 256MB
    };
    
    uint32_t maskedValue = value & masks[channelId];
    uint32_t oldValue = channels[channelId].getSourceAddress();
    uint32_t oldIntSrc = channels[channelId].internalSource;
    channels[channelId].setSourceAddress(maskedValue);
    
    // Log sound DMA source reprogramming
    if ((channelId == 1 || channelId == 2) && maskedValue != oldValue && g_current_frame >= 140) {
        static int srcLog = 0;
        if (srcLog++ < 40) {
            fprintf(stderr, "[DMA%d-SRC] frame=%u old=0x%08X new=0x%08X (intSrc was 0x%08X)\n",
                    channelId, g_current_frame, oldValue, maskedValue, oldIntSrc);
        }
    }
    
    // IMPORTANT: On real GBA hardware, writing to DMAxSAD always updates the internal
    // source register, regardless of whether DMA is enabled or what mode it's in.
    // This is critical for sound DMA where games reconfigure the buffer address.
    channels[channelId].internalSource = maskedValue;
}

void DMAController::writeDestAddress(int channelId, uint32_t value) {
    if (channelId < 0 || channelId >= 4) return;
    
    // Mask based on channel
    const uint32_t masks[4] = {
        0x07FFFFFF,  // DMA0: max 128MB
        0x07FFFFFF,  // DMA1: max 128MB
        0x07FFFFFF,  // DMA2: max 128MB
        0x0FFFFFFF   // DMA3: max 256MB
    };
    
    channels[channelId].setDestAddress(value & masks[channelId]);
}

void DMAController::writeWordCount(int channelId, uint16_t value) {
    if (channelId < 0 || channelId >= 4) return;
    
    // Mask based on channel (different max word counts)
    const uint16_t masks[4] = {
        0x3FFF,  // DMA0: max 16384 words
        0x3FFF,  // DMA1: max 16384 words
        0x3FFF,  // DMA2: max 16384 words
        0xFFFF   // DMA3: max 65536 words
    };
    
    channels[channelId].setWordCount(value & masks[channelId]);
}

void DMAController::writeControl(int channelId, uint16_t value) {
    if (channelId < 0 || channelId >= 4) return;
    
    bool wasEnabled = channels[channelId].isEnabled();
    channels[channelId].setControl(value);
    bool isEnabled = channels[channelId].isEnabled();
    
    // Log sound DMA control transitions
    if ((channelId == 1 || channelId == 2) && g_current_frame >= 140 && g_current_frame <= 200) {
        if (wasEnabled != isEnabled) {
            fprintf(stderr, "[DMA%d-CTRL] f%u %s→%s ctrl=0x%04X regSrc=0x%08X intSrc=0x%08X\n",
                    channelId, g_current_frame,
                    wasEnabled ? "EN" : "DIS", isEnabled ? "EN" : "DIS",
                    value, channels[channelId].getSourceAddress(),
                    channels[channelId].internalSource);
        }
    }
    
    // If DMA just got enabled, start the transfer
    if (!wasEnabled && isEnabled) {
        startTransfer(channelId);
        // Log the reload for sound DMA
        if ((channelId == 1 || channelId == 2) && g_current_frame >= 140 && g_current_frame <= 200) {
            fprintf(stderr, "[DMA%d-RELOAD] f%u intSrc=0x%08X intDst=0x%08X intCnt=%u mode=%d\n",
                    channelId, g_current_frame,
                    channels[channelId].internalSource,
                    channels[channelId].internalDest,
                    channels[channelId].internalCount,
                    (int)channels[channelId].getTimingMode());
        }
    }
    
    // If DMA got disabled, stop any active transfer
    if (wasEnabled && !isEnabled) {
        channels[channelId].active = false;
    }
}

void DMAController::startTransfer(int channelId) {
    DMAChannel& channel = channels[channelId];
    
    // Initialize internal registers
    channel.internalSource = channel.getSourceAddress();
    channel.internalDest = channel.getDestAddress();
    
    // Word count of 0 means max transfer
    if (channel.getWordCount() == 0) {
        if (channelId == 3) {
            channel.internalCount = 65536;  // DMA3: 65536 transfers
        } else {
            channel.internalCount = 16384;  // DMA0-2: 16384
        }
    } else {
        channel.internalCount = channel.getWordCount();
    }
    
    // Check timing mode
    DMATimingMode mode = channel.getTimingMode();
    
    if (mode == DMATimingMode::IMMEDIATE) {
        // Immediate DMA starts right away
        channel.active = true;
        performTransfer(channelId);
    } else {
        // Other modes wait for trigger
        channel.active = false;
        // Will be triggered by triggerVBlank(), triggerHBlank(), etc.
    }
}

void DMAController::performTransfer(int channelId) {
    if (!memory || !scheduler) return;
    
    DMAChannel& channel = channels[channelId];
    if (!channel.active) return;
    
    // Check if higher priority channel is active
    for (int i = 0; i < channelId; i++) {
        if (channels[i].active) {
            // Higher priority DMA is active, wait
            return;
        }
    }
    
    uint32_t srcAddr = channel.internalSource;
    uint32_t destAddr = channel.internalDest;
    uint16_t count = channel.internalCount;
    bool is32bit = channel.is32Bit();
    
    // Debug sound DMA data - only log non-zero transfers
    bool isSoundDMA = (channelId == 1 || channelId == 2) && 
                      (destAddr == 0x040000A0 || destAddr == 0x040000A4);
    
    // Track sound DMA source addresses and data content
    if (isSoundDMA) {
        if (destAddr == 0x040000A0) { g_soundDmaSourceA = srcAddr; g_soundDmaFirstWordA = 0; }
        else { g_soundDmaSourceB = srcAddr; g_soundDmaFirstWordB = 0; }
    }
    
    // For sound FIFO DMA: clamp reads to prevent buffer overrun.
    // M4A's PCM DMA buffers are sized as pcmDmaPeriod * samplesPerVBlank (typically 0x630).
    // Due to occasionally missed VBlank IRQs, the DMA source can advance past the
    // end of the PCM buffer into M4A internal structures, producing garbage (the "chug").
    // Safety clamp: if source has advanced more than 0x800 past the reload address,
    // substitute zero. This is generous enough to never clamp valid audio (buffer is 0x630)
    // but catches severe overruns that read ROM pointers as audio data.
    uint32_t soundDmaBase = channel.getSourceAddress();  // registered (reload) source
    static constexpr uint32_t SOUND_BUFFER_LIMIT = 0x800; // Safety net: 2KB

    LOG_DMA("[DMA%d] STARTING TRANSFER: src=0x%08X dst=0x%08X count=%d size=%s\n",
           channelId, srcAddr, destAddr, count, is32bit ? "32bit" : "16bit");
    
    // Track DMA for debug output (when memory writes happen, we can see it's from DMA)
    g_current_dma_channel = channelId;
    g_dma_source_addr = srcAddr;
    g_dma_dest_addr = destAddr;
    
    // Perform all transfers
    for (uint16_t i = 0; i < count; i++) {
        // Read from source
        uint32_t value;
        
        // Sound DMA buffer overrun safety clamp
        bool soundClamped = false;
        if (isSoundDMA && srcAddr >= soundDmaBase + SOUND_BUFFER_LIMIT) {
            value = 0;
            soundClamped = true;
        } else if (is32bit) {
            value = memory->read32(srcAddr);
        } else {
            value = memory->read16(srcAddr);
        }
        
        // Update tracking before write (so memory.cpp can see what DMA is doing)
        g_dma_source_addr = srcAddr;
        g_dma_dest_addr = destAddr;
        
        // Write to destination
        if (is32bit) {
            memory->write32(destAddr, value);
        } else {
            memory->write16(destAddr, static_cast<uint16_t>(value));
        }
        
        // Track sound DMA data content
        if (isSoundDMA) {
            if (destAddr == 0x040000A0) {
                if (i == 0) g_soundDmaFirstWordA = value;
                if (value == 0) g_soundDmaZeroWordsA++;
                else {
                    g_soundDmaNonZeroWordsA++;
                    // Log non-zero sound DMA words ONLY after BIOS boot (frame 135+)
                    if (g_current_frame > 135) {
                        static int nzLogCount = 0;
                        if (nzLogCount++ < 50) {
                            fprintf(stderr, "[SNDMA] FIFO_A non-zero! src=0x%08X word[%d]=0x%08X (bytes: %d %d %d %d) frame=%u\n",
                                    srcAddr, i, value,
                                    (int)(int8_t)(value & 0xFF),
                                    (int)(int8_t)((value >> 8) & 0xFF),
                                    (int)(int8_t)((value >> 16) & 0xFF),
                                    (int)(int8_t)((value >> 24) & 0xFF),
                                    g_current_frame);
                        }
                    }
                }
            } else {
                if (i == 0) g_soundDmaFirstWordB = value;
                if (value == 0) g_soundDmaZeroWordsB++;
                else {
                    g_soundDmaNonZeroWordsB++;
                    if (g_current_frame > 135) {
                        static int nzLogCountB = 0;
                        if (nzLogCountB++ < 50) {
                            fprintf(stderr, "[SNDMA] FIFO_B non-zero! src=0x%08X word[%d]=0x%08X (bytes: %d %d %d %d) frame=%u\n",
                                    srcAddr, i, value,
                                    (int)(int8_t)(value & 0xFF),
                                    (int)(int8_t)((value >> 8) & 0xFF),
                                    (int)(int8_t)((value >> 16) & 0xFF),
                                    (int)(int8_t)((value >> 24) & 0xFF),
                                    g_current_frame);
                        }
                    }
                }
            }
        }
        
        // Advance scheduler: 2 cycles per transfer (internal + 1 access)
        scheduler->advanceCycles(2);
        
        // Update addresses
        updateAddresses(channelId, srcAddr, destAddr);
    }
    
    // Update internal registers
    channel.internalSource = srcAddr;
    channel.internalDest = destAddr;
    
    // Check if we should repeat
    if (channel.isRepeat() && channel.getTimingMode() != DMATimingMode::IMMEDIATE) {
        // For repeat mode with HBlank/VBlank timing:
        // - Source address does NOT reload (continues from where it left off)
        //   This allows games to use incrementing source to read different values each scanline
        // - Destination with INCREMENT_RELOAD mode DOES reload
        // - Count is always reloaded
        
        // NOTE: Source is only reloaded when DMA is re-enabled by CPU (writing to control register)
        // or at VBlank for HBlank DMA. We keep the incremented source address.
        // (The srcAddr was already updated after the transfer above)
        
        // Destination always reloads in repeat mode for HBlank/VBlank DMA
        // This ensures scroll registers etc get the correct destination address each time
        channel.internalDest = channel.getDestAddress();
        
        // Word count is reloaded automatically
        if (channel.getWordCount() == 0) {
            // Cast 65536 to uint16_t (wraps to 0, which represents max transfer count for DMA3)
            channel.internalCount = (channelId == 3) ? static_cast<uint16_t>(65536) : 16384;
        } else {
            channel.internalCount = channel.getWordCount();
        }
        // Set active=false so it can be retriggered
        channel.active = false;
        LOG_DMA("[DMA%d] TRANSFER COMPLETE (repeat mode, keeping enabled)\n", channelId);
        // Keep enabled bit set (don't disable the DMA)
    } else {
        // Transfer complete, disable DMA
        channel.active = false;
        uint16_t ctrl = channel.getControl();
        ctrl &= ~DMA_ENABLE;  // Clear enable bit
        channel.setControl(ctrl);
        LOG_DMA("[DMA%d] TRANSFER COMPLETE (one-shot, disabling DMA, control now=0x%04X)\n", channelId, ctrl);
    }
    
    // Trigger IRQ if enabled
    if (channel.isIRQEnabled() && interruptController) {
        uint16_t irqFlags[4] = { 0x0100, 0x0200, 0x0400, 0x0800 };  // DMA0-3 IRQ bits
        interruptController->requestInterrupt(irqFlags[channelId]);
    }
    
    // Clear DMA tracking
    g_current_dma_channel = -1;
}

void DMAController::updateAddresses(int channelId, uint32_t& srcAddr, uint32_t& destAddr) {
    DMAChannel& channel = channels[channelId];
    uint32_t transferSize = channel.is32Bit() ? 4 : 2;
    
    // Update source address
    switch (channel.getSrcControl()) {
        case DMAAddressControl::INCREMENT:
            srcAddr += transferSize;
            break;
        case DMAAddressControl::DECREMENT:
            srcAddr -= transferSize;
            break;
        case DMAAddressControl::FIXED:
            // No change
            break;
        case DMAAddressControl::INCREMENT_RELOAD:
            // Prohibited for source, treat as increment
            srcAddr += transferSize;
            break;
    }
    
    // For Sound FIFO destinations (0x040000A0 and 0x040000A4), always treat as FIXED
    // The GBA hardware always writes to the same FIFO register address
    if (destAddr == 0x040000A0 || destAddr == 0x040000A4) {
        // Don't update destination for FIFO
        return;
    }
    
    // Update destination address
    switch (channel.getDestControl()) {
        case DMAAddressControl::INCREMENT:
        case DMAAddressControl::INCREMENT_RELOAD:  // During transfer, behaves like increment
            destAddr += transferSize;
            break;
        case DMAAddressControl::DECREMENT:
            destAddr -= transferSize;
            break;
        case DMAAddressControl::FIXED:
            // No change
            break;
    }
}

void DMAController::triggerVBlank() {
    // For HBlank DMA channels with repeat mode, reload source addresses at VBlank
    // This is the proper time to reset the scanline scroll table pointer
    for (int i = 0; i < 4; i++) {
        DMAChannel& channel = channels[i];
        if (channel.isEnabled() && channel.isRepeat() && 
            channel.getTimingMode() == DMATimingMode::HBLANK) {
            // Reload source address from register at VBlank
            channel.internalSource = channel.getSourceAddress();
            // Reload count as well
            if (channel.getWordCount() == 0) {
                channel.internalCount = (i == 3) ? static_cast<uint16_t>(65536) : 16384;
            } else {
                channel.internalCount = channel.getWordCount();
            }
            LOG_DMA("[DMA%d] VBlank: reloading HBlank DMA source to 0x%08X\n", 
                   i, channel.internalSource);
        }
    }
    startTriggeredTransfers(DMATimingMode::VBLANK);
}

void DMAController::triggerHBlank() {
    #define HBLANK_DMA_DEBUG 0
    #if HBLANK_DMA_DEBUG
    // Debug: check if any channel is configured for HBlank DMA
    static int frame30_scanline_count = 0;
    for (int i = 0; i < 4; i++) {
        DMAChannel& channel = channels[i];
        if (channel.isEnabled() && channel.getTimingMode() == DMATimingMode::HBLANK) {
            // Log first 5 scanlines of frame 30 to see source address progression
            if (g_current_frame == 30 && frame30_scanline_count < 5) {
                frame30_scanline_count++;
                fprintf(stderr, "[HBLANK F30 S%d] DMA%d src=0x%08X dst=0x%08X cnt=%d\n",
                        frame30_scanline_count, i, channel.internalSource, channel.internalDest, channel.internalCount);
            }
        }
    }
    #endif
    startTriggeredTransfers(DMATimingMode::HBLANK);
}

static int g_sound_dma_debug_count = 0;

void DMAController::triggerSoundFIFO(int fifoIndex) {
    // Sound DMA uses channels 1 and 2 with SPECIAL timing mode
    // fifoIndex: 0 = FIFO A (address 0x040000A0), 1 = FIFO B (address 0x040000A4)
    
    const uint32_t fifoAddresses[2] = { 0x040000A0, 0x040000A4 };
    uint32_t targetFifoAddr = fifoAddresses[fifoIndex];
    
    // Check DMA channels 1 and 2 (they can target sound FIFOs)
    for (int i = 1; i <= 2; i++) {
        DMAChannel& channel = channels[i];
        
        // Check if this channel is enabled, in SPECIAL timing mode, and targets the correct FIFO
        if (channel.isEnabled() && 
            !channel.active && 
            channel.getTimingMode() == DMATimingMode::SPECIAL &&
            channel.getDestAddress() == targetFifoAddr) {
            
            // Sound DMA triggered - no debug output
            
            // For sound DMA, always transfer 4 words (16 bytes = 16 samples)
            // This matches the GBA hardware behavior for FIFO DMA
            channel.internalCount = 4;
            channel.active = true;
            
            performTransfer(i);
            break;  // Only one channel should service each FIFO
        }
    }
}

void DMAController::startTriggeredTransfers(DMATimingMode mode) {
    // Start all channels waiting for this trigger, in priority order
    for (int i = 0; i < 4; i++) {
        DMAChannel& channel = channels[i];
        if (channel.isEnabled() && !channel.active && channel.getTimingMode() == mode) {
            channel.active = true;
            performTransfer(i);
        }
    }
}

bool DMAController::isAnyChannelActive() const {
    for (int i = 0; i < 4; i++) {
        if (channels[i].active) return true;
    }
    return false;
}
