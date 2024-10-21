#include "spinlock.h"

#include <vector>
#include <thread>

#include <catch2/catch_test_macros.hpp>

using namespace std::chrono_literals;

TEST_CASE("Correctness") {
    SpinLock spin;
    std::jthread first{[&] {
        spin.Lock();
        std::this_thread::sleep_for(1500ms);
        spin.Unlock();
    }};
    std::this_thread::sleep_for(300ms);
    std::jthread second{[&] {
        auto start = std::chrono::steady_clock::now();
        spin.Lock();
        auto diff = std::chrono::steady_clock::now() - start;
        REQUIRE(diff > 1.1s);
        REQUIRE(diff < 1.3s);
        spin.Unlock();
    }};
}

TEST_CASE("Concurrency") {
    static constexpr auto kThreadsCount = 16;
    static constexpr auto kNumLocks = 1'000;
    std::vector<std::jthread> threads;
    auto counter = 0;
    SpinLock spin;
    for (auto i = 0u; i < kThreadsCount; ++i) {
        threads.emplace_back([&] {
            for (auto j = 0; j < kNumLocks; ++j) {
                spin.Lock();
                ++counter;
                spin.Unlock();
            }
        });
    }
    threads.clear();
    REQUIRE(counter == kThreadsCount * kNumLocks);
}
