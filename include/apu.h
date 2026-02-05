#ifndef APU_H
#define APU_H

#include <cstdint>
#include <array>
#include <atomic>
#include <SDL2/SDL.h>

// Forward declarations
class Memory;

// GBA APU (Audio Processing Unit)
// Handles all sound generation: 4 PSG channels + 2 Direct Sound (FIFO) channels
class APU {
public:
    // Audio constants
    static constexpr int SAMPLE_RATE = 32768;        // Output sample rate
    static constexpr int BUFFER_SIZE = 2048;         // Samples per buffer
    static constexpr int CPU_CLOCK = 16777216;       // GBA CPU clock rate
    static constexpr int CYCLES_PER_SAMPLE = CPU_CLOCK / SAMPLE_RATE;  // ~512 cycles
    
    // FIFO buffer size (32 bytes = 32 samples of 8-bit audio)
    static constexpr int FIFO_SIZE = 32;
    
private:
    Memory* memory;
    
    // SDL Audio
    SDL_AudioDeviceID audioDevice;
    bool audioEnabled;
    
    // Sample buffer for SDL callback
    std::array<int16_t, BUFFER_SIZE * 2> sampleBuffer;  // Stereo
    std::atomic<int> sampleBufferPos;
    
    // Cycle tracking
    uint64_t cycleCounter;
    uint64_t lastSampleCycle;
    
    // ===== Sound Control Registers =====
    // SOUNDCNT_L (0x04000080) - PSG Control
    uint16_t soundcnt_l;  // PSG volume and enable
    
    // SOUNDCNT_H (0x04000082) - Direct Sound Control
    uint16_t soundcnt_h;  // DMA sound control
    
    // SOUNDCNT_X (0x04000084) - Master Control
    uint16_t soundcnt_x;  // Master enable and status
    
    // SOUNDBIAS (0x04000088) - PWM Control
    uint16_t soundbias;
    
    // ===== Direct Sound FIFOs =====
    struct FIFO {
        std::array<int8_t, FIFO_SIZE> buffer;
        int readPos;
        int writePos;
        int count;
        int8_t currentSample;  // Latched sample for playback
        
        void reset() {
            buffer.fill(0);
            readPos = 0;
            writePos = 0;
            count = 0;
            currentSample = 0;
        }
        
        void write(uint32_t data) {
            // Write 4 bytes (32-bit write to FIFO)
            for (int i = 0; i < 4 && count < FIFO_SIZE; i++) {
                buffer[writePos] = static_cast<int8_t>((data >> (i * 8)) & 0xFF);
                writePos = (writePos + 1) % FIFO_SIZE;
                count++;
            }
        }
        
        int8_t read() {
            if (count > 0) {
                currentSample = buffer[readPos];
                readPos = (readPos + 1) % FIFO_SIZE;
                count--;
            }
            return currentSample;
        }
        
        bool needsRefill() const {
            return count <= FIFO_SIZE / 2;  // Refill when half empty
        }
    };
    
    FIFO fifoA;
    FIFO fifoB;
    
    // ===== PSG Channels (placeholder for Phase 3) =====
    // Channel 1: Square with sweep
    struct Channel1 {
        uint16_t cnt_l;  // Sweep (0x04000060)
        uint16_t cnt_h;  // Duty/Envelope (0x04000062)
        uint16_t cnt_x;  // Frequency (0x04000064)
        bool enabled;
        int volume;
        int frequency;
        int dutyPos;
        int lengthCounter;
        int envelopeCounter;
        int sweepCounter;
    } ch1;
    
    // Channel 2: Square
    struct Channel2 {
        uint16_t cnt_l;  // Duty/Envelope (0x04000068)
        uint16_t cnt_h;  // Frequency (0x0400006C)
        bool enabled;
        int volume;
        int frequency;
        int dutyPos;
        int lengthCounter;
        int envelopeCounter;
    } ch2;
    
    // Channel 3: Wave
    struct Channel3 {
        uint16_t cnt_l;  // Control (0x04000070)
        uint16_t cnt_h;  // Length/Volume (0x04000072)
        uint16_t cnt_x;  // Frequency (0x04000074)
        std::array<uint8_t, 16> waveRam;  // 32 x 4-bit samples
        bool enabled;
        int position;
        int frequency;
        int lengthCounter;
    } ch3;
    
    // Channel 4: Noise
    struct Channel4 {
        uint16_t cnt_l;  // Envelope (0x04000078)
        uint16_t cnt_h;  // Frequency (0x0400007C)
        bool enabled;
        int volume;
        uint16_t lfsr;  // Linear feedback shift register
        int lengthCounter;
        int envelopeCounter;
    } ch4;
    
    // Frame sequencer for PSG timing (512 Hz)
    int frameSequencerStep;
    int frameSequencerCounter;
    
    // Internal methods
    void initSDLAudio();
    void generateSamples(int16_t* buffer, int numSamples);
    int16_t mixChannels();
    
    // PSG sample generation (Phase 3)
    int16_t generatePSGSample();
    int16_t generateChannel1Sample();
    int16_t generateChannel2Sample();
    int16_t generateChannel3Sample();
    int16_t generateChannel4Sample();
    
    // SDL audio callback (static wrapper)
    static void audioCallback(void* userdata, uint8_t* stream, int len);
    
public:
    APU();
    ~APU();
    
    // Initialize with memory reference
    void init(Memory* mem);
    
    // Called every CPU cycle to track timing
    void tick(int cycles);
    
    // Called when timer overflows (for FIFO playback)
    void onTimerOverflow(int timerIndex);
    
    // Register read/write
    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);
    void write8(uint32_t address, uint8_t value);
    void write16(uint32_t address, uint16_t value);
    void write32(uint32_t address, uint32_t value);
    
    // FIFO writes (called from memory/DMA)
    void writeFIFO_A(uint32_t data);
    void writeFIFO_B(uint32_t data);
    
    // Control
    void reset();
    void enable(bool enabled);
    bool isEnabled() const { return audioEnabled && (soundcnt_x & 0x80); }
    
    // Debug
    void dumpState() const;
};

#endif // APU_H
