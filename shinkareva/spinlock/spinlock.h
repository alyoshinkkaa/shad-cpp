#include <atomic>
#include <thread>

class SpinLock {
public:
    SpinLock() : flag_(false) {
    }

    void Lock() {
        while (flag_.exchange(true, std::memory_order_acquire)) {
            while (flag_.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
        }
    }

    void Unlock() {
        flag_.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> flag_;
};