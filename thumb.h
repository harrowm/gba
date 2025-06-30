#ifndef THUMB_H
#define THUMB_H

#include <stdint.h>

// Trie node structure for Thumb instruction decoding
typedef struct ThumbTrieNode {
    struct ThumbTrieNode* children[2]; // Binary tree for 0 and 1 bits
    void (*handler)(uint16_t instruction); // Handler function for the instruction
    uint8_t cycles; // Cycle count for the instruction
} ThumbTrieNode;

// Function prototypes
void thumb_init_trie();
uint8_t thumb_decode_and_execute(uint16_t instruction);

#endif // THUMB_H
