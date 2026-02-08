#include "apu.h"
#include "memory.h"
#include "dma.h"
#include "scheduler.h"
#include <cstdio>
#include <cstring>

// Sound register addresses
constexpr uint32_t REG_SOUND1CNT_L = 0x04000060;
constexpr uint32_t REG_SOUND1CNT_H = 0x04000062;
constexpr uint32_t REG_SOUND1CNT_X = 0x04000064;
constexpr uint32_t REG_SOUND2CNT_L = 0x04000068;
constexpr uint32_t REG_SOUND2CNT_H = 0x0400006C;
constexpr uint32_t REG_SOUND3CNT_L = 0x04000070;
constexpr uint32_t REG_SOUND3CNT_H = 0x04000072;
constexpr uint32_t REG_SOUND3CNT_X = 0x04000074;
constexpr uint32_t REG_SOUND4CNT_L = 0x04000078;
constexpr uint32_t REG_SOUND4CNT_H = 0x0400007C;
constexpr uint32_t REG_SOUNDCNT_L  = 0x04000080;
constexpr uint32_t REG_SOUNDCNT_H  = 0x04000082;
constexpr uint32_t REG_SOUNDCNT_X  = 0x04000084;
constexpr uint32_t REG_SOUNDBIAS   = 0x04000088;
constexpr uint32_t REG_WAVE_RAM    = 0x04000090;
constexpr uint32_t REG_FIFO_A      = 0x040000A0;
constexpr uint32_t REG_FIFO_B      = 0x040000A4;

APU::APU()
    : memory(nullptr)
    , dmaController(nullptr)
    , scheduler(nullptr)
    , audioDevice(0)
    , audioEnabled(false)
    , cycleCounter(0)
    , sampleFracAccum(0)
    , adjustedRate(SAMPLE_RATE)
    , soundcnt_l(0)
    , soundcnt_h(0)
    , soundcnt_x(0)
    , soundbias(0x200)
    , frameSequencerStep(0)
    , frameSequencerCounter(0)
{
    frameSamples.reserve(MAX_SAMPLES_PER_FRAME * 2);
    reset();
}

APU::~APU() {
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
    }
}

void APU::init(Memory* mem, DMAController* dma, Scheduler* sched) {
    memory = mem;
    dmaController = dma;
    scheduler = sched;
    initSDLAudio();
}

void APU::initSDLAudio() {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "[APU] Failed to initialize SDL audio: %s\n", SDL_GetError());
            return;
        }
    }

    SDL_AudioSpec desired, obtained;
    SDL_memset(&desired, 0, sizeof(desired));

    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = BUFFER_SIZE;
    desired.callback = nullptr;  // Push mode - no callback
    desired.userdata = nullptr;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);

    if (audioDevice == 0) {
        fprintf(stderr, "[APU] Failed to open audio device: %s\n", SDL_GetError());
        return;
    }

    printf("[APU] Audio initialized (push mode): %d Hz, %d channels, %d buffer\n",
           obtained.freq, obtained.channels, obtained.samples);

    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
    audioEnabled = true;

    // Pre-fill the queue with silence to match PLL target level.
    // The PLL in pushAudio() targets TARGET_QUEUE (4096) samples;
    // starting at the same level avoids an initial rate transient.
    static const int PREFILL = 4096;
    std::vector<int16_t> silence(PREFILL * 2, 0);  // stereo
    SDL_QueueAudio(audioDevice, silence.data(), silence.size() * sizeof(int16_t));
    printf("[APU] Pre-filled queue with %d samples of silence\n", PREFILL);
}

// Called after each frame to push accumulated samples to SDL
void APU::pushAudio() {
    if (!audioDevice) return;
    
    if (frameSamples.empty()) return;

    uint32_t queued = getQueuedSamples();

    // Push raw samples — no resampling, no interpolation.
    // Rate adjustment happens in tick() via adjustedRate, which smoothly
    // varies sample production to keep the queue stable.
    static constexpr uint32_t MAX_QUEUED = 16384;
    if (queued < MAX_QUEUED) {
        SDL_QueueAudio(audioDevice, frameSamples.data(),
                       frameSamples.size() * sizeof(int16_t));
    }

    // PLL: adjust the production rate for the NEXT frame based on queue level.
    // This is the same principle as mGBA's fauxClock — a feedback loop that
    // nudges the sample rate by tiny amounts to keep the queue at target.
    //
    // Key design: exponential smoothing prevents per-frame pitch jumps.
    // The rate changes by at most ~0.5% per frame, and the smoothing
    // ensures the change is gradual (time constant ~20 frames = 333ms).
    static constexpr int TARGET_QUEUE = 4096;  // ~85ms at 48kHz
    float error = (float)(TARGET_QUEUE - (int)queued);

    // Convert queue error to rate adjustment (Hz)
    // ±4096 samples error → ±200 Hz adjustment (±0.4% of 48000)
    float rateNudge = error * 0.05f;

    // Clamp to ±500 Hz max (~1% pitch shift)
    if (rateNudge > 500.0f) rateNudge = 500.0f;
    if (rateNudge < -500.0f) rateNudge = -500.0f;

    float targetRate = (float)SAMPLE_RATE + rateNudge;

    // Exponential smoothing: adjustedRate converges to targetRate over ~20 frames
    adjustedRate = adjustedRate * 0.95f + targetRate * 0.05f;

    // Safety clamp: never stray more than ±2% from nominal
    float minRate = SAMPLE_RATE * 0.98f;
    float maxRate = SAMPLE_RATE * 1.02f;
    if (adjustedRate < minRate) adjustedRate = minRate;
    if (adjustedRate > maxRate) adjustedRate = maxRate;

    frameSamples.clear();
}

void APU::mixChannels(int16_t& outLeft, int16_t& outRight) {
    int32_t left = 0, right = 0;

    // FIFO A — SOUNDCNT_H bits: 2=volume, 8=right, 9=left, 10=timer
    bool fifoA_right = (soundcnt_h & 0x0100) != 0;
    bool fifoA_left  = (soundcnt_h & 0x0200) != 0;
    bool fifoA_vol   = (soundcnt_h & 0x0004) != 0;  // 0=50%, 1=100%

    if (fifoA_left || fifoA_right) {
        // FIFO sample is int8 (-128..127).
        // 100% volume: ×256 maps to -32768..32512 (full 16-bit range per channel)
        // 50% volume:  ×128 maps to -16384..16256
        int32_t scaledA = (int32_t)fifoA.currentSample * (fifoA_vol ? 256 : 128);
        if (fifoA_left)  left  += scaledA;
        if (fifoA_right) right += scaledA;
    }

    // FIFO B — SOUNDCNT_H bits: 3=volume, 12=right, 13=left, 14=timer
    bool fifoB_right = (soundcnt_h & 0x1000) != 0;
    bool fifoB_left  = (soundcnt_h & 0x2000) != 0;
    bool fifoB_vol   = (soundcnt_h & 0x0008) != 0;

    if (fifoB_left || fifoB_right) {
        int32_t scaledB = (int32_t)fifoB.currentSample * (fifoB_vol ? 256 : 128);
        if (fifoB_left)  left  += scaledB;
        if (fifoB_right) right += scaledB;
    }

    // Clamp to 16-bit signed
    if (left  >  32767) left  =  32767;
    if (left  < -32768) left  = -32768;
    if (right >  32767) right =  32767;
    if (right < -32768) right = -32768;

    outLeft  = static_cast<int16_t>(left);
    outRight = static_cast<int16_t>(right);
}

// Schedule the next AUDIO_SAMPLE event on the scheduler.
// Uses Bresenham-style fractional accumulator with PLL-adjusted rate.
// adjustedRate (~48000 Hz ±2%) is tuned by pushAudio() to keep the SDL
// queue stable, compensating for VSync frame-rate mismatch.
void APU::scheduleSampleEvent() {
    if (!scheduler) return;

    // Convert PLL-adjusted rate to fixed-point (×256) for integer Bresenham.
    // adjustedRate ≈ 48000, so rateFixed ≈ 12288000.
    // interval = CPU_CLOCK / adjustedRate, tracked with fractional remainder.
    int rateFixed = (int)(adjustedRate * 256.0f);
    if (rateFixed <= 0) rateFixed = SAMPLE_RATE * 256;  // safety

    int64_t numerator = (int64_t)CPU_CLOCK << 8;  // CPU_CLOCK * 256
    int interval = (int)(numerator / rateFixed);
    int remainder = (int)(numerator % rateFixed);

    sampleFracAccum += remainder;
    if (sampleFracAccum >= rateFixed) {
        sampleFracAccum -= rateFixed;
        interval++;
    }

    scheduler->schedule(interval, [this]() { onSampleEvent(); },
                        EventType::AUDIO_SAMPLE, 0x18);
}

// Called by the scheduler every ~350 cycles. Mixes one stereo sample.
void APU::onSampleEvent() {
    static constexpr size_t MAX_FRAME_SAMPLES = 4096;

    if (frameSamples.size() < MAX_FRAME_SAMPLES * 2) {
        int16_t sampleL = 0, sampleR = 0;
        if (soundcnt_x & 0x80) {
            mixChannels(sampleL, sampleR);
        }
        frameSamples.push_back(sampleL);
        frameSamples.push_back(sampleR);
    }

    // Reschedule for next sample
    scheduleSampleEvent();
}

// Start the recurring sample event chain on the scheduler.
// Called once after init, or after reset.
void APU::startSampling() {
    if (!scheduler) return;
    scheduler->cancelEventsOfType(EventType::AUDIO_SAMPLE);
    sampleFracAccum = 0;
    scheduleSampleEvent();
}

void APU::onTimerOverflow(int timerIndex) {
    // Check master sound enable (matches mGBA: gba->audio.enable check)
    if (!(soundcnt_x & 0x80)) return;

    int fifoA_timer = (soundcnt_h & 0x0400) ? 1 : 0;
    int fifoB_timer = (soundcnt_h & 0x4000) ? 1 : 0;

    // Match mGBA's GBAAudioSampleFIFO order:
    // 1. Check DMA refill FIRST (before consuming)
    // 2. Consume one byte from internal buffer
    if (timerIndex == fifoA_timer) {
        if (fifoA.needsRefill() && dmaController) {
            dmaController->triggerSoundFIFO(0);
        }
        fifoA.consume();
    }

    if (timerIndex == fifoB_timer) {
        if (fifoB.needsRefill() && dmaController) {
            dmaController->triggerSoundFIFO(1);
        }
        fifoB.consume();
    }
}

void APU::reset() {
    fifoA.reset();
    fifoB.reset();

    soundcnt_l = 0;
    soundcnt_h = 0;
    soundcnt_x = 0;
    soundbias = 0x200;

    ch1 = {};
    ch2 = {};
    ch3 = {};
    ch3.waveRam.fill(0);
    ch4 = {};
    ch4.lfsr = 0x7FFF;

    frameSequencerStep = 0;
    frameSequencerCounter = 0;
    cycleCounter = 0;
    sampleFracAccum = 0;
    adjustedRate = SAMPLE_RATE;

    frameSamples.clear();
    frameSamples.reserve(MAX_SAMPLES_PER_FRAME * 2);
}

// ===== Register Read/Write =====

uint8_t APU::read8(uint32_t address) {
    uint16_t val16 = read16(address & ~1);
    return (address & 1) ? (val16 >> 8) : (val16 & 0xFF);
}

uint16_t APU::read16(uint32_t address) {
    switch (address) {
        case REG_SOUND1CNT_L: return ch1.cnt_l;
        case REG_SOUND1CNT_H: return ch1.cnt_h;
        case REG_SOUND1CNT_X: return ch1.cnt_x & 0x4000;

        case REG_SOUND2CNT_L: return ch2.cnt_l;
        case REG_SOUND2CNT_H: return ch2.cnt_h & 0x4000;

        case REG_SOUND3CNT_L: return ch3.cnt_l;
        case REG_SOUND3CNT_H: return ch3.cnt_h;
        case REG_SOUND3CNT_X: return ch3.cnt_x & 0x4000;

        case REG_SOUND4CNT_L: return ch4.cnt_l;
        case REG_SOUND4CNT_H: return ch4.cnt_h & 0x4000;

        case REG_SOUNDCNT_L: return soundcnt_l;
        case REG_SOUNDCNT_H: return soundcnt_h;
        case REG_SOUNDCNT_X: {
            uint16_t status = soundcnt_x & 0x80;
            if (ch1.enabled) status |= 0x01;
            if (ch2.enabled) status |= 0x02;
            if (ch3.enabled) status |= 0x04;
            if (ch4.enabled) status |= 0x08;
            return status;
        }

        case REG_SOUNDBIAS: return soundbias;

        default:
            if (address >= REG_WAVE_RAM && address < REG_WAVE_RAM + 16) {
                int offset = address - REG_WAVE_RAM;
                return ch3.waveRam[offset] | (ch3.waveRam[offset + 1] << 8);
            }
            break;
    }
    return 0;
}

void APU::write8(uint32_t address, uint8_t value) {
    if (address >= REG_WAVE_RAM && address < REG_WAVE_RAM + 16) {
        ch3.waveRam[address - REG_WAVE_RAM] = value;
        return;
    }

    uint16_t val16 = read16(address & ~1);
    if (address & 1) {
        val16 = (val16 & 0x00FF) | (value << 8);
    } else {
        val16 = (val16 & 0xFF00) | value;
    }
    write16(address & ~1, val16);
}

void APU::write16(uint32_t address, uint16_t value) {
    switch (address) {
        case REG_SOUND1CNT_L: ch1.cnt_l = value; break;
        case REG_SOUND1CNT_H: ch1.cnt_h = value; break;
        case REG_SOUND1CNT_X:
            ch1.cnt_x = value;
            if (value & 0x8000) {
                ch1.enabled = true;
                ch1.volume = (ch1.cnt_h >> 12) & 0xF;
                ch1.frequency = ch1.cnt_x & 0x7FF;
                ch1.dutyPos = 0;
                ch1.lengthCounter = 64 - (ch1.cnt_h & 0x3F);
                ch1.envelopeCounter = (ch1.cnt_h >> 8) & 0x7;
                ch1.sweepCounter = (ch1.cnt_l >> 4) & 0x7;
            }
            break;

        case REG_SOUND2CNT_L: ch2.cnt_l = value; break;
        case REG_SOUND2CNT_H:
            ch2.cnt_h = value;
            if (value & 0x8000) ch2.enabled = true;
            break;

        case REG_SOUND3CNT_L:
            ch3.cnt_l = value;
            if (!(value & 0x80)) ch3.enabled = false;
            break;
        case REG_SOUND3CNT_H: ch3.cnt_h = value; break;
        case REG_SOUND3CNT_X:
            ch3.cnt_x = value;
            if (value & 0x8000) ch3.enabled = (ch3.cnt_l & 0x80) != 0;
            break;

        case REG_SOUND4CNT_L: ch4.cnt_l = value; break;
        case REG_SOUND4CNT_H:
            ch4.cnt_h = value;
            if (value & 0x8000) {
                ch4.enabled = true;
                ch4.lfsr = 0x7FFF;
            }
            break;

        case REG_SOUNDCNT_L: soundcnt_l = value; break;
        case REG_SOUNDCNT_H:
            if (value & 0x0800) fifoA.reset();
            if (value & 0x8000) fifoB.reset();
            soundcnt_h = value & ~0x8800;  // Bits 11,15 are write-only reset flags
            break;

        case REG_SOUNDCNT_X:
            soundcnt_x = (soundcnt_x & 0x7F) | (value & 0x80);
            if (!(value & 0x80)) {
                ch1.enabled = false;
                ch2.enabled = false;
                ch3.enabled = false;
                ch4.enabled = false;
            }
            break;

        case REG_SOUNDBIAS:
            soundbias = value & 0xC3FE;
            break;

        default:
            if (address >= REG_WAVE_RAM && address < REG_WAVE_RAM + 16) {
                int offset = address - REG_WAVE_RAM;
                ch3.waveRam[offset] = value & 0xFF;
                ch3.waveRam[offset + 1] = value >> 8;
            }
            break;
    }
}

void APU::write32(uint32_t address, uint32_t value) {
    if (address == REG_FIFO_A) { writeFIFO_A(value); return; }
    if (address == REG_FIFO_B) { writeFIFO_B(value); return; }
    write16(address, value & 0xFFFF);
    write16(address + 2, value >> 16);
}

void APU::writeFIFO_A(uint32_t data) { fifoA.write(data); }
void APU::writeFIFO_B(uint32_t data) { fifoB.write(data); }


