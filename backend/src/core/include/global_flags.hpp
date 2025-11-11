#ifndef VTS_GLOBAL_FLAGS_HPP
#define VTS_GLOBAL_FLAGS_HPP

#include <atomic>

namespace vts {

/**
 * @brief Global trip flag for sequence engine coordination
 * 
 * This flag is set by the GOOSE sniffer when a trip rule is triggered.
 * The sequence engine can monitor this flag to coordinate test execution
 * based on GOOSE messages (e.g., relay trip signals).
 * 
 * Thread-safe: Uses atomic operations for concurrent access.
 */
extern std::atomic<bool> GLOBAL_TRIP_FLAG;

/**
 * @brief Set the global trip flag
 * 
 * Thread-safe operation to set the trip flag to true.
 */
inline void setTripFlag() {
    GLOBAL_TRIP_FLAG.store(true, std::memory_order_release);
}

/**
 * @brief Clear the global trip flag
 * 
 * Thread-safe operation to reset the trip flag to false.
 */
inline void clearTripFlag() {
    GLOBAL_TRIP_FLAG.store(false, std::memory_order_release);
}

/**
 * @brief Check if the trip flag is set
 * 
 * Thread-safe operation to read the current state of the trip flag.
 * 
 * @return true if the flag is set, false otherwise
 */
inline bool isTripFlagSet() {
    return GLOBAL_TRIP_FLAG.load(std::memory_order_acquire);
}

} // namespace vts

#endif // VTS_GLOBAL_FLAGS_HPP
