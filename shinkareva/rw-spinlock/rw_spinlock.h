#include <atomic>
#include <thread>

class RWSpinLock {
public:
    RWSpinLock() : data_(0) {
    }

    void LockRead() {
        while (true) {
            int value = data_.load();
            if (value == 1) {
                std::this_thread::yield();
            }
            if ((value & 1) == 0) {
                if (data_.compare_exchange_weak(value, value + 2)) {
                    break;
                }
            }
        }
    }

    void UnlockRead() {
        data_.fetch_sub(2);
    }

    void LockWrite() {
        while (true) {
            int value = data_.load();
            if (value == 1) {
                std::this_thread::yield();
            }
            if (value == 0) {
                if (data_.compare_exchange_weak(value, 1)) {
                    break;
                }
            }
        }
    }

    void UnlockWrite() {
        data_.store(0);
    }

private:
    std::atomic<int> data_;
};