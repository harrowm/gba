#include "dma.h"
#include "memory.h"
#include "scheduler.h"
#include "interrupt.h"
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
    
    channels[channelId].setSourceAddress(value & masks[channelId]);
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
    
    printf("[DMA%d] STARTING TRANSFER: src=0x%08X dst=0x%08X count=%d size=%s\n",
           channelId, srcAddr, destAddr, count, is32bit ? "32bit" : "16bit");
    
    // Perform all transfers
    for (uint16_t i = 0; i < count; i++) {
        // Read from source
        uint32_t value;
        if (is32bit) {
            value = memory->read32(srcAddr);
        } else {
            value = memory->read16(srcAddr);
        }
        
        // Write to destination
        if (is32bit) {
            memory->write32(destAddr, value);
        } else {
            memory->write16(destAddr, static_cast<uint16_t>(value));
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
        // For repeat mode, reload both source and destination addresses
        channel.internalSource = channel.getSourceAddress();
        if (channel.getDestControl() == DMAAddressControl::INCREMENT_RELOAD) {
            channel.internalDest = channel.getDestAddress();
        } else {
            // Even without INCREMENT_RELOAD, dest should reload in repeat mode
            channel.internalDest = channel.getDestAddress();
        }
        // Word count is reloaded automatically
        if (channel.getWordCount() == 0) {
            // Cast 65536 to uint16_t (wraps to 0, which represents max transfer count for DMA3)
            channel.internalCount = (channelId == 3) ? static_cast<uint16_t>(65536) : 16384;
        } else {
            channel.internalCount = channel.getWordCount();
        }
        // Set active=false so it can be retriggered
        channel.active = false;
        printf("[DMA%d] TRANSFER COMPLETE (repeat mode, keeping enabled)\n", channelId);
        // Keep enabled bit set (don't disable the DMA)
    } else {
        // Transfer complete, disable DMA
        channel.active = false;
        uint16_t ctrl = channel.getControl();
        ctrl &= ~DMA_ENABLE;  // Clear enable bit
        channel.setControl(ctrl);
        printf("[DMA%d] TRANSFER COMPLETE (one-shot, disabling DMA, control now=0x%04X)\n", channelId, ctrl);
    }
    
    // Trigger IRQ if enabled
    if (channel.isIRQEnabled() && interruptController) {
        uint16_t irqFlags[4] = { 0x0100, 0x0200, 0x0400, 0x0800 };  // DMA0-3 IRQ bits
        interruptController->requestInterrupt(irqFlags[channelId]);
    }
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
    startTriggeredTransfers(DMATimingMode::VBLANK);
}

void DMAController::triggerHBlank() {
    startTriggeredTransfers(DMATimingMode::HBLANK);
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
