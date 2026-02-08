#pragma once

#include <cstdint>
#include <SDL2/SDL.h>

// Forward declaration
class Memory;

// GBA Button bits in KEYINPUT register (active LOW - 0 = pressed, 1 = released)
enum GBAButton {
    BTN_A      = 0,   // Bit 0
    BTN_B      = 1,   // Bit 1
    BTN_SELECT = 2,   // Bit 2
    BTN_START  = 3,   // Bit 3
    BTN_RIGHT  = 4,   // Bit 4
    BTN_LEFT   = 5,   // Bit 5
    BTN_UP     = 6,   // Bit 6
    BTN_DOWN   = 7,   // Bit 7
    BTN_R      = 8,   // Bit 8
    BTN_L      = 9    // Bit 9
};

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
    
    // Key input state (directly mapped to KEYINPUT register format)
    // Bit = 0 means pressed, Bit = 1 means released
    uint16_t keyState;  // Current key state
    Memory* memory;     // Pointer to memory for updating KEYINPUT register
    
    // GBA native resolution
    static constexpr int GBA_WIDTH = 240;
    static constexpr int GBA_HEIGHT = 160;
    
    // Update a button state
    void updateButton(GBAButton button, bool pressed);
    
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
    
    // Set memory pointer for KEYINPUT register updates
    void setMemory(Memory* mem) { memory = mem; }
};
