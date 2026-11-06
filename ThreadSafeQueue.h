#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue {
private:
    std::deque<T> items_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool closed_ = false;

public:
    bool Push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) return false;
        items_.push_back(item);
        cond_.notify_one();
        return true;
    }

    bool WaitPop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() { return !items_.empty() || closed_; });
        if (items_.empty() && closed_) return false;
        item = items_.front();
        items_.pop_front();
        return true;
    }

    bool TryPop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.empty()) return false;
        item = items_.front();
        items_.pop_front();
        return true;
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cond_.notify_all();
    }
};