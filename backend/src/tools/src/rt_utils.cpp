#include "rt_utils.hpp"
#include "compat.hpp"
#include "logger.hpp"
#include <cstring>
#include <cerrno>
#include <string>
#include <vector>

// Linux-specific headers
#ifdef VTS_PLATFORM_LINUX
#include <sys/mman.h>      // mlockall, MCL_CURRENT, MCL_FUTURE
#include <sched.h>         // sched_setscheduler, sched_param, SCHED_FIFO
#include <pthread.h>       // pthread_setaffinity_np
#include <time.h>          // clock_nanosleep, CLOCK_MONOTONIC, TIMER_ABSTIME
#include <unistd.h>        // sysconf, _SC_NPROCESSORS_ONLN
#include <fcntl.h>         // open
#include <sys/stat.h>      // open
#endif

// Windows-specific headers
#ifdef VTS_PLATFORM_WINDOWS
#include <windows.h>       // SetThreadPriority, SetThreadAffinityMask, Sleep
#include <processthreadsapi.h>
#endif

bool rt_lock_memory() {
#ifdef VTS_PLATFORM_LINUX
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        LOG_WARN("RT", "mlockall() failed: %s (requires CAP_IPC_LOCK or root)", std::strerror(errno));
        return false;
    }
    LOG_INFO("RT", "Memory locked (mlockall MCL_CURRENT | MCL_FUTURE)");
    return true;
#elif defined(VTS_PLATFORM_WINDOWS)
    // Windows: Lock working set size (best-effort memory locking)
    SIZE_T minWorkingSetSize = 0;
    SIZE_T maxWorkingSetSize = 0;
    
    // Get current working set size
    if (GetProcessWorkingSetSize(GetCurrentProcess(), &minWorkingSetSize, &maxWorkingSetSize)) {
        // Set minimum = maximum to lock pages in memory
        SIZE_T desiredSize = 512 * 1024 * 1024;  // 512 MB default
        if (SetProcessWorkingSetSize(GetCurrentProcess(), desiredSize, desiredSize)) {
            LOG_INFO("RT", "Memory working set locked on Windows (best-effort, %zu MB)", desiredSize / (1024 * 1024));
            return true;
        }
    }
    LOG_WARN("RT", "SetProcessWorkingSetSize() failed (requires SeIncreaseQuotaPrivilege or admin rights)");
    return false;
#elif defined(VTS_PLATFORM_MAC)
    LOG_INFO("RT", "rt_lock_memory() not available on macOS (no-op)");
    return false;
#else
    LOG_INFO("RT", "rt_lock_memory() not supported on this platform (no-op)");
    return false;
#endif
}

bool rt_set_realtime(int priority) {
#ifdef VTS_PLATFORM_LINUX
    struct sched_param sp = {};
    sp.sched_priority = (priority > 0 && priority <= 99) ? priority : 80;
    if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
        LOG_WARN("RT", "sched_setscheduler(SCHED_FIFO, priority=%d) failed: %s (requires CAP_SYS_NICE or root)", 
                 sp.sched_priority, std::strerror(errno));
        return false;
    }
    LOG_INFO("RT", "Set SCHED_FIFO with priority=%d", sp.sched_priority);
    return true;
#elif defined(VTS_PLATFORM_WINDOWS)
    // Windows: Map Linux priority (1-99) to Windows priority classes
    // Linux 1-99 scale:
    //   90-99: REALTIME_PRIORITY_CLASS (use with caution!)
    //   70-89: HIGH_PRIORITY_CLASS
    //   40-69: ABOVE_NORMAL_PRIORITY_CLASS
    //   1-39:  NORMAL_PRIORITY_CLASS
    
    DWORD priorityClass = NORMAL_PRIORITY_CLASS;
    int threadPriority = THREAD_PRIORITY_NORMAL;
    
    if (priority >= 90) {
        priorityClass = REALTIME_PRIORITY_CLASS;
        threadPriority = THREAD_PRIORITY_TIME_CRITICAL;
        LOG_INFO("RT", "Using Windows REALTIME_PRIORITY_CLASS (mapped from Linux priority %d)", priority);
    } else if (priority >= 70) {
        priorityClass = HIGH_PRIORITY_CLASS;
        threadPriority = THREAD_PRIORITY_HIGHEST;
        LOG_INFO("RT", "Using Windows HIGH_PRIORITY_CLASS (mapped from Linux priority %d)", priority);
    } else if (priority >= 40) {
        priorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
        threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
        LOG_INFO("RT", "Using Windows ABOVE_NORMAL_PRIORITY_CLASS (mapped from Linux priority %d)", priority);
    } else {
        LOG_INFO("RT", "Using Windows NORMAL_PRIORITY_CLASS (mapped from Linux priority %d)", priority);
    }
    
    // Set process priority class
    if (!SetPriorityClass(GetCurrentProcess(), priorityClass)) {
        LOG_WARN("RT", "SetPriorityClass() failed (error %lu)", GetLastError());
        return false;
    }
    
    // Set thread priority within the class
    if (!SetThreadPriority(GetCurrentThread(), threadPriority)) {
        LOG_WARN("RT", "SetThreadPriority() failed (error %lu)", GetLastError());
        return false;
    }
    
    return true;
#elif defined(VTS_PLATFORM_MAC)
    (void)priority;  // Unused on macOS
    LOG_INFO("RT", "rt_set_realtime() not supported on this platform (no-op)");
    return false;
#else
    (void)priority;  // Unused on non-Linux platforms
    LOG_INFO("RT", "rt_set_realtime() not supported on this platform (no-op)");
    return false;
#endif
}

bool rt_set_affinity(const std::vector<int>& cpu_ids) {
#ifdef VTS_PLATFORM_LINUX
    if (cpu_ids.empty()) {
        LOG_WARN("RT", "Empty CPU affinity list");
        return false;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    
    // Get number of available CPUs
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus <= 0) {
        num_cpus = 1; // fallback
    }
    
    // Add valid CPU IDs to the set
    bool any_valid = false;
    for (int cpu_id : cpu_ids) {
        if (cpu_id >= 0 && cpu_id < num_cpus) {
            CPU_SET(cpu_id, &cpuset);
            any_valid = true;
        } else {
            LOG_WARN("RT", "Ignoring invalid CPU ID %d (valid range: 0-%ld)", cpu_id, num_cpus - 1);
        }
    }
    
    if (!any_valid) {
        LOG_WARN("RT", "No valid CPU IDs in affinity list");
        return false;
    }
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        LOG_WARN("RT", "pthread_setaffinity_np() failed: %s", std::strerror(errno));
        return false;
    }
    
    // Build CPU list string for logging
    std::string cpu_list;
    for (int cpu_id : cpu_ids) {
        if (cpu_id >= 0 && cpu_id < num_cpus) {
            if (!cpu_list.empty()) cpu_list += " ";
            cpu_list += std::to_string(cpu_id);
        }
    }
    LOG_INFO("RT", "CPU affinity set to: %s", cpu_list.c_str());
    return true;
#elif defined(VTS_PLATFORM_WINDOWS)
    if (cpu_ids.empty()) {
        LOG_WARN("RT", "Empty CPU affinity list");
        return false;
    }
    
    // Get system info for CPU count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numCpus = sysInfo.dwNumberOfProcessors;
    
    // Build affinity mask from CPU IDs
    DWORD_PTR affinityMask = 0;
    bool any_valid = false;
    
    for (int cpu_id : cpu_ids) {
        if (cpu_id >= 0 && cpu_id < (int)numCpus) {
            affinityMask |= (1ULL << cpu_id);
            any_valid = true;
        } else {
            LOG_WARN("RT", "Ignoring invalid CPU ID %d (valid range: 0-%lu)", cpu_id, numCpus - 1);
        }
    }
    
    if (!any_valid) {
        LOG_WARN("RT", "No valid CPU IDs in affinity list");
        return false;
    }
    
    // Set thread affinity
    if (SetThreadAffinityMask(GetCurrentThread(), affinityMask) == 0) {
        LOG_WARN("RT", "SetThreadAffinityMask() failed (error %lu)", GetLastError());
        return false;
    }
    
    // Build CPU list string for logging
    std::string cpu_list;
    for (size_t i = 0; i < cpu_ids.size(); ++i) {
        if (i > 0) cpu_list += ",";
        cpu_list += std::to_string(cpu_ids[i]);
    }
    LOG_INFO("RT", "CPU affinity set to: %s (Windows)", cpu_list.c_str());
    return true;
#elif defined(VTS_PLATFORM_MAC)
    // Build CPU list string for logging
    std::string cpu_list;
    for (size_t i = 0; i < cpu_ids.size(); ++i) {
        if (i > 0) cpu_list += ",";
        cpu_list += std::to_string(cpu_ids[i]);
    }
    LOG_INFO("RT", "rt_set_affinity(cpus=[%s]) not available on macOS (no-op)", cpu_list.c_str());
    return false;
#else
    LOG_INFO("RT", "rt_set_affinity() not supported on this platform (no-op)");
    return false;
#endif
}

bool rt_sleep_abs(uint64_t target_ns) {
#ifdef VTS_PLATFORM_LINUX
    struct timespec target_ts;
    target_ts.tv_sec = static_cast<time_t>(target_ns / 1000000000ULL);
    target_ts.tv_nsec = static_cast<long>(target_ns % 1000000000ULL);
    
    int result;
    do {
        result = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target_ts, nullptr);
        // Retry if interrupted by signal (EINTR)
    } while (result == EINTR);
    
    if (result != 0) {
        LOG_ERROR("RT", "clock_nanosleep() failed: %s", std::strerror(result));
        return false;
    }
    
    return true;
#elif defined(VTS_PLATFORM_MAC)
    // On macOS, use nanosleep as a fallback (not absolute, but functional for no-op mode)
    LOG_INFO("RT", "rt_sleep_abs() using nanosleep fallback on macOS");
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(target_ns / 1000000000ULL);
    ts.tv_nsec = static_cast<long>(target_ns % 1000000000ULL);
    
    int result;
    do {
        result = nanosleep(&ts, &ts);
    } while (result == -1 && errno == EINTR);
    
    return (result == 0);
#elif defined(VTS_PLATFORM_WINDOWS)
    // Windows: Convert absolute time to relative sleep
    // Use QueryPerformanceCounter for high-resolution timing
    LARGE_INTEGER frequency, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&currentTime);
    
    // Convert current time to nanoseconds
    uint64_t current_ns = (currentTime.QuadPart * 1000000000ULL) / frequency.QuadPart;
    
    if (target_ns <= current_ns) {
        // Already past target time
        return true;
    }
    
    uint64_t sleep_ns = target_ns - current_ns;
    uint64_t sleep_ms = sleep_ns / 1000000ULL;  // Convert to milliseconds
    
    if (sleep_ms > 0) {
        Sleep(static_cast<DWORD>(sleep_ms));
    }
    
    // For sub-millisecond precision, busy-wait (best we can do on Windows)
    uint64_t remaining_ns = sleep_ns % 1000000ULL;
    if (remaining_ns > 10000) {  // Only spin if > 10us
        QueryPerformanceCounter(&currentTime);
        uint64_t spin_end = ((currentTime.QuadPart * 1000000000ULL) / frequency.QuadPart) + remaining_ns;
        while (true) {
            QueryPerformanceCounter(&currentTime);
            uint64_t now = (currentTime.QuadPart * 1000000000ULL) / frequency.QuadPart;
            if (now >= spin_end) break;
        }
    }
    
    return true;
#else
    LOG_INFO("RT", "rt_sleep_abs() not supported on this platform (no-op)");
    return false;
#endif
}

int rt_open_phc(const char* ptp_device) {
#ifdef VTS_PLATFORM_LINUX
    int fd = open(ptp_device, O_RDWR);
    if (fd < 0) {
        LOG_WARN("RT", "Failed to open PTP device %s: %s", ptp_device, std::strerror(errno));
        return -1;
    }
    
    LOG_INFO("RT", "Opened PTP hardware clock: %s (fd=%d)", ptp_device, fd);
    return fd;
#elif defined(VTS_PLATFORM_WINDOWS)
    (void)ptp_device;  // Unused on Windows
    LOG_INFO("RT", "rt_open_phc() not available on Windows (no-op)");
    return -1;
#elif defined(VTS_PLATFORM_MAC)
    (void)ptp_device;  // Unused on macOS
    LOG_INFO("RT", "rt_open_phc(%s) not available on macOS (no-op)", ptp_device);
    return -1;
#else
    (void)ptp_device;  // Unused on other platforms
    LOG_INFO("RT", "rt_open_phc() not supported on this platform (no-op)");
    return -1;
#endif
}
