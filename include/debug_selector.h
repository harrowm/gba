#ifndef DEBUG_SELECTOR_H
#define DEBUG_SELECTOR_H

/**
 * This header automatically selects the appropriate debug implementation based on build flags.
 * This is a legacy file for backward compatibility with code that hasn't been updated to use
 * the macro-based debug system directly.
 * 
 * For new code, include "debug_macros.h" directly instead of this file.
 * 
 * Benefits of the macro-based debug system:
 * - Zero runtime overhead in release builds (debug code is eliminated at compile time)
 * - Lazy evaluation for expensive operations (only executes when debug is enabled)
 * - Consistent interface for both debug and release builds
 * - Ability to conditionally compile debug code based on debug levels
 */

// Include the modern macro-based debug system
#include "debug_macros.h"

#endif // DEBUG_SELECTOR_H
