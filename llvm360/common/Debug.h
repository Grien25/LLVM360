#pragma once

// Cross-platform debug break that reliably trips a debugger.
// Avoids relying on __builtin_trap on non-Windows, which can be optimized
// into an illegal instruction without necessarily breaking under a debugger.

#if defined(_WIN32) || defined(_WIN64)
  #if defined(_MSC_VER)
    #include <intrin.h>
    #ifndef DebugBreak
      #define DebugBreak() __debugbreak()
    #endif
  #else
    // MinGW/Clang-cl typically provide __debugbreak as well
    #ifndef DebugBreak
      #define DebugBreak() __builtin_trap()
    #endif
  #endif
#else
  // Prefer compiler-provided debug trap when available
  #if defined(__has_builtin)
    #if __has_builtin(__builtin_debugtrap)
      #ifndef DebugBreak
        #define DebugBreak() __builtin_debugtrap()
      #endif
    #endif
  #endif
  #ifndef DebugBreak
    // Fallback: raise SIGTRAP so debuggers stop here (works on macOS/Linux)
    #include <signal.h>
    #define DebugBreak() raise(SIGTRAP)
  #endif
#endif

#ifdef DISABLE_DEBUGBREAK
#undef DebugBreak
#define DebugBreak() ((void)0)
#endif
