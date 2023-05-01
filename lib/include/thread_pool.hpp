#pragma once
#include "classic_b_queue.hpp"
#include "task.hpp"

int foo1(int a, int b) {
    return a ^ b;
}

std::string foo2(const std::string& s1, const std::string& s2) {
    return s1 + s2;
}

template <typename TaskT>
class ThreadPool {
protected:
    uint64_t nThreads_;
    uint64_t nTasks_;

    mutable std::mutex taskMut_;

    ClassicBQueue<TaskT>& queue_;

    std::vector<std::thread> threads_;
    bool finished_ = false;

public:
    ThreadPool(uint64_t nThreads, uint64_t nTastks, ClassicBQueue<TaskT>& queue)
            : nThreads_(nThreads), nTasks_(nTastks), queue_(queue) {
    }

    virtual ~ThreadPool(){};
};

template <typename TaskT>
class ProducerPool : public ThreadPool<TaskT> {
private:
    using ThreadPool<TaskT>::nTasks_;
    using ThreadPool<TaskT>::nThreads_;
    using ThreadPool<TaskT>::taskMut_;
    using ThreadPool<TaskT>::queue_;
    using ThreadPool<TaskT>::threads_;
    using ThreadPool<TaskT>::finished_;

    bool check_finish() {
        std::lock_guard<std::mutex> guard{taskMut_};
        if (nTasks_ == 0) {
            return true;
        }
        nTasks_ -= 1;
        return false;
    }

    void produce() {
        for (;;) {
            INFO("producer iter");
            if (check_finish())
                break;

            if (nTasks_ % 2 == 0) {
                INFO("task 0");
                auto&& [task, future] = createTask(foo1, 42, 23);
                queue_.push(std::move(task));
            } else {
                INFO("task 1");
                auto&& [task, future] = createTask(foo2, "42", "23");
                queue_.push(std::move(task));
            }
        }
        INFO("producer finished");
    }

public:
    ProducerPool(uint64_t nProducers, uint64_t nTastks, ClassicBQueue<TaskT>& queue)
            : ThreadPool<TaskT>(nProducers, nTastks, queue) {
    }

    ~ProducerPool() override {
        if (!finished_) {
            finish_produce();
        }
    }

    void start_produce() {
        for (uint64_t thread = 0; thread < nThreads_; thread++) {
            INFO("producer thread launched");
            threads_.emplace_back(&ProducerPool::produce, this);
        }
    }

    void finish_produce() {
        for (auto& thread : threads_) {
            thread.join();
        }
        queue_.wake_and_done();
        finished_ = true;
    }
};

template <typename TaskT>
class ConsumerPool : ThreadPool<TaskT> {
private:
    using ThreadPool<TaskT>::nTasks_;
    using ThreadPool<TaskT>::nThreads_;
    using ThreadPool<TaskT>::taskMut_;
    using ThreadPool<TaskT>::queue_;
    using ThreadPool<TaskT>::threads_;
    using ThreadPool<TaskT>::finished_;

    mutable std::mutex consMut_;
    uint64_t consumed_{};

    bool check_finish() {
        std::lock_guard<std::mutex> guard{taskMut_};
        if (consumed_ == nTasks_ && queue_.is_empty_and_done()) {
            return true;
        }
        return false;
    }

    void consume() {
        for (;;) {
            INFO("consumer iter");
            if (check_finish())
                break;

            Task t;
            INFO("queue_.pop()");
            bool success = queue_.pop(t);
            if (success) {
                INFO("success");
                std::lock_guard<std::mutex> guard{consMut_};
                consumed_++;
            }
        }
    }

public:
    ConsumerPool(uint64_t nConsumers, uint64_t nTasks, ClassicBQueue<TaskT>& queue)
            : ThreadPool<TaskT>(nConsumers, nTasks, queue) {
    }

    ~ConsumerPool() override {
        if (!finished_) {
            finish_consume();
        }
    }

    void start_consume() {
        for (uint64_t thread = 0; thread < nThreads_; thread++) {
            INFO("consumer thread launched");
            threads_.emplace_back(&ConsumerPool::consume, this);
        }
    }

    void finish_consume() {
        for (auto& thread : threads_) {
            thread.join();
        }
        finished_ = true;
    }

    uint64_t consumed() const {
        return consumed_;
    }
};
