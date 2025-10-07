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

void Display::renderFrame(const uint16_t* framebuffer) {
    if (!initialized || !framebuffer) {
        return;
    }
    
    // Debug: Print first few pixels to verify VRAM contents
    static int debugCount = 0;
    if (debugCount < 5) {
        printf("[Display #%d] framebuffer=%p, First 10 pixels: ", debugCount, (void*)framebuffer);
        for (int i = 0; i < 10; i++) {
            printf("0x%04X ", framebuffer[i]);
        }
        printf("\n");
        debugCount++;
    }
    
    // Lock texture for pixel updates
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) < 0) {
        fprintf(stderr, "Texture lock failed: %s\n", SDL_GetError());
        return;
    }
    
    // Convert BGR555 (GBA format) to ARGB8888 (SDL format)
    uint32_t* pixelData = static_cast<uint32_t*>(pixels);
    
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
    
    SDL_UnlockTexture(texture);
    
    // Clear renderer
    SDL_RenderClear(renderer);
    
    // Copy texture to renderer
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    
    // Present frame
    SDL_RenderPresent(renderer);
}

void Display::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
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
}
