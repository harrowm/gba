#ifndef ARM_INSTRUCTION_CACHE_H
#define ARM_INSTRUCTION_CACHE_H

#include <cstdint>
#include <cstring>
#include "utility_macros.h"

// Cache statistics collection control
// Define ARM_CACHE_STATS to enable cache statistics collection
// This adds overhead but provides useful performance monitoring
#ifndef ARM_CACHE_STATS
#define ARM_CACHE_STATS 1
#endif

// Forward declaration
class ARMCPU;

/**
 * ARM Instruction Cache
 * 
 * Caches decoded ARM instruction information to eliminate redundant decode operations.
 * Uses a direct-mapped cache with PC as the index to achieve high hit rates for
 * typical ARM code patterns (loops, function calls).
 */

// Cache configuration
constexpr uint32_t ARM_ICACHE_SIZE = 1024;  // Number of cache entries (must be power of 2)
constexpr uint32_t ARM_ICACHE_MASK = ARM_ICACHE_SIZE - 1;
constexpr uint32_t ARM_ICACHE_TAG_SHIFT = 10; // log2(ARM_ICACHE_SIZE)

// Instruction types for fast dispatch
enum class ARMInstructionType : uint8_t {
    DATA_PROCESSING = 0,
    MULTIPLY = 1,
    BX = 2,
    SINGLE_DATA_TRANSFER = 3,
    BLOCK_DATA_TRANSFER = 4,
    BRANCH = 5,
    SOFTWARE_INTERRUPT = 6,
    PSR_TRANSFER = 7,
    COPROCESSOR_OP = 8,
    COPROCESSOR_TRANSFER = 9,
    COPROCESSOR_REGISTER = 10,
    UNDEFINED = 11
};

// Data processing operation types for fast dispatch (using existing typedef)
// ARMDataProcessingOp is defined in arm_timing.h

/**
 * Cached instruction decode information
 * Optimized for common ARM instruction patterns
 */
struct ARMCachedInstruction {
    // Cache validity and identification
    uint32_t pc_tag;                    // Upper bits of PC for cache validation
    uint32_t instruction;               // Original instruction word
    bool valid;                         // Cache entry validity
    
    uint8_t shift_type;                 // 2 bit shift type in operand2
    bool reg_shift;                     // true if the shift treats rs as a register, false if rs is actually an imm value
    uint8_t rotate_imm;                 // 4 bit rotate value in operand2
    uint8_t imm8;                       // 8 bit immediate va;ue in operand2
    
    // Pre-decoded instruction information
    ARMInstructionType type;            // Instruction category
    uint8_t condition;                  // Condition code (4 bits)
    bool pc_modified;                   // Whether instruction modifies PC
    
    // Data processing specific fields
    ARMDataProcessingOp dp_op;          // Data processing operation
    uint8_t rd, rn, rm, rs;             // Register indices (add rs for multiply)
    // Multiply-long specific fields
    uint8_t rdLo, rdHi;                 // Destination registers for multiply-long
    bool signed_op;                     // Signed/unsigned op for multiply-long
    bool set_flags;                     // S bit (data proc or multiply)
    bool accumulate;                    // Accumulate bit for multiply/MLA
    bool immediate;                     // Immediate operand flag
    
    // Pre-computed operand2 information for immediate operands
    uint32_t imm_value;                 // Pre-rotated immediate value
    uint32_t imm_carry;                 // Carry out from immediate rotation
    bool imm_valid;                     // Whether immediate values are pre-computed
    
    // Memory transfer specific fields
    bool load;                          // Load vs store
    uint8_t offset_type;                // Offset addressing mode
    int32_t offset_value;               // Pre-computed offset for immediate addressing
    
    // Branch specific fields
    int32_t branch_offset;              // Pre-computed branch offset
    bool link;                          // Branch with link flag
    
    // Block transfer specific fields
    uint16_t register_list;             // Register list for LDM/STM
    uint8_t addressing_mode;            // Addressing mode bits
    
    // Function pointer for direct execution (critical optimization)
    void (ARMCPU::*execute_func)(const ARMCachedInstruction&);
    
    // Default constructor
    ARMCachedInstruction() : valid(false) {}
};

/**
 * ARM Instruction Cache Class
 */
class ARMInstructionCache {
private:
    ARMCachedInstruction cache[ARM_ICACHE_SIZE];
    
    // Cache statistics for performance monitoring (conditionally compiled)
#if ARM_CACHE_STATS
    uint64_t hits;
    uint64_t misses;
    uint64_t invalidations;
#endif
    
public:
    ARMInstructionCache() 
#if ARM_CACHE_STATS
        : hits(0), misses(0), invalidations(0) 
#endif
    {
        clear();
    }
    
    // Clear entire cache
    FORCE_INLINE void clear() {
        for (uint32_t i = 0; i < ARM_ICACHE_SIZE; i++) {
            cache[i].valid = false;
        }
    }
    
    // Lookup instruction in cache
    FORCE_INLINE ARMCachedInstruction* lookup(uint32_t pc, uint32_t instruction) {
        uint32_t index = (pc >> 2) & ARM_ICACHE_MASK;
        uint32_t tag = pc >> (ARM_ICACHE_TAG_SHIFT + 2);
        
        ARMCachedInstruction* entry = &cache[index];
        
        if (entry->valid && entry->pc_tag == tag && entry->instruction == instruction) {
#if ARM_CACHE_STATS
            hits++;
#endif
            return entry;
        }
        
#if ARM_CACHE_STATS
        misses++;
#endif
        return nullptr;
    }
    
    // Insert decoded instruction into cache
    FORCE_INLINE void insert(uint32_t pc, const ARMCachedInstruction& decoded) {
        uint32_t index = (pc >> 2) & ARM_ICACHE_MASK;
        uint32_t tag = pc >> (ARM_ICACHE_TAG_SHIFT + 2);
        
        cache[index] = decoded;
        cache[index].pc_tag = tag;
        cache[index].valid = true;
    }
    
    // Invalidate cache entries for a memory range (for self-modifying code)
    void invalidate_range(uint32_t start_addr, uint32_t end_addr) {
        uint32_t start_index = (start_addr >> 2) & ARM_ICACHE_MASK;
        uint32_t end_index = (end_addr >> 2) & ARM_ICACHE_MASK;
        
        // Handle wrap-around case
        if (start_index <= end_index) {
            for (uint32_t i = start_index; i <= end_index; i++) {
                if (cache[i].valid) {
                    cache[i].valid = false;
#if ARM_CACHE_STATS
                    invalidations++;
#endif
                }
            }
        } else {
            // Wrapped around
            for (uint32_t i = start_index; i < ARM_ICACHE_SIZE; i++) {
                if (cache[i].valid) {
                    cache[i].valid = false;
#if ARM_CACHE_STATS
                    invalidations++;
#endif
                }
            }
            for (uint32_t i = 0; i <= end_index; i++) {
                if (cache[i].valid) {
                    cache[i].valid = false;
#if ARM_CACHE_STATS
                    invalidations++;
#endif
                }
            }
        }
    }
    
    // Get cache statistics
    struct CacheStats {
        uint64_t hits;
        uint64_t misses;
        uint64_t invalidations;
        double hit_rate;
    };
    
    CacheStats getStats() const {
        CacheStats stats;
#if ARM_CACHE_STATS
        stats.hits = hits;
        stats.misses = misses;
        stats.invalidations = invalidations;
        uint64_t total = hits + misses;
        stats.hit_rate = total > 0 ? (double)hits / total : 0.0;
#else
        stats.hits = 0;
        stats.misses = 0;
        stats.invalidations = 0;
        stats.hit_rate = 0.0;
#endif
        return stats;
    }
    
    // Reset statistics
    void resetStats() {
#if ARM_CACHE_STATS
        hits = misses = invalidations = 0;
#endif
    }
};

#endif // ARM_INSTRUCTION_CACHE_H
