#pragma once

#include "common.h"

// lock-based bounded queue. One buffer implementation
template <typename T>
class ClassicBQueue {
private:
    uint64_t size_;

    mutable std::mutex mut_;
    std::condition_variable condProd_;
    std::condition_variable condCons_;

    std::vector<T> buffer_;

    uint64_t frontIdx_;
    uint64_t backIdx_;

    bool full_ = false;
    bool empty_ = true;

    // 34****12
    //  |    |
    //  b    f

public:
    ClassicBQueue(): ClassicBQueue(32) {};
    ClassicBQueue(uint64_t size): buffer_(size), frontIdx_(0), backIdx_(0) {
    }

    // Some producer sends the data to the queue
    // Then condProd_ waits while buffer is full or skips waiting if it's not full
    // Then it pushes data to the end of the queue (if backIdx_ overtakes frontIdx then the queue is full)
    void push(T&& task) {
        std::unique_lock<std::mutex> lk{mut_};

        condProd_.wait(lk, [this] {
            return !full_;
        });

        buffer_[backIdx_] = std::move(task);
        backIdx_ = (backIdx_ + 1) % buffer_.size();

        if (backIdx_ == frontIdx_) {
            full_ = true;
        } else {
            full_ = false;
        }

        lk.unlock();
        condCons_.notify_one();
    }


    bool try_pop(T& data) {
        std::unique_lock<std::mutex> lk{mut_};

        condCons_.wait(lk, [this] {
            return !empty_;
        });

        if (empty_) {
            return false;
        }

        data = buffer_[frontIdx_];
        frontIdx_ = (frontIdx_ + 1) % buffer_.size();

        if (frontIdx_ == backIdx_) {
            empty_ = true;
        } else {
            empty_ = false;
        }

        lk.unlock();
        condProd_.notify_one();
        return true;
    }


};
