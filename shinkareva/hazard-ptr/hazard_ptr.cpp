#include "hazard_ptr.h"
#include <algorithm>

thread_local std::atomic<void*> hazard_ptr{nullptr};
std::atomic<size_t> approximate_free_list_size{0};
std::unordered_set<ThreadState*> threads;
std::mutex scan_lock;
std::atomic<RetiredPtr*> free_list{nullptr};
std::mutex threads_lock;

void ScanFreeList() {
    approximate_free_list_size.store(0);

    if (!scan_lock.try_lock()) {
        return;
    }

    std::unique_lock guard{scan_lock, std::adopt_lock};

    RetiredPtr* retired = free_list.exchange(nullptr);

    std::vector<void*> hazard;
    {
        std::lock_guard<std::mutex> threads_guard(threads_lock);
        for (auto* thread : threads) {
            if (auto* ptr = thread->ptr->load()) {
                hazard.push_back(ptr);
            }
        }
    }

    while (retired != nullptr) {
        RetiredPtr* next = retired->next;

        if (std::ranges::find(hazard, retired->value) == hazard.end()) {
            retired->deleter(retired->value);
            delete retired;
        } else {
            while (!free_list.compare_exchange_weak(retired->next, retired)) {
            }
        }
        retired = next;
    }
}

void RegisterThread() {
    ThreadState* thread_state = new ThreadState();
    thread_state->ptr = &hazard_ptr;

    std::lock_guard<std::mutex> guard(threads_lock);
    threads.insert(thread_state);
}

void UnregisterThread() {
    std::unique_lock guard{threads_lock};
    auto it = std::find_if(threads.begin(), threads.end(),
                           [&](ThreadState* ts) { return ts->ptr == &hazard_ptr; });

    auto ptr = *it;
    threads.erase(it);
    delete ptr;
    if (threads.empty()) {
        guard.unlock();
        ScanFreeList();
    }
}