#include "apu.h"
#include "memory.h"
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
constexpr uint32_t REG_WAVE_RAM    = 0x04000090;  // 16 bytes (0x90-0x9F)
constexpr uint32_t REG_FIFO_A      = 0x040000A0;
constexpr uint32_t REG_FIFO_B      = 0x040000A4;

APU::APU() 
    : memory(nullptr)
    , audioDevice(0)
    , audioEnabled(false)
    , sampleBufferPos(0)
    , cycleCounter(0)
    , lastSampleCycle(0)
    , soundcnt_l(0)
    , soundcnt_h(0)
    , soundcnt_x(0)
    , soundbias(0x200)  // Default bias
    , frameSequencerStep(0)
    , frameSequencerCounter(0)
{
    sampleBuffer.fill(0);
    reset();
}

APU::~APU() {
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
    }
}

void APU::init(Memory* mem) {
    memory = mem;
    initSDLAudio();
}

void APU::initSDLAudio() {
    // Check if SDL audio is already initialized
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            fprintf(stderr, "[APU] Failed to initialize SDL audio: %s\n", SDL_GetError());
            return;
        }
    }
    
    SDL_AudioSpec desired, obtained;
    SDL_memset(&desired, 0, sizeof(desired));
    
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_S16SYS;  // Signed 16-bit native endian
    desired.channels = 2;           // Stereo
    desired.samples = BUFFER_SIZE;
    desired.callback = audioCallback;
    desired.userdata = this;
    
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    
    if (audioDevice == 0) {
        fprintf(stderr, "[APU] Failed to open audio device: %s\n", SDL_GetError());
        return;
    }
    
    printf("[APU] Audio initialized: %d Hz, %d channels, %d samples/buffer\n",
           obtained.freq, obtained.channels, obtained.samples);
    
    // Start audio playback
    SDL_PauseAudioDevice(audioDevice, 0);
    audioEnabled = true;
}

void APU::audioCallback(void* userdata, uint8_t* stream, int len) {
    APU* apu = static_cast<APU*>(userdata);
    int16_t* out = reinterpret_cast<int16_t*>(stream);
    int numSamples = len / sizeof(int16_t) / 2;  // Stereo samples
    
    apu->generateSamples(out, numSamples);
}

void APU::generateSamples(int16_t* buffer, int numSamples) {
    // Check if master sound is enabled
    bool masterEnable = (soundcnt_x & 0x80) != 0;
    
    for (int i = 0; i < numSamples; i++) {
        int16_t sample = 0;
        
        if (masterEnable) {
            sample = mixChannels();
        }
        
        // Output stereo (same sample to both channels for now)
        buffer[i * 2] = sample;      // Left
        buffer[i * 2 + 1] = sample;  // Right
    }
}

int16_t APU::mixChannels() {
    int32_t mixedSample = 0;
    
    // ===== Direct Sound (FIFO) channels =====
    // These are the primary sound source for most GBA games
    
    // FIFO A
    bool fifoA_enabled_L = (soundcnt_h & 0x0200) != 0;
    bool fifoA_enabled_R = (soundcnt_h & 0x0100) != 0;
    bool fifoA_volume = (soundcnt_h & 0x0004) != 0;  // 0 = 50%, 1 = 100%
    
    if (fifoA_enabled_L || fifoA_enabled_R) {
        int32_t sampleA = fifoA.currentSample;
        // Scale 8-bit signed to ~14-bit range
        sampleA *= fifoA_volume ? 512 : 256;
        mixedSample += sampleA;
    }
    
    // FIFO B
    bool fifoB_enabled_L = (soundcnt_h & 0x2000) != 0;
    bool fifoB_enabled_R = (soundcnt_h & 0x1000) != 0;
    bool fifoB_volume = (soundcnt_h & 0x0008) != 0;  // 0 = 50%, 1 = 100%
    
    if (fifoB_enabled_L || fifoB_enabled_R) {
        int32_t sampleB = fifoB.currentSample;
        sampleB *= fifoB_volume ? 512 : 256;
        mixedSample += sampleB;
    }
    
    // ===== PSG channels (Phase 3 - placeholder) =====
    // int16_t psgSample = generatePSGSample();
    // mixedSample += psgSample;
    
    // Clamp to 16-bit range
    if (mixedSample > 32767) mixedSample = 32767;
    if (mixedSample < -32768) mixedSample = -32768;
    
    return static_cast<int16_t>(mixedSample);
}

void APU::tick(int cycles) {
    cycleCounter += cycles;
    
    // Generate samples at the appropriate rate
    while (cycleCounter - lastSampleCycle >= CYCLES_PER_SAMPLE) {
        lastSampleCycle += CYCLES_PER_SAMPLE;
        
        // Frame sequencer runs at 512 Hz (every 32768 cycles)
        frameSequencerCounter += CYCLES_PER_SAMPLE;
        if (frameSequencerCounter >= 32768) {
            frameSequencerCounter -= 32768;
            frameSequencerStep = (frameSequencerStep + 1) % 8;
            // TODO: Update PSG length/envelope/sweep based on step
        }
    }
}

void APU::onTimerOverflow(int timerIndex) {
    // SOUNDCNT_H bits:
    // Bit 10: FIFO A timer select (0 = Timer 0, 1 = Timer 1)
    // Bit 14: FIFO B timer select (0 = Timer 0, 1 = Timer 1)
    
    int fifoA_timer = (soundcnt_h & 0x0400) ? 1 : 0;
    int fifoB_timer = (soundcnt_h & 0x4000) ? 1 : 0;
    
    // When the selected timer overflows, read next sample from FIFO
    if (timerIndex == fifoA_timer) {
        fifoA.read();
        
        // Request DMA refill if FIFO is running low
        if (fifoA.needsRefill()) {
            // DMA will be triggered by checking this flag
            // The actual DMA trigger happens in the DMA controller
        }
    }
    
    if (timerIndex == fifoB_timer) {
        fifoB.read();
        
        if (fifoB.needsRefill()) {
            // DMA refill needed
        }
    }
}

void APU::reset() {
    fifoA.reset();
    fifoB.reset();
    
    soundcnt_l = 0;
    soundcnt_h = 0;
    soundcnt_x = 0;
    soundbias = 0x200;
    
    // Reset PSG channels
    ch1 = {};
    ch2 = {};
    ch3 = {};
    ch3.waveRam.fill(0);
    ch4 = {};
    ch4.lfsr = 0x7FFF;  // Initial LFSR state
    
    frameSequencerStep = 0;
    frameSequencerCounter = 0;
    cycleCounter = 0;
    lastSampleCycle = 0;
}

void APU::enable(bool enabled) {
    audioEnabled = enabled;
    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, enabled ? 0 : 1);
    }
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
        case REG_SOUND1CNT_X: return ch1.cnt_x & 0x4000;  // Only bit 14 readable
        
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
            // Bit 7: Master enable
            // Bits 0-3: Channel 1-4 playing status
            uint16_t status = soundcnt_x & 0x80;
            if (ch1.enabled) status |= 0x01;
            if (ch2.enabled) status |= 0x02;
            if (ch3.enabled) status |= 0x04;
            if (ch4.enabled) status |= 0x08;
            return status;
        }
        
        case REG_SOUNDBIAS: return soundbias;
        
        // Wave RAM (0x04000090 - 0x0400009F)
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
    // Wave RAM can be written byte-by-byte
    if (address >= REG_WAVE_RAM && address < REG_WAVE_RAM + 16) {
        ch3.waveRam[address - REG_WAVE_RAM] = value;
        return;
    }
    
    // Other registers: read-modify-write
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
        case REG_SOUND1CNT_L:
            ch1.cnt_l = value;
            break;
            
        case REG_SOUND1CNT_H:
            ch1.cnt_h = value;
            break;
            
        case REG_SOUND1CNT_X:
            ch1.cnt_x = value;
            if (value & 0x8000) {  // Restart bit
                ch1.enabled = true;
                // TODO: Initialize channel 1
            }
            break;
            
        case REG_SOUND2CNT_L:
            ch2.cnt_l = value;
            break;
            
        case REG_SOUND2CNT_H:
            ch2.cnt_h = value;
            if (value & 0x8000) {
                ch2.enabled = true;
            }
            break;
            
        case REG_SOUND3CNT_L:
            ch3.cnt_l = value;
            if (!(value & 0x80)) {  // Bit 7 = enable
                ch3.enabled = false;
            }
            break;
            
        case REG_SOUND3CNT_H:
            ch3.cnt_h = value;
            break;
            
        case REG_SOUND3CNT_X:
            ch3.cnt_x = value;
            if (value & 0x8000) {
                ch3.enabled = (ch3.cnt_l & 0x80) != 0;
            }
            break;
            
        case REG_SOUND4CNT_L:
            ch4.cnt_l = value;
            break;
            
        case REG_SOUND4CNT_H:
            ch4.cnt_h = value;
            if (value & 0x8000) {
                ch4.enabled = true;
                ch4.lfsr = 0x7FFF;
            }
            break;
            
        case REG_SOUNDCNT_L:
            soundcnt_l = value;
            break;
            
        case REG_SOUNDCNT_H:
            soundcnt_h = value;
            // Bit 11: Reset FIFO A
            if (value & 0x0800) {
                fifoA.reset();
            }
            // Bit 15: Reset FIFO B
            if (value & 0x8000) {
                fifoB.reset();
            }
            break;
            
        case REG_SOUNDCNT_X:
            // Only bit 7 (master enable) is writable
            soundcnt_x = (soundcnt_x & 0x7F) | (value & 0x80);
            if (!(value & 0x80)) {
                // Master disable - turn off all channels
                ch1.enabled = false;
                ch2.enabled = false;
                ch3.enabled = false;
                ch4.enabled = false;
            }
            break;
            
        case REG_SOUNDBIAS:
            soundbias = value & 0xC3FE;  // Mask valid bits
            break;
            
        default:
            // Wave RAM
            if (address >= REG_WAVE_RAM && address < REG_WAVE_RAM + 16) {
                int offset = address - REG_WAVE_RAM;
                ch3.waveRam[offset] = value & 0xFF;
                ch3.waveRam[offset + 1] = value >> 8;
            }
            break;
    }
}

void APU::write32(uint32_t address, uint32_t value) {
    // FIFO writes are 32-bit
    if (address == REG_FIFO_A) {
        writeFIFO_A(value);
        return;
    }
    if (address == REG_FIFO_B) {
        writeFIFO_B(value);
        return;
    }
    
    // Otherwise split into two 16-bit writes
    write16(address, value & 0xFFFF);
    write16(address + 2, value >> 16);
}

void APU::writeFIFO_A(uint32_t data) {
    fifoA.write(data);
}

void APU::writeFIFO_B(uint32_t data) {
    fifoB.write(data);
}

// ===== PSG Sample Generation (Phase 3 placeholders) =====

int16_t APU::generatePSGSample() {
    int32_t sample = 0;
    
    // PSG master volume from SOUNDCNT_L
    int psgVolumeL = (soundcnt_l >> 0) & 0x07;
    int psgVolumeR = (soundcnt_l >> 4) & 0x07;
    
    // TODO: Implement actual PSG generation
    // sample += generateChannel1Sample();
    // sample += generateChannel2Sample();
    // sample += generateChannel3Sample();
    // sample += generateChannel4Sample();
    
    return static_cast<int16_t>(sample);
}

int16_t APU::generateChannel1Sample() {
    if (!ch1.enabled) return 0;
    // TODO: Square wave with sweep
    return 0;
}

int16_t APU::generateChannel2Sample() {
    if (!ch2.enabled) return 0;
    // TODO: Square wave
    return 0;
}

int16_t APU::generateChannel3Sample() {
    if (!ch3.enabled) return 0;
    // TODO: Wave channel
    return 0;
}

int16_t APU::generateChannel4Sample() {
    if (!ch4.enabled) return 0;
    // TODO: Noise channel
    return 0;
}

void APU::dumpState() const {
    printf("\n=== APU State ===\n");
    printf("Master Enable: %s\n", (soundcnt_x & 0x80) ? "ON" : "OFF");
    printf("SOUNDCNT_L: 0x%04X\n", soundcnt_l);
    printf("SOUNDCNT_H: 0x%04X\n", soundcnt_h);
    printf("SOUNDCNT_X: 0x%04X\n", soundcnt_x);
    printf("SOUNDBIAS: 0x%04X\n", soundbias);
    printf("\nFIFO A: %d samples, current=0x%02X\n", fifoA.count, (uint8_t)fifoA.currentSample);
    printf("FIFO B: %d samples, current=0x%02X\n", fifoB.count, (uint8_t)fifoB.currentSample);
    printf("PSG Ch1: %s, Ch2: %s, Ch3: %s, Ch4: %s\n",
           ch1.enabled ? "ON" : "OFF",
           ch2.enabled ? "ON" : "OFF",
           ch3.enabled ? "ON" : "OFF",
           ch4.enabled ? "ON" : "OFF");
    printf("=================\n\n");
}
