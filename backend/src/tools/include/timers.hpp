#ifndef TIMERS_HPP
#define TIMERS_HPP

#include <time.h>
#include <iostream>
#include <cerrno>


class Timer{
public:
    struct timespec next_period;

    void increment_period(long period_ns) {
        next_period.tv_nsec += period_ns;
        while (next_period.tv_nsec >= 1000000000) {
            next_period.tv_sec += 1;
            next_period.tv_nsec -= 1000000000;
        }
    }

    void start_period(long period_ns) {
        clock_gettime(CLOCK_MONOTONIC, &next_period);
        increment_period(period_ns);
    }

    void start_period(struct timespec initial_time) {
        next_period.tv_sec = initial_time.tv_sec;
        next_period.tv_nsec = initial_time.tv_nsec;
    }

    void wait_period(long period_ns) {
#ifdef __linux__
        int ret;
        do {
            ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, NULL);
        } while (ret == EINTR);
        
        if(ret != 0 && ret != EINTR){
            std::cerr << "Error in clock_nanosleep: " << ret << std::endl;
        }
#else
        // macOS: Use nanosleep as a fallback (not real-time, but functional)
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        
        struct timespec sleep_time;
        sleep_time.tv_sec = next_period.tv_sec - now.tv_sec;
        sleep_time.tv_nsec = next_period.tv_nsec - now.tv_nsec;
        
        if (sleep_time.tv_nsec < 0) {
            sleep_time.tv_sec--;
            sleep_time.tv_nsec += 1000000000L;
        }
        
        if (sleep_time.tv_sec >= 0) {
            nanosleep(&sleep_time, NULL);
        }
#endif
        increment_period(period_ns);
    }
};















#endif // TIMERS_HPP