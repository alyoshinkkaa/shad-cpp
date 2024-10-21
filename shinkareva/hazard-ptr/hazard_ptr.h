#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#include <vector>

extern std::shared_mutex lock;

extern thread_local std::atomic<void*> hazard_ptr;

class ThreadState {
public:
    std::atomic<void*>* ptr;
};

extern std::mutex threads_lock;
extern std::unordered_set<ThreadState*> threads;

void RegisterThread();
void UnregisterThread();

template <class T>
T* Acquire(std::atomic<T*>* ptr) {
    auto* value = ptr->load();  // (2)

    do {
        hazard_ptr.store(value);

        auto* new_value = ptr->load();  // (3)
        if (new_value == value) {       // (1)
            return value;
        }

        value = new_value;
    } while (true);
}

inline void Release() {
    hazard_ptr.store(nullptr);
}

extern std::mutex scan_lock;

struct RetiredPtr {
    void* value;
    std::function<void(void*)> deleter;
    RetiredPtr* next;
};

extern std::atomic<RetiredPtr*> free_list;
extern std::atomic<size_t> approximate_free_list_size;

void ScanFreeList();

template <class T, class Deleter = std::default_delete<T>>
void Retire(T* ptr, Deleter deleter = {}) {
    RetiredPtr* new_retired =
        new RetiredPtr{ptr, [deleter](void* a) { deleter(reinterpret_cast<T*>(a)); }, nullptr};

    RetiredPtr* old_head = free_list.load();
    do {
        new_retired->next = old_head;
    } while (!free_list.compare_exchange_weak(old_head, new_retired));

    approximate_free_list_size.fetch_add(1);

    if (approximate_free_list_size.load() > 100) {
        ScanFreeList();
    }
}