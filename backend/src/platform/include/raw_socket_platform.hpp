#ifndef RAW_SOCKET_PLATFORM_HPP
#define RAW_SOCKET_PLATFORM_HPP

// ============================================================================
// Platform-Aware RawSocket Selector (Phase 11)
// ============================================================================
// This header automatically selects the correct RawSocket implementation
// based on the target platform:
//
// - Linux: Use real raw_socket.hpp with AF_PACKET support
// - macOS: Use raw_socket_stub.hpp with safe no-ops
//
// Usage:
//   #include "raw_socket_platform.hpp"
//   
//   // RawSocket class will be available regardless of platform
//   RawSocket socket;
// ============================================================================

#include "compat.hpp"

#ifdef VTS_PLATFORM_LINUX
    // Linux: Use real implementation
    #include "raw_socket.hpp"
#else
    // macOS and other platforms: Use stub
    #include "raw_socket_stub.hpp"
#endif

#endif // RAW_SOCKET_PLATFORM_HPP
