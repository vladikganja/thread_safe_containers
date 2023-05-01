#pragma once

#include "common.h"

// lock-based bounded queue. One buffer implementation
template <typename T>
class ClassicBQueue {
private:
    uint64_t size_{};

    mutable std::mutex mut_;
    std::condition_variable condProd_;
    std::condition_variable condCons_;

    std::vector<T> buffer_;

    uint64_t frontIdx_{};
    uint64_t backIdx_{};

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
    // Then it pushes data to the end of the queue (if backIdx_ overtakes frontIdx then the queue is full)
    void push(T&& task) {
        std::unique_lock<std::mutex> lk{mut_};

        condProd_.wait(lk, [this] {
            return !full_;
        });

        buffer_[backIdx_] = std::move(task);
        backIdx_ = (backIdx_ + 1) % buffer_.size();

        size_++;
        if (size_ == buffer_.size()) {
            full_ = true;
        }
        empty_ = false;

        lk.unlock();
        condCons_.notify_one();
    }

    bool pop(T& data) {
        INFO("pop");
        std::unique_lock<std::mutex> lk{mut_};

        INFO("wait");
        condCons_.wait(lk, [this] {
            std::cout << empty_ << "\n";
            return !empty_ || done_;
        });

        INFO("next");
        std::this_thread::sleep_for(10ms);
        if (empty_) {
            return false;
        }

        data = std::move(buffer_[frontIdx_]);
        frontIdx_ = (frontIdx_ + 1) % buffer_.size();

        size_--;
        if (size_ == 0) {
            empty_ = true;
        }
        full_ = false;

        lk.unlock();
        condProd_.notify_one();
        return true;
    }

    void wake_and_done() {
        std::unique_lock<std::mutex> guard{mut_};
        done_ = true;
        guard.unlock();
        condCons_.notify_all();
    }

    bool is_empty_and_done() const {
        std::unique_lock<std::mutex> Lk{mut_};
        return empty_ && done_;
    }
};
