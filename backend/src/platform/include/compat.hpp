#ifndef PLATFORM_COMPAT_HPP
#define PLATFORM_COMPAT_HPP

// ============================================================================
// Platform Detection and Compatibility Macros
// ============================================================================
// This header provides platform detection macros for conditional compilation
// of platform-specific code. Virtual TestSet supports:
//
// - Linux: Full functionality (raw sockets, real-time, packet I/O)
// - macOS: Limited functionality (no raw networking, no-op RT functions)
//
// Usage:
//   #ifdef VTS_PLATFORM_LINUX
//     // Linux-specific code
//   #elif defined(VTS_PLATFORM_MAC)
//     // macOS-specific code
//   #endif
// ============================================================================

// Platform detection
#if defined(__linux__)
    #define VTS_PLATFORM_LINUX 1
    #define VTS_PLATFORM_NAME "Linux"
    #define VTS_HAS_RAW_SOCKETS 1
    #define VTS_HAS_REALTIME 1
    #define VTS_HAS_PACKET_MMAP 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define VTS_PLATFORM_MAC 1
    #define VTS_PLATFORM_NAME "macOS"
    #define VTS_HAS_RAW_SOCKETS 1  // Using BPF for raw packet I/O
    #define VTS_HAS_BPF 1          // Berkeley Packet Filter support
    #define VTS_HAS_REALTIME 0
    #define VTS_HAS_PACKET_MMAP 0
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define VTS_PLATFORM_WINDOWS 1
    #define VTS_PLATFORM_NAME "Windows"
    #define VTS_HAS_RAW_SOCKETS 1  // Using Npcap for raw packet I/O
    #define VTS_HAS_NPCAP 1         // Npcap/WinPcap support
    #define VTS_HAS_REALTIME 0  // Windows has different RT APIs (SetThreadPriority)
    #define VTS_HAS_PACKET_MMAP 0
#else
    #define VTS_PLATFORM_UNKNOWN
    #define VTS_PLATFORM_NAME "Unknown"
    #define VTS_HAS_RAW_SOCKETS 0
    #define VTS_HAS_REALTIME 0
    #define VTS_HAS_PACKET_MMAP 0
    #warning "Unsupported platform detected. Building with limited functionality."
#endif

// Feature detection helpers
#ifdef VTS_PLATFORM_LINUX
    #include <linux/version.h>
    
    // Check for TPACKET_V3 support (kernel >= 3.2)
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
        #define VTS_HAS_TPACKET_V3 1
    #else
        #define VTS_HAS_TPACKET_V3 0
    #endif
#else
    #define VTS_HAS_TPACKET_V3 0
#endif

// Compiler hints for platform-specific optimizations
#if defined(VTS_PLATFORM_LINUX) || defined(VTS_PLATFORM_MAC)
    // GCC/Clang-specific compiler hints
    #define VTS_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define VTS_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define VTS_PREFETCH(addr) __builtin_prefetch(addr)
#elif defined(VTS_PLATFORM_WINDOWS) && defined(_MSC_VER)
    // MSVC-specific hints
    #define VTS_LIKELY(x)   (x)
    #define VTS_UNLIKELY(x) (x)
    #define VTS_PREFETCH(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
    #include <xmmintrin.h>  // For _mm_prefetch
#else
    // Generic fallbacks
    #define VTS_LIKELY(x)   (x)
    #define VTS_UNLIKELY(x) (x)
    #define VTS_PREFETCH(addr) ((void)(addr))
#endif

// Platform capability summary
namespace vts {
namespace platform {

struct Capabilities {
    static constexpr bool has_raw_sockets = VTS_HAS_RAW_SOCKETS;
    static constexpr bool has_realtime = VTS_HAS_REALTIME;
    static constexpr bool has_packet_mmap = VTS_HAS_PACKET_MMAP;
    static constexpr bool has_tpacket_v3 = VTS_HAS_TPACKET_V3;
    static constexpr const char* platform_name = VTS_PLATFORM_NAME;
};

// Get platform info string
inline const char* get_platform_info() {
#ifdef VTS_PLATFORM_LINUX
    return "Linux (full functionality: raw sockets, RT, TPACKET_V3)";
#elif defined(VTS_PLATFORM_MAC)
    return "macOS (limited: no raw sockets, no Linux RT, use --no-net mode)";
#elif defined(VTS_PLATFORM_WINDOWS)
    return "Windows (limited: no raw sockets, best-effort RT via thread priorities)";
#else
    return "Unknown platform (limited functionality)";
#endif
}

// Check if network operations are supported
inline bool network_operations_supported() {
#ifdef VTS_PLATFORM_LINUX
    return true;
#else
    return false;  // Windows and macOS need --no-net mode
#endif
}

// Check if real-time operations are supported (native Linux RT)
inline bool realtime_operations_supported() {
#ifdef VTS_PLATFORM_LINUX
    return true;
#else
    return false;  // Windows and macOS have alternative mechanisms
#endif
}

// Check if platform has best-effort RT capabilities (thread priorities)
inline bool has_thread_priority_support() {
#if defined(VTS_PLATFORM_LINUX) || defined(VTS_PLATFORM_WINDOWS)
    return true;  // Both support thread priorities
#else
    return false;  // macOS limited support
#endif
}

} // namespace platform
} // namespace vts

#endif // PLATFORM_COMPAT_HPP
