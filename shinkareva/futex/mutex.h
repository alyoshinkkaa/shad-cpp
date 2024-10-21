#pragma once
#include <atomic>

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <thread>
#include <cassert>

// Atomically do the following:
//    if (*value == expected_value) {
//        sleep_on_address(value)
//    }

inline void FutexWait(std::atomic<int>* value, int expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expected_value, nullptr, nullptr, 0);
}

// Wakeup 'count' threads sleeping on address of value (-1 wakes all)
inline void FutexWake(std::atomic<int>* value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

int Cmpxchg(std::atomic<int>* atom, int expected, int desired) {
    int* ep = &expected;
    std::atomic_compare_exchange_strong(atom, ep, desired);
    return *ep;
}

class Mutex {
public:
    Mutex() : state_(0) {
    }
    void Lock() {
        int expected = 0;
        if (state_.compare_exchange_strong(expected, 1)) {
            return;
        }
        while (state_.exchange(2) != 0) {
            FutexWait(&state_, 2);
        }
    }

    void Unlock() {
        std::atomic<int> old = state_.exchange(0);
        if (old == 2) {
            FutexWake(&state_, 1);
        }
    }

private:
    std::atomic<int> state_;
};