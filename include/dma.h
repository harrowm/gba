#ifndef DMA_H
#define DMA_H

#include <cstdint>

// Forward declarations
class Memory;
class Scheduler;
class InterruptController;

// DMA Control Register Bits
#define DMA_ENABLE          0x8000
#define DMA_IRQ_ENABLE      0x4000
#define DMA_TIMING_MASK     0x3000
#define DMA_TIMING_SHIFT    12
#define DMA_GAME_PAK_DRQ    0x0800
#define DMA_TYPE_32BIT      0x0400
#define DMA_DEST_CTRL_MASK  0x0300
#define DMA_DEST_CTRL_SHIFT 8
#define DMA_SRC_CTRL_MASK   0x0180
#define DMA_SRC_CTRL_SHIFT  7
#define DMA_REPEAT          0x0200

// Timing modes
enum class DMATimingMode {
    IMMEDIATE = 0,
    VBLANK = 1,
    HBLANK = 2,
    SPECIAL = 3
};

// Address control modes
enum class DMAAddressControl {
    INCREMENT = 0,
    DECREMENT = 1,
    FIXED = 2,
    INCREMENT_RELOAD = 3  // Only valid for destination
};

// Individual DMA channel
class DMAChannel {
public:
    DMAChannel();
    void reset();
    
    // Register access
    void setSourceAddress(uint32_t addr);
    void setDestAddress(uint32_t addr);
    void setWordCount(uint16_t count);
    void setControl(uint16_t control);
    
    uint32_t getSourceAddress() const { return sourceAddr; }
    uint32_t getDestAddress() const { return destAddr; }
    uint16_t getWordCount() const { return wordCount; }
    uint16_t getControl() const { return control; }
    
    // State queries
    bool isEnabled() const { return (control & DMA_ENABLE) != 0; }
    bool isIRQEnabled() const { return (control & DMA_IRQ_ENABLE) != 0; }
    bool isRepeat() const { return (control & DMA_REPEAT) != 0; }
    bool is32Bit() const { return (control & DMA_TYPE_32BIT) != 0; }
    
    DMATimingMode getTimingMode() const {
        return static_cast<DMATimingMode>((control & DMA_TIMING_MASK) >> DMA_TIMING_SHIFT);
    }
    
    DMAAddressControl getDestControl() const {
        return static_cast<DMAAddressControl>((control & DMA_DEST_CTRL_MASK) >> DMA_DEST_CTRL_SHIFT);
    }
    
    DMAAddressControl getSrcControl() const {
        return static_cast<DMAAddressControl>((control & DMA_SRC_CTRL_MASK) >> DMA_SRC_CTRL_SHIFT);
    }
    
    // Internal state for active transfers
    uint32_t internalSource;
    uint32_t internalDest;
    uint16_t internalCount;
    bool active;

private:
    // Hardware registers
    uint32_t sourceAddr;
    uint32_t destAddr;
    uint16_t wordCount;
    uint16_t control;
};

// DMA Controller managing all 4 channels
class DMAController {
public:
    DMAController();
    
    void setMemory(Memory* mem) { memory = mem; }
    void setScheduler(Scheduler* sched) { scheduler = sched; }
    void setInterruptController(InterruptController* ic) { interruptController = ic; }
    
    // Register I/O
    uint32_t readSourceAddress(int channelId) const;
    uint32_t readDestAddress(int channelId) const;
    uint16_t readWordCount(int channelId) const;
    uint16_t readControl(int channelId) const;
    
    void writeSourceAddress(int channelId, uint32_t value);
    void writeDestAddress(int channelId, uint32_t value);
    void writeWordCount(int channelId, uint16_t value);
    void writeControl(int channelId, uint16_t value);
    
    // Trigger methods (called by GPU)
    void triggerVBlank();
    void triggerHBlank();
    
    // Check if any DMA is active
    bool isAnyChannelActive() const;
    
    // Reset all channels
    void reset();

private:
    DMAChannel channels[4];
    Memory* memory;
    Scheduler* scheduler;
    InterruptController* interruptController;
    
    // Transfer execution
    void startTransfer(int channelId);
    void performTransfer(int channelId);
    void updateAddresses(int channelId, uint32_t& srcAddr, uint32_t& destAddr);
    
    // Timing-triggered transfers
    void startTriggeredTransfers(DMATimingMode mode);
};

#endif // DMA_H
