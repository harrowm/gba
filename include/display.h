#pragma once

#include <cstdint>
#include <SDL2/SDL.h>

// Display class for rendering GBA screen using SDL2
// Handles window creation, texture management, and frame presentation
class Display {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    
    int windowWidth;
    int windowHeight;
    int scale;
    
    bool initialized;
    bool quit;
    
    // GBA native resolution
    static constexpr int GBA_WIDTH = 240;
    static constexpr int GBA_HEIGHT = 160;
    
public:
    // Create display with given scale factor (e.g., 3 = 720x480 window)
    explicit Display(int scale = 3);
    ~Display();
    
    // Disable copy/move
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;
    
    // Render a frame from GBA framebuffer
    // For Mode 3/5: framebuffer contains RGB555 colors
    // For Mode 4: framebuffer contains 8-bit palette indices, palette is the 256-color palette
    void renderFrame(const uint16_t* framebuffer, const uint16_t* palette = nullptr, int videoMode = 3);
    
    // Handle SDL events (keyboard, window close, etc.)
    void handleEvents();
    
    // Check if user requested quit
    bool shouldQuit() const { return quit; }
    
    // Get SDL window for additional manipulation if needed
    SDL_Window* getWindow() { return window; }
};
