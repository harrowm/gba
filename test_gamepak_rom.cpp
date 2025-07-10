// Simple test ROM for GBA - ARM assembly equivalent in C++
// This will be compiled to ARM code and placed at GamePak ROM start (0x08000000)

// Entry point - infinite loop with some cache-testing patterns
void gameEntry() {
    // Simple loop that should generate good cache hit patterns
    volatile int counter = 0;
    for (int i = 0; i < 1000000; i++) {
        counter++;
        // Add some branching to test cache with different instruction patterns
        if (counter % 100 == 0) {
            counter = counter * 2;
        } else {
            counter = counter + 1;
        }
    }
    
    // After loop, do some more complex patterns
    for (int j = 0; j < 1000; j++) {
        for (int k = 0; k < 100; k++) {
            counter += j * k;
        }
    }
    
    // Infinite loop to keep running
    while (true) {
        counter++;
    }
}

// ARM vector table for GamePak (simplified)
extern "C" {
    void _start() {
        gameEntry();
    }
}
