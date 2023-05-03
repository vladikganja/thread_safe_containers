#pragma once

#include <lib/include/common/task.hpp>

// lock-based bounded queue. One buffer implementation
template <typename TaskT>
class ClassicBQueue {
private:
    uint64_t size_{};

    mutable std::mutex mut_;
    std::condition_variable condProd_;
    std::condition_variable condCons_;

    std::vector<TaskT> buffer_;

    uint64_t dequeuePos_{};
    uint64_t enqueuePos_{};

    bool full_ = false;
    bool empty_ = true;
    bool done_ = false;

    // 34****12
    //  |    |
    //  b    f

public:
    ClassicBQueue(): ClassicBQueue(32) {};
    ClassicBQueue(uint64_t size): buffer_(size) {
    }

    // Some producer sends the data to the queue
    // Then condProd_ waits while buffer is full or skips waiting if it's not full
    // Then it pushes data to the end of the queue (if enqueuePos_ overtakes frontIdx then the queue is full)
    void enqueue(TaskT&& task) {
        std::unique_lock<std::mutex> guard{mut_};

        condProd_.wait(guard, [this] {
            return !full_;
        });

        buffer_[enqueuePos_] = std::move(task);
        enqueuePos_ = (enqueuePos_ + 1) % buffer_.size();

        size_++;
        if (size_ == buffer_.size()) {
            full_ = true;
        }
        empty_ = false;

        guard.unlock();
        condCons_.notify_one();
    }

    bool dequeue(TaskT& task) {
        std::unique_lock<std::mutex> guard{mut_};

        condCons_.wait(guard, [this] {
            return !empty_ || done_;
        });

        if (empty_) {
            return false;
        }

        task = std::move(buffer_[dequeuePos_]);
        dequeuePos_ = (dequeuePos_ + 1) % buffer_.size();

        size_--;
        if (size_ == 0) {
            empty_ = true;
        }
        full_ = false;

        guard.unlock();
        condProd_.notify_one();
        return true;
    }

    void doneWakeUp() {
        std::unique_lock<std::mutex> guard{mut_};
        done_ = true;
        guard.unlock();
        condCons_.notify_all();
    }

    bool emptyAndDone() const {
        std::unique_lock<std::mutex> guard{mut_};
        return empty_ && done_;
    }
};
