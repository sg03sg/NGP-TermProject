#pragma once
#include <cstddef>
#include <utility>
#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue {
private:
    std::deque<T> items_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    bool closed_ = false;

public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() { Close(); }

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // 비동기 큐에 아이템 푸시. 이미 Close된 큐면 false.
    bool Push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) return false;
        items_.push_back(std::move(item));
        cond_.notify_one();
        return true;
    }

    // 아이템 대기 및 팝 (블로킹). Close되고 비어 있으면 false.
    bool WaitPop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() { return !items_.empty() || closed_; });
        if (items_.empty()) return false;   // closed_ 이고 빈 경우
        item = std::move(items_.front());
        items_.pop_front();
        return true;
    }

    // 논블로킹 팝 시도
    bool TryPop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (items_.empty()) return false;
        item = std::move(items_.front());
        items_.pop_front();
        return true;
    }

    // 큐 종료 처리 및 대기 스레드 깨우기
    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        cond_.notify_all();
    }

    bool IsClosed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return items_.size();
    }
};