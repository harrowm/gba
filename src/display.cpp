#include "display.h"
#include "debug.h"
#include <cstring>

Display::Display(int scale) 
    : window(nullptr)
    , renderer(nullptr)
    , texture(nullptr)
    , windowWidth(GBA_WIDTH * scale)
    , windowHeight(GBA_HEIGHT * scale)
    , scale(scale)
    , initialized(false)
    , quit(false)
{
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return;
    }
    
    // Create window
    window = SDL_CreateWindow(
        "GBA Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }
    
    // Create renderer with VSync for smooth 60 FPS
    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    
    // Set logical size for automatic scaling
    SDL_RenderSetLogicalSize(renderer, GBA_WIDTH, GBA_HEIGHT);
    
    // Create texture for framebuffer
    // Using ARGB8888 format for easy conversion from RGB555
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        GBA_WIDTH,
        GBA_HEIGHT
    );
    
    if (!texture) {
        fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    
    initialized = true;
    printf("Display initialized: %dx%d window (%dx scale)\n", 
           windowWidth, windowHeight, scale);
}

Display::~Display() {
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    DEBUG_INFO("Display shut down");
}

void Display::renderFrame(const uint16_t* framebuffer, const uint16_t* palette, int videoMode) {
    if (!initialized || !framebuffer) {
        static int noFramebufCount = 0;
        if (noFramebufCount++ < 5) {
            fprintf(stderr, "[Display] renderFrame skipped: init=%d fb=%p\n", initialized, (void*)framebuffer);
        }
        return;
    }
    
    // Debug: Print pixels from center of screen where logo would be
    static int debugCount = 0;
    static int lastMode = -1;
    if (debugCount < 20 || videoMode != lastMode) {
        // Check center of screen (y=80, x around 100-140)
        int centerY = 80;
        int nonWhiteCount = 0;
        for (int x = 0; x < 240; x++) {
            uint16_t px = framebuffer[centerY * 240 + x];
            if (px != 0x7FFF && px != 0xFFFF && px != 0) {
                nonWhiteCount++;
            }
        }
        fprintf(stderr, "[Display #%d] Mode=%d, centerRow nonWhite=%d, center: 0x%04X 0x%04X 0x%04X 0x%04X\n", 
                debugCount, videoMode, nonWhiteCount,
                framebuffer[centerY*240+100], framebuffer[centerY*240+110], 
                framebuffer[centerY*240+120], framebuffer[centerY*240+130]);
        lastMode = videoMode;
        debugCount++;
        if (videoMode == 4) {
            const uint8_t* fb8 = reinterpret_cast<const uint8_t*>(framebuffer);
            for (int i = 0; i < 10; i++) {
                LOG_TRACE_CAT("0x%02X ", fb8[i]);
            }
        } else {
            for (int i = 0; i < 10; i++) {
                LOG_TRACE_CAT("0x%04X ", framebuffer[i]);
            }
        }
        LOG_TRACE_CAT("\n");
        debugCount++;
    }
    
    // Lock texture for pixel updates
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) < 0) {
        fprintf(stderr, "Texture lock failed: %s\n", SDL_GetError());
        return;
    }
    
    // Convert GBA format to ARGB8888 (SDL format)
    uint32_t* pixelData = static_cast<uint32_t*>(pixels);
    
    if (videoMode == 4) {
        // Mode 4: 8-bit palettized bitmap (240x160, palette indices)
        const uint8_t* fb8 = reinterpret_cast<const uint8_t*>(framebuffer);
        
        for (int y = 0; y < GBA_HEIGHT; y++) {
            for (int x = 0; x < GBA_WIDTH; x++) {
                // Read 8-bit palette index
                uint8_t paletteIndex = fb8[y * GBA_WIDTH + x];
                
                // Look up color in palette (default to white if no palette)
                uint16_t bgr555 = palette ? palette[paletteIndex] : 0xFFFF;
                
                // Extract BGR555 components (5 bits each) - GBA uses BGR order!
                uint8_t r5 = (bgr555 >> 0) & 0x1F;
                uint8_t g5 = (bgr555 >> 5) & 0x1F;
                uint8_t b5 = (bgr555 >> 10) & 0x1F;
                
                // Convert 5-bit to 8-bit (multiply by 255/31 ≈ 8.23)
                uint8_t r8 = (r5 << 3) | (r5 >> 2);
                uint8_t g8 = (g5 << 3) | (g5 >> 2);
                uint8_t b8 = (b5 << 3) | (b5 >> 2);
                
                // Pack into ARGB8888 format
                pixelData[y * (pitch / 4) + x] = 
                    (0xFF << 24) |  // Alpha (fully opaque)
                    (r8 << 16) |    // Red
                    (g8 << 8) |     // Green
                    (b8 << 0);      // Blue
            }
        }
    } else {
        // Mode 3/5: 16-bit direct color bitmap (BGR555)
        for (int y = 0; y < GBA_HEIGHT; y++) {
            for (int x = 0; x < GBA_WIDTH; x++) {
                // Read BGR555 pixel from GBA framebuffer
                uint16_t bgr555 = framebuffer[y * GBA_WIDTH + x];
                
                // Extract BGR555 components (5 bits each) - GBA uses BGR order!
                uint8_t r5 = (bgr555 >> 0) & 0x1F;
                uint8_t g5 = (bgr555 >> 5) & 0x1F;
                uint8_t b5 = (bgr555 >> 10) & 0x1F;
                
                // Convert 5-bit to 8-bit (multiply by 255/31 ≈ 8.23)
                uint8_t r8 = (r5 << 3) | (r5 >> 2);
                uint8_t g8 = (g5 << 3) | (g5 >> 2);
                uint8_t b8 = (b5 << 3) | (b5 >> 2);
                
                // Pack into ARGB8888 format
                pixelData[y * (pitch / 4) + x] = 
                    (0xFF << 24) |  // Alpha (fully opaque)
                    (r8 << 16) |    // Red
                    (g8 << 8) |     // Green
                    (b8 << 0);      // Blue
            }
        }
    }
    
    SDL_UnlockTexture(texture);
    
    // Clear renderer
    SDL_RenderClear(renderer);
    
    // Copy texture to renderer
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    
    // Present frame
    SDL_RenderPresent(renderer);
}

void Display::handleEvents() {
    static int callCount = 0;
    if (callCount < 10) {
        fprintf(stderr, "[handleEvents] call #%d this=%p\n", callCount, (void*)this);
    }
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (callCount < 10) {
            fprintf(stderr, "[handleEvents] got event type=%d\n", event.type);
        }
        switch (event.type) {
            case SDL_QUIT:
                quit = true;
                DEBUG_INFO("Quit requested");
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                    DEBUG_INFO("ESC pressed, quitting");
                }
                // TODO: Add GBA button mappings here later
                break;
                
            case SDL_KEYUP:
                // TODO: Add GBA button handling here later
                break;
                
            default:
                break;
        }
    }
    
    if (callCount < 10) {
        fprintf(stderr, "[handleEvents] done #%d\n", callCount);
        callCount++;
    }
}
