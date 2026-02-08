#include "gba.h"
#include "display.h"
#include "debug.h"
#include "sp_trace.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

// Scripted input event: at a given frame, set the GBA key state
struct InputEvent {
    int frame;
    uint16_t keyState;  // active low: 0=pressed, 1=released
};

// Parse button name to GBA button bit mask
static uint16_t parseButtonMask(const char* name) {
    if (strcasecmp(name, "A") == 0)      return (1 << 0);
    if (strcasecmp(name, "B") == 0)      return (1 << 1);
    if (strcasecmp(name, "SELECT") == 0) return (1 << 2);
    if (strcasecmp(name, "START") == 0)  return (1 << 3);
    if (strcasecmp(name, "RIGHT") == 0)  return (1 << 4);
    if (strcasecmp(name, "LEFT") == 0)   return (1 << 5);
    if (strcasecmp(name, "UP") == 0)     return (1 << 6);
    if (strcasecmp(name, "DOWN") == 0)   return (1 << 7);
    if (strcasecmp(name, "R") == 0)      return (1 << 8);
    if (strcasecmp(name, "L") == 0)      return (1 << 9);
    return 0;
}

// Parse input script string: "frame:BTN[+BTN];frame:BTN;frame:;"
// Example: "30:DOWN;35:;40:DOWN;45:;50:A;55:;"
// Empty button list means release all
static std::vector<InputEvent> parseInputScript(const char* script) {
    std::vector<InputEvent> events;
    std::string s(script);
    size_t pos = 0;
    
    while (pos < s.size()) {
        size_t semi = s.find(';', pos);
        if (semi == std::string::npos) semi = s.size();
        
        std::string entry = s.substr(pos, semi - pos);
        size_t colon = entry.find(':');
        if (colon != std::string::npos) {
            int frame = std::stoi(entry.substr(0, colon));
            std::string buttons = entry.substr(colon + 1);
            
            uint16_t pressed = 0;  // bits for pressed buttons
            if (!buttons.empty()) {
                size_t bpos = 0;
                while (bpos < buttons.size()) {
                    size_t plus = buttons.find('+', bpos);
                    if (plus == std::string::npos) plus = buttons.size();
                    std::string btn = buttons.substr(bpos, plus - bpos);
                    pressed |= parseButtonMask(btn.c_str());
                    bpos = plus + 1;
                }
            }
            // Active low: 0x3FF = all released, clear bits for pressed buttons
            events.push_back({frame, (uint16_t)(0x03FF & ~pressed)});
        }
        pos = semi + 1;
    }
    return events;
}

// Suite name → index mapping for --run-suite
static const char* const suiteNames[] = {
    "memory", "io-read", "timing", "timers", "timer-irq",
    "shifter", "carry", "multiply-long", "bios-math", "dma",
    "sio-read", "sio-timing", "misc-edge", "video", nullptr
};

static int findSuiteIndex(const char* name) {
    if (strcmp(name, "all") == 0) return -2;  // special: run all
    for (int i = 0; suiteNames[i]; i++) {
        if (strcasecmp(name, suiteNames[i]) == 0) return i;
    }
    // Try numeric index
    char* end;
    long idx = strtol(name, &end, 10);
    if (*end == '\0' && idx >= 0 && idx <= 13) return (int)idx;
    return -1;  // not found
}

// Generate input script to navigate to suite index N and press A
// The suite ROM starts with cursor at index 0 after ~30 frames of boot
static std::vector<InputEvent> generateSuiteScript(int suiteIndex) {
    std::vector<InputEvent> events;
    int frame = 30;  // allow boot time
    
    // Press DOWN suiteIndex times to navigate to the desired suite
    for (int i = 0; i < suiteIndex; i++) {
        events.push_back({frame, (uint16_t)(0x03FF & ~(1 << 7))});  // DOWN pressed
        frame += 5;
        events.push_back({frame, 0x03FF});  // release all
        frame += 5;
    }
    
    // Press A to select the suite
    events.push_back({frame, (uint16_t)(0x03FF & ~(1 << 0))});  // A pressed
    frame += 5;
    events.push_back({frame, 0x03FF});  // release all
    
    return events;
}

// Generate input script to run all suites sequentially
// Returns events that press DOWN + A through each suite, with B to go back
// exitFrame is updated to track where we are
static std::vector<InputEvent> generateAllSuitesScript() {
    std::vector<InputEvent> events;
    // For "all" mode, we run suite 0 first, then use B+DOWN+A for each subsequent
    // But since we don't know when each suite finishes, we use exit-on-text per suite
    // For simplicity, just run the first suite (memory) and rely on exit-on-text 
    // mechanism with a special multi-suite approach
    // 
    // Actually, for --run-suite=all, we'll handle it differently in the main loop:
    // use a state machine that watches for "END:" and then presses B, DOWN, A
    return events;  // empty - handled by state machine
}

// Simple test pattern generator
void fillTestPattern(uint16_t* framebuffer) {
    // Create a gradient test pattern
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 240; x++) {
            // Create RGB555 gradient
            // Red increases left to right
            // Green increases top to bottom
            // Blue stays at max
            int r = (x * 31) / 239;      // 0-31 across width
            int g = (y * 31) / 159;      // 0-31 across height
            int b = 31;                   // Full blue
            
            uint16_t color = (b << 10) | (g << 5) | r;
            framebuffer[y * 240 + x] = color;
        }
    }
}

void printUsage(const char* programName) {
    printf("GBA Emulator - Game Boy Advance Emulator\n\n");
    printf("Usage: %s [options] [rom_path]\n\n", programName);
    printf("Arguments:\n");
    printf("  rom_path            Path to GBA ROM file (.gba)\n\n");
    printf("Options:\n");
    printf("  -h, --help               Show this help message\n");
    printf("  -t, --test-pattern       Use test pattern instead of ROM\n");
    printf("  --skip-bios              Skip BIOS and jump directly to ROM (for homebrew ROMs)\n");
    printf("  --run-suite=NAME         Auto-run a test suite (timing, memory, all, etc.)\n");
    printf("  --input-script=SPEC      Scripted key inputs (frame:BTN[+BTN];frame:;...)\n");
    printf("  --exit-on-text=TEXT      Auto-exit when mGBA debug output contains TEXT\n");
    printf("  --exit-after=N           Auto-exit after N frames\n");
    printf("  --trace-bios             Enable detailed BIOS execution tracing\n");
    printf("  --trace-instructions     Trace first 1000 instructions to /tmp/gba_instruction_trace.log\n");
    printf("  --trace-memory           Trace first 5000 instructions with memory to /tmp/gba_memory_trace.log\n\n");
    printf("Logging Options:\n");
    printf("  --log=CAT1,CAT2,...      Enable specific log categories (comma-separated)\n");
    printf("  --log-all                Enable all log categories\n");
    printf("  --log-frames=START-END   Only log within frame range (e.g., --log-frames=2230-2240)\n\n");
    printf("Log Categories: BIOS, IRQ, DMA, STACK, LDR, BL, REGION, VRAM, FEATURE, REG, TRACE, TIMER, CRASH, ALL\n\n");
    printf("Examples:\n");
    printf("  %s game.gba                    # Load and run game.gba\n", programName);
    printf("  %s assets/roms/sonic.bin       # Load ROM from assets\n", programName);
    printf("  %s --skip-bios test.gba        # Skip BIOS for homebrew ROMs\n", programName);
    printf("  %s --test-pattern              # Run with gradient test pattern\n", programName);
    printf("  %s --run-suite=timing rom.gba  # Auto-run timing test suite\n\n", programName);
    printf("Test Suite Names:\n");
    printf("  memory, io-read, timing, timers, timer-irq, shifter, carry,\n");
    printf("  multiply-long, bios-math, dma, sio-read, sio-timing, misc-edge, video, all\n\n");
    printf("Controls:\n");
    printf("  ESC                 Quit emulator\n");
    printf("\nNotes:\n");
    printf("  - Most homebrew ROMs require --skip-bios flag\n");
    printf("  - Commercial ROMs may boot through BIOS (experimental)\n");
}

// External trace flags from arm_cpu.cpp
extern bool g_trace_bios;
extern bool g_trace_all;
extern uint32_t g_trace_max_instructions;

int main(int argc, char* argv[]) {
    printf("GBA Emulator Starting...\n");
    
    // Parse command-line arguments
    const char* romPath = nullptr;
    bool useTestPattern = false;
    
    bool skipBIOS = false;
    bool enableInstructionTrace = false;
    bool enableMemoryTrace = false;
    const char* traceFile = "/tmp/gba_instruction_trace.log";
    const char* memoryTraceFile = "/tmp/gba_memory_trace.log";
    uint32_t maxTraceInstructions = 1000;
    uint32_t maxMemoryTraceInstructions = 50000;
    
    // Auto-test support
    std::vector<InputEvent> inputScript;
    const char* exitOnText = nullptr;
    int exitAfterFrames = 0;
    int runSuiteIndex = -1;    // -1 = none, -2 = all, 0-13 = specific suite
    bool runAllSuites = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--test-pattern") == 0 || strcmp(argv[i], "-t") == 0) {
            useTestPattern = true;
        } else if (strcmp(argv[i], "--skip-bios") == 0) {
            skipBIOS = true;
        } else if (strcmp(argv[i], "--trace-bios") == 0) {
            g_trace_bios = true;
            printf("BIOS tracing enabled\n");
        } else if (strcmp(argv[i], "--trace-all") == 0) {
            g_trace_all = true;
            // Check if next argument is a number
            if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                g_trace_max_instructions = atoi(argv[++i]);
            }
            printf("All-instruction tracing enabled (max %u instructions)\n", g_trace_max_instructions);
        } else if (strcmp(argv[i], "--trace-instructions") == 0) {
            enableInstructionTrace = true;
            printf("Instruction tracing enabled - writing to %s\n", traceFile);
        } else if (strcmp(argv[i], "--trace-memory") == 0) {
            enableMemoryTrace = true;
            printf("Memory tracing enabled - writing to %s\n", memoryTraceFile);
        } else if (strncmp(argv[i], "--log=", 6) == 0) {
            g_log_categories = parse_log_categories(argv[i]);
            printf("Log categories: 0x%08X\n", g_log_categories);
        } else if (strcmp(argv[i], "--log-all") == 0) {
            g_log_categories = LOG_CAT_ALL;
            printf("All log categories enabled\n");
        } else if (strncmp(argv[i], "--log-frames=", 13) == 0) {
            parse_log_frames(argv[i]);
        } else if (strncmp(argv[i], "--run-suite=", 12) == 0) {
            const char* suiteName = argv[i] + 12;
            runSuiteIndex = findSuiteIndex(suiteName);
            if (runSuiteIndex == -1) {
                fprintf(stderr, "Error: Unknown suite '%s'\n", suiteName);
                fprintf(stderr, "Available suites: memory, io-read, timing, timers, timer-irq,\n");
                fprintf(stderr, "  shifter, carry, multiply-long, bios-math, dma,\n");
                fprintf(stderr, "  sio-read, sio-timing, misc-edge, video, all\n");
                return 1;
            }
            if (runSuiteIndex == -2) {
                runAllSuites = true;
                runSuiteIndex = 0;  // start with first suite
            }
            skipBIOS = true;  // always skip BIOS for test runs
        } else if (strncmp(argv[i], "--input-script=", 15) == 0) {
            inputScript = parseInputScript(argv[i] + 15);
            printf("Loaded %zu scripted input events\n", inputScript.size());
        } else if (strncmp(argv[i], "--exit-on-text=", 15) == 0) {
            exitOnText = argv[i] + 15;
        } else if (strncmp(argv[i], "--exit-after=", 13) == 0) {
            exitAfterFrames = atoi(argv[i] + 13);
        } else if (argv[i][0] != '-') {
            // Assume it's a ROM path
            if (romPath != nullptr) {
                fprintf(stderr, "Error: Multiple ROM files specified\n");
                printUsage(argv[0]);
                return 1;
            }
            romPath = argv[i];
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            printUsage(argv[0]);
            return 1;
        }
    }
    
    try {
        // Create display with 3x scaling (720x480 window)
        Display display(3);
        printf("SDL2 Display initialized\n");
        
        // Create GBA instance
        GBA gba;
        printf("GBA initialized\n");
        
        // Connect display to memory for key input
        display.setMemory(&gba.getMemory());
        
        // Load ROM or use test pattern
        if (useTestPattern || !romPath) {
            if (!romPath && !useTestPattern) {
                printf("No ROM specified, using test pattern (use --help for usage)\n\n");
            }
            // Use test pattern mode
            GPU& gpu = gba.getGPU();
            uint16_t* vram = gpu.getFrameBuffer();
            fillTestPattern(vram);
            printf("Test pattern loaded into VRAM\n");
        } else {
            // Load BIOS first
            const char* biosPath = "assets/bios.bin";
            printf("\nLoading BIOS: %s\n", biosPath);
            if (!gba.loadBIOS(biosPath)) {
                fprintf(stderr, "Failed to load BIOS file (needed for proper boot)\n");
                fprintf(stderr, "Hint: Place GBA BIOS at assets/bios.bin\n");
                return 1;
            }
            
            // Load ROM from file
            printf("Loading ROM: %s\n", romPath);
            if (!gba.loadROM(romPath)) {
                fprintf(stderr, "Failed to load ROM file\n");
                return 1;
            }
            printf("\n");
            
            // Skip BIOS if requested via flag
            if (skipBIOS) {
                printf("Skipping BIOS boot (--skip-bios flag)\n");
                gba.skipBIOS();
            } else {
                // BIOS is loaded - boot through it
                printf("Booting through BIOS (PC will start at 0x00000000)\n");
                printf("This will show Nintendo logo animation, then run ROM\n");
                // Don't call skipBIOS() - let it run from 0x0
            }
        }
        
        // Only set Mode 3 for test patterns (ROMs will set their own display mode)
        if (useTestPattern || !romPath) {
            // Set GPU to Mode 3 (bitmap mode)
            // DISPCNT at 0x04000000, Mode 3 = bits 0-2 = 3, BG2 enable = bit 10
            Memory& mem = gba.getMemory();
            mem.write16(0x04000000, 0x0403);  // Mode 3 | BG2 enable
            printf("GPU set to Mode 3 (test pattern mode)\n");
        } else {
            printf("ROM will set its own display mode\n");
        }
        
        // Get GPU and Memory references for rendering
        GPU& gpu = gba.getGPU();
        Memory& mem = gba.getMemory();
        CPU& cpu = gba.getCPU();
        
        // Set up auto-test features
        if (runSuiteIndex >= 0) {
            // Generate input script to navigate to the suite and press A
            if (inputScript.empty()) {
                inputScript = generateSuiteScript(runSuiteIndex);
            }
            // Default exit-on-text if not specified
            if (!exitOnText) {
                exitOnText = "END:";
            }
            printf("Auto-test: will run suite #%d, exit on \"%s\"\n", runSuiteIndex, exitOnText);
        }
        if (exitOnText) {
            mem.setExitOnText(exitOnText);
        }
        // Register crashing suites to skip (detected by BEGIN message)
        if (runAllSuites) {
            mem.addSkipOnText("BEGIN: Timer count-up");
            mem.addSkipOnText("BEGIN: Timer IRQ");
            mem.addSkipOnText("BEGIN: Video");
        }
        
        // State machine for --run-suite=all
        int allSuitesCurrent = runAllSuites ? 0 : -1;
        int allSuitesWaitFrame = 0;  // frame to wait until before next action
        int allSuitesState = 0;      // 0=running, 1=waiting to press B, 2=navigating down, 3=pressing A
        int allSuitesDownPresses = 0; // how many DOWN presses remaining (for skipping suites)
        // Suites that crash the emulator — skip these in run-all mode
        auto shouldSkipSuite = [](int idx) -> bool {
            return idx == 3 || idx == 4 || idx == 13;  // timers, timer-irq, video
        };
        
        // Enable instruction tracing if requested
        if (enableInstructionTrace) {
            cpu.enableTracing(traceFile, maxTraceInstructions);
            printf("Instruction tracing started - will trace %u instructions\n", maxTraceInstructions);
        }
        if (enableMemoryTrace) {
            cpu.enableMemoryTracing(memoryTraceFile, maxMemoryTraceInstructions);
            printf("Memory tracing started - will trace %u instructions\n", maxMemoryTraceInstructions);
        }
        
        printf("\nStarting main loop...\n");
        printf("Press ESC or close window to quit\n\n");
        
        // Start scheduler-driven audio sampling (~48kHz recurring event)
        gba.getAPU().startSampling();
        
        int frameCount = 0;
        size_t scriptIndex = 0;  // next input event to process
        auto startTime = std::chrono::high_resolution_clock::now();
        auto lastFpsTime = startTime;
        int lastFpsFrame = 0;
        
        while (!display.shouldQuit()) {
            
            // Run one frame of emulation (280,896 cycles)
            // tick() accumulates audio samples into per-frame buffer
            gba.runFrame();
            
            // Push this frame's audio samples to SDL
            // SDL plays from its internal queue at 32768 Hz independently
            gba.getAPU().pushAudio();
            
            // Exit if memory tracing is complete
            if (enableMemoryTrace && cpu.isMemoryTracingComplete()) {
                printf("\n[main loop] Memory tracing complete - exiting\n");
                break;
            }
            
            // Check if we entered a crashing suite that needs to be skipped
            if (mem.shouldSkipSuite()) {
                printf("[auto-test] Skipping crashing suite (index %d), pressing B to bail out\n", allSuitesCurrent);
                mem.clearSkipSuite();
                // Immediately press B to return to menu
                mem.setKeyState(0x03FF & ~(1 << 1));  // B pressed
                allSuitesState = 2;  // next step: press DOWN to advance
                allSuitesWaitFrame = frameCount + 10;  // longer wait for crash recovery
                // Figure out how many DOWNs to skip remaining bad suites
                allSuitesCurrent++;
                allSuitesDownPresses = 1;
                while (allSuitesCurrent < 14 && shouldSkipSuite(allSuitesCurrent)) {
                    allSuitesCurrent++;
                    allSuitesDownPresses++;
                }
                if (allSuitesCurrent >= 14) {
                    printf("\n[auto-test] All suites complete, stopping\n");
                    break;
                }
                // Reset exit trigger in case END: fired during the crashing suite
                mem.setExitOnText(exitOnText);
                continue;
            }

            // Check exit-on-text trigger
            if (mem.shouldExitOnText()) {
                if (runAllSuites && allSuitesCurrent < 13) {
                    // Advance past the current suite and any that should be skipped
                    allSuitesCurrent++;
                    allSuitesDownPresses = 1;
                    while (allSuitesCurrent < 14 && shouldSkipSuite(allSuitesCurrent)) {
                        allSuitesCurrent++;
                        allSuitesDownPresses++;
                    }
                    if (allSuitesCurrent >= 14) {
                        printf("\n[auto-test] All suites complete, stopping\n");
                        break;
                    }
                    allSuitesState = 1;  // press B
                    allSuitesWaitFrame = frameCount + 5;
                    // Reset the trigger for the next suite
                    mem.setExitOnText(exitOnText);
                } else {
                    printf("\n[auto-test] Exit text matched, stopping\n");
                    break;
                }
            }
            
            // Exit after N frames
            if (exitAfterFrames > 0 && frameCount >= exitAfterFrames) {
                printf("\n[auto-test] Reached frame %d, stopping\n", frameCount);
                break;
            }
            
            // Apply scripted input events
            while (scriptIndex < inputScript.size() && inputScript[scriptIndex].frame <= frameCount) {
                mem.setKeyState(inputScript[scriptIndex].keyState);
                scriptIndex++;
            }
            
            // State machine for run-all-suites
            if (runAllSuites && allSuitesState > 0 && frameCount >= allSuitesWaitFrame) {
                switch (allSuitesState) {
                    case 1:  // Press B to return to menu
                        mem.setKeyState(0x03FF & ~(1 << 1));  // B pressed
                        allSuitesWaitFrame = frameCount + 5;
                        allSuitesState = 2;
                        break;
                    case 2:  // Press DOWN to go to next suite
                        mem.setKeyState(0x03FF & ~(1 << 7));  // DOWN pressed
                        allSuitesDownPresses--;
                        allSuitesWaitFrame = frameCount + 3;
                        allSuitesState = 5;  // release DOWN first
                        break;
                    case 5:  // Release DOWN (needed between consecutive presses)
                        mem.setKeyState(0x03FF);  // all released
                        allSuitesWaitFrame = frameCount + 3;
                        allSuitesState = (allSuitesDownPresses > 0) ? 2 : 3;  // more DOWNs or proceed to A
                        break;
                    case 3:  // Press A to select suite
                        mem.setKeyState(0x03FF & ~(1 << 0));  // A pressed
                        allSuitesWaitFrame = frameCount + 5;
                        allSuitesState = 4;
                        break;
                    case 4:  // Release all, wait for tests to run
                        mem.setKeyState(0x03FF);  // all released
                        allSuitesState = 0;
                        break;
                }
            }
            
            // Memory barrier to prevent compiler optimization bugs
            // Without this, optimized builds can corrupt GPU object references
            std::atomic_thread_fence(std::memory_order_seq_cst);
            
            // Get framebuffer pointer each frame (mode can change during execution)
            uint16_t* framebuffer = gpu.getFrameBuffer();
            
            // Get video mode and palette for display rendering
            uint16_t dispcnt = mem.read16(0x04000000);  // REG_DISPCNT
            int videoMode = dispcnt & 0x7;
            uint16_t* palette = reinterpret_cast<uint16_t*>(mem.getPaletteRAM());
            
            // Render frame (VSync provides 60fps pacing)
            display.renderFrame(framebuffer, palette, videoMode);
            
            // Handle SDL events (keyboard, window close)
            display.handleEvents();
            
            frameCount++;
            
            // Print FPS every second
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFpsTime).count();
            if (elapsed >= 1000) {
                int framesThisSecond = frameCount - lastFpsFrame;
                fprintf(stderr, "[FPS] %d fps | audio queue: %u samples (frame %d)\n", 
                        framesThisSecond, gba.getAPU().getQueuedSamples(), frameCount);
                lastFpsFrame = frameCount;
                lastFpsTime = now;
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        printf("\nEmulator shutdown cleanly\n");
        printf("Total frames rendered: %d in %lld ms (%.1f fps avg)\n", 
               frameCount, totalMs, frameCount * 1000.0 / totalMs);
        
        // Close SP tracing
        close_sp_trace();
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
