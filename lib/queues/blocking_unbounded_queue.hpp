#pragma once

#include <lib/common/task.hpp>

// lock-based bounded queue. One buffer implementation
template <typename TaskT>
class BlockingUnboundedQueue {
private:
    mutable std::mutex mut_;
    std::queue<TaskT, std::deque<TaskT>> queue_;

public:
    void enqueue(TaskT&& task) {
        std::lock_guard<std::mutex> guard{mut_};
        queue_.push(std::move(task));
    }

    bool dequeue(TaskT& task) {
        std::unique_lock<std::mutex> guard{mut_};

        if (queue_.empty()) {
            return false;
        }

        task = std::move(queue_.front());
        queue_.pop();

        return true;
    }

    bool empty() const {
        return queue_.empty();
    }
};
