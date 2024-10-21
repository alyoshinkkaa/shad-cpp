#include "spinlock.h"
#include "runner.h"
#include "util.h"

#include <string>
#include <chrono>
#include <thread>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

static constexpr auto kNow = std::chrono::steady_clock::now;

using namespace std::chrono_literals;

static void RunBenchmark(uint32_t num_threads) {
    static constexpr auto kNumIterations = 1'000'000;
    SpinLock lock;
    int counter{};
    BENCHMARK(std::to_string(num_threads)) {
        counter = 0;
        Runner runner{kNumIterations};
        for (auto i = 0u; i < num_threads; ++i) {
            runner.Do([&] {
                lock.Lock();
                ++counter;
                lock.Unlock();
            });
        }
    };
    REQUIRE(counter == kNumIterations);
}

TEST_CASE("Benchmark") {
    for (auto num_threads : {1, 2, 4, 8, 16, 32}) {
        RunBenchmark(num_threads);
    }
}

TEST_CASE("WithoutSleep") {
    static constexpr auto kThreadsCount = 4u;
    if (std::thread::hardware_concurrency() < kThreadsCount) {
        FAIL("hardware_concurrency < " << kThreadsCount);
    }
    size_t counter = 0u;
    SpinLock spin;
    TimeRunner runner{1s};
    CPUTimer timer;
    for (auto i = 0u; i < kThreadsCount; ++i) {
        runner.Do([&] {
            spin.Lock();
            ++counter;
            spin.Unlock();
        });
    }
    runner.Wait();

    auto cpu_time = timer.GetTimes().cpu_time;
    CHECK(cpu_time > 400ms * kThreadsCount);
}

TEST_CASE("SmallDelay") {
    static constexpr auto kThreadsCount = 4u;
    static constexpr auto kNumIterations = 30'000'000;

    if (std::thread::hardware_concurrency() < kThreadsCount) {
        FAIL("hardware_concurrency < " << kThreadsCount);
    }
    auto counter = 0;
    auto prev = kThreadsCount + 1;
    SpinLock spin;

    auto sum_wait = std::chrono::steady_clock::duration::zero();
    size_t num_switches = 0;

    Runner runner{kNumIterations};
    for (auto i = 0u; i < kThreadsCount; ++i) {
        runner.Do([&, i, start = kNow()]() mutable {
            spin.Lock();
            if (prev != i) {
                sum_wait += kNow() - start;
                ++num_switches;
                prev = i;
            }
            ++counter;
            spin.Unlock();
            start = kNow();
        });
    }
    runner.Wait();
    REQUIRE(counter == kNumIterations);
    REQUIRE(30 * num_switches > kNumIterations);
    REQUIRE(sum_wait / num_switches < 3us);
}
