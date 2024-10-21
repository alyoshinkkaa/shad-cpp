#include "mpsc_queue.h"

#include <algorithm>
#include <thread>
#include <ranges>
#include <map>
#include <tuple>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("PushPop") {
    MPSCQueue<int> queue;
    queue.Push(1);
    queue.Push(2);

    auto [value, ok] = queue.Pop();
    REQUIRE(ok);
    REQUIRE(value == 2);

    std::tie(value, ok) = queue.Pop();
    REQUIRE(ok);
    REQUIRE(value == 1);

    std::tie(value, ok) = queue.Pop();
    REQUIRE_FALSE(ok);
}

TEST_CASE("SingleThread") {
    static constexpr auto kRange = std::views::iota(0, 1024);
    MPSCQueue<int> queue;
    for (auto i : kRange) {
        queue.Push(i);
    }

    std::vector<int> dequeued;
    queue.DequeueAll([&](int value) { dequeued.push_back(value); });
    REQUIRE(std::ranges::equal(dequeued, kRange | std::views::reverse));
}

TEST_CASE("Destructor") {
    MPSCQueue<int> queue;
    queue.Push(1);
    queue.Push(2);
    queue.Push(3);
}

TEST_CASE("Threaded") {
    static constexpr auto kN = 100'000;
    static constexpr auto kRange = std::views::iota(0, kN);
    static constexpr auto kNumProducers = 8;
    MPSCQueue<int> queue;
    std::map<int, size_t> dequeued;

    std::jthread consumer{[&](std::stop_token token) {
        auto save = [&](int value) { ++dequeued[value]; };
        while (!token.stop_requested()) {
            queue.DequeueAll(save);
        }
        queue.DequeueAll(save);
    }};
    std::vector<std::jthread> threads;
    for (auto i = 0; i < kNumProducers; ++i) {
        threads.emplace_back([&] {
            for (auto i : kRange) {
                queue.Push(i);
            }
        });
    }

    threads.clear();
    consumer.request_stop();
    consumer.join();
    REQUIRE(std::ranges::equal(dequeued | std::views::keys, kRange));
    REQUIRE(std::ranges::count(dequeued | std::views::values, kNumProducers) == kN);
}
