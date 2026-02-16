#include "dma.h"

extern uint32_t g_current_frame;
extern uint32_t g_cpu_pc;

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
    : memory(nullptr), scheduler(nullptr), interruptController(nullptr), dmaOpenBus(0) {
}

void DMAController::reset() {
    for (int i = 0; i < 4; i++) {
        channels[i].reset();
    }
    dmaOpenBus = 0;
    pendingDMAActive = false;
    pendingDMAActivationCycle = 0;
    pendingDMAChannel = -1;
}

uint32_t DMAController::readSourceAddress(int channelId) const {
    if (channelId < 0 || channelId >= 4) return 0;
    return channels[channelId].getSourceAddress();
}

uint32_t DMAController::readDestAddress(int channelId) const {
    if (channelId < 0 || channelId >= 4) return 0;
    return channels[channelId].getDestAddress();
}

uint16_t DMAController::readControl(int channelId) const {
    if (channelId < 0 || channelId >= 4) return 0;
    return channels[channelId].getControl();
}

void DMAController::writeSourceAddress(int channelId, uint32_t value) {
    if (channelId < 0 || channelId >= 4) return;
    
    // All channels use a 28-bit mask (halfword-aligned), matching mGBA's 0x0FFFFFFE.
    // The bus width limitation for DMA0 is enforced at transfer time, not at address
    // write time.  Storing the full (masked) address lets the transfer loop detect
    // when DMA0 tries to read SRAM (0x0E) — which its narrower internal bus cannot
    // reach — and correctly return 0 instead of wrapping into VRAM.
    //
    // DMA0 cannot source from Game Pak ROM (0x08-0x0D): if the address falls in that
    // range the stored source is forced to 0, matching mGBA's _isValidDMASAD().
    uint32_t maskedValue = value & 0x0FFFFFFE;
    
    if (channelId == 0 && maskedValue >= 0x08000000 && maskedValue < 0x0E000000) {
        // DMA0 source bus cannot reach ROM region — force to 0
        maskedValue = 0;
    }
    
    channels[channelId].setSourceAddress(maskedValue);
    
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
    
    // If DMA just got enabled, start the transfer
    if (!wasEnabled && isEnabled) {
        startTransfer(channelId);
    }
    
    // If DMA got disabled, stop any active transfer
    if (wasEnabled && !isEnabled) {
        channels[channelId].active = false;
        // Cancel pending deferred DMA if it was for this channel
        if (pendingDMAActive && pendingDMAChannel == channelId) {
            pendingDMAActive = false;
        }
    }
}

void DMAController::startTransfer(int channelId) {
    DMAChannel& channel = channels[channelId];
    
    // Initialize internal registers
    channel.internalSource = channel.getSourceAddress();
    channel.internalDest = channel.getDestAddress();
    
    // GBA DMA hardware force-aligns addresses based on transfer size
    // 32-bit transfers: bits 0-1 forced to 0 (word-aligned)
    // 16-bit transfers: bit 0 forced to 0 (halfword-aligned)
    if (channel.is32Bit()) {
        channel.internalSource &= ~3u;
        channel.internalDest &= ~3u;
    } else {
        channel.internalSource &= ~1u;
        channel.internalDest &= ~1u;
    }
    
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
        // Defer DMA start by 3 cycles (matching mGBA's GBADMASchedule:
        // info->when = mTimingCurrentTime(&gba->timing) + 3).
        //
        // This delay is critical for timing accuracy: from fast memory (IWRAM),
        // the CPU executes 1-2 more instructions before DMA takes over the
        // bus. The test suite's DMA timing tests rely on this — the timer read
        // instruction executes BEFORE DMA fires from fast memory, making DMA
        // cycles invisible to the timer (expected value = 2 = just STR cost).
        // From slow memory (ROM), the instruction fetch takes >= 3 cycles, so
        // DMA fires before the next instruction and its cost IS visible.
        //
        // Implementation: set a pending flag checked in GBA::runFrame() after
        // each instruction, rather than using scheduler events (which would
        // move currentCycle backward when the event fires late).
        channel.active = true;
        if (scheduler) {
            pendingDMAActive = true;
            pendingDMAActivationCycle = scheduler->getCurrentCycle() + 3;
            pendingDMAChannel = channelId;
        } else {
            // No scheduler (test mode) — run synchronously
            performTransfer(channelId);
        }
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
    
    // GBA DMA hardware aligns addresses to the transfer unit size.
    // Bit 0 forced to 0 for 16-bit transfers; bits 0-1 forced to 0 for 32-bit.
    if (is32bit) {
        srcAddr &= ~3u;
        destAddr &= ~3u;
    } else {
        srcAddr &= ~1u;
        destAddr &= ~1u;
    }
    
    LOG_DMA("[DMA%d] STARTING TRANSFER: src=0x%08X dst=0x%08X count=%d size=%s\n",
           channelId, srcAddr, destAddr, count, is32bit ? "32bit" : "16bit");
    
    // === DMA Timing (matching mGBA's GBADMAService / GBADMASchedule) ===
    // DMA transfers use their own cycle accounting, not the normal memory access
    // wait cycle path.  We bypass addWaitCycles() and charge cycles directly.
    //
    // Per-unit cost: 2 (internal) + srcWait + dstWait
    //   First unit:  nonsequential waits for both source and destination
    //   Subsequent:  sequential waits (cached, recalculated on region crossing)
    //
    // Additional fixed costs:
    //   Startup:  3 cycles before first unit (handled by deferred scheduling
    //             in startTransfer — scheduler->schedule(3, ...) )
    //   Teardown: 2 cycles after last unit if either side is non-ROM (<0x08)
    
    // DMA uses the Game Pak bus, invalidating any prefetched data.
    // Without this flush, the CPU would incorrectly benefit from stale
    // prefetch state after DMA completes (matching mGBA where CPU is blocked
    // during DMA and no prefetch occurs).
    memory->flushPrefetch();
    
    // Bypass normal memory wait-cycle charging — we compute DMA waits ourselves
    memory->setWaitCyclesBypass(true);
    
    // Cache sequential costs for subsequent transfers (recalculated on region crossing)
    uint32_t srcRegionCur = (srcAddr >> 24) & 0xFF;
    uint32_t dstRegionCur = (destAddr >> 24) & 0xFF;
    int32_t cachedSeqCycles;
    if (is32bit) {
        cachedSeqCycles = memory->getSeqWaitCycles32(srcAddr)
                        + memory->getSeqWaitCycles32(destAddr);
    } else {
        cachedSeqCycles = memory->getSeqWaitCycles16(srcAddr)
                        + memory->getSeqWaitCycles16(destAddr);
    }
    
    // Perform all transfers
    for (uint16_t i = 0; i < count; i++) {
        // --- Cycle cost for this transfer unit ---
        int32_t unitCycles = 2;  // Fixed 2-cycle internal overhead per unit
        
        if (i == 0) {
            // First unit uses nonsequential wait states
            if (is32bit) {
                unitCycles += memory->getNonseqWaitCycles32(srcAddr)
                            + memory->getNonseqWaitCycles32(destAddr);
            } else {
                unitCycles += memory->getNonseqWaitCycles16(srcAddr)
                            + memory->getNonseqWaitCycles16(destAddr);
            }
        } else {
            // Subsequent units use cached sequential wait states
            unitCycles += cachedSeqCycles;
        }
        
        // VRAM/Palette/OAM bus contention during HDraw: the GPU is reading
        // these regions during visible scanlines, costing +1 wait state per
        // DMA access to region 0x05-0x07 (same penalty as CPU accesses).
        if (memory->isHDrawActive()) {
            uint8_t srcReg = (srcAddr >> 24) & 0xFF;
            uint8_t dstReg = (destAddr >> 24) & 0xFF;
            if (srcReg == 0x05 || srcReg == 0x06 || srcReg == 0x07) {
                unitCycles += 1;
            }
            if (dstReg == 0x05 || dstReg == 0x06 || dstReg == 0x07) {
                unitCycles += 1;
            }
        }
        
        scheduler->advanceCycles(unitCycles);
        
        // --- Data transfer ---
        uint32_t value;
        
        // Source address is already stored with a 28-bit mask (0x0FFFFFFE) for all
        // channels.  No additional per-channel masking is needed here — the bus
        // width limitation for DMA0 is handled by blocking inaccessible regions.
        uint32_t effectiveSrc = srcAddr & 0x0FFFFFFE;
        
        // Check if effective source address is DMA-readable.
        // Regions 0x00 (BIOS) and 0x01 (unmapped) return the DMA open bus latch.
        uint32_t srcRegion = effectiveSrc >> 24;
        bool srcReadable = (srcRegion >= 0x02 && srcRegion <= 0x0F);
        
        // DMA0 cannot read from SRAM (0x0E-0x0F): its internal bus is too narrow
        // to reach the cart SRAM.  On real hardware the read returns 0.
        // This matches mGBA's "performingDMA == 1" check in GBALoad8.
        bool dma0SramBlock = (channelId == 0 && srcRegion >= 0x0E);
        
        if (dma0SramBlock) {
            // DMA0 SRAM: the read physically returns 0 (bus can't reach SRAM).
            // The DMA transfer register is updated to 0, matching mGBA behavior
            // where GBALoad8 returns 0 and LOAD_SRAM replicates it to 32 bits.
            value = 0;
            dmaOpenBus = 0;
        } else if (!srcReadable) {
            // Open bus / inaccessible region: return the DMA latch value.
            // For DMA0 SRAM reads the latch is not updated, so the previous
            // latch value (often 0 after DMA state clearing) is written.
            if (is32bit) {
                value = dmaOpenBus;
            } else {
                // For 16-bit, return the appropriate halfword based on address bit 1
                value = (effectiveSrc & 2) ? ((dmaOpenBus >> 16) & 0xFFFF) : (dmaOpenBus & 0xFFFF);
            }
        } else if (is32bit) {
            value = memory->read32(effectiveSrc);
            dmaOpenBus = value;
        } else {
            value = memory->read16(effectiveSrc);
            // Update open bus latch: 16-bit value occupies one halfword of the 32-bit bus
            dmaOpenBus = value | (value << 16);
        }
        
        // Write to destination
        if (is32bit) {
            memory->write32(destAddr, value);
        } else {
            memory->write16(destAddr, static_cast<uint16_t>(value));
        }
        
        // Update addresses
        updateAddresses(channelId, srcAddr, destAddr);
        
        // If source or destination crossed a region boundary, recalculate cached
        // sequential costs (matching mGBA's boundary crossing check)
        uint32_t newSrcRegion = (srcAddr >> 24) & 0xFF;
        uint32_t newDstRegion = (destAddr >> 24) & 0xFF;
        if (newSrcRegion != srcRegionCur || newDstRegion != dstRegionCur) {
            srcRegionCur = newSrcRegion;
            dstRegionCur = newDstRegion;
            if (is32bit) {
                cachedSeqCycles = memory->getSeqWaitCycles32(srcAddr)
                                + memory->getSeqWaitCycles32(destAddr);
            } else {
                cachedSeqCycles = memory->getSeqWaitCycles16(srcAddr)
                                + memory->getSeqWaitCycles16(destAddr);
            }
        }
    }
    
    // Teardown: 2 extra cycles if either source or destination is non-ROM
    // (matching mGBA: sourceRegion < GBA_REGION_ROM0 || destRegion < GBA_REGION_ROM0)
    if (srcRegionCur < 0x08 || dstRegionCur < 0x08) {
        scheduler->advanceCycles(2);
    }
    
    // Re-enable normal memory wait-cycle charging
    memory->setWaitCyclesBypass(false);
    
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
        const uint16_t irqFlags[4] = { 0x0100, 0x0200, 0x0400, 0x0800 };  // DMA0-3 IRQ bits
        interruptController->requestInterrupt(irqFlags[channelId]);
    }
}

void DMAController::updateAddresses(int channelId, uint32_t& srcAddr, uint32_t& destAddr) {
    const DMAChannel& channel = channels[channelId];
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

void DMAController::executePendingDMA() {
    if (!pendingDMAActive) return;
    pendingDMAActive = false;
    int ch = pendingDMAChannel;
    pendingDMAChannel = -1;
    if (ch >= 0 && ch < 4 && channels[ch].active && channels[ch].isEnabled()) {
        performTransfer(ch);
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
            
            // DMA arbitration startup cost: ~2 cycles before first transfer.
            // Immediate DMA uses 3 cycles (includes pipeline effect), but
            // triggered DMA (sound, HBlank, VBlank) uses 2 cycles.
            if (scheduler) {
                scheduler->advanceCycles(2);
            }
            
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
            // DMA arbitration startup cost: ~2 cycles before first transfer.
            // Matches the startup in triggerSoundFIFO and the 3-cycle immediate
            // DMA deferred start (which includes pipeline effect).
            if (scheduler) {
                scheduler->advanceCycles(2);
            }
            performTransfer(i);
        }
    }
}


