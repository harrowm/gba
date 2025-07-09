#ifndef UTILITY_MACROS_H
#define UTILITY_MACROS_H

// Macro to suppress unused parameter warnings
#define UNUSED(x) (void)(x)

// Force inlining of critical functions for performance
#ifdef __GNUC__
    #define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define FORCE_INLINE __forceinline
#else
    #define FORCE_INLINE inline
#endif

#endif // UTILITY_MACROS_H
