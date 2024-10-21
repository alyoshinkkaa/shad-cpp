#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

template <class T>
class UnbufferedChannel {
    struct Element {
        std::shared_ptr<std::condition_variable> cond_var;
        T value;
        std::shared_ptr<bool> is_received;
    };

public:
    void Send(const T& value) {
        std::unique_lock<std::mutex> write(edit_queue_);
        auto may_exit_ptr = std::make_shared<std::condition_variable>();
        auto is_received_ptr = std::make_shared<bool>(false);
        queue_.emplace(may_exit_ptr, value, is_received_ptr);
        may_read_.notify_one();

        may_exit_ptr->wait(
            write, [this, &is_received_ptr] { return *is_received_ptr || is_closed_.test(); });
        if (!*is_received_ptr) {
            throw std::runtime_error("Channel is closed");
        }
    }

    void Send(T&& value) {
        std::unique_lock<std::mutex> write(edit_queue_);
        auto may_exit_ptr = std::make_shared<std::condition_variable>();
        auto is_received_ptr = std::make_shared<bool>(false);
        queue_.emplace(may_exit_ptr, std::forward<T>(value), is_received_ptr);
        may_read_.notify_one();

        may_exit_ptr->wait(
            write, [this, &is_received_ptr] { return *is_received_ptr || is_closed_.test(); });
        if (!*is_received_ptr) {
            throw std::runtime_error("Channel is closed");
        }
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> read(edit_queue_);
        may_read_.wait(read, [this] { return !queue_.empty() || is_closed_.test(); });
        if (is_closed_.test()) {
            return std::nullopt;
        }

        auto elem = std::move(queue_.front());
        T value = std::move(elem.value);
        *elem.is_received = true;
        elem.cond_var->notify_one();
        queue_.pop();
        return std::optional<T>(std::move(value));
    }

    void Close() {
        std::unique_lock<std::mutex> close(edit_queue_);
        is_closed_.test_and_set();
        while (!queue_.empty()) {
            queue_.front().cond_var->notify_one();
            queue_.pop();
        }
        may_read_.notify_all();
    }

private:
    std::mutex edit_queue_;
    std::condition_variable may_read_;
    std::queue<Element> queue_;
    std::atomic_flag is_closed_;
};
