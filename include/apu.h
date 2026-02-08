#ifndef APU_H
#define APU_H

#include <cstdint>
#include <array>
#include <vector>
#include <SDL2/SDL.h>

class Memory;
class DMAController;
class Scheduler;

// GBA APU - Scheduler-driven audio with SDL_QueueAudio
//
// A recurring AUDIO_SAMPLE event fires every ~350 CPU cycles (48kHz).
// The scheduler handles this naturally during both normal execution and HALT.
// pushAudio() sends accumulated samples to SDL after each frame.
class APU {
public:
    static constexpr int SAMPLE_RATE = 48000;     // Native macOS CoreAudio rate
    static constexpr int BUFFER_SIZE = 1024;
    static constexpr int CPU_CLOCK = 16777216;
    // CPU_CLOCK / SAMPLE_RATE = 349.525... (not integer)
    // Scheduler event uses Bresenham-style alternating interval (349/350)
    static constexpr int FIFO_WORD_SIZE = 8;  // 8 uint32_t words, matching mGBA
    static constexpr int MAX_SAMPLES_PER_FRAME = 900;  // ~804 at 48kHz/60fps
    static constexpr int FRAME_SEQ_PERIOD = 32768;     // CPU_CLOCK / 512 Hz

    // Scheduler sample interval: alternate 349 and 350 to average 349.525
    static constexpr int SAMPLE_INTERVAL_BASE = CPU_CLOCK / SAMPLE_RATE; // 349
    static constexpr int SAMPLE_INTERVAL_FRAC_NUM = CPU_CLOCK % SAMPLE_RATE; // remainder

private:
    Memory* memory;
    DMAController* dmaController;
    Scheduler* scheduler;

    SDL_AudioDeviceID audioDevice;
    bool audioEnabled;

    // Per-frame sample accumulation (stereo interleaved: L R L R ...)
    std::vector<int16_t> frameSamples;

    uint64_t cycleCounter;
    int sampleFracAccum;           // Bresenham fractional accumulator for sample interval
    float adjustedRate;            // Smoothed sample rate for queue-level PLL (near SAMPLE_RATE)

    uint16_t soundcnt_l;
    uint16_t soundcnt_h;
    uint16_t soundcnt_x;
    uint16_t soundbias;

    // FIFO matching mGBA's GBAAudioFIFO exactly:
    // - 8-entry circular buffer of uint32_t words
    // - internalSample holds current word, consumed byte-by-byte (>>8)
    // - internalRemaining counts bytes left in current word (4→0)
    // - DMA refill when fewer than 4 words in FIFO (before consuming)
    struct FIFO {
        std::array<uint32_t, FIFO_WORD_SIZE> fifo;
        int fifoWrite;
        int fifoRead;
        uint32_t internalSample;
        int internalRemaining;
        int8_t currentSample;  // last output byte (for mixer)

        void reset() {
            fifo.fill(0);
            fifoWrite = fifoRead = 0;
            internalSample = 0;
            internalRemaining = 0;
            currentSample = 0;
        }

        // DMA writes one uint32_t word to the FIFO
        void write(uint32_t data) {
            fifo[fifoWrite] = data;
            fifoWrite = (fifoWrite + 1) % FIFO_WORD_SIZE;
        }

        // Get current FIFO size in words
        int size() const {
            if (fifoWrite >= fifoRead)
                return fifoWrite - fifoRead;
            else
                return FIFO_WORD_SIZE - fifoRead + fifoWrite;
        }

        // Check if DMA refill is needed (matches mGBA: FIFO_SIZE - fifoSize > 4)
        bool needsRefill() const {
            return (FIFO_WORD_SIZE - size()) > 4;
        }

        // Consume one byte from internal buffer. Called on timer overflow.
        // If internal buffer is empty, dequeue next word from FIFO.
        void consume() {
            int fifoSize = size();
            if (!internalRemaining && fifoSize) {
                internalSample = fifo[fifoRead];
                internalRemaining = 4;
                fifoRead = (fifoRead + 1) % FIFO_WORD_SIZE;
            }
            // Output the low byte of internalSample
            currentSample = static_cast<int8_t>(internalSample & 0xFF);
            // Shift to next byte
            if (internalRemaining) {
                internalSample >>= 8;
                --internalRemaining;
            }
        }
    };

    FIFO fifoA;
    FIFO fifoB;

    struct Channel1 {
        uint16_t cnt_l = 0, cnt_h = 0, cnt_x = 0;
        bool enabled = false;
        int volume = 0, frequency = 0, dutyPos = 0;
        int lengthCounter = 0, envelopeCounter = 0, sweepCounter = 0;
    } ch1;

    struct Channel2 {
        uint16_t cnt_l = 0, cnt_h = 0;
        bool enabled = false;
        int volume = 0, frequency = 0, dutyPos = 0;
        int lengthCounter = 0, envelopeCounter = 0;
    } ch2;

    struct Channel3 {
        uint16_t cnt_l = 0, cnt_h = 0, cnt_x = 0;
        std::array<uint8_t, 16> waveRam = {};
        bool enabled = false;
        int position = 0, frequency = 0, lengthCounter = 0;
    } ch3;

    struct Channel4 {
        uint16_t cnt_l = 0, cnt_h = 0;
        bool enabled = false;
        int volume = 0;
        uint16_t lfsr = 0x7FFF;
        int lengthCounter = 0, envelopeCounter = 0;
    } ch4;

    int frameSequencerStep;
    int frameSequencerCounter;

    void initSDLAudio();
    void mixChannels(int16_t& outLeft, int16_t& outRight);
    void scheduleSampleEvent();   // Schedule next AUDIO_SAMPLE on scheduler
    void onSampleEvent();         // Called by scheduler every ~350 cycles


public:
    APU();
    ~APU();

    void init(Memory* mem, DMAController* dma, Scheduler* sched);
    void startSampling();           // Begin recurring sample events on scheduler
    void pushAudio();
    void onTimerOverflow(int timerIndex);

    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);
    void write8(uint32_t address, uint8_t value);
    void write16(uint32_t address, uint16_t value);
    void write32(uint32_t address, uint32_t value);

    void writeFIFO_A(uint32_t data);
    void writeFIFO_B(uint32_t data);



    uint32_t getQueuedBytes() const {
        return audioDevice ? SDL_GetQueuedAudioSize(audioDevice) : 0;
    }
    uint32_t getQueuedSamples() const {
        return getQueuedBytes() / (2 * sizeof(int16_t));
    }


    void reset();
    bool isEnabled() const { return audioEnabled && (soundcnt_x & 0x80); }
};

#endif
