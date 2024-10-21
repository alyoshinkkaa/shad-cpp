#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

class Incrementer {
public:
    explicit Incrementer(std::shared_ptr<std::atomic<int64_t>> counter) : counter_{counter} {
    }

    void Increment() {
        counter_->fetch_add(1, std::memory_order_relaxed);
    }

private:
    std::shared_ptr<std::atomic<int64_t>> counter_;
};

class ReadWriteAtomicCounter {
public:
    std::unique_ptr<Incrementer> GetIncrementer() {
        auto counter_ptr = std::make_shared<std::atomic<int64_t>>(0);
        std::unique_lock lock(mutex_);
        counters_.emplace_back(counter_ptr);
        lock.unlock();
        threads_cnt_ += 1;
        return std::make_unique<Incrementer>(counter_ptr);
    }

    int64_t GetValue() const {
        int64_t res = 0;
        for (size_t i = 0; i < threads_cnt_; ++i) {
            res += counters_[i]->load(std::memory_order_relaxed);
        }

        return res;
    }

private:
    std::vector<std::shared_ptr<std::atomic<int64_t>>> counters_;
    std::mutex mutex_;
    std::atomic_size_t threads_cnt_ = 0;
};
