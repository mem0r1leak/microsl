#pragma once

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define MSL_DEBUG_BREAK() __asm__ volatile("int3")
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
    #define MSL_DEBUG_BREAK() __asm__ volatile("brk #0")
#else
    #define MSL_DEBUG_BREAK() (*(volatile int*)0 = 0)
#endif

#ifndef NDEBUG
    #define assert(cond) \
    do { \
    if (!(cond)) { \
    MSL_DEBUG_BREAK(); \
    } \
    } while (0)
#else
    #define assert(cond) ((void)0)
#endif
