#ifndef OPTIMIZED_ARM_INSTRUCTION_CACHE_H
#define OPTIMIZED_ARM_INSTRUCTION_CACHE_H

#include <cstdint>
#include <cstring>
#include "utility_macros.h"

// Cache statistics collection control
#ifndef ARM_CACHE_STATS
#define ARM_CACHE_STATS 0
#endif

// Forward declaration
class ARMCPU;

/**
 * Optimized ARM Instruction Cache
 * 
 * Enhanced cache implementation with:
 * 1. Larger cache size for better hit rates
 * 2. 2-way set associative structure to reduce thrashing
 * 3. Optimized lookup path with minimal operations
 * 4. Pre-computed flag check results for common conditions
 */

// Cache configuration - increased size and set associativity
constexpr uint32_t ARM_ICACHE_SIZE = 4096;  // Total number of cache entries (must be power of 2)
constexpr uint32_t ARM_ICACHE_WAYS = 2;     // Number of ways (set associativity)
constexpr uint32_t ARM_ICACHE_SETS = ARM_ICACHE_SIZE / ARM_ICACHE_WAYS;  // Number of sets
constexpr uint32_t ARM_ICACHE_SET_MASK = ARM_ICACHE_SETS - 1;
constexpr uint32_t ARM_ICACHE_TAG_SHIFT = 12; // log2(ARM_ICACHE_SIZE)

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

/**
 * Enhanced cached instruction structure
 * Optimized for both memory efficiency and performance
 */
struct OptimizedARMCachedInstruction {
    // Cache validation fields
    uint32_t pc_tag;                    // Upper bits of PC for cache validation
    uint32_t instruction;               // Original instruction word for validation
    
    // Pre-decoded flags (packed in a single byte for cache efficiency)
    uint8_t flags;                      // Bit 0: valid, Bit 1: pc_modified, Bit 2: immediate, Bit 3: set_flags, etc.
    
    // Condition masks for quick checking - these are pre-computed masks that can be directly applied to CPSR
    uint8_t condition_mask;             // Mask to apply to status flags
    uint8_t condition_result;           // Expected result after masking for condition to be true
    
    // Pre-decoded instruction information
    ARMInstructionType type;            // Instruction category
    
    // Register indices
    uint8_t rd, rn, rm;
    
    // Data processing specific fields
    ARMDataProcessingOp dp_op;          // Data processing operation
    
    // Pre-computed operand values
    uint32_t imm_value;                 // Pre-computed immediate value
    uint32_t imm_carry;                 // Pre-computed carry value
    
    // Memory transfer specific fields
    uint8_t offset_type;                // Offset addressing mode
    int32_t offset_value;               // Pre-computed offset for immediate addressing
    
    // Branch specific fields
    int32_t branch_offset;              // Pre-computed branch offset
    
    // Block transfer specific fields
    uint16_t register_list;             // Register list for LDM/STM
    uint8_t addressing_mode;            // Addressing mode bits
    
    // Function pointer for direct execution 
    void (ARMCPU::*execute_func)(const OptimizedARMCachedInstruction&);
    
    // Inline accessors for packed flags
    FORCE_INLINE bool isValid() const { return flags & 0x01; }
    FORCE_INLINE void setValid(bool valid) { flags = (flags & ~0x01) | (valid ? 0x01 : 0); }
    
    FORCE_INLINE bool modifiesPC() const { return flags & 0x02; }
    FORCE_INLINE void setModifiesPC(bool modifies) { flags = (flags & ~0x02) | (modifies ? 0x02 : 0); }
    
    FORCE_INLINE bool isImmediate() const { return flags & 0x04; }
    FORCE_INLINE void setImmediate(bool imm) { flags = (flags & ~0x04) | (imm ? 0x04 : 0); }
    
    FORCE_INLINE bool setsFlags() const { return flags & 0x08; }
    FORCE_INLINE void setSetsFlags(bool sf) { flags = (flags & ~0x08) | (sf ? 0x08 : 0); }
    
    FORCE_INLINE bool isLoad() const { return flags & 0x10; }
    FORCE_INLINE void setLoad(bool load) { flags = (flags & ~0x10) | (load ? 0x10 : 0); }
    
    FORCE_INLINE bool isLink() const { return flags & 0x20; }
    FORCE_INLINE void setLink(bool link) { flags = (flags & ~0x20) | (link ? 0x20 : 0); }
    
    // Default constructor - initializes all fields
    OptimizedARMCachedInstruction() : pc_tag(0), instruction(0), flags(0), condition_mask(0), 
                                      condition_result(0), type(ARMInstructionType::UNDEFINED),
                                      rd(0), rn(0), rm(0), dp_op(ARMDataProcessingOp(0)),
                                      imm_value(0), imm_carry(0), offset_type(0),
                                      offset_value(0), branch_offset(0), register_list(0),
                                      addressing_mode(0), execute_func(nullptr) {}
};

/**
 * Optimized ARM Instruction Cache Class
 */
class OptimizedARMInstructionCache {
private:
    // 2-way set associative cache structure
    OptimizedARMCachedInstruction cache[ARM_ICACHE_SETS][ARM_ICACHE_WAYS];
    
    // LRU tracking - one bit per set to track which way was least recently used
    uint32_t lru_bits[(ARM_ICACHE_SETS + 31) / 32];
    
    // Cache statistics for performance monitoring
#if ARM_CACHE_STATS
    uint64_t hits;
    uint64_t misses;
    uint64_t invalidations;
#endif

    // Get LRU bit for a set
    FORCE_INLINE bool getLRUBit(uint32_t set) const {
        uint32_t word_index = set / 32;
        uint32_t bit_index = set % 32;
        return (lru_bits[word_index] >> bit_index) & 1;
    }
    
    // Set LRU bit for a set
    FORCE_INLINE void setLRUBit(uint32_t set, bool value) {
        uint32_t word_index = set / 32;
        uint32_t bit_index = set % 32;
        if (value) {
            lru_bits[word_index] |= (1 << bit_index);
        } else {
            lru_bits[word_index] &= ~(1 << bit_index);
        }
    }
    
    // Update LRU information
    FORCE_INLINE void updateLRU(uint32_t set, uint32_t way) {
        // Mark the other way as LRU
        setLRUBit(set, way == 0);
    }
    
public:
    OptimizedARMInstructionCache() 
#if ARM_CACHE_STATS
        : hits(0), misses(0), invalidations(0) 
#endif
    {
        clear();
    }
    
    // Clear entire cache
    FORCE_INLINE void clear() {
        for (uint32_t i = 0; i < ARM_ICACHE_SETS; i++) {
            for (uint32_t j = 0; j < ARM_ICACHE_WAYS; j++) {
                cache[i][j].setValid(false);
            }
        }
        
        // Reset LRU bits
        std::memset(lru_bits, 0, sizeof(lru_bits));
    }
    
    // Lookup instruction in cache with optimized path
    FORCE_INLINE OptimizedARMCachedInstruction* lookup(uint32_t pc, uint32_t instruction) {
        // Fast hash calculation - direct index from PC bits
        uint32_t set = (pc >> 2) & ARM_ICACHE_SET_MASK;
        uint32_t tag = pc >> (ARM_ICACHE_TAG_SHIFT + 2);
        
        // Check way 0 first (most recently used typically)
        if (cache[set][0].isValid() && cache[set][0].pc_tag == tag && cache[set][0].instruction == instruction) {
#if ARM_CACHE_STATS
            hits++;
#endif
            updateLRU(set, 0);
            return &cache[set][0];
        }
        
        // Check way 1
        if (cache[set][1].isValid() && cache[set][1].pc_tag == tag && cache[set][1].instruction == instruction) {
#if ARM_CACHE_STATS
            hits++;
#endif
            updateLRU(set, 1);
            return &cache[set][1];
        }
        
#if ARM_CACHE_STATS
        misses++;
#endif
        return nullptr;
    }
    
    // Insert decoded instruction into cache
    FORCE_INLINE void insert(uint32_t pc, const OptimizedARMCachedInstruction& decoded) {
        uint32_t set = (pc >> 2) & ARM_ICACHE_SET_MASK;
        uint32_t tag = pc >> (ARM_ICACHE_TAG_SHIFT + 2);
        
        // Get LRU way to replace
        uint32_t way = getLRUBit(set);
        
        // Insert into cache
        cache[set][way] = decoded;
        cache[set][way].pc_tag = tag;
        cache[set][way].setValid(true);
        
        // Update LRU to mark this way as most recently used
        updateLRU(set, way);
    }
    
    // Invalidate cache entries for a memory range (for self-modifying code)
    void invalidate_range(uint32_t start_addr, uint32_t end_addr) {
        uint32_t start_set = (start_addr >> 2) & ARM_ICACHE_SET_MASK;
        uint32_t end_set = (end_addr >> 2) & ARM_ICACHE_SET_MASK;
        
        // Compute tag ranges (conservative approach for faster invalidation)
        uint32_t start_tag = start_addr >> (ARM_ICACHE_TAG_SHIFT + 2);
        uint32_t end_tag = end_addr >> (ARM_ICACHE_TAG_SHIFT + 2);
        
        // Handle wrap-around case
        if (start_set <= end_set) {
            for (uint32_t set = start_set; set <= end_set; set++) {
                for (uint32_t way = 0; way < ARM_ICACHE_WAYS; way++) {
                    OptimizedARMCachedInstruction& entry = cache[set][way];
                    if (entry.isValid() && entry.pc_tag >= start_tag && entry.pc_tag <= end_tag) {
                        entry.setValid(false);
#if ARM_CACHE_STATS
                        invalidations++;
#endif
                    }
                }
            }
        } else {
            // Wrapped around
            for (uint32_t set = start_set; set < ARM_ICACHE_SETS; set++) {
                for (uint32_t way = 0; way < ARM_ICACHE_WAYS; way++) {
                    OptimizedARMCachedInstruction& entry = cache[set][way];
                    if (entry.isValid() && entry.pc_tag >= start_tag) {
                        entry.setValid(false);
#if ARM_CACHE_STATS
                        invalidations++;
#endif
                    }
                }
            }
            
            for (uint32_t set = 0; set <= end_set; set++) {
                for (uint32_t way = 0; way < ARM_ICACHE_WAYS; way++) {
                    OptimizedARMCachedInstruction& entry = cache[set][way];
                    if (entry.isValid() && entry.pc_tag <= end_tag) {
                        entry.setValid(false);
#if ARM_CACHE_STATS
                        invalidations++;
#endif
                    }
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

// Pre-computed condition check masks and results
// For direct application to CPSR[31:28] without individual flag extraction
struct ConditionCheckLUT {
    static constexpr uint8_t MASKS[16] = {
        0x40, 0x40, 0x20, 0x20,   // EQ, NE, CS, CC 
        0x80, 0x80, 0x10, 0x10,   // MI, PL, VS, VC
        0x60, 0x60, 0x90, 0x90,   // HI, LS, GE, LT
        0xD0, 0xD0, 0x00, 0x00    // GT, LE, AL, NV
    };
    
    static constexpr uint8_t RESULTS[16] = {
        0x40, 0x00, 0x20, 0x00,   // EQ, NE, CS, CC
        0x80, 0x00, 0x10, 0x00,   // MI, PL, VS, VC
        0x20, 0x00, 0x00, 0x80,   // HI, LS, GE, LT
        0x00, 0x40, 0x00, 0x00    // GT, LE, AL, NV
    };
};

#endif // OPTIMIZED_ARM_INSTRUCTION_CACHE_H
