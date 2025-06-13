#pragma once

#include "common.hpp"

namespace bplayer{

template<typename T>
class BlockingQueue {
public:
    // maxSize = 0 means no limit
    explicit BlockingQueue(size_t maxSize = 0)
        : maxSize_(maxSize), isShutdown_(false)
    {}

    template<typename U>
    bool push(U&& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (maxSize_ != 0) {
            cv_NotFull_.wait(lock, [&]() {
                return isShutdown_ || queue_.size() < maxSize_;
            });
        }
        if(isShutdown_) {
            return false;
        }
        queue_.emplace(std::forward<U>(item));
        cv_NotEmpty_.notify_one();
        return true;
    }

    template<typename U>
    bool try_push(U&& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (maxSize_ > 0 && queue_.size() >= maxSize_) {
            return false;
        }
        queue_.emplace(std::forward<U>(item));
        cv_NotEmpty_.notify_one();
        return true;
    }

    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_NotEmpty_.wait(lock, [&]() {
            return isShutdown_ || !queue_.empty();
        });
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        cv_NotFull_.notify_one();
        return true;
    }

    bool popFor(T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_NotEmpty_.wait_for(lock, timeout, [&]() {
            return isShutdown_ || !queue_.empty();
        })) {
            return false;
        }
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        cv_NotFull_.notify_one();
        return true;
    }

    bool front(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        item = queue_.front();
        return true;
    }

    void flush() {
        std::unique_lock<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
        cv_NotFull_.notify_all();
    }

    void shutdown() {
        std::unique_lock<std::mutex> lock(mutex_);
        isShutdown_ = true;
        cv_NotEmpty_.notify_all();
        cv_NotFull_.notify_all();
    }

    size_t size() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

    size_t capacity() {
        std::unique_lock<std::mutex> lock(mutex_);
        return maxSize_;
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cv_NotEmpty_;
    std::condition_variable cv_NotFull_;
    size_t maxSize_;
    bool isShutdown_;
};

}