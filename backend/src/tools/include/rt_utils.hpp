#ifndef RT_UTILS_HPP
#define RT_UTILS_HPP

#include "compat.hpp"
#include <vector>
#include <cstdint>

// Phase 7: Real-Time Utilities (Linux-specific)
// Phase 11: macOS Portability (no-op implementations with INFO logs)
//
// These functions provide real-time capabilities for time-critical operations.
// On Linux: Full functionality (requires elevated privileges).
// On macOS: Safe no-ops that log INFO messages and return false.
//
// Note: Most Linux functions require elevated privileges (CAP_SYS_NICE or root).

/**
 * Lock all current and future memory pages to prevent paging.
 * Linux: Uses mlockall(MCL_CURRENT | MCL_FUTURE).
 * macOS: No-op with INFO log.
 * Returns: true on success, false on failure (logs error or INFO).
 * Requires: CAP_IPC_LOCK capability or root privileges (Linux only).
 */
bool rt_lock_memory();

/**
 * Set the calling thread to SCHED_FIFO with specified priority.
 * Linux: Priority range: 1-99 (higher = more urgent).
 * macOS: No-op with INFO log.
 * Returns: true on success, false on failure (logs error or INFO).
 * Requires: CAP_SYS_NICE capability or root privileges (Linux only).
 */
bool rt_set_realtime(int priority);

/**
 * Set CPU affinity for the calling thread.
 * Linux: cpu_ids: Vector of CPU core IDs to bind to (e.g., {0, 1, 2}).
 * macOS: No-op with INFO log showing requested CPUs.
 * Returns: true on success, false on failure (logs warning or INFO, non-fatal).
 * Note: Silently ignores invalid CPU IDs on Linux.
 */
bool rt_set_affinity(const std::vector<int>& cpu_ids);

/**
 * Absolute sleep using CLOCK_MONOTONIC with EINTR retry.
 * Linux: target_ns: Target time in nanoseconds (from CLOCK_MONOTONIC).
 * macOS: Uses nanosleep fallback (relative time, but functional).
 * Returns: true on successful sleep, false on error.
 * Automatically retries if interrupted by signal (EINTR).
 */
bool rt_sleep_abs(uint64_t target_ns);

/**
 * Optional: Open PTP hardware clock device (e.g., /dev/ptp0).
 * Linux: ptp_device: Path to PTP device (default: "/dev/ptp0").
 * macOS: No-op with INFO log, returns -1.
 * Returns: File descriptor on success, -1 on failure.
 * Caller responsible for closing fd with close() on Linux.
 */
int rt_open_phc(const char* ptp_device = "/dev/ptp0");

#endif // RT_UTILS_HPP
