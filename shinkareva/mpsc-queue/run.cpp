#include "mpsc_queue.h"
#include "runner.h"

#include <chrono>

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

using namespace std::chrono_literals;

TEST_CASE("Benchmark") {
    MPSCQueue<int> queue;

    TimeRunner pop_runner{1200ms};
    pop_runner.Do([&] { queue.Pop(); });

    TimeRunner runner{1s};
    for (auto i = 0; i < 8; ++i) {
        runner.Do([&] { queue.Push(42); });
    }
    REQUIRE(runner.Wait() < 150ns);
}
